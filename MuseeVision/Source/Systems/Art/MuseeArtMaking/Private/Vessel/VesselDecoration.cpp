#include "Vessel/VesselDecoration.h"

#include "Algo/BinarySearch.h"
#include "Async/ParallelFor.h"
#include "Misc/Paths.h"

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

		/** The vessel's outer envelope is known at so many heights, from its foot to its top. */
		constexpr int32 EnvelopeHeights = 2048;

		/** A line of sight is followed through the vessel in steps of this share of its widest radius. */
		constexpr double SightStepOfRadius = 1.0 / 128.0;

		/**
		 * A place shows inside the vessel's silhouette by a margin where the lines of sight through the photograph that far
		 * from it, every way round (so many ways, a pixel's eight neighbours'), all pass through the vessel: past a limb or
		 * past a crease (a foot's edge, a rim's) alike.
		 */
		constexpr int32 EdgeDirections = 8;

		/**
		 * The vessel's outer envelope: the widest its section reaches at each height (RadiusAt), a solid of revolution no
		 * line of sight passes through where the vessel is.
		 */
		struct FEnvelope
		{
			double Bottom = 0.0;
			double Top = 0.0;
			double Most = 0.0;
			double Step = 0.0;
			TArray<double> Radius;

			/** Its lines of sight are followed in steps of SightStepOfRadius of its widest radius, or half the clearance if finer. */
			FEnvelope(TConstArrayView<FVector2D> Section, double ClearanceMeters)
			{
				Bottom = TNumericLimits<double>::Max();
				Top = -TNumericLimits<double>::Max();
				for (const FVector2D& Point : Section)
				{
					Bottom = FMath::Min(Bottom, Point.Y);
					Top = FMath::Max(Top, Point.Y);
					Most = FMath::Max(Most, Point.X);
				}
				Step = FMath::Min(Most * SightStepOfRadius, 0.5 * ClearanceMeters);
				Radius.SetNum(EnvelopeHeights);
				for (int32 Index = 0; Index < EnvelopeHeights; ++Index)
				{
					Radius[Index] = RadiusAt(Section, Bottom + (Top - Bottom) * Index / (EnvelopeHeights - 1));
				}
			}

			/** The envelope's radius at a height within it. */
			double At(double Height) const
			{
				const double Place = FMath::Clamp((Height - Bottom) / FMath::Max(Top - Bottom, UE_DOUBLE_SMALL_NUMBER), 0.0, 1.0) * (EnvelopeHeights - 1);
				const int32 Below = FMath::Min(FMath::FloorToInt32(Place), EnvelopeHeights - 2);
				return FMath::Lerp(Radius[Below], Radius[Below + 1], Place - Below);
			}

			/**
			 * Where a line From + t Direction (Direction a unit vector), t from 0 to Length, runs within the envelope's bounds
			 * (its widest radius and its heights): [OutFrom, OutTo]; false where it never does.
			 */
			bool Span(const FVector& From, const FVector& Direction, double Length, double& OutFrom, double& OutTo) const
			{
				OutFrom = 0.0;
				OutTo = Length;
				if (FMath::Abs(Direction.Z) > UE_DOUBLE_SMALL_NUMBER)
				{
					const double A = (Bottom - From.Z) / Direction.Z;
					const double B = (Top - From.Z) / Direction.Z;
					OutFrom = FMath::Max(OutFrom, FMath::Min(A, B));
					OutTo = FMath::Min(OutTo, FMath::Max(A, B));
				}
				else if (From.Z < Bottom || From.Z > Top)
				{
					return false;
				}
				const double A = Direction.X * Direction.X + Direction.Y * Direction.Y;
				const double B = 2.0 * (From.X * Direction.X + From.Y * Direction.Y);
				const double C = From.X * From.X + From.Y * From.Y - Most * Most;
				if (A > UE_DOUBLE_SMALL_NUMBER)
				{
					const double Discriminant = B * B - 4.0 * A * C;
					if (Discriminant < 0.0)
					{
						return false;
					}
					const double Root = FMath::Sqrt(Discriminant);
					OutFrom = FMath::Max(OutFrom, (-B - Root) / (2.0 * A));
					OutTo = FMath::Min(OutTo, (-B + Root) / (2.0 * A));
				}
				else if (C > 0.0)
				{
					return false;
				}
				return OutFrom <= OutTo;
			}

			/**
			 * The least of a line's distance from the axis less the envelope's radius, from t = From to To in steps; it stops
			 * once it is Enough (a line that far inside the vessel is wholly in it).
			 */
			double LeastGap(const FVector& Origin, const FVector& Direction, double From, double To, double Enough) const
			{
				double Least = TNumericLimits<double>::Max();
				const int32 Steps = FMath::Max(1, FMath::CeilToInt32((To - From) / Step));
				for (int32 K = 0; K <= Steps && Least > Enough; ++K)
				{
					const FVector Point = Origin + Direction * FMath::Lerp(From, To, double(K) / Steps);
					if (Point.Z >= Bottom && Point.Z <= Top)
					{
						Least = FMath::Min(Least, FVector2D(Point.X, Point.Y).Size() - At(Point.Z));
					}
				}
				return Least;
			}
		};

		/**
		 * How fully a view sees a place of the outside, 0 to 1 (its squareness aside): nothing where it faces away; where
		 * it faces the camera, the line of sight must pass the rest of the vessel clear (coming in over ClearanceMeters as
		 * it clears it), and the place must show inside the vessel's silhouette by the camera's miss and an edge's blur:
		 * the lines through the photograph that much further from it, every way round (EdgeDirections), must pass through
		 * the vessel (coming in over ClearanceMeters of it).
		 */
		double SightFactor(const FArtVesselComposition& How, const FEnvelope& Envelope, const FVesselCamera& Camera, const FSurfacePoint& At)
		{
			const FVector Eye = Camera.Position();
			const FVector ToEye = Eye - At.Point;
			const double Far = ToEye.Size();
			const FVector Toward = ToEye / FMath::Max(Far, UE_DOUBLE_SMALL_NUMBER);
			FVector2D Pixel;
			if (FVector::DotProduct(At.Normal, Toward) <= 0.0 || !Camera.Project(At.Point, Pixel))
			{
				return 0.0;
			}
			double From = 0.0;
			double To = 0.0;
			double Clear = 1.0;
			if (Envelope.Span(At.Point, Toward, Far, From, To) && To > Envelope.Step)
			{
				const double Least = Envelope.LeastGap(At.Point, Toward, FMath::Max(From, Envelope.Step), To, -How.ClearanceMeters);
				Clear = FMath::Clamp(1.0 + Least / How.ClearanceMeters, 0.0, 1.0);
			}
			double Edge = 1.0;
			const double Margin = Camera.FitPixels + How.EdgePixels;
			for (int32 Way = 0; Way < EdgeDirections && Clear * Edge > 0.0; ++Way)
			{
				const double Angle = 2.0 * UE_DOUBLE_PI * Way / EdgeDirections;
				const FVector Line = Camera.Sight(Pixel + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Margin);
				const double Into = Envelope.Span(Eye, Line, TNumericLimits<double>::Max(), From, To)
					? -Envelope.LeastGap(Eye, Line, From, To, -How.ClearanceMeters) : 0.0;
				Edge = FMath::Min(Edge, FMath::Clamp(Into / How.ClearanceMeters, 0.0, 1.0));
			}
			return Clear * Edge;
		}

		/**
		 * A view's sight of the outside is found on a grid of so many angles round and places along it: where the four
		 * cells round a place agree (all seen whole, or none), it is theirs; else, near the edge of what the view sees, it
		 * is found at the place itself.
		 */
		constexpr int32 SightColumns = 512;
		constexpr int32 SightRows = 256;

		/** How fully a view sees each place of the outside (SightFactor), on the sight grid. */
		struct FSightMap
		{
			/** The outside's first and last lengths along the layout. */
			double From = 0.0;
			double To = 0.0;
			TArray<float> Factor;

			/**
			 * The four cells round a share of the turn from the front and a length along the layout (wrapping round): true,
			 * with their one value, where they agree.
			 */
			bool Agreed(double Turn, double Length, double& OutFactor) const
			{
				const double X = Turn * SightColumns - 0.5;
				const double Y = FMath::Clamp((Length - From) / FMath::Max(To - From, UE_DOUBLE_SMALL_NUMBER) * SightRows - 0.5, 0.0, SightRows - 1.0);
				const int32 X0 = FMath::FloorToInt32(X);
				const int32 Y0 = FMath::Min(FMath::FloorToInt32(Y), SightRows - 2);
				auto Cell = [this](int32 Column, int32 Row) { return Factor[Row * SightColumns + ((Column % SightColumns) + SightColumns) % SightColumns]; };
				const float Corner = Cell(X0, Y0);
				OutFactor = Corner;
				return Cell(X0 + 1, Y0) == Corner && Cell(X0, Y0 + 1) == Corner && Cell(X0 + 1, Y0 + 1) == Corner;
			}
		};

		FSightMap SightMapOf(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Normals, const FVesselLayout& Layout,
			const FEnvelope& Envelope, const FVesselCamera& Camera)
		{
			FSightMap Map;
			Map.Factor.SetNumZeroed(SightColumns * SightRows);
			if (Vessel.Outside.Num() != 2 || !Layout.Lengths.IsValidIndex(Vessel.Outside[0]) || !Layout.Lengths.IsValidIndex(Vessel.Outside[1]))
			{
				return Map;
			}
			Map.From = Layout.Lengths[Vessel.Outside[0]];
			Map.To = Layout.Lengths[Vessel.Outside[1]];
			ParallelFor(SightRows, [&](int32 Row)
			{
				const FSectionPlace Place = PlaceAlong(Layout, Map.From + (Row + 0.5) / SightRows * (Map.To - Map.From));
				for (int32 Column = 0; Column < SightColumns; ++Column)
				{
					const FSurfacePoint At = Surface(Vessel.Section, Normals, Place, 2.0 * UE_DOUBLE_PI * (Column + 0.5) / SightColumns);
					Map.Factor[Row * SightColumns + Column] = float(SightFactor(How, Envelope, Camera, At));
				}
			});
			return Map;
		}

		/**
		 * A photograph's white: the glaze's colour as it shows it (its hue, at a lightness of 1), and the glaze's lightness
		 * over the vessel as its light falls and as the view sees it: a plane in the surface's normal and in how squarely the
		 * view sees it (a glaze darkens or brightens towards the silhouette as the lamps round the camera light it and its
		 * surface reflects them), fitted where the photograph shows plain glaze, so dividing by it takes the photograph's
		 * shading off and leaves the painting.
		 */
		struct FViewWhite
		{
			FLinearColor Colour = FLinearColor::White;
			double Constant = 1.0;
			FVector Slope = FVector::ZeroVector;
			double Askance = 0.0;
			double Least = 0.0;

			/** The glaze's plane at a normal and a squareness, and its lightness there (never under Least). */
			double PlaneAt(const FVector& Normal, double Square) const
			{
				return Constant + FVector::DotProduct(Slope, Normal) + Askance * Square;
			}

			double LightnessAt(const FVector& Normal, double Square) const
			{
				return FMath::Max(PlaneAt(Normal, Square), Least);
			}
		};

		/** A sample of a photograph on the vessel: its colour, the surface's normal there, and how squarely the view sees it. */
		struct FWhiteSample
		{
			FLinearColor Colour;
			FVector Normal;
			double Square = 0.0;
		};

		/** The fits of the glaze's lightness: this many rounds, each leaving out the samples the painting darkens. */
		constexpr int32 ShadeRounds = 3;

		/** The plane's terms: one, the normal's three components, and the squareness. */
		constexpr int32 ShadeTerms = 5;

		/** The fewest plain-glaze samples the plane's terms are fitted to: four for each, as a surveyor's rule. */
		constexpr int32 ShadeLeastSamples = 4 * ShadeTerms;

		/**
		 * The plane's slopes are held towards level by this share of the samples' weight, so a view that sees the vessel
		 * from one side only (its normals all alike) keeps a flat white rather than an unfounded tilt.
		 */
		constexpr double ShadeRidge = 1e-3;

		/** Solves the N x N system A x = B by Gaussian elimination with partial pivoting; false when it has no solution. */
		template <int32 N>
		bool Solve(double (&A)[N][N], double (&B)[N], double (&Out)[N])
		{
			for (int32 Column = 0; Column < N; ++Column)
			{
				int32 Pivot = Column;
				for (int32 Row = Column + 1; Row < N; ++Row)
				{
					Pivot = FMath::Abs(A[Row][Column]) > FMath::Abs(A[Pivot][Column]) ? Row : Pivot;
				}
				if (FMath::Abs(A[Pivot][Column]) < UE_DOUBLE_SMALL_NUMBER)
				{
					return false;
				}
				for (int32 K = 0; K < N; ++K)
				{
					Swap(A[Column][K], A[Pivot][K]);
				}
				Swap(B[Column], B[Pivot]);
				for (int32 Row = Column + 1; Row < N; ++Row)
				{
					const double Factor = A[Row][Column] / A[Column][Column];
					for (int32 K = Column; K < N; ++K)
					{
						A[Row][K] -= Factor * A[Column][K];
					}
					B[Row] -= Factor * B[Column];
				}
			}
			for (int32 Row = N - 1; Row >= 0; --Row)
			{
				double Sum = B[Row];
				for (int32 K = Row + 1; K < N; ++K)
				{
					Sum -= A[Row][K] * Out[K];
				}
				Out[Row] = Sum / A[Row][Row];
			}
			return true;
		}

		/**
		 * The glaze's lightness as a plane, Constant + Slope . Normal + Askance * Square, fitted to the plain-glaze samples
		 * by least squares; each round leaves out the samples more than a standard deviation below the plane (the painting
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
				double A[ShadeTerms][ShadeTerms] = {};
				double B[ShadeTerms] = {};
				for (const FWhiteSample& Glaze : Plain)
				{
					const double X[ShadeTerms] = { 1.0, Glaze.Normal.X, Glaze.Normal.Y, Glaze.Normal.Z, Glaze.Square };
					const double Y = Luminance(Glaze.Colour);
					for (int32 I = 0; I < ShadeTerms; ++I)
					{
						for (int32 J = 0; J < ShadeTerms; ++J)
						{
							A[I][J] += X[I] * X[J];
						}
						B[I] += X[I] * Y;
					}
				}
				for (int32 I = 1; I < ShadeTerms; ++I)
				{
					A[I][I] += ShadeRidge * Plain.Num();
				}
				double Coefficients[ShadeTerms] = {};
				if (!Solve(A, B, Coefficients))
				{
					return Round > 0;
				}
				White.Constant = Coefficients[0];
				White.Slope = FVector(Coefficients[1], Coefficients[2], Coefficients[3]);
				White.Askance = Coefficients[4];
				double Squares = 0.0;
				for (const FWhiteSample& Glaze : Plain)
				{
					Squares += FMath::Square(Luminance(Glaze.Colour) - White.PlaneAt(Glaze.Normal, Glaze.Square));
				}
				const double Deviation = FMath::Sqrt(Squares / Plain.Num());
				Plain.RemoveAll([&White, Deviation](const FWhiteSample& Glaze)
				{
					return Luminance(Glaze.Colour) - White.PlaneAt(Glaze.Normal, Glaze.Square) < -Deviation;
				});
			}
			return true;
		}

		/**
		 * A view's white, from what its photograph shows where it sees the outside whole (its sight map's factor 1). Its
		 * lightness: the WhiteQuantile of what it shows where it sees the outside at least WhiteSquareness squarely (a
		 * painted porcelain's ground is its glaze, and most of what a view sees face on), or of all it sees whole where it
		 * sees too little so squarely (a view from under the vessel); its colour the mean of those samples within WhiteBand
		 * of that. Its shading: the plane fitted where the view shows plain glaze (samples no more saturated than
		 * GlazeChroma and no darker than GlazeLightness of its white), never below ShadeFloor of its white; flat at its
		 * white where too few samples show glaze to fit.
		 */
		FViewWhite WhiteOf(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Normals, const FVesselLayout& Layout,
			const FSightMap& Sight, const FVesselPicture& View)
		{
			TArray<FWhiteSample> Seen;
			TArray<double> Square;
			const FVector2D Size = Layout.SizeMeters();
			for (int32 Along = 0; Along < WhitePlaces; ++Along)
			{
				const double Length = (Along + 0.5) / WhitePlaces * Size.Y;
				const FSectionPlace Place = PlaceAlong(Layout, Length);
				if (!IsOutside(Vessel, Place.Segment))
				{
					continue;
				}
				for (int32 Round = 0; Round < WhiteAngles; ++Round)
				{
					const double Turn = (Round + 0.5) / WhiteAngles;
					const FSurfacePoint At = Surface(Vessel.Section, Normals, Place, 2.0 * UE_DOUBLE_PI * Turn);
					const double Facing = Squareness(View.Camera, At);
					FVector2D Pixel;
					FLinearColor Colour;
					double Fully = 0.0;
					if (Facing > 0.0 && Sight.Agreed(Turn, Length, Fully) && Fully >= 1.0 && View.Camera.Project(At.Point, Pixel) && Sample(View.Image, Pixel, Colour))
					{
						Seen.Add(FWhiteSample{ Colour, At.Normal, Facing });
						Square.Add(Facing);
					}
				}
			}
			FViewWhite White;
			double Squarely = How.WhiteSquareness;
			TArray<double> Lights;
			for (int32 Index = 0; Index < Seen.Num(); ++Index)
			{
				if (Square[Index] >= Squarely)
				{
					Lights.Add(Luminance(Seen[Index].Colour));
				}
			}
			if (Lights.Num() < ShadeLeastSamples)
			{
				Squarely = 0.0;
				Lights.Reset();
				for (const FWhiteSample& Glaze : Seen)
				{
					Lights.Add(Luminance(Glaze.Colour));
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
				if (Square[Index] >= Squarely && FMath::Abs(Light / Lightness - 1.0) <= WhiteBand)
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
				White.Askance = 0.0;
			}
			return White;
		}

		/**
		 * The views' colour at a place of the outside, each divided channel by channel by its photograph's white there (its
		 * glaze's hue, at the lightness its light gives the glaze facing that way), so the glaze is white in every
		 * photograph, whatever its light and white balance: the glaze's own colour is Matter's. Each view that sees the
		 * place lends by the square of how squarely it sees it, times how fully (its sight map); and their summed weight.
		 */
		void Gather(const FArtVesselComposition& How, TConstArrayView<FVesselPicture> Views, TConstArrayView<FViewWhite> Whites,
			TConstArrayView<FSightMap> Sights, const FEnvelope& Envelope, const FSurfacePoint& At, double Turn, double Length, FLinearColor& OutColour,
			double& OutWeight)
		{
			FLinearColor Sum(0.0f, 0.0f, 0.0f, 0.0f);
			OutWeight = 0.0;
			for (int32 Index = 0; Index < Views.Num(); ++Index)
			{
				const double Square = Squareness(Views[Index].Camera, At);
				if (Square <= 0.0)
				{
					continue;
				}
				double Fully = 0.0;
				if (!Sights[Index].Agreed(Turn, Length, Fully))
				{
					Fully = SightFactor(How, Envelope, Views[Index].Camera, At);
				}
				FVector2D Pixel;
				FLinearColor Colour;
				if (Fully <= 0.0 || !Views[Index].Camera.Project(At.Point, Pixel) || !Sample(Views[Index].Image, Pixel, Colour))
				{
					continue;
				}
				const FViewWhite& White = Whites[Index];
				const float Lightness = float(White.LightnessAt(At.Normal, Square));
				Colour = FLinearColor(Colour.R / FMath::Max(White.Colour.R * Lightness, UE_SMALL_NUMBER),
					Colour.G / FMath::Max(White.Colour.G * Lightness, UE_SMALL_NUMBER), Colour.B / FMath::Max(White.Colour.B * Lightness, UE_SMALL_NUMBER), 1.0f);
				const double Weight = Square * Square * Fully * (Luminance(Colour) > How.HighlightOverWhite ? How.HighlightWeight : 1.0);
				Sum += Colour * float(Weight);
				OutWeight += Weight;
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

		// -------------------------------------------------------------------------------------------------------------
		// The views' overlaps, as the derivation registers them (MuseeVision/Scripts/vessels.py: unwrap, detail, weights,
		// correlation).
		// -------------------------------------------------------------------------------------------------------------

		/** The outside's profile up its height, as the derivation unwraps it: its heights, rising, and its radius at each. */
		void ProfileOf(const FArtVessel& Vessel, TArray<double>& OutHeights, TArray<double>& OutRadii)
		{
			for (int32 Index = Vessel.Outside[0]; Index <= FMath::Min(Vessel.Outside[1], Vessel.Section.Num() - 1); ++Index)
			{
				const FVector2D& Point = Vessel.Section[Index];
				if (OutHeights.IsEmpty() || Point.Y > OutHeights.Last())
				{
					OutHeights.Add(Point.Y);
					OutRadii.Add(Point.X);
				}
			}
		}

		/** Linear interpolation in a rising table, held at its ends (numpy's interp). */
		double Interpolate(TConstArrayView<double> Xs, TConstArrayView<double> Ys, double X)
		{
			if (X <= Xs[0])
			{
				return Ys[0];
			}
			if (X >= Xs.Last())
			{
				return Ys.Last();
			}
			const int32 Above = Algo::UpperBound(Xs, X);
			const double Share = (X - Xs[Above - 1]) / (Xs[Above] - Xs[Above - 1]);
			return FMath::Lerp(Ys[Above - 1], Ys[Above], Share);
		}

		/** The CIE L* of an sRGB colour given in 8-bit levels (0 to 255, not rounded), scaled 0 to 255 as OpenCV gives it. */
		double LightnessOf(double Blue, double Green, double Red)
		{
			auto Linear = [](double Level)
			{
				const double C = Level / 255.0;
				return C <= 0.04045 ? C / 12.92 : FMath::Pow((C + 0.055) / 1.055, 2.4);
			};
			const double Y = LuminanceR * Linear(Red) + LuminanceG * Linear(Green) + LuminanceB * Linear(Blue);
			// CIE 1976: L* = 116 f(Y) - 16, f the cube root above (6/29)^3 and linear below it.
			constexpr double Knee = 216.0 / 24389.0;
			constexpr double Slope = 24389.0 / 27.0;
			const double L = Y > Knee ? 116.0 * FMath::Pow(Y, 1.0 / 3.0) - 16.0 : Slope * Y;
			return L * 255.0 / 100.0;
		}

		/** Reflects an index into [0, Count) about its ends' cells (OpenCV's default border, BORDER_REFLECT_101). */
		int32 Reflect(int32 Index, int32 Count)
		{
			while (Index < 0 || Index >= Count)
			{
				Index = Index < 0 ? -Index : 2 * Count - 2 - Index;
			}
			return Index;
		}

		/**
		 * A grid blurred by a Gaussian of Sigma cells, as OpenCV's GaussianBlur blurs a float grid given its sigma: a kernel
		 * 8 sigma + 1 cells wide (made odd), the edges reflected.
		 */
		TArray<float> Blur(const TArray<float>& Grid, int32 Width, int32 Height, double Sigma)
		{
			const int32 Size = FMath::RoundToInt32(Sigma * 8.0 + 1.0) | 1;
			const int32 Half = Size / 2;
			TArray<double> Kernel;
			double Total = 0.0;
			for (int32 K = -Half; K <= Half; ++K)
			{
				Kernel.Add(FMath::Exp(-double(K * K) / (2.0 * Sigma * Sigma)));
				Total += Kernel.Last();
			}
			for (double& Weight : Kernel)
			{
				Weight /= Total;
			}
			TArray<float> Across;
			Across.SetNumUninitialized(Grid.Num());
			for (int32 Row = 0; Row < Height; ++Row)
			{
				for (int32 Column = 0; Column < Width; ++Column)
				{
					double Sum = 0.0;
					for (int32 K = -Half; K <= Half; ++K)
					{
						Sum += Kernel[K + Half] * Grid[Row * Width + Reflect(Column + K, Width)];
					}
					Across[Row * Width + Column] = float(Sum);
				}
			}
			TArray<float> Out;
			Out.SetNumUninitialized(Grid.Num());
			for (int32 Row = 0; Row < Height; ++Row)
			{
				for (int32 Column = 0; Column < Width; ++Column)
				{
					double Sum = 0.0;
					for (int32 K = -Half; K <= Half; ++K)
					{
						Sum += Kernel[K + Half] * Across[Reflect(Row + K, Height) * Width + Column];
					}
					Out[Row * Width + Column] = float(Sum);
				}
			}
			return Out;
		}

		/** A view's picture of the outside unwrapped as the derivation registers it: its detail and its weight in each cell. */
		struct FUnwrapped
		{
			TArray<float> Detail;
			TArray<float> Weight;
		};

		/**
		 * A view unwrapped: rows up the outside's height, columns round from the front towards the right; in each cell the
		 * lightness its photograph shows there (none off the photograph), band-passed between the fine and the coarse blur,
		 * and its weight, how far past the overlaps' cut it sees the cell squarely (none off the photograph).
		 */
		FUnwrapped UnwrapFor(const FArtVesselRegistration& How, TConstArrayView<double> Heights, TConstArrayView<double> Radii, const FVesselPicture& View)
		{
			const int32 Width = How.AzimuthSteps;
			const int32 Height = How.HeightSteps;
			const double Top = Heights.Last();
			TArray<double> Ring;
			for (int32 Row = 0; Row < Height; ++Row)
			{
				Ring.Add(Interpolate(Heights, Radii, (Row + 0.5) / Height * Top));
			}
			const double Cell = Top / Height;
			TArray<float> Lightness;
			FUnwrapped Out;
			Lightness.SetNumZeroed(Width * Height);
			Out.Weight.SetNumZeroed(Width * Height);
			const FVector Eye = View.Camera.Position();
			const TArrayView64<const FColor> Colours = View.Image.AsBGRA8();
			const int32 SizeX = View.Image.SizeX;
			const int32 SizeY = View.Image.SizeY;
			for (int32 Row = 0; Row < Height; ++Row)
			{
				// numpy's gradient: central differences inside, one-sided at the ends
				const double Slope = Row == 0 ? (Ring[1] - Ring[0]) / Cell
					: Row == Height - 1 ? (Ring[Row] - Ring[Row - 1]) / Cell : (Ring[Row + 1] - Ring[Row - 1]) / (2.0 * Cell);
				const double Across = 1.0 / FMath::Sqrt(1.0 + Slope * Slope);
				const double Z = (Row + 0.5) / Height * Top;
				for (int32 Column = 0; Column < Width; ++Column)
				{
					const double Phi = (Column + 0.5) / Width * 2.0 * UE_DOUBLE_PI;
					const double S = FMath::Sin(Phi);
					const double C = FMath::Cos(Phi);
					const FVector Point(Ring[Row] * S, Ring[Row] * C, Z);
					const FVector Normal(Across * S, Across * C, -Slope * Across);
					FVector2D Pixel;
					if (!View.Camera.Project(Point, Pixel) || Pixel.X <= 0.0 || Pixel.Y <= 0.0 || Pixel.X >= SizeX || Pixel.Y >= SizeY)
					{
						continue;
					}
					// the photograph's levels, bilinear about pixel centres (+0.5), as OpenCV's remap takes them
					const double X = FMath::Clamp(Pixel.X - 0.5, 0.0, SizeX - 1.0);
					const double Y = FMath::Clamp(Pixel.Y - 0.5, 0.0, SizeY - 1.0);
					const int32 X0 = FMath::Min(FMath::FloorToInt32(X), SizeX - 2);
					const int32 Y0 = FMath::Min(FMath::FloorToInt32(Y), SizeY - 2);
					const double Fx = X - X0;
					const double Fy = Y - Y0;
					auto Level = [&](int32 Channel)
					{
						auto At = [&](int32 Px, int32 Py)
						{
							const FColor& Colour = Colours[int64(Py) * SizeX + Px];
							return double(Channel == 0 ? Colour.B : Channel == 1 ? Colour.G : Colour.R);
						};
						return FMath::Lerp(FMath::Lerp(At(X0, Y0), At(X0 + 1, Y0), Fx), FMath::Lerp(At(X0, Y0 + 1), At(X0 + 1, Y0 + 1), Fx), Fy);
					};
					Lightness[Row * Width + Column] = float(LightnessOf(Level(0), Level(1), Level(2)));
					const double Square = FMath::Clamp(FVector::DotProduct(Normal, (Eye - Point).GetSafeNormal()), 0.0, 1.0);
					Out.Weight[Row * Width + Column] = float(FMath::Clamp((Square - How.OverlapCut) / (1.0 - How.OverlapCut), 0.0, 1.0));
				}
			}
			const TArray<float> Fine = Blur(Lightness, Width, Height, How.FineCells);
			const TArray<float> Coarse = Blur(Lightness, Width, Height, How.CoarseCells);
			Out.Detail.SetNumUninitialized(Width * Height);
			for (int32 Index = 0; Index < Width * Height; ++Index)
			{
				Out.Detail[Index] = Fine[Index] - Coarse[Index];
			}
			return Out;
		}

		/** Two unwrapped views' overlap: the mean of their weights' product, and their detail's weighted correlation there. */
		FViewOverlap OverlapOf(const FArtVesselRegistration& How, const FUnwrapped& A, const FUnwrapped& B)
		{
			FViewOverlap Out;
			const int32 Count = A.Weight.Num();
			double Total = 0.0;
			double MeanA = 0.0;
			double MeanB = 0.0;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const double W = double(A.Weight[Index]) * B.Weight[Index];
				Total += W;
				MeanA += W * A.Detail[Index];
				MeanB += W * B.Detail[Index];
			}
			Out.Share = Total / FMath::Max(Count, 1);
			if (Total < How.LeastShare * Count)
			{
				return Out;
			}
			MeanA /= Total;
			MeanB /= Total;
			double Product = 0.0;
			double SquaresA = 0.0;
			double SquaresB = 0.0;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const double W = double(A.Weight[Index]) * B.Weight[Index];
				const double Da = A.Detail[Index] - MeanA;
				const double Db = B.Detail[Index] - MeanB;
				Product += W * Da * Db;
				SquaresA += W * Da * Da;
				SquaresB += W * Db * Db;
			}
			// the derivation's guard against a flat picture's zero spread
			constexpr double Floor = 1e-9;
			Out.Correlation = Product / FMath::Sqrt(SquaresA * SquaresB + Floor);
			return Out;
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

	FVector FVesselCamera::Sight(const FVector2D& Pixel) const
	{
		// Project's steps undone for a point at unit depth: its pitch (an involution), then its turn.
		const double Across = (Pixel.X - AxisPixels.X) / FocalPixels;
		const double Up = -(Pixel.Y - AxisPixels.Y) / FocalPixels;
		const double Cp = FMath::Cos(PitchRadians);
		const double Sp = FMath::Sin(PitchRadians);
		const double Dy = -Cp - Sp * Up;
		const double Dz = -Sp + Cp * Up;
		const double Ca = FMath::Cos(AzimuthRadians);
		const double Sa = FMath::Sin(AzimuthRadians);
		return FVector(Across * Ca + Dy * Sa, -Across * Sa + Dy * Ca, Dz).GetSafeNormal();
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
		Camera.FitPixels = View.FitPixels;
		return Camera;
	}

	FIntPoint DecorationPixels(const FArtVesselComposition& How, const FArtVessel& Vessel, const FVesselLayout& Layout)
	{
		// A view resolves focal / distance pixels a metre at the vessel's middle, on its axis half its height up.
		double Top = 0.0;
		for (const FVector2D& Point : Vessel.Section)
		{
			Top = FMath::Max(Top, Point.Y);
		}
		double Finest = 0.0;
		for (const FArtVesselView& View : Vessel.Views)
		{
			const double Far = (CameraOf(View).Position() - FVector(0.0, 0.0, 0.5 * Top)).Size();
			Finest = FMath::Max(Finest, Far > 0.0 ? View.FocalPixels / Far : 0.0);
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
		const FEnvelope Envelope(Vessel.Section, How.ClearanceMeters);
		TArray<FSightMap> Sights;
		TArray<FViewWhite> Whites;
		for (const FVesselPicture& View : Views)
		{
			Sights.Add(SightMapOf(How, Vessel, Normals, Layout, Envelope, View.Camera));
			Whites.Add(WhiteOf(How, Vessel, Normals, Layout, Sights.Last(), View));
		}
		// Every photograph's white is the glaze's: where no view lends its colour, the glaze.
		const FLinearColor Glaze = FLinearColor::White;
		const FColor GlazePixel = Glaze.ToFColor(true);

		Out.Init(Pixels.X, Pixels.Y, 1, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		const TArrayView64<FColor> Dest = Out.AsBGRA8();
		const FVector2D Size = Layout.SizeMeters();
		TArray<int64> SeenRows;
		TArray<int64> RepeatedRows;
		TArray<uint8> OutsideRows;
		SeenRows.SetNumZeroed(Pixels.Y);
		RepeatedRows.SetNumZeroed(Pixels.Y);
		OutsideRows.SetNumZeroed(Pixels.Y);
		ParallelFor(Pixels.Y, [&](int32 Row)
		{
			const double Length = (Row + 0.5) / Pixels.Y * Size.Y;
			const FSectionPlace Place = PlaceAlong(Layout, Length);
			FColor* const Line = &Dest[int64(Row) * Pixels.X];
			if (!IsOutside(Vessel, Place.Segment))
			{
				for (int32 Column = 0; Column < Pixels.X; ++Column)
				{
					Line[Column] = GlazePixel;
				}
				return;
			}
			TArray<FLinearColor> RowColours;
			TArray<double> RowWeights;
			RowColours.SetNum(Pixels.X);
			RowWeights.SetNum(Pixels.X);
			for (int32 Column = 0; Column < Pixels.X; ++Column)
			{
				const double Turn = (Column + 0.5) / Pixels.X;
				const FSurfacePoint At = Surface(Vessel.Section, Normals, Place, 2.0 * UE_DOUBLE_PI * Turn);
				Gather(How, Views, Whites, Sights, Envelope, At, Turn, Length, RowColours[Column], RowWeights[Column]);
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
					RepeatedRows[Row] += RepeatWeight > 0.0 ? 1 : 0;
				}
				FLinearColor Colour = FMath::Lerp(Fill, RowColours[Column], float(Whole));
				Colour.A = 1.0f;
				Line[Column] = Colour.GetClamped().ToFColor(true);
				SeenRows[Row] += Whole >= 1.0 ? 1 : 0;
			}
			OutsideRows[Row] = 1;
		});
		int64 OutsideTexels = 0;
		int64 SeenTexels = 0;
		int64 RepeatedTexels = 0;
		for (int32 Row = 0; Row < Pixels.Y; ++Row)
		{
			OutsideTexels += OutsideRows[Row] ? Pixels.X : 0;
			SeenTexels += SeenRows[Row];
			RepeatedTexels += RepeatedRows[Row];
		}
		OutStats.Seen = OutsideTexels > 0 ? double(SeenTexels) / OutsideTexels : 0.0;
		OutStats.Repeated = OutsideTexels > 0 ? double(RepeatedTexels) / OutsideTexels : 0.0;
	}

	TArray<TArray<FViewOverlap>> ViewOverlaps(const FArtVesselRegistration& How, const FArtVessel& Vessel, TConstArrayView<FVesselPicture> Views)
	{
		TArray<TArray<FViewOverlap>> Out;
		Out.SetNum(Views.Num());
		for (TArray<FViewOverlap>& Row : Out)
		{
			Row.SetNum(Views.Num());
		}
		TArray<double> Heights;
		TArray<double> Radii;
		if (Vessel.Outside.Num() != 2 || How.AzimuthSteps < 2 || How.HeightSteps < 2)
		{
			return Out;
		}
		ProfileOf(Vessel, Heights, Radii);
		if (Heights.Num() < 2)
		{
			return Out;
		}
		TArray<FUnwrapped> Unwrapped;
		for (const FVesselPicture& View : Views)
		{
			Unwrapped.Add(UnwrapFor(How, Heights, Radii, View));
		}
		for (int32 I = 0; I < Views.Num(); ++I)
		{
			for (int32 J = I + 1; J < Views.Num(); ++J)
			{
				Out[I][J] = OverlapOf(How, Unwrapped[I], Unwrapped[J]);
				Out[J][I] = Out[I][J];
			}
		}
		return Out;
	}

	TArray<FString> PlacementFaults(const FArtVesselRegistration& How, const FArtVessel& Vessel, TConstArrayView<FVesselPicture> Views)
	{
		TArray<FString> Out;
		if (Views.Num() != Vessel.Views.Num())
		{
			Out.Add(FString::Printf(TEXT("%s: %d photographs for its %d views"), *Vessel.Work, Views.Num(), Vessel.Views.Num()));
			return Out;
		}
		const TArray<TArray<FViewOverlap>> Overlaps = ViewOverlaps(How, Vessel, Views);
		for (int32 K = 0; K < Views.Num(); ++K)
		{
			const FArtVesselView& View = Vessel.Views[K];
			if (View.Placement != TEXT("registered"))
			{
				continue;
			}
			const FString Name = FPaths::GetCleanFilename(View.Image);
			if (View.Overlaps.IsEmpty())
			{
				Out.Add(FString::Printf(TEXT("%s's view %s is registered, but its record lists no overlap that placed it"), *Vessel.Work, *Name));
			}
			for (const FArtVesselOverlap& Listed : View.Overlaps)
			{
				if (!Vessel.Views.ContainsByPredicate([&Listed](const FArtVesselView& Other) { return FPaths::GetCleanFilename(Other.Image) == Listed.Image; }))
				{
					Out.Add(FString::Printf(TEXT("%s's view %s lists an overlap with %s, which is none of its views"), *Vessel.Work, *Name, *Listed.Image));
				}
			}
			for (int32 J = 0; J < Views.Num(); ++J)
			{
				if (J == K)
				{
					continue;
				}
				const FString Other = FPaths::GetCleanFilename(Vessel.Views[J].Image);
				const FViewOverlap& Found = Overlaps[K][J];
				const FArtVesselOverlap* Listed = View.Overlaps.FindByPredicate([&Other](const FArtVesselOverlap& O) { return O.Image == Other; });
				if (Listed)
				{
					if (FMath::Abs(Found.Share - Listed->Share) > How.ShareTolerance * Listed->Share
						|| Found.Correlation < Listed->Correlation - How.CorrelationTolerance)
					{
						Out.Add(FString::Printf(TEXT("%s's view %s at %.1f deg shares %.4f of the outside with %s at a correlation of %.2f; its record, "
							"which placed it there, gives %.4f at %.2f"), *Vessel.Work, *Name, View.AzimuthDegrees, Found.Share, *Other, Found.Correlation,
							Listed->Share, Listed->Correlation));
					}
				}
				else if (Found.Share > How.LeastShare * (1.0 + How.ShareTolerance))
				{
					Out.Add(FString::Printf(TEXT("%s's view %s at %.1f deg shares %.4f of the outside with %s, an overlap its record doesn't list"),
						*Vessel.Work, *Name, View.AzimuthDegrees, Found.Share, *Other));
				}
			}
		}
		return Out;
	}
}
