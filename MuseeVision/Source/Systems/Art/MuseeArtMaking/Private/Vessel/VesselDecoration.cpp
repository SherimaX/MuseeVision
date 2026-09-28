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
		 * A view's white is known on a grid over the outside, each cell so many of the sight grid's cells each way; the
		 * white at a cell is its glaze's quantile over the cells round it, so many each side.
		 */
		constexpr int32 WhiteCellSamples = 4;
		constexpr int32 WhiteRows = SightRows / WhiteCellSamples;
		constexpr int32 WhiteColumns = SightColumns / WhiteCellSamples;
		constexpr int32 WhiteWindow = 1;

		/** The fewest samples of glaze a cell's white is taken from: a sixth of its window's. */
		constexpr int32 WhiteLeastSamples = (2 * WhiteWindow + 1) * (2 * WhiteWindow + 1) * WhiteCellSamples * WhiteCellSamples / 6;

		/**
		 * Where no cell nearby shows glaze (a band painted solid), its white is spread from the cells that do, by blurs of
		 * so many cells, finer first; and the whole is then smoothed by the finest.
		 */
		constexpr double WhiteSpreads[] = { 1.0, 2.0, 4.0, 8.0, 16.0, 32.0 };

		/**
		 * A photograph's white: the glaze's colour as it shows it (its hue, at a lightness of 1), its lightness overall, and
		 * its lightness at each place of the outside as its light falls there and as the view sees it, read on the glaze
		 * itself round that place (WhiteOf); dividing by it takes the photograph's light off and leaves the painting.
		 */
		struct FViewWhite
		{
			FLinearColor Colour = FLinearColor::White;
			double Lightness = 1.0;

			/** The outside's first and last lengths along the layout, and the white's lightness on its grid (none: Lightness). */
			double From = 0.0;
			double To = 0.0;
			TArray<float> Field;

			/** The white's lightness at a share of the turn from the front and a length along the layout, bilinear. */
			double LightnessAt(double Turn, double Length) const
			{
				if (Field.Num() != WhiteRows * WhiteColumns)
				{
					return Lightness;
				}
				const double X = Turn * WhiteColumns - 0.5;
				const double Y = FMath::Clamp((Length - From) / FMath::Max(To - From, UE_DOUBLE_SMALL_NUMBER) * WhiteRows - 0.5, 0.0, WhiteRows - 1.0);
				const int32 X0 = FMath::FloorToInt32(X);
				const int32 Y0 = FMath::Min(FMath::FloorToInt32(Y), WhiteRows - 2);
				const double Fx = X - X0;
				const double Fy = Y - Y0;
				auto Cell = [this](int32 Column, int32 Row) { return double(Field[Row * WhiteColumns + ((Column % WhiteColumns) + WhiteColumns) % WhiteColumns]); };
				return FMath::Lerp(FMath::Lerp(Cell(X0, Y0), Cell(X0 + 1, Y0), Fx), FMath::Lerp(Cell(X0, Y0 + 1), Cell(X0 + 1, Y0 + 1), Fx), Fy);
			}
		};

		/** A grid of the white's cells blurred by a Gaussian of Sigma cells, round the turn wrapping and along the outside held. */
		TArray<float> BlurCells(const TArray<float>& Grid, double Sigma)
		{
			const int32 Half = FMath::CeilToInt32(3.0 * Sigma);
			TArray<double> Kernel;
			for (int32 K = -Half; K <= Half; ++K)
			{
				Kernel.Add(FMath::Exp(-double(K * K) / (2.0 * Sigma * Sigma)));
			}
			TArray<float> Across;
			Across.SetNumZeroed(Grid.Num());
			for (int32 Row = 0; Row < WhiteRows; ++Row)
			{
				for (int32 Column = 0; Column < WhiteColumns; ++Column)
				{
					double Sum = 0.0;
					for (int32 K = -Half; K <= Half; ++K)
					{
						Sum += Kernel[K + Half] * Grid[Row * WhiteColumns + (((Column + K) % WhiteColumns) + WhiteColumns) % WhiteColumns];
					}
					Across[Row * WhiteColumns + Column] = float(Sum);
				}
			}
			TArray<float> Out;
			Out.SetNumZeroed(Grid.Num());
			for (int32 Row = 0; Row < WhiteRows; ++Row)
			{
				for (int32 Column = 0; Column < WhiteColumns; ++Column)
				{
					double Sum = 0.0;
					for (int32 K = -Half; K <= Half; ++K)
					{
						Sum += Kernel[K + Half] * Across[FMath::Clamp(Row + K, 0, WhiteRows - 1) * WhiteColumns + Column];
					}
					Out[Row * WhiteColumns + Column] = float(Sum);
				}
			}
			return Out;
		}

		/**
		 * The white's grid from each cell's known lightness (negative where unknown): where unknown, spread from the known
		 * cells round it (WhiteSpreads, normalised by the known cells' weight), finest first; then smoothed by the finest
		 * spread. False where no cell is known.
		 */
		bool SpreadCells(TArray<float>& Grid)
		{
			TArray<float> Values;
			TArray<float> Known;
			Values.SetNumZeroed(Grid.Num());
			Known.SetNumZeroed(Grid.Num());
			bool bAny = false;
			for (int32 Index = 0; Index < Grid.Num(); ++Index)
			{
				if (Grid[Index] >= 0.0f)
				{
					Values[Index] = Grid[Index];
					Known[Index] = 1.0f;
					bAny = true;
				}
			}
			if (!bAny)
			{
				return false;
			}
			for (const double Sigma : WhiteSpreads)
			{
				const TArray<float> Sum = BlurCells(Values, Sigma);
				const TArray<float> Weight = BlurCells(Known, Sigma);
				for (int32 Index = 0; Index < Grid.Num(); ++Index)
				{
					if (Grid[Index] < 0.0f && Weight[Index] > 1e-3f)
					{
						Grid[Index] = Sum[Index] / Weight[Index];
					}
				}
			}
			for (float& Value : Grid)
			{
				// what even the widest spread didn't reach (a grid known in one place alone) takes the known cells' mean
				if (Value < 0.0f)
				{
					double Sum = 0.0;
					int32 Count = 0;
					for (int32 Index = 0; Index < Values.Num(); ++Index)
					{
						Sum += Known[Index] > 0.0f ? Values[Index] : 0.0;
						Count += Known[Index] > 0.0f ? 1 : 0;
					}
					Value = float(Sum / FMath::Max(Count, 1));
				}
			}
			Grid = BlurCells(Grid, WhiteSpreads[0]);
			TArray<float> Ones;
			Ones.Init(1.0f, Grid.Num());
			const TArray<float> Norm = BlurCells(Ones, WhiteSpreads[0]);
			for (int32 Index = 0; Index < Grid.Num(); ++Index)
			{
				Grid[Index] /= FMath::Max(Norm[Index], 1e-6f);
			}
			return true;
		}

		/** A sample of a photograph on the vessel: its colour, its lightness, and how squarely the view sees it. */
		struct FWhiteSample
		{
			FLinearColor Colour;
			float Light = 0.0f;
			float Square = 0.0f;
			bool bSeen = false;
		};

		/** The quantile of some lightnesses (sorted in place). */
		double QuantileOf(TArray<float>& Lights, double Quantile)
		{
			Lights.Sort();
			return Lights[FMath::Clamp(int32(Quantile * Lights.Num()), 0, Lights.Num() - 1)];
		}

		/**
		 * A view's white, from what its photograph shows where it sees the outside whole (its sight map's factor 1), on the
		 * sight grid. Overall: the WhiteQuantile of the lightness it shows where it sees the outside at least
		 * WhiteSquareness squarely (a painted porcelain's ground is its glaze, and most of what a view sees face on), or of
		 * all it sees whole where it sees too little so squarely (a view from under the vessel); its hue the mean of those
		 * samples within WhiteBand of that. At each place: the WhiteQuantile of the glaze's samples round it (no more
		 * saturated than GlazeChroma relative to that hue and no darker than ShadeFloor of the white; then, the white round
		 * them known, no darker than GlazeLightness of it), spread where too few samples show glaze (SpreadCells), never
		 * under ShadeFloor of the white. A gallery's light falling off down a vase, or a cup's wall darkening as it turns
		 * under, is the glaze's own shading, and follows it.
		 */
		FViewWhite WhiteOf(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Normals, const FVesselLayout& Layout,
			const FSightMap& Sight, const FVesselPicture& View)
		{
			FViewWhite White;
			White.From = Sight.From;
			White.To = Sight.To;
			TArray<FWhiteSample> Samples;
			Samples.SetNum(SightRows * SightColumns);
			if (Sight.To <= Sight.From)
			{
				return White;
			}
			ParallelFor(SightRows, [&](int32 Row)
			{
				const FSectionPlace Place = PlaceAlong(Layout, Sight.From + (Row + 0.5) / SightRows * (Sight.To - Sight.From));
				for (int32 Column = 0; Column < SightColumns; ++Column)
				{
					FWhiteSample& Taken = Samples[Row * SightColumns + Column];
					const FSurfacePoint At = Surface(Vessel.Section, Normals, Place, 2.0 * UE_DOUBLE_PI * (Column + 0.5) / SightColumns);
					const double Facing = Squareness(View.Camera, At);
					FVector2D Pixel;
					if (Facing > 0.0 && Sight.Factor[Row * SightColumns + Column] >= 1.0f && View.Camera.Project(At.Point, Pixel)
						&& Sample(View.Image, Pixel, Taken.Colour))
					{
						Taken.Light = float(Luminance(Taken.Colour));
						Taken.Square = float(Facing);
						Taken.bSeen = true;
					}
				}
			});
			double Squarely = How.WhiteSquareness;
			TArray<float> Lights;
			for (const FWhiteSample& Taken : Samples)
			{
				if (Taken.bSeen && Taken.Square >= Squarely)
				{
					Lights.Add(Taken.Light);
				}
			}
			if (Lights.Num() < WhiteLeastSamples)
			{
				Squarely = 0.0;
				Lights.Reset();
				for (const FWhiteSample& Taken : Samples)
				{
					if (Taken.bSeen)
					{
						Lights.Add(Taken.Light);
					}
				}
			}
			if (Lights.IsEmpty())
			{
				return White;
			}
			const double Lightness = FMath::Max(QuantileOf(Lights, How.WhiteQuantile), UE_DOUBLE_SMALL_NUMBER);
			FLinearColor Sum(0.0f, 0.0f, 0.0f, 0.0f);
			int32 Count = 0;
			for (const FWhiteSample& Taken : Samples)
			{
				if (Taken.bSeen && Taken.Square >= Squarely && FMath::Abs(Taken.Light / Lightness - 1.0) <= WhiteBand)
				{
					Sum += Taken.Colour;
					++Count;
				}
			}
			const FLinearColor Mean = Count > 0 ? Sum / float(Count) : FLinearColor(float(Lightness), float(Lightness), float(Lightness), 1.0f);
			White.Colour = Mean / float(FMath::Max(Luminance(Mean), UE_DOUBLE_SMALL_NUMBER));
			White.Colour.A = 1.0f;
			White.Lightness = Lightness;

			// The glaze: no more saturated than GlazeChroma once the white's hue is divided out, and not in deep shade.
			TArray<uint8> Glaze;
			Glaze.SetNumZeroed(Samples.Num());
			for (int32 Index = 0; Index < Samples.Num(); ++Index)
			{
				const FWhiteSample& Taken = Samples[Index];
				if (!Taken.bSeen || Taken.Light < How.ShadeFloor * Lightness)
				{
					continue;
				}
				const float R = Taken.Colour.R / FMath::Max(White.Colour.R, UE_SMALL_NUMBER);
				const float G = Taken.Colour.G / FMath::Max(White.Colour.G, UE_SMALL_NUMBER);
				const float B = Taken.Colour.B / FMath::Max(White.Colour.B, UE_SMALL_NUMBER);
				const float Most = FMath::Max3(R, G, B);
				Glaze[Index] = Most > 0.0f && (Most - FMath::Min3(R, G, B)) / Most <= How.GlazeChroma ? 1 : 0;
			}
			// Each cell's white: the quantile of the glaze round it; twice, the second time leaving out what is darker than
			// GlazeLightness of the first (a dark stroke of low colour is painting, not glaze in shade).
			auto CellsOf = [&](const TArray<float>* First)
			{
				TArray<float> Grid;
				Grid.Init(-1.0f, WhiteRows * WhiteColumns);
				ParallelFor(WhiteRows, [&](int32 CellRow)
				{
					TArray<float> Round;
					for (int32 CellColumn = 0; CellColumn < WhiteColumns; ++CellColumn)
					{
						Round.Reset();
						for (int32 Row = (CellRow - WhiteWindow) * WhiteCellSamples; Row < (CellRow + WhiteWindow + 1) * WhiteCellSamples; ++Row)
						{
							if (Row < 0 || Row >= SightRows)
							{
								continue;
							}
							for (int32 Step = (CellColumn - WhiteWindow) * WhiteCellSamples; Step < (CellColumn + WhiteWindow + 1) * WhiteCellSamples; ++Step)
							{
								const int32 Column = ((Step % SightColumns) + SightColumns) % SightColumns;
								const int32 Index = Row * SightColumns + Column;
								if (!Glaze[Index])
								{
									continue;
								}
								if (First)
								{
									const double Near = (*First)[(Row / WhiteCellSamples) * WhiteColumns + Column / WhiteCellSamples];
									if (Samples[Index].Light < How.GlazeLightness * Near)
									{
										continue;
									}
								}
								Round.Add(Samples[Index].Light);
							}
						}
						if (Round.Num() >= WhiteLeastSamples)
						{
							Grid[CellRow * WhiteColumns + CellColumn] = float(QuantileOf(Round, How.WhiteQuantile));
						}
					}
				});
				return Grid;
			};
			TArray<float> First = CellsOf(nullptr);
			if (!SpreadCells(First))
			{
				return White;
			}
			TArray<float> Field = CellsOf(&First);
			if (!SpreadCells(Field))
			{
				Field = MoveTemp(First);
			}
			for (float& Value : Field)
			{
				Value = FMath::Max(Value, float(How.ShadeFloor * Lightness));
			}
			White.Field = MoveTemp(Field);
			return White;
		}

		/**
		 * What one view lends a place of the outside: its colour there divided channel by channel by its photograph's white
		 * there (its glaze's hue, at the lightness its light gives the glaze there), so the glaze is white in every
		 * photograph, whatever its light and white balance (the glaze's own colour is Matter's); and its weight, the square
		 * of how squarely it sees the place times how fully (its sight map), a highlight's by HighlightWeight. Zero where it
		 * doesn't see the place.
		 */
		double Lend(const FArtVesselComposition& How, const FVesselPicture& View, const FViewWhite& White, const FSightMap& Sight,
			const FEnvelope& Envelope, const FSurfacePoint& At, double Turn, double Length, FLinearColor& OutColour)
		{
			const double Square = Squareness(View.Camera, At);
			if (Square <= 0.0)
			{
				return 0.0;
			}
			double Fully = 0.0;
			if (!Sight.Agreed(Turn, Length, Fully))
			{
				Fully = SightFactor(How, Envelope, View.Camera, At);
			}
			FVector2D Pixel;
			FLinearColor Colour;
			if (Fully <= 0.0 || !View.Camera.Project(At.Point, Pixel) || !Sample(View.Image, Pixel, Colour))
			{
				return 0.0;
			}
			const float Lightness = float(White.LightnessAt(Turn, Length));
			OutColour = FLinearColor(Colour.R / FMath::Max(White.Colour.R * Lightness, UE_SMALL_NUMBER),
				Colour.G / FMath::Max(White.Colour.G * Lightness, UE_SMALL_NUMBER), Colour.B / FMath::Max(White.Colour.B * Lightness, UE_SMALL_NUMBER), 1.0f);
			return Square * Square * Fully * (Luminance(OutColour) > How.HighlightOverWhite ? How.HighlightWeight : 1.0);
		}

		/** A seam's grid round the turn: finer than the sight grid's, so it runs where the two paintings agree. */
		constexpr int32 SeamColumns = 2048;

		/** How far a seam may move round from one row of the sight grid to the next, columns. */
		constexpr int32 SeamStep = 3;

		/**
		 * How a seam is drawn to where the two views see the surface alike squarely, as a share of the colours' full
		 * difference: where the paintings agree as well either way, it runs between the two views, each keeping the side it
		 * sees the more squarely.
		 */
		constexpr double SeamPull = 0.05;

		/** A share of the turn, 0 to 1. */
		double Turned(double Turn)
		{
			return Turn - FMath::FloorToDouble(Turn);
		}

		/**
		 * Where two views that aren't blended meet (their overlap's paintings further apart than the composition's
		 * BlendMeters): on each row of the sight grid, the overlap's run round the turn, where both lend, and the seam's
		 * place in it; First keeps its side of the seam (the side of its own bearing), Second the other. Its columns count
		 * round from Start, opposite the two views' bearings, so an overlap never wraps.
		 */
		struct FViewSeam
		{
			int32 First = INDEX_NONE;
			int32 Second = INDEX_NONE;
			double Start = 0.0;
			double From = 0.0;
			double To = 0.0;
			TArray<int32> Lo;
			TArray<int32> Hi;
			TArray<double> At;

			/**
			 * The share of its weight a view keeps at a place (at RadiusMeters from the axis): all of it off the seam's
			 * overlap; else, across the seam, cross-faded over SeamMeters from its own side to the other's.
			 */
			double KeptBy(int32 View, double Turn, double Length, double RadiusMeters, double SeamMeters) const
			{
				if ((View != First && View != Second) || At.Num() != SightRows)
				{
					return 1.0;
				}
				const double Place = (Length - From) / FMath::Max(To - From, UE_DOUBLE_SMALL_NUMBER) * SightRows - 0.5;
				const int32 Below = FMath::Clamp(FMath::FloorToInt32(Place), 0, SightRows - 1);
				const int32 Above = FMath::Min(Below + 1, SightRows - 1);
				const int32 Nearest = Place - Below < 0.5 ? Below : Above;
				if (Lo[Nearest] == INDEX_NONE || (At[Below] < 0.0 && At[Above] < 0.0))
				{
					return 1.0;
				}
				const double Seam = At[Below] >= 0.0 && At[Above] >= 0.0 ? FMath::Lerp(At[Below], At[Above], FMath::Clamp(Place - Below, 0.0, 1.0))
					: FMath::Max(At[Below], At[Above]);
				const double Column = Turned(Turn - Start) * SeamColumns;
				const double MetersPerColumn = 2.0 * UE_DOUBLE_PI * FMath::Max(RadiusMeters, UE_DOUBLE_SMALL_NUMBER) / SeamColumns;
				const double Margin = SeamMeters / MetersPerColumn;
				if (Column < Lo[Nearest] - Margin || Column > Hi[Nearest] + 1 + Margin)
				{
					return 1.0;
				}
				// how far past the seam towards Second's side, metres; First keeps what lies before it
				const double Past = (Column - Seam) * MetersPerColumn;
				const double Share = FMath::Clamp(0.5 - Past / FMath::Max(SeamMeters, UE_DOUBLE_SMALL_NUMBER), 0.0, 1.0);
				const double Kept = Share * Share * (3.0 - 2.0 * Share);
				return View == First ? Kept : 1.0 - Kept;
			}
		};

		/**
		 * The seam between two views: on each row of the sight grid, over the run where both lend, the place where their
		 * colours differ least (and, as SeamPull draws it, where they see the surface alike squarely), and where both lend at
		 * least FullWeight (so the seam's cross-fade has both views whole); moving at most SeamStep columns from row to row,
		 * the least costly path through the rows (by dynamic programming), a run of rows at a time.
		 */
		FViewSeam SeamOf(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Normals, const FVesselLayout& Layout,
			const FEnvelope& Envelope, TConstArrayView<FVesselPicture> Views, TConstArrayView<FViewWhite> Whites, TConstArrayView<FSightMap> Sights,
			int32 A, int32 B)
		{
			FViewSeam Seam;
			Seam.Lo.Init(INDEX_NONE, SightRows);
			Seam.Hi.Init(INDEX_NONE, SightRows);
			Seam.At.Init(-1.0, SightRows);
			Seam.From = Sights[A].From;
			Seam.To = Sights[A].To;
			const double BearingA = Views[A].Camera.AzimuthRadians;
			const double BearingB = Views[B].Camera.AzimuthRadians;
			const double Middle = FMath::Atan2(FMath::Sin(BearingA) + FMath::Sin(BearingB), FMath::Cos(BearingA) + FMath::Cos(BearingB));
			Seam.Start = Turned(Middle / (2.0 * UE_DOUBLE_PI) + 0.5);
			const bool bAFirst = Turned(BearingA / (2.0 * UE_DOUBLE_PI) - Seam.Start) < Turned(BearingB / (2.0 * UE_DOUBLE_PI) - Seam.Start);
			Seam.First = bAFirst ? A : B;
			Seam.Second = bAFirst ? B : A;
			if (Seam.To <= Seam.From)
			{
				return Seam;
			}
			TArray<TArray<double>> Costs;
			Costs.SetNum(SightRows);
			ParallelFor(SightRows, [&](int32 Row)
			{
				const double Length = Seam.From + (Row + 0.5) / SightRows * (Seam.To - Seam.From);
				const FSectionPlace Place = PlaceAlong(Layout, Length);
				TArray<double> Cost;
				Cost.Init(-1.0, SeamColumns);
				int32 BestLo = INDEX_NONE;
				int32 BestHi = INDEX_NONE;
				int32 RunLo = INDEX_NONE;
				for (int32 Column = 0; Column <= SeamColumns; ++Column)
				{
					bool bBoth = false;
					if (Column < SeamColumns)
					{
						const double Turn = Turned(Seam.Start + (Column + 0.5) / SeamColumns);
						const FSurfacePoint At = Surface(Vessel.Section, Normals, Place, 2.0 * UE_DOUBLE_PI * Turn);
						FLinearColor First;
						FLinearColor Second;
						const double WeightFirst = Lend(How, Views[Seam.First], Whites[Seam.First], Sights[Seam.First], Envelope, At, Turn, Length, First);
						const double WeightSecond = WeightFirst > 0.0
							? Lend(How, Views[Seam.Second], Whites[Seam.Second], Sights[Seam.Second], Envelope, At, Turn, Length, Second) : 0.0;
						bBoth = WeightFirst > 0.0 && WeightSecond > 0.0;
						if (bBoth)
						{
							const FLinearColor P = First.GetClamped(0.0f, 1.2f);
							const FLinearColor Q = Second.GetClamped(0.0f, 1.2f);
							const double Least = FMath::Min(WeightFirst, WeightSecond);
							Cost[Column] = FMath::Abs(P.R - Q.R) + FMath::Abs(P.G - Q.G) + FMath::Abs(P.B - Q.B)
								+ SeamPull * FMath::Abs(WeightFirst - WeightSecond) / (WeightFirst + WeightSecond)
								+ (1.0 - FMath::Clamp(Least / How.FullWeight, 0.0, 1.0));
						}
					}
					if (bBoth && RunLo == INDEX_NONE)
					{
						RunLo = Column;
					}
					else if (!bBoth && RunLo != INDEX_NONE)
					{
						if (BestLo == INDEX_NONE || Column - 1 - RunLo > BestHi - BestLo)
						{
							BestLo = RunLo;
							BestHi = Column - 1;
						}
						RunLo = INDEX_NONE;
					}
				}
				Seam.Lo[Row] = BestLo;
				Seam.Hi[Row] = BestHi;
				Costs[Row] = MoveTemp(Cost);
			});
			// The path: the least cost to reach each column of each row's run, and the column it came from.
			TArray<TArray<double>> Reached;
			TArray<TArray<int32> > Came;
			TArray<uint8> Fresh;
			Reached.SetNum(SightRows);
			Came.SetNum(SightRows);
			Fresh.SetNumZeroed(SightRows);
			for (int32 Row = 0; Row < SightRows; ++Row)
			{
				if (Seam.Lo[Row] == INDEX_NONE)
				{
					continue;
				}
				const int32 Width = Seam.Hi[Row] - Seam.Lo[Row] + 1;
				Reached[Row].Init(TNumericLimits<double>::Max(), Width);
				Came[Row].Init(INDEX_NONE, Width);
				const bool bAfter = Row > 0 && Seam.Lo[Row - 1] != INDEX_NONE;
				bool bAny = false;
				for (int32 K = 0; K < Width; ++K)
				{
					const int32 Column = Seam.Lo[Row] + K;
					double Best = TNumericLimits<double>::Max();
					int32 From = INDEX_NONE;
					if (bAfter)
					{
						for (int32 Prior = FMath::Max(Column - SeamStep, Seam.Lo[Row - 1]); Prior <= FMath::Min(Column + SeamStep, Seam.Hi[Row - 1]); ++Prior)
						{
							const double Before = Reached[Row - 1][Prior - Seam.Lo[Row - 1]];
							if (Before < Best)
							{
								Best = Before;
								From = Prior;
							}
						}
					}
					if (From != INDEX_NONE)
					{
						Reached[Row][K] = Best + Costs[Row][Column];
						Came[Row][K] = From;
						bAny = true;
					}
				}
				if (!bAny)
				{
					// a run of rows begins here: no column of it follows on from the row before
					Fresh[Row] = 1;
					for (int32 K = 0; K < Width; ++K)
					{
						Reached[Row][K] = Costs[Row][Seam.Lo[Row] + K];
						Came[Row][K] = INDEX_NONE;
					}
				}
			}
			int32 Next = INDEX_NONE;
			for (int32 Row = SightRows - 1; Row >= 0; --Row)
			{
				if (Seam.Lo[Row] == INDEX_NONE)
				{
					Next = INDEX_NONE;
					continue;
				}
				const bool bLast = Row == SightRows - 1 || Seam.Lo[Row + 1] == INDEX_NONE || Fresh[Row + 1] || Next == INDEX_NONE;
				int32 Column = INDEX_NONE;
				if (!bLast)
				{
					Column = Came[Row + 1][Next - Seam.Lo[Row + 1]];
				}
				if (Column == INDEX_NONE)
				{
					double Best = TNumericLimits<double>::Max();
					for (int32 K = 0; K < Reached[Row].Num(); ++K)
					{
						if (Reached[Row][K] < Best)
						{
							Best = Reached[Row][K];
							Column = Seam.Lo[Row] + K;
						}
					}
				}
				Seam.At[Row] = Column + 0.5;
				Next = Column;
			}
			return Seam;
		}

		/** Everything a composition needs of its views: each one's sight, its white, and the seams where two aren't blended. */
		struct FComposing
		{
			FVesselLayout Layout;
			TArray<FVector2D> Normals;
			FEnvelope Envelope;
			TArray<FSightMap> Sights;
			TArray<FViewWhite> Whites;
			TArray<FViewSeam> Seams;

			FComposing(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Sized, TConstArrayView<FVesselPicture> Views,
				bool bSeams)
				: Layout(LayoutOf(Sized))
				, Normals(PointNormals(Vessel.Section))
				, Envelope(Vessel.Section, How.ClearanceMeters)
			{
				for (const FVesselPicture& View : Views)
				{
					Sights.Add(SightMapOf(How, Vessel, Normals, Layout, Envelope, View.Camera));
					Whites.Add(WhiteOf(How, Vessel, Normals, Layout, Sights.Last(), View));
				}
				if (!bSeams || Views.Num() != Vessel.Views.Num())
				{
					return;
				}
				// A pair of views is seamed where either's record gives their overlap's paintings further apart than BlendMeters.
				for (int32 I = 0; I < Vessel.Views.Num(); ++I)
				{
					for (const FArtVesselOverlap& Overlap : Vessel.Views[I].Overlaps)
					{
						const int32 J = Vessel.Views.IndexOfByPredicate([&Overlap](const FArtVesselView& Other)
						{
							return FPaths::GetCleanFilename(Other.Image) == Overlap.Image;
						});
						const bool bKnown = Seams.ContainsByPredicate([I, J](const FViewSeam& Seam)
						{
							return (Seam.First == I && Seam.Second == J) || (Seam.First == J && Seam.Second == I);
						});
						if (J != INDEX_NONE && J != I && !bKnown && Overlap.OffsetMeters > How.BlendMeters)
						{
							Seams.Add(SeamOf(How, Vessel, Normals, Layout, Envelope, Views, Whites, Sights, I, J));
						}
					}
				}
			}
		};

		/**
		 * The views' colour at a place of the outside (each view's as Lend gives it, and as much of its weight as its seams
		 * leave it), and their summed weight.
		 */
		void Gather(const FArtVesselComposition& How, TConstArrayView<FVesselPicture> Views, const FComposing& With, const FSurfacePoint& At, double Turn,
			double Length, FLinearColor& OutColour, double& OutWeight)
		{
			FLinearColor Sum(0.0f, 0.0f, 0.0f, 0.0f);
			OutWeight = 0.0;
			const double Radius = FVector2D(At.Point.X, At.Point.Y).Size();
			for (int32 Index = 0; Index < Views.Num(); ++Index)
			{
				FLinearColor Colour;
				double Weight = Lend(How, Views[Index], With.Whites[Index], With.Sights[Index], With.Envelope, At, Turn, Length, Colour);
				for (const FViewSeam& Seam : With.Seams)
				{
					Weight *= Weight > 0.0 ? Seam.KeptBy(Index, Turn, Length, Radius, How.SeamMeters) : 1.0;
				}
				if (Weight > 0.0)
				{
					Sum += Colour * float(Weight);
					OutWeight += Weight;
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

		/**
		 * How far apart two views' paintings lie where the composition blends them (BlendOffsets), as the derivation measures
		 * their overlap (MuseeVision/Scripts/vessels.py, offset_of): blocks of OffsetBlock cells of the registration's grid,
		 * each laid on the other view by the rise and turn (within OffsetReach cells) at which its detail correlates best,
		 * trusted at OffsetTrust or better (the same stretch of painting); a block is measured where the composition blends
		 * the two there, the lesser weight at least OffsetBlended of the greater and OffsetLeast of a cell's own.
		 */
		constexpr int32 OffsetBlock = 24;
		constexpr int32 OffsetReach = 10;
		constexpr double OffsetTrust = 0.8;
		constexpr double OffsetLeast = 0.01;
		constexpr double OffsetBlended = 0.25;

		/**
		 * A painting's darkness against its white (PaintingContrast): the ContrastDark quantile of its lightness over the
		 * ContrastWhite, in windows of a band ContrastSectors round (each of ContrastWindowSamples or more), their median.
		 */
		constexpr double ContrastDark = 0.2;
		constexpr double ContrastWhite = 0.9;
		constexpr int32 ContrastSectors = 30;
		constexpr int32 ContrastWindowSamples = 64;

		/** A photograph's own white is saturated where every channel is at this 8-bit level or over. */
		constexpr int32 SaturatedLevel = 250;

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
		const FComposing With(How, Vessel, Sized, Views, true);
		const FVesselLayout& Layout = With.Layout;
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
				const FSurfacePoint At = Surface(Vessel.Section, With.Normals, Place, 2.0 * UE_DOUBLE_PI * Turn);
				Gather(How, Views, With, At, Turn, Length, RowColours[Column], RowWeights[Column]);
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

	TArray<FBlendOffset> BlendOffsets(const FArtVesselComposition& How, const FArtVesselRegistration& Registration, const FArtVessel& Vessel,
		TConstArrayView<FVesselPicture> Views, bool bSeams)
	{
		TArray<FBlendOffset> Out;
		const int32 Width = Registration.AzimuthSteps;
		const int32 Height = Registration.HeightSteps;
		if (Views.Num() < 2 || Views.Num() != Vessel.Views.Num() || Vessel.Outside.Num() != 2 || Width < 2 * OffsetBlock || Height < 2 * OffsetBlock + 2 * OffsetReach)
		{
			return Out;
		}
		const FComposing With(How, Vessel, Vessel.Section, Views, bSeams);
		// The outside's rising points (as the registration unwraps it) and their lengths along the layout.
		TArray<double> Heights;
		TArray<double> Radii;
		TArray<double> Lengths;
		for (int32 Index = Vessel.Outside[0]; Index <= FMath::Min(Vessel.Outside[1], Vessel.Section.Num() - 1); ++Index)
		{
			const FVector2D& Point = Vessel.Section[Index];
			if (Heights.IsEmpty() || Point.Y > Heights.Last())
			{
				Heights.Add(Point.Y);
				Radii.Add(Point.X);
				Lengths.Add(With.Layout.Lengths[Index]);
			}
		}
		if (Heights.Num() < 2)
		{
			return Out;
		}
		// Each view's detail as the registration unwraps it, and its weight in each cell as the composition gives it.
		TArray<FUnwrapped> Unwrapped;
		TArray<TArray<float>> Weights;
		for (int32 View = 0; View < Views.Num(); ++View)
		{
			Unwrapped.Add(UnwrapFor(Registration, Heights, Radii, Views[View]));
			TArray<float>& Weight = Weights.AddDefaulted_GetRef();
			Weight.SetNumZeroed(Width * Height);
			ParallelFor(Height, [&](int32 Row)
			{
				const double Length = Interpolate(Heights, Lengths, (Row + 0.5) / Height * Heights.Last());
				const FSectionPlace Place = PlaceAlong(With.Layout, Length);
				for (int32 Column = 0; Column < Width; ++Column)
				{
					const double Turn = (Column + 0.5) / Width;
					const FSurfacePoint At = Surface(Vessel.Section, With.Normals, Place, 2.0 * UE_DOUBLE_PI * Turn);
					FLinearColor Colour;
					double Lent = Lend(How, Views[View], With.Whites[View], With.Sights[View], With.Envelope, At, Turn, Length, Colour);
					for (const FViewSeam& Seam : With.Seams)
					{
						Lent *= Lent > 0.0 ? Seam.KeptBy(View, Turn, Length, FVector2D(At.Point.X, At.Point.Y).Size(), How.SeamMeters) : 1.0;
					}
					Weight[Row * Width + Column] = float(Lent);
				}
			});
		}
		const double CellHeight = Heights.Last() / Height;
		const double CellTurn = 2.0 * UE_DOUBLE_PI / Width;
		for (int32 I = 0; I < Views.Num(); ++I)
		{
			for (int32 J = I + 1; J < Views.Num(); ++J)
			{
				TArray<double> Found;
				for (int32 Row0 = OffsetReach; Row0 + OffsetBlock + OffsetReach <= Height; Row0 += OffsetBlock / 2)
				{
					for (int32 Column0 = 0; Column0 < Width; Column0 += OffsetBlock / 2)
					{
						// what the composition blends here: the lesser of the two weights, against the greater
						double Blended = 0.0;
						double Greater = 0.0;
						for (int32 R = Row0; R < Row0 + OffsetBlock; ++R)
						{
							for (int32 C = Column0; C < Column0 + OffsetBlock; ++C)
							{
								const int32 Index = R * Width + C % Width;
								Blended += FMath::Min(Weights[I][Index], Weights[J][Index]);
								Greater += FMath::Max(Weights[I][Index], Weights[J][Index]);
							}
						}
						if (Blended < OffsetLeast * OffsetBlock * OffsetBlock || Blended < OffsetBlended * Greater)
						{
							continue;
						}
						double Best = -2.0;
						int32 BestRise = 0;
						int32 BestTurn = 0;
						for (int32 Rise = -OffsetReach; Rise <= OffsetReach; ++Rise)
						{
							for (int32 Round = -OffsetReach; Round <= OffsetReach; ++Round)
							{
								double Total = 0.0;
								double MeanA = 0.0;
								double MeanB = 0.0;
								auto Each = [&](auto&& Visit)
								{
									for (int32 R = Row0; R < Row0 + OffsetBlock; ++R)
									{
										for (int32 C = Column0; C < Column0 + OffsetBlock; ++C)
										{
											const int32 Here = R * Width + C % Width;
											const int32 There = (R + Rise) * Width + ((C + Round) % Width + Width) % Width;
											const double W = FMath::Min(Weights[I][Here], Weights[J][Here]) * (Unwrapped[J].Weight[There] > 0.0f ? 1.0 : 0.0);
											Visit(W, Unwrapped[I].Detail[Here], Unwrapped[J].Detail[There]);
										}
									}
								};
								Each([&](double W, double A, double B) { Total += W; MeanA += W * A; MeanB += W * B; });
								if (Total <= 0.0)
								{
									continue;
								}
								MeanA /= Total;
								MeanB /= Total;
								double Product = 0.0;
								double SquaresA = 0.0;
								double SquaresB = 0.0;
								Each([&](double W, double A, double B)
								{
									Product += W * (A - MeanA) * (B - MeanB);
									SquaresA += W * (A - MeanA) * (A - MeanA);
									SquaresB += W * (B - MeanB) * (B - MeanB);
								});
								const double Correlation = Product / FMath::Sqrt(SquaresA * SquaresB + 1e-9);
								if (Correlation > Best)
								{
									Best = Correlation;
									BestRise = Rise;
									BestTurn = Round;
								}
							}
						}
						if (Best >= OffsetTrust)
						{
							const double Radius = Interpolate(Heights, Radii, (Row0 + 0.5 * OffsetBlock) / Height * Heights.Last());
							Found.Add(FMath::Sqrt(FMath::Square(BestRise * CellHeight) + FMath::Square(BestTurn * CellTurn * Radius)));
						}
					}
				}
				if (!Found.IsEmpty())
				{
					Found.Sort();
					Out.Add({ I, J, Found[Found.Num() / 2], Found.Num() });
				}
			}
		}
		return Out;
	}

	TArray<FContrastBand> PaintingContrast(const FArtVesselComposition& How, const FArtVessel& Vessel, const FVesselPicture& View,
		TConstArrayView<FVector2D> Sized, const FImage& Decoration, int32 Bands)
	{
		TArray<FContrastBand> Out;
		const FVesselLayout Layout = LayoutOf(Sized);
		if (Vessel.Outside.Num() != 2 || Bands < 1 || Decoration.SizeX < 2 || Decoration.SizeY < 2)
		{
			return Out;
		}
		const TArray<FVector2D> Normals = PointNormals(Vessel.Section);
		const double From = Layout.Lengths[Vessel.Outside[0]];
		const double To = Layout.Lengths[Vessel.Outside[1]];
		const FVector2D Size = Layout.SizeMeters();
		const TArrayView64<const FColor> Texels = Decoration.AsBGRA8();
		const TArray<float>& Levels = LinearLevels();
		for (int32 Band = 0; Band < Bands; ++Band)
		{
			FContrastBand& Found = Out.AddDefaulted_GetRef();
			Found.FromMeters = FMath::Lerp(From, To, double(Band) / Bands);
			Found.ToMeters = FMath::Lerp(From, To, double(Band + 1) / Bands);
			// each window of the band (ContrastSectors round) on its own, so the photograph's light falling across the band
			// isn't taken for its painting
			TArray<TArray<float>> Photograph;
			TArray<TArray<float>> Composed;
			Photograph.SetNum(ContrastSectors);
			Composed.SetNum(ContrastSectors);
			int32 Clipped = 0;
			int32 Saturated = 0;
			const int32 First = FMath::CeilToInt32(Found.FromMeters / Size.Y * Decoration.SizeY - 0.5);
			const int32 Last = FMath::FloorToInt32(Found.ToMeters / Size.Y * Decoration.SizeY - 0.5);
			for (int32 Row = FMath::Max(First, 0); Row <= FMath::Min(Last, Decoration.SizeY - 1); ++Row)
			{
				const double Length = (Row + 0.5) / Decoration.SizeY * Size.Y;
				const FSectionPlace Place = PlaceAlong(Layout, Length);
				if (!IsOutside(Vessel, Place.Segment))
				{
					continue;
				}
				for (int32 Column = 0; Column < Decoration.SizeX; ++Column)
				{
					const double Turn = (Column + 0.5) / Decoration.SizeX;
					const FSurfacePoint At = Surface(Vessel.Section, Normals, Place, 2.0 * UE_DOUBLE_PI * Turn);
					FVector2D Pixel;
					FLinearColor Colour;
					if (Squareness(View.Camera, At) < How.WhiteSquareness || !View.Camera.Project(At.Point, Pixel) || !Sample(View.Image, Pixel, Colour))
					{
						continue;
					}
					const FColor& Texel = Texels[int64(Row) * Decoration.SizeX + Column];
					const int32 Sector = FMath::Min(FMath::FloorToInt32(Turn * ContrastSectors), ContrastSectors - 1);
					Photograph[Sector].Add(float(Luminance(Colour)));
					Composed[Sector].Add(float(Luminance(FLinearColor(Levels[Texel.R], Levels[Texel.G], Levels[Texel.B], 1.0f))));
					Clipped += Texel.R == 255 && Texel.G == 255 && Texel.B == 255 ? 1 : 0;
					Saturated += FMath::Min3(Colour.R, Colour.G, Colour.B) >= Levels[SaturatedLevel] ? 1 : 0;
					++Found.Samples;
				}
			}
			TArray<float> PhotographRatios;
			TArray<float> ComposedRatios;
			for (int32 Sector = 0; Sector < ContrastSectors; ++Sector)
			{
				if (Photograph[Sector].Num() < ContrastWindowSamples)
				{
					continue;
				}
				PhotographRatios.Add(float(QuantileOf(Photograph[Sector], ContrastDark) / FMath::Max(QuantileOf(Photograph[Sector], ContrastWhite), UE_DOUBLE_SMALL_NUMBER)));
				ComposedRatios.Add(float(QuantileOf(Composed[Sector], ContrastDark) / FMath::Max(QuantileOf(Composed[Sector], ContrastWhite), UE_DOUBLE_SMALL_NUMBER)));
			}
			Found.Windows = PhotographRatios.Num();
			if (Found.Windows == 0)
			{
				continue;
			}
			Found.Photograph = QuantileOf(PhotographRatios, 0.5);
			Found.Composed = QuantileOf(ComposedRatios, 0.5);
			Found.Clipped = double(Clipped) / FMath::Max(Found.Samples, 1);
			Found.Saturated = double(Saturated) / FMath::Max(Found.Samples, 1);
		}
		return Out;
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
