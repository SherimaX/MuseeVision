#include "Stone/StoneMesh.h"

#include "Async/ParallelFor.h"
#include "FormMesh.h"
#include "Math/RandomStream.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** Water rounds the edges where the body's sides meet its top over this share of its height. */
		constexpr double EdgeRoundShare = 0.04;
		/** The holes are worn through between these shares of its height. */
		constexpr double HoleLowShare = 0.15;
		constexpr double HoleHighShare = 0.85;
		/** How far from straight front to back a hole turns, at most: its yaw and its pitch, in degrees. */
		constexpr double HoleYawDegrees = 35.0;
		constexpr double HolePitchDegrees = 25.0;
		/** A hole passes this share of its section's half-width off the section's centre, at most, so it runs through the body. */
		constexpr double HoleSpreadShare = 0.6;
		/** A hole is at most this share of its section's half-width across, so it never cuts the rock in two. */
		constexpr double HoleWaistShare = 0.3;
		/** The pits are worn between these shares of its height. */
		constexpr double PitLowShare = 0.05;
		constexpr double PitHighShare = 0.95;
		/** The wrinkles' wavelength across, as a share of the height, and how many times longer they run up it. */
		constexpr double WrinkleLengthShare = 0.12;
		constexpr double WrinkleStretch = 3.0;
		/** The wrinkles are two octaves of noise, the second half the first's depth: together they reach this many depths. */
		constexpr double WrinkleReach = 1.5;
		/** The section's sway changes over this share of the height. */
		constexpr double SwayLengthShare = 0.5;
		/** A peak strays from even spacing along the top by up to this share of the spacing, and stands up to this share lower than the tallest. */
		constexpr double PeakStrayShare = 0.25;
		constexpr double PeakDropShare = 0.12;
		/** How far into the noise each rock's own part of it lies, so no two rocks share their wear. */
		constexpr double NoiseReach = 100.0;
		/** The grid reaches this many cells past the rock's form every way, so the surface closes inside it. */
		constexpr int32 PadCells = 2;
		/** The fewest cells up a rock's height that still show its form. */
		constexpr int32 FewestCells = 8;

		struct FStoneHole
		{
			FVector Point = FVector::ZeroVector;
			FVector Direction = FVector::ForwardVector;
			double Radius = 0.0;
		};

		struct FStonePit
		{
			FVector Centre = FVector::ZeroVector;
			double Radius = 0.0;
		};

		/** The smooth maximum of two distances, blended over K (a polynomial smooth maximum). */
		double SmoothMax(double A, double B, double K)
		{
			const double H = FMath::Max(K - FMath::Abs(A - B), 0.0) / K;
			return FMath::Max(A, B) + 0.25 * H * H * K;
		}

		/**
		 * The distance from a plan point to an ellipse of semi-axes A and B about the origin, negative inside: the ellipse's
		 * implicit function over its gradient's length, exact on the curve and close near it.
		 */
		double EllipseDistance(double X, double Y, double A, double B)
		{
			const double K0 = FMath::Sqrt(FMath::Square(X / A) + FMath::Square(Y / B));
			const double K1 = FMath::Sqrt(FMath::Square(X / (A * A)) + FMath::Square(Y / (B * B)));
			return K1 > UE_SMALL_NUMBER ? K0 * (K0 - 1.0) / K1 : -FMath::Min(A, B);
		}

		/** The rock as a signed distance field: negative inside, positive outside, about metres near its surface. */
		class FStoneField
		{
		public:
			FStoneField(const FStoneSpec& Spec, double InWidth)
				: Kind(*Spec.Kind)
				, Height(Spec.HeightMeters)
				, Width(InWidth)
				, Depth(InWidth * Spec.Kind->DepthPerWidth)
			{
				FRandomStream Random(Spec.Seed);
				SwayOffset = Random.GetUnitVector() * NoiseReach;
				WrinkleOffset = Random.GetUnitVector() * NoiseReach;
				PlacePeaks(Random);
				PlaceHoles(Random);
				PlacePits(Random);
			}

			double Eval(const FVector& P) const
			{
				const FVector2D Centre = Sway(P.Z);
				const double Share = ShareAt(P.Z / Height);
				const double A = FMath::Max(0.5 * Width * Share, UE_KINDA_SMALL_NUMBER);
				const double B = FMath::Max(0.5 * Depth * Share, UE_KINDA_SMALL_NUMBER);
				double F = SmoothMax(EllipseDistance(P.X - Centre.X, P.Y - Centre.Y, A, B), P.Z - Top(P.X - Centre.X), EdgeRoundShare * Height);
				for (const FStoneHole& Hole : Holes)
				{
					const FVector Rel = P - Hole.Point;
					const double Off = (Rel - Hole.Direction * (Rel | Hole.Direction)).Size() - Hole.Radius;
					F = SmoothMax(F, -Off, 0.5 * Hole.Radius);
				}
				for (const FStonePit& Pit : Pits)
				{
					const double Off = FVector::Dist(P, Pit.Centre) - Pit.Radius;
					if (Off < Pit.Radius)
					{
						F = SmoothMax(F, -Off, 0.5 * Pit.Radius);
					}
				}
				// The wrinkled skin, then the foot sawn flat, so the rock stands on its plane and nothing of it below.
				return FMath::Max(F + Kind.Wrinkle * Height * Wrinkles(P), -P.Z);
			}

			FVector Gradient(const FVector& P, double Step) const
			{
				return FVector(Eval(P + FVector(Step, 0.0, 0.0)) - Eval(P - FVector(Step, 0.0, 0.0)),
					Eval(P + FVector(0.0, Step, 0.0)) - Eval(P - FVector(0.0, Step, 0.0)),
					Eval(P + FVector(0.0, 0.0, Step)) - Eval(P - FVector(0.0, 0.0, Step)));
			}

			/** The box its form lies in: its widest section swayed as far as it may, its wrinkles standing proud. */
			FBox Bounds() const
			{
				const double Proud = Kind.Warp * Height + WrinkleReach * Kind.Wrinkle * Height;
				return FBox(FVector(-0.5 * Width - Proud, -0.5 * Depth - Proud, 0.0), FVector(0.5 * Width + Proud, 0.5 * Depth + Proud, Height + Proud));
			}

		private:
			const FArtStoneKind& Kind;
			double Height = 0.0;
			double Width = 0.0;
			double Depth = 0.0;
			FVector SwayOffset = FVector::ZeroVector;
			FVector WrinkleOffset = FVector::ZeroVector;
			TArray<double> PeakX;
			TArray<double> PeakTop;
			double PeakHalfWidth = 0.0;
			TArray<FStoneHole> Holes;
			TArray<FStonePit> Pits;

			/** The profile's half-width share at a share of the height, between its points. */
			double ShareAt(double T) const
			{
				const TArray<FVector2D>& Profile = Kind.Profile;
				const double At = FMath::Clamp(T, 0.0, 1.0);
				for (int32 Index = 1; Index < Profile.Num(); ++Index)
				{
					if (At <= Profile[Index].X)
					{
						const double Span = FMath::Max(Profile[Index].X - Profile[Index - 1].X, UE_SMALL_NUMBER);
						return FMath::Lerp(Profile[Index - 1].Y, Profile[Index].Y, (At - Profile[Index - 1].X) / Span);
					}
				}
				return Profile.Last().Y;
			}

			/** Where the section's centre stands at a height: swaying more the higher it rises, not at all at its foot. */
			FVector2D Sway(double Z) const
			{
				const double Rise = FMath::Clamp(Z / Height, 0.0, 1.0);
				const double Along = Z / (SwayLengthShare * Height);
				return FVector2D(FMath::PerlinNoise3D(FVector(Along, 0.0, 0.0) + SwayOffset), FMath::PerlinNoise3D(FVector(0.0, Along, 0.0) + SwayOffset))
					* (Kind.Warp * Height * Rise);
			}

			/** The top's height over a point across the rock: the highest of its peaks there, never below the saddles. */
			double Top(double X) const
			{
				const double Saddle = Kind.Saddle * Height;
				double Out = Saddle;
				for (int32 Index = 0; Index < PeakX.Num(); ++Index)
				{
					Out = FMath::Max(Out, Saddle + (PeakTop[Index] - Saddle) * FMath::Exp(-FMath::Square((X - PeakX[Index]) / PeakHalfWidth)));
				}
				return Out;
			}

			/** The skin's wrinkles, drawn out up its height: two octaves of noise, about -1.5 to 1.5. */
			double Wrinkles(const FVector& P) const
			{
				const double Across = WrinkleLengthShare * Height;
				const FVector Q(P.X / Across, P.Y / Across, P.Z / (WrinkleStretch * Across));
				return FMath::PerlinNoise3D(Q + WrinkleOffset) + 0.5 * FMath::PerlinNoise3D(2.0 * Q + WrinkleOffset);
			}

			/** The peaks along its top: spread across its width, each straying a little and a little lower, the tallest its height. */
			void PlacePeaks(FRandomStream& Random)
			{
				const int32 Count = FMath::Max(1, Kind.Peaks);
				const double Spacing = Width / Count;
				PeakHalfWidth = 0.5 * Spacing;
				double Tallest = 0.0;
				for (int32 Index = 0; Index < Count; ++Index)
				{
					PeakX.Add(-0.5 * Width + (Index + 0.5) * Spacing + Random.FRandRange(-1.0, 1.0) * PeakStrayShare * Spacing);
					PeakTop.Add(Height * (1.0 - Random.FRandRange(0.0, PeakDropShare)));
					Tallest = FMath::Max(Tallest, PeakTop.Last());
				}
				for (double& Peak : PeakTop)
				{
					Peak *= Height / Tallest;
				}
			}

			/** The holes worn through it: each round a line through its body, mostly front to back, never wider than its waist allows. */
			void PlaceHoles(FRandomStream& Random)
			{
				for (int32 Index = 0; Index < Kind.Holes; ++Index)
				{
					const double Rise = Random.FRandRange(HoleLowShare, HoleHighShare);
					const double Z = Rise * Height;
					const FVector2D Centre = Sway(Z);
					const double HalfWidth = 0.5 * Width * ShareAt(Rise);
					FStoneHole& Hole = Holes.AddDefaulted_GetRef();
					Hole.Point = FVector(Centre.X + Random.FRandRange(-1.0, 1.0) * HoleSpreadShare * HalfWidth, Centre.Y, Z);
					const double Yaw = FMath::DegreesToRadians(Random.FRandRange(-HoleYawDegrees, HoleYawDegrees));
					const double Pitch = FMath::DegreesToRadians(Random.FRandRange(-HolePitchDegrees, HolePitchDegrees));
					Hole.Direction = FVector(FMath::Sin(Yaw) * FMath::Cos(Pitch), FMath::Cos(Yaw) * FMath::Cos(Pitch), FMath::Sin(Pitch));
					Hole.Radius = FMath::Min(Random.FRandRange(Kind.HoleRadius[0], Kind.HoleRadius[1]) * Height, HoleWaistShare * HalfWidth);
				}
			}

			/** The pits worn into its skin: each a hollow centred on the section's edge at its height. */
			void PlacePits(FRandomStream& Random)
			{
				for (int32 Index = 0; Index < Kind.Pits; ++Index)
				{
					const double Rise = Random.FRandRange(PitLowShare, PitHighShare);
					const double Z = Rise * Height;
					const FVector2D Centre = Sway(Z);
					const double Share = ShareAt(Rise);
					const double Angle = Random.FRandRange(0.0, 2.0 * UE_DOUBLE_PI);
					FStonePit& Pit = Pits.AddDefaulted_GetRef();
					Pit.Centre = FVector(Centre.X + 0.5 * Width * Share * FMath::Cos(Angle), Centre.Y + 0.5 * Depth * Share * FMath::Sin(Angle), Z);
					Pit.Radius = Random.FRandRange(Kind.PitRadius[0], Kind.PitRadius[1]) * Height;
				}
			}
		};

		/** The field sampled at every point of a grid. */
		struct FStoneGrid
		{
			FVector Origin = FVector::ZeroVector;
			double Cell = 0.0;
			FIntVector Count = FIntVector::ZeroValue;
			TArray<float> Values;

			int32 Index(int32 I, int32 J, int32 K) const { return (K * Count.Y + J) * Count.X + I; }
			FVector At(int32 I, int32 J, int32 K) const { return Origin + FVector(I, J, K) * Cell; }
			bool Inside(int32 I, int32 J, int32 K) const { return Values[Index(I, J, K)] < 0.0f; }
		};

		FStoneGrid Sample(const FStoneField& Field, double Cell)
		{
			FStoneGrid Grid;
			const FBox Box = Field.Bounds().ExpandBy(PadCells * Cell);
			const FVector Size = Box.GetSize();
			Grid.Origin = Box.Min;
			Grid.Cell = Cell;
			Grid.Count = FIntVector(FMath::CeilToInt(Size.X / Cell) + 1, FMath::CeilToInt(Size.Y / Cell) + 1, FMath::CeilToInt(Size.Z / Cell) + 1);
			Grid.Values.SetNumUninitialized(Grid.Count.X * Grid.Count.Y * Grid.Count.Z);
			ParallelFor(Grid.Count.Z, [&Grid, &Field](int32 K)
			{
				for (int32 J = 0; J < Grid.Count.Y; ++J)
				{
					for (int32 I = 0; I < Grid.Count.X; ++I)
					{
						Grid.Values[Grid.Index(I, J, K)] = static_cast<float>(Field.Eval(Grid.At(I, J, K)));
					}
				}
			});
			return Grid;
		}

		/** The corners of a cell, and its twelve edges between them. */
		constexpr int32 CellCorners[8][3] = { {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1} };
		constexpr int32 CellEdges[12][2] = { {0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7} };

		/** A vertex in every cell the surface crosses, at the mean of its edges' crossings, facing along the field's gradient. */
		TArray<int32> CellVertices(const FStoneField& Field, const FStoneGrid& Grid, FFormMesh& Out)
		{
			const FIntVector& N = Grid.Count;
			TArray<int32> Vertices;
			Vertices.Init(INDEX_NONE, N.X * N.Y * N.Z);
			for (int32 K = 0; K + 1 < N.Z; ++K)
			{
				for (int32 J = 0; J + 1 < N.Y; ++J)
				{
					for (int32 I = 0; I + 1 < N.X; ++I)
					{
						float Values[8];
						int32 Inside = 0;
						for (int32 C = 0; C < 8; ++C)
						{
							Values[C] = Grid.Values[Grid.Index(I + CellCorners[C][0], J + CellCorners[C][1], K + CellCorners[C][2])];
							Inside += Values[C] < 0.0f ? 1 : 0;
						}
						if (Inside == 0 || Inside == 8)
						{
							continue;
						}
						FVector Sum = FVector::ZeroVector;
						int32 Crossings = 0;
						for (const int32 (&Edge)[2] : CellEdges)
						{
							const float A = Values[Edge[0]];
							const float B = Values[Edge[1]];
							if ((A < 0.0f) == (B < 0.0f))
							{
								continue;
							}
							const FVector From = Grid.At(I + CellCorners[Edge[0]][0], J + CellCorners[Edge[0]][1], K + CellCorners[Edge[0]][2]);
							const FVector To = Grid.At(I + CellCorners[Edge[1]][0], J + CellCorners[Edge[1]][1], K + CellCorners[Edge[1]][2]);
							Sum += FMath::Lerp(From, To, double(A / (A - B)));
							++Crossings;
						}
						const FVector Point = Sum / Crossings;
						const FVector Normal = Field.Gradient(Point, 0.5 * Grid.Cell).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
						Vertices[Grid.Index(I, J, K)] = Out.AddVertex(Point, Normal, FVector2D(Point.X + Point.Y, Point.Z));
					}
				}
			}
			return Vertices;
		}

		/** A quad across every edge of the grid the surface crosses, joining the four cells round it. */
		void CellQuads(const FStoneGrid& Grid, const TArray<int32>& Vertices, FFormMesh& Out)
		{
			const FIntVector& N = Grid.Count;
			const auto Cell = [&Grid, &Vertices, &N](int32 I, int32 J, int32 K)
			{
				const bool bOut = I < 0 || J < 0 || K < 0 || I + 1 >= N.X || J + 1 >= N.Y || K + 1 >= N.Z;
				return bOut ? INDEX_NONE : Vertices[Grid.Index(I, J, K)];
			};
			const auto Quad = [&Out](int32 A, int32 B, int32 C, int32 D)
			{
				if (A != INDEX_NONE && B != INDEX_NONE && C != INDEX_NONE && D != INDEX_NONE)
				{
					Out.AddQuad(A, B, C, D);
				}
			};
			for (int32 K = 0; K < N.Z; ++K)
			{
				for (int32 J = 0; J < N.Y; ++J)
				{
					for (int32 I = 0; I < N.X; ++I)
					{
						const bool bIn = Grid.Inside(I, J, K);
						if (I + 1 < N.X && Grid.Inside(I + 1, J, K) != bIn)
						{
							Quad(Cell(I, J - 1, K - 1), Cell(I, J, K - 1), Cell(I, J, K), Cell(I, J - 1, K));
						}
						if (J + 1 < N.Y && Grid.Inside(I, J + 1, K) != bIn)
						{
							Quad(Cell(I - 1, J, K - 1), Cell(I, J, K - 1), Cell(I, J, K), Cell(I - 1, J, K));
						}
						if (K + 1 < N.Z && Grid.Inside(I, J, K + 1) != bIn)
						{
							Quad(Cell(I - 1, J - 1, K), Cell(I, J - 1, K), Cell(I, J, K), Cell(I - 1, J, K));
						}
					}
				}
			}
		}
	}

	TArray<FString> StoneKindFaults(const FArtStoneKind& Kind)
	{
		TArray<FString> Out;
		const TArray<FVector2D>& Profile = Kind.Profile;
		bool bRising = Profile.Num() >= 2 && Profile[0].X == 0.0 && Profile.Last().X == 1.0;
		for (int32 Index = 0; bRising && Index < Profile.Num(); ++Index)
		{
			bRising = Profile[Index].Y > 0.0 && Profile[Index].Y <= 1.0 && (Index == 0 || Profile[Index].X > Profile[Index - 1].X);
		}
		if (!bRising)
		{
			Out.Add(TEXT("its profile must rise from height share 0 to 1, each half-width share more than 0 and at most 1"));
		}
		if (Kind.DepthPerWidth <= 0.0 || Kind.DepthPerWidth > 1.0)
		{
			Out.Add(TEXT("its depth is a share of its width, more than 0 and at most 1"));
		}
		if (Kind.Peaks < 1 || Kind.Saddle <= 0.0 || Kind.Saddle > 1.0)
		{
			Out.Add(TEXT("it has one peak or more, over saddles a share of its height more than 0 and at most 1"));
		}
		const auto GoodRange = [](const TArray<double>& Range) { return Range.Num() == 2 && Range[0] > 0.0 && Range[0] <= Range[1]; };
		if (Kind.Holes < 0 || (Kind.Holes > 0 && !GoodRange(Kind.HoleRadius)))
		{
			Out.Add(TEXT("its holes need their radii [least, most], more than zero"));
		}
		if (Kind.Pits < 0 || (Kind.Pits > 0 && !GoodRange(Kind.PitRadius)))
		{
			Out.Add(TEXT("its pits need their radii [least, most], more than zero"));
		}
		if (Kind.Wrinkle < 0.0 || Kind.Warp < 0.0)
		{
			Out.Add(TEXT("its wrinkle and warp are zero or more"));
		}
		if (Kind.Cells < FewestCells)
		{
			Out.Add(FString::Printf(TEXT("it needs %d cells or more up its height"), FewestCells));
		}
		return Out;
	}

	double StoneWidthMeters(const FStoneSpec& Spec)
	{
		if (!Spec.Kind || Spec.Kind->Profile.IsEmpty() || Spec.Kind->Profile[0].Y <= 0.0)
		{
			return 0.0;
		}
		const double Foot = Spec.FootRadiusMeters - WrinkleReach * Spec.Kind->Wrinkle * Spec.HeightMeters;
		return Foot > 0.0 ? 2.0 * Foot / Spec.Kind->Profile[0].Y : 0.0;
	}

	double StoneCellMeters(const FStoneSpec& Spec)
	{
		return Spec.Kind && Spec.Kind->Cells > 0 ? Spec.HeightMeters / Spec.Kind->Cells : 0.0;
	}

	void BuildStone(const FStoneSpec& Spec, FFormMesh& Out)
	{
		const double Width = StoneWidthMeters(Spec);
		if (!Spec.Kind || Width <= 0.0 || Spec.HeightMeters <= 0.0 || !StoneKindFaults(*Spec.Kind).IsEmpty())
		{
			return;
		}
		const FStoneField Field(Spec, Width);
		const FStoneGrid Grid = Sample(Field, Spec.HeightMeters / Spec.Kind->Cells);
		const TArray<int32> Vertices = CellVertices(Field, Grid, Out);
		CellQuads(Grid, Vertices, Out);
	}
}
