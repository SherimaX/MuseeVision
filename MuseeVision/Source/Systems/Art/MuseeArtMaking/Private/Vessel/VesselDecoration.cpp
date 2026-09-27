#include "Vessel/VesselDecoration.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/**
		 * The relative luminance of linear sRGB light: the Rec. 709 weights of its primaries under D65 (IEC 61966-2-1,
		 * the sRGB standard, which shares Rec. 709's primaries).
		 */
		constexpr double LuminanceR = 0.2126;
		constexpr double LuminanceG = 0.7152;
		constexpr double LuminanceB = 0.0722;

		/** The share of a view's white a sample's lightness may stray from it and still count towards its white's colour. */
		constexpr double WhiteBand = 0.05;

		/** The grid a view's white is sampled on: so many angles round the outside, so many places along it. */
		constexpr int32 WhiteAngles = 256;
		constexpr int32 WhitePlaces = 128;

		double Luminance(const FLinearColor& Colour)
		{
			return LuminanceR * Colour.R + LuminanceG * Colour.G + LuminanceB * Colour.B;
		}

		/** The linear light of each 8-bit sRGB level. */
		const TArray<float>& LinearLevels()
		{
			static const TArray<float> Levels = []()
			{
				TArray<float> Out;
				for (int32 Level = 0; Level < 256; ++Level)
				{
					Out.Add(FLinearColor::FromSRGBColor(FColor(uint8(Level), uint8(Level), uint8(Level))).R);
				}
				return Out;
			}();
			return Levels;
		}

		/** The photograph's colour at a place (pixel centres at +0.5), bilinear in linear light; false off the photograph. */
		bool Sample(const FImage& Image, const FVector2D& Pixel, FLinearColor& Out)
		{
			const double X = Pixel.X - 0.5;
			const double Y = Pixel.Y - 0.5;
			if (Image.SizeX < 2 || Image.SizeY < 2 || X < 0.0 || Y < 0.0 || X > Image.SizeX - 1 || Y > Image.SizeY - 1)
			{
				return false;
			}
			const int32 X0 = FMath::Min(FMath::FloorToInt32(X), Image.SizeX - 2);
			const int32 Y0 = FMath::Min(FMath::FloorToInt32(Y), Image.SizeY - 2);
			const float Fx = float(X - X0);
			const float Fy = float(Y - Y0);
			const TArrayView64<const FColor> Colours = Image.AsBGRA8();
			const TArray<float>& Levels = LinearLevels();
			auto At = [&](int32 Px, int32 Py)
			{
				const FColor& C = Colours[int64(Py) * Image.SizeX + Px];
				return FLinearColor(Levels[C.R], Levels[C.G], Levels[C.B], 1.0f);
			};
			const FLinearColor Top = FMath::Lerp(At(X0, Y0), At(X0 + 1, Y0), Fx);
			const FLinearColor Bottom = FMath::Lerp(At(X0, Y0 + 1), At(X0 + 1, Y0 + 1), Fx);
			Out = FMath::Lerp(Top, Bottom, Fy);
			return true;
		}

		/** A place on the vessel's outside: its point and its outward normal in the vessel's frame. */
		struct FSurfacePoint
		{
			FVector Point;
			FVector Normal;
		};

		/**
		 * Each point's normal: the mean of its two segments' (an end's, its one segment's), so the surface's normal turns
		 * smoothly along the section as its mesh's does, and how squarely a view sees it doesn't jump from ring to ring.
		 */
		TArray<FVector2D> PointNormals(TConstArrayView<FVector2D> Section)
		{
			TArray<FVector2D> Out;
			const int32 Last = Section.Num() - 1;
			for (int32 Index = 0; Index <= Last; ++Index)
			{
				const FVector2D Sum = NormalOf(Section, FMath::Clamp(Index - 1, 0, Last - 1)) + NormalOf(Section, FMath::Clamp(Index, 0, Last - 1));
				Out.Add(Sum.IsNearlyZero() ? NormalOf(Section, FMath::Clamp(Index, 0, Last - 1)) : Sum.GetSafeNormal());
			}
			return Out;
		}

		/** The record's section at a place along it, turned to an angle from the front towards the right. */
		FSurfacePoint Surface(TConstArrayView<FVector2D> Section, TConstArrayView<FVector2D> Normals, const FSectionPlace& Place, double Angle)
		{
			const FVector2D Point = PointAt(Section, Place);
			const FVector2D Normal = FMath::Lerp(Normals[Place.Segment], Normals[Place.Segment + 1], Place.Along).GetSafeNormal();
			const double S = FMath::Sin(Angle);
			const double C = FMath::Cos(Angle);
			return { FVector(Point.X * S, Point.X * C, Point.Y), FVector(Normal.X * S, Normal.X * C, Normal.Y) };
		}

		/** How squarely a camera sees a place: the cosine between its normal and the line of sight to the camera. */
		double Squareness(const FVesselCamera& Camera, const FSurfacePoint& At)
		{
			return FVector::DotProduct(At.Normal, (Camera.Position() - At.Point).GetSafeNormal());
		}

		bool IsOutside(const FArtVessel& Vessel, int32 Segment)
		{
			return Vessel.Outside.Num() == 2 && Segment >= Vessel.Outside[0] && Segment < Vessel.Outside[1];
		}

		/**
		 * A photograph's white: the glaze's colour as it shows it (its hue, at a lightness of 1), and the glaze's lightness
		 * over the vessel as its light falls: a plane in the surface's normal, fitted where the photograph shows plain glaze,
		 * so dividing by it takes the photograph's shading off and leaves the painting.
		 */
		struct FViewWhite
		{
			FLinearColor Colour = FLinearColor::White;
			double Constant = 1.0;
			FVector Slope = FVector::ZeroVector;
			double Least = 0.0;

			/** The glaze's lightness where the surface faces this way. */
			double LightnessAt(const FVector& Normal) const
			{
				return FMath::Max(Constant + FVector::DotProduct(Slope, Normal), Least);
			}
		};

		/** A sample of a photograph on the vessel: its colour, and the surface's normal there. */
		struct FWhiteSample
		{
			FLinearColor Colour;
			FVector Normal;
		};

		/** The fits of the glaze's lightness: this many rounds, each leaving out the samples the painting darkens. */
		constexpr int32 ShadeRounds = 3;

		/** The fewest plain-glaze samples a plane of four numbers is fitted to: four for each, as a surveyor's rule. */
		constexpr int32 ShadeLeastSamples = 16;

		/**
		 * The plane's slope is held towards level by this share of the samples' weight, so a view that sees the vessel from
		 * one side only (its normals all alike) keeps a flat white rather than an unfounded tilt.
		 */
		constexpr double ShadeRidge = 1e-3;

		/** Solves the 4 x 4 system A x = B by Gaussian elimination with partial pivoting; false when it has no solution. */
		bool Solve4(double A[4][4], double B[4], double Out[4])
		{
			for (int32 Column = 0; Column < 4; ++Column)
			{
				int32 Pivot = Column;
				for (int32 Row = Column + 1; Row < 4; ++Row)
				{
					Pivot = FMath::Abs(A[Row][Column]) > FMath::Abs(A[Pivot][Column]) ? Row : Pivot;
				}
				if (FMath::Abs(A[Pivot][Column]) < UE_DOUBLE_SMALL_NUMBER)
				{
					return false;
				}
				for (int32 K = 0; K < 4; ++K)
				{
					Swap(A[Column][K], A[Pivot][K]);
				}
				Swap(B[Column], B[Pivot]);
				for (int32 Row = Column + 1; Row < 4; ++Row)
				{
					const double Factor = A[Row][Column] / A[Column][Column];
					for (int32 K = Column; K < 4; ++K)
					{
						A[Row][K] -= Factor * A[Column][K];
					}
					B[Row] -= Factor * B[Column];
				}
			}
			for (int32 Row = 3; Row >= 0; --Row)
			{
				double Sum = B[Row];
				for (int32 K = Row + 1; K < 4; ++K)
				{
					Sum -= A[Row][K] * Out[K];
				}
				Out[Row] = Sum / A[Row][Row];
			}
			return true;
		}

		/**
		 * The glaze's lightness as a plane in the normal, Constant + Slope . Normal, fitted to the plain-glaze samples by
		 * least squares; each round leaves out the samples more than a standard deviation below the plane (the painting
		 * darkens the glaze; nothing lightens it but a highlight, which the white's quantile keeps out). False with too few.
		 */
		bool FitShade(TArray<FWhiteSample> Plain, FViewWhite& White)
		{
			for (int32 Round = 0; Round < ShadeRounds; ++Round)
			{
				if (Plain.Num() < ShadeLeastSamples)
				{
					return Round > 0;
				}
				double A[4][4] = {};
				double B[4] = {};
				for (const FWhiteSample& Glaze : Plain)
				{
					const double X[4] = { 1.0, Glaze.Normal.X, Glaze.Normal.Y, Glaze.Normal.Z };
					const double Y = Luminance(Glaze.Colour);
					for (int32 I = 0; I < 4; ++I)
					{
						for (int32 J = 0; J < 4; ++J)
						{
							A[I][J] += X[I] * X[J];
						}
						B[I] += X[I] * Y;
					}
				}
				for (int32 I = 1; I < 4; ++I)
				{
					A[I][I] += ShadeRidge * Plain.Num();
				}
				double Coefficients[4] = {};
				if (!Solve4(A, B, Coefficients))
				{
					return Round > 0;
				}
				White.Constant = Coefficients[0];
				White.Slope = FVector(Coefficients[1], Coefficients[2], Coefficients[3]);
				double Squares = 0.0;
				for (const FWhiteSample& Glaze : Plain)
				{
					Squares += FMath::Square(Luminance(Glaze.Colour) - (White.Constant + FVector::DotProduct(White.Slope, Glaze.Normal)));
				}
				const double Deviation = FMath::Sqrt(Squares / Plain.Num());
				Plain.RemoveAll([&White, Deviation](const FWhiteSample& Glaze)
				{
					return Luminance(Glaze.Colour) - (White.Constant + FVector::DotProduct(White.Slope, Glaze.Normal)) < -Deviation;
				});
			}
			return true;
		}

		/**
		 * A view's white. Its lightness: the WhiteQuantile of what its photograph shows where it sees the outside at least
		 * WhiteSquareness squarely (a painted vessel's ground is its glaze, and most of what a view sees face on); its colour
		 * the mean of the samples within WhiteBand of that. Its shading: the plane fitted where the view shows plain glaze
		 * (samples no more saturated than GlazeChroma and no darker than GlazeLightness of its white), never below
		 * ShadeFloor of its white; flat at its white where too few samples show glaze to fit.
		 */
		FViewWhite WhiteOf(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Normals, const FVesselLayout& Layout,
			const FVesselPicture& View)
		{
			TArray<FWhiteSample> Seen;
			TArray<double> Square;
			const FVector2D Size = Layout.SizeMeters();
			for (int32 Along = 0; Along < WhitePlaces; ++Along)
			{
				const FSectionPlace Place = PlaceAlong(Layout, (Along + 0.5) / WhitePlaces * Size.Y);
				if (!IsOutside(Vessel, Place.Segment))
				{
					continue;
				}
				for (int32 Round = 0; Round < WhiteAngles; ++Round)
				{
					const FSurfacePoint At = Surface(Vessel.Section, Normals, Place, 2.0 * UE_DOUBLE_PI * (Round + 0.5) / WhiteAngles);
					const double Facing = Squareness(View.Camera, At);
					FVector2D Pixel;
					FLinearColor Colour;
					if (Facing >= How.SquarenessCut && View.Camera.Project(At.Point, Pixel) && Sample(View.Image, Pixel, Colour))
					{
						Seen.Add(FWhiteSample{ Colour, At.Normal });
						Square.Add(Facing);
					}
				}
			}
			FViewWhite White;
			TArray<double> Lights;
			for (int32 Index = 0; Index < Seen.Num(); ++Index)
			{
				if (Square[Index] >= How.WhiteSquareness)
				{
					Lights.Add(Luminance(Seen[Index].Colour));
				}
			}
			if (Lights.IsEmpty())
			{
				return White;
			}
			Lights.Sort();
			const double Lightness = FMath::Max(Lights[FMath::Clamp(int32(How.WhiteQuantile * Lights.Num()), 0, Lights.Num() - 1)], UE_DOUBLE_SMALL_NUMBER);
			FLinearColor Sum(0.0f, 0.0f, 0.0f, 0.0f);
			int32 Count = 0;
			TArray<FWhiteSample> Plain;
			for (int32 Index = 0; Index < Seen.Num(); ++Index)
			{
				const FLinearColor& Colour = Seen[Index].Colour;
				const double Light = Luminance(Colour);
				if (Square[Index] >= How.WhiteSquareness && FMath::Abs(Light / Lightness - 1.0) <= WhiteBand)
				{
					Sum += Colour;
					++Count;
				}
				const float Most = FMath::Max3(Colour.R, Colour.G, Colour.B);
				const float Least = FMath::Min3(Colour.R, Colour.G, Colour.B);
				if (Most > 0.0f && (Most - Least) / Most <= How.GlazeChroma && Light >= How.GlazeLightness * Lightness)
				{
					Plain.Add(Seen[Index]);
				}
			}
			const FLinearColor Mean = Count > 0 ? Sum / float(Count) : FLinearColor(float(Lightness), float(Lightness), float(Lightness), 1.0f);
			White.Colour = Mean / float(FMath::Max(Luminance(Mean), UE_DOUBLE_SMALL_NUMBER));
			White.Colour.A = 1.0f;
			White.Constant = Lightness;
			White.Least = How.ShadeFloor * Lightness;
			if (!FitShade(MoveTemp(Plain), White))
			{
				White.Constant = Lightness;
				White.Slope = FVector::ZeroVector;
			}
			return White;
		}

		/**
		 * The views' colour at a place of the outside, each divided channel by channel by its photograph's white there (its
		 * glaze's hue, at the lightness its light gives the glaze facing that way), so the glaze is white in every
		 * photograph, whatever its light and white balance: the glaze's own colour is Matter's. And their summed weight.
		 */
		void Gather(const FArtVesselComposition& How, TConstArrayView<FVesselPicture> Views, TConstArrayView<FViewWhite> Whites, const FSurfacePoint& At,
			FLinearColor& OutColour, double& OutWeight, FDecorationStats& Stats)
		{
			FLinearColor Sum(0.0f, 0.0f, 0.0f, 0.0f);
			OutWeight = 0.0;
			for (int32 Index = 0; Index < Views.Num(); ++Index)
			{
				const double Square = Squareness(Views[Index].Camera, At);
				FVector2D Pixel;
				FLinearColor Colour;
				if (Square < How.SquarenessCut || !Views[Index].Camera.Project(At.Point, Pixel) || !Sample(Views[Index].Image, Pixel, Colour))
				{
					continue;
				}
				const FViewWhite& White = Whites[Index];
				const float Lightness = float(White.LightnessAt(At.Normal));
				Colour = FLinearColor(Colour.R / FMath::Max(White.Colour.R * Lightness, UE_SMALL_NUMBER),
					Colour.G / FMath::Max(White.Colour.G * Lightness, UE_SMALL_NUMBER), Colour.B / FMath::Max(White.Colour.B * Lightness, UE_SMALL_NUMBER), 1.0f);
				const double Past = (Square - How.SquarenessCut) / (1.0 - How.SquarenessCut);
				const double Weight = Past * Past * (Luminance(Colour) > How.HighlightOverWhite ? How.HighlightWeight : 1.0);
				if (Weight > 0.0)
				{
					Sum += Colour * float(Weight);
					OutWeight += Weight;
					Stats.MostStretch = FMath::Max(Stats.MostStretch, 1.0 / Square);
				}
			}
			OutColour = OutWeight > 0.0 ? Sum / float(OutWeight) : FLinearColor::Black;
		}

		/** The band of ornament a height of the outside lies in, or none. */
		const FArtVesselBand* BandAt(const FArtVessel& Vessel, double Height)
		{
			return Vessel.Bands.FindByPredicate([Height](const FArtVesselBand& Band)
			{
				return Height >= Band.FromMeters && Height <= Band.ToMeters && Band.PeriodDegrees > 0.0;
			});
		}

		/**
		 * A band's own repeat at a column: the colours the views gave the columns a whole number of repeats away, each by
		 * the weight they were given it with, so where one repeat's sight gives way to another's they cross-fade rather
		 * than meet at a seam; and their summed weight (zero where no repeat was seen).
		 */
		FLinearColor RepeatAt(const FArtVesselBand& Band, int32 Column, TConstArrayView<FLinearColor> Colours, TConstArrayView<double> Weights,
			double& OutWeight)
		{
			const int32 Width = Weights.Num();
			const int32 Repeats = FMath::Max(1, FMath::RoundToInt32(360.0 / Band.PeriodDegrees));
			FLinearColor Sum(0.0f, 0.0f, 0.0f, 0.0f);
			OutWeight = 0.0;
			for (int32 Repeat = 1; Repeat < Repeats; ++Repeat)
			{
				const int32 Other = (Column + FMath::RoundToInt32(double(Repeat) * Width / Repeats)) % Width;
				Sum += Colours[Other] * float(Weights[Other]);
				OutWeight += Weights[Other];
			}
			return OutWeight > 0.0 ? Sum / float(OutWeight) : FLinearColor::White;
		}
	}

	FVector FVesselCamera::Position() const
	{
		return FVector(DistanceMeters * FMath::Sin(AzimuthRadians), DistanceMeters * FMath::Cos(AzimuthRadians), HeightMeters);
	}

	bool FVesselCamera::Project(const FVector& Point, FVector2D& OutPixel) const
	{
		// Turn the vessel so the camera stands on its +y, then look down the pitch towards the axis.
		const double Ca = FMath::Cos(AzimuthRadians);
		const double Sa = FMath::Sin(AzimuthRadians);
		const double X = Point.X * Ca - Point.Y * Sa;
		const double Y = Point.X * Sa + Point.Y * Ca;
		const double Dy = Y - DistanceMeters;
		const double Dz = Point.Z - HeightMeters;
		const double Cp = FMath::Cos(PitchRadians);
		const double Sp = FMath::Sin(PitchRadians);
		const double Depth = -Dy * Cp - Dz * Sp;
		const double Up = -Dy * Sp + Dz * Cp;
		if (Depth <= UE_DOUBLE_KINDA_SMALL_NUMBER)
		{
			return false;
		}
		OutPixel = FVector2D(AxisPixels.X + FocalPixels * X / Depth, AxisPixels.Y - FocalPixels * Up / Depth);
		return true;
	}

	FVesselCamera CameraOf(const FArtVesselView& View)
	{
		FVesselCamera Camera;
		Camera.AzimuthRadians = FMath::DegreesToRadians(View.AzimuthDegrees);
		Camera.DistanceMeters = View.DistanceMeters;
		Camera.HeightMeters = View.HeightMeters;
		Camera.PitchRadians = FMath::DegreesToRadians(View.PitchDegrees);
		Camera.FocalPixels = View.FocalPixels;
		Camera.AxisPixels = View.AxisPixels.Num() == 2 ? FVector2D(View.AxisPixels[0], View.AxisPixels[1]) : FVector2D::ZeroVector;
		return Camera;
	}

	FIntPoint DecorationPixels(const FArtVesselComposition& How, const FArtVessel& Vessel, const FVesselLayout& Layout)
	{
		// A view resolves focal / distance pixels a metre at the vessel's axis.
		double Finest = 0.0;
		for (const FArtVesselView& View : Vessel.Views)
		{
			Finest = FMath::Max(Finest, View.DistanceMeters > 0.0 ? View.FocalPixels / View.DistanceMeters : 0.0);
		}
		const FVector2D Size = Layout.SizeMeters() * FMath::Min(Finest, How.MostPixelsPerMeter);
		const double Fit = FMath::Min(1.0, double(How.MostPixels) / FMath::Max(Size.GetMax(), 1.0));
		return FIntPoint(FMath::Max(2, FMath::RoundToInt32(Size.X * Fit)), FMath::Max(2, FMath::RoundToInt32(Size.Y * Fit)));
	}

	void ComposeDecoration(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Sized,
		TConstArrayView<FVesselPicture> Views, FIntPoint Pixels, FImage& Out, FDecorationStats& OutStats)
	{
		OutStats = FDecorationStats();
		const FVesselLayout Layout = LayoutOf(Sized);
		const TArray<FVector2D> Normals = PointNormals(Vessel.Section);
		TArray<FViewWhite> Whites;
		for (const FVesselPicture& View : Views)
		{
			Whites.Add(WhiteOf(How, Vessel, Normals, Layout, View));
		}
		// Every photograph's white is the glaze's: where no view lends its colour, the glaze.
		const FLinearColor Glaze = FLinearColor::White;
		const FColor GlazePixel = Glaze.ToFColor(true);

		Out.Init(Pixels.X, Pixels.Y, 1, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		const TArrayView64<FColor> Dest = Out.AsBGRA8();
		const FVector2D Size = Layout.SizeMeters();
		TArray<FLinearColor> RowColours;
		TArray<double> RowWeights;
		RowColours.SetNum(Pixels.X);
		RowWeights.SetNum(Pixels.X);
		int64 OutsideTexels = 0;
		int64 SeenTexels = 0;
		int64 RepeatedTexels = 0;
		for (int32 Row = 0; Row < Pixels.Y; ++Row)
		{
			const FSectionPlace Place = PlaceAlong(Layout, (Row + 0.5) / Pixels.Y * Size.Y);
			FColor* const Line = &Dest[int64(Row) * Pixels.X];
			if (!IsOutside(Vessel, Place.Segment))
			{
				for (int32 Column = 0; Column < Pixels.X; ++Column)
				{
					Line[Column] = GlazePixel;
				}
				continue;
			}
			for (int32 Column = 0; Column < Pixels.X; ++Column)
			{
				const FSurfacePoint At = Surface(Vessel.Section, Normals, Place, 2.0 * UE_DOUBLE_PI * (Column + 0.5) / Pixels.X);
				Gather(How, Views, Whites, At, RowColours[Column], RowWeights[Column], OutStats);
			}
			const FArtVesselBand* Band = BandAt(Vessel, PointAt(Vessel.Section, Place).Y);
			for (int32 Column = 0; Column < Pixels.X; ++Column)
			{
				const double Whole = FMath::Clamp(RowWeights[Column] / How.FullWeight, 0.0, 1.0);
				FLinearColor Fill = Glaze;
				if (Band && Whole < 1.0)
				{
					double RepeatWeight = 0.0;
					const FLinearColor Repeat = RepeatAt(*Band, Column, RowColours, RowWeights, RepeatWeight);
					Fill = FMath::Lerp(Glaze, Repeat, float(FMath::Clamp(RepeatWeight / How.FullWeight, 0.0, 1.0)));
					RepeatedTexels += RepeatWeight > 0.0 ? 1 : 0;
				}
				FLinearColor Colour = FMath::Lerp(Fill, RowColours[Column], float(Whole));
				Colour.A = 1.0f;
				Line[Column] = Colour.GetClamped().ToFColor(true);
				SeenTexels += Whole >= 1.0 ? 1 : 0;
			}
			OutsideTexels += Pixels.X;
		}
		OutStats.Seen = OutsideTexels > 0 ? double(SeenTexels) / OutsideTexels : 0.0;
		OutStats.Repeated = OutsideTexels > 0 ? double(RepeatedTexels) / OutsideTexels : 0.0;
	}
}
