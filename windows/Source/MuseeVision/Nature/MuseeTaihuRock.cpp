#include "Nature/MuseeTaihuRock.h"

#include "Nature/NatureMesh.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Async/ParallelFor.h"
#include "Materials/MaterialInterface.h"   // (Chenghuai) RockMaterial
#include "ProceduralMeshComponent.h"

namespace MuseeRockGen
{
	namespace MN = MuseeNature;

	/** ChinesePlan.rockSilhouette: the N–S section, (plan y, height), from the north foot over both peaks to the south foot. */
	const FVector2D Silhouette[] = {
		FVector2D(24.30, 0.0), FVector2D(24.50, 0.93), FVector2D(24.36, 1.77), FVector2D(24.70, 2.52), FVector2D(24.50, 3.27),
		FVector2D(24.90, 4.08), FVector2D(25.30, 3.78), FVector2D(25.62, 4.20), FVector2D(26.02, 3.36), FVector2D(25.78, 2.60),
		FVector2D(26.20, 1.85), FVector2D(25.98, 0.93), FVector2D(26.30, 0.0)};
	constexpr double CentreY = 25.26;   // ChinesePlan.rock.y

	/** The Swift's four hollows (plan y, height, width, height), worn through from east to west. */
	const double Hollows[4][4] = {{25.14, 3.11, 0.32, 0.50}, {25.56, 1.98, 0.28, 0.76}, {24.96, 1.51, 0.20, 0.42}, {25.70, 3.36, 0.16, 0.42}};

	struct FTunnel { FVector Point; FVector Dir; double Radius; };
	struct FPit { FVector Centre; double Radius; };

	double SMax(double A, double B, double K)
	{
		const double H = FMath::Max(K - FMath::Abs(A - B), 0.0) / K;
		return FMath::Max(A, B) + H * H * K * 0.25;
	}

	class FRockField
	{
	public:
		FRockField(int32 InSeed, double InErosion) : Rng(InSeed * 7919 + 77), Erosion(InErosion), NoiseSeed(InSeed)
		{
			// The plan's section, smoothed (Catmull-Rom through its points), its feet carried below the ground.
			const int32 NumPoints = UE_ARRAY_COUNT(Silhouette);
			auto Point = [](int32 I) { return FVector2D(Silhouette[I].X - CentreY, Silhouette[I].Y); };
			Section.Add(FVector2D(Point(0).X, -0.5));
			for (int32 i = 0; i + 1 < NumPoints; ++i)
			{
				const FVector2D P0 = Point(FMath::Max(i - 1, 0)), P1 = Point(i), P2 = Point(i + 1), P3 = Point(FMath::Min(i + 2, NumPoints - 1));
				for (int32 k = 0; k < 8; ++k)
				{
					const double T = k / 8.0;
					Section.Add(0.5 * ((2.0 * P1) + (P2 - P0) * T + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * (T * T) + (3.0 * P1 - P0 - 3.0 * P2 + P3) * (T * T * T)));
				}
			}
			Section.Add(Point(NumPoints - 1));
			Section.Add(FVector2D(Point(NumPoints - 1).X, -0.5));
			Twist = Rng.FRandRange(0.0, MN::Tau);
			// More holes and pits, placed inside the body and on its skin.
			const int32 Extra = FMath::RoundToInt(9 * Erosion);
			for (int32 t = 0; t < Extra; ++t)
			{
				for (int32 Try = 0; Try < 20; ++Try)
				{
					const double Z = Rng.FRandRange(0.5, 3.5);
					const double Y = Rng.FRandRange(-0.8, 0.9);
					double HW = 0;
					if (!HalfWidth(Y, Z, HW) || HW < 0.25) { continue; }
					const double Yaw = FMath::DegreesToRadians(Rng.FRandRange(-50.0, 50.0));
					const double Pitch = FMath::DegreesToRadians(Rng.FRandRange(-25.0, 25.0));
					FTunnel& T = Tunnels.AddDefaulted_GetRef();
					T.Point = FVector(OffsetX(Z), Y, Z);
					T.Dir = FVector(FMath::Cos(Yaw) * FMath::Cos(Pitch), FMath::Sin(Yaw) * FMath::Cos(Pitch), FMath::Sin(Pitch));
					T.Radius = Rng.FRandRange(0.08, 0.2);
					break;
				}
			}
			const int32 NumPits = FMath::RoundToInt(55 * Erosion);
			for (int32 p = 0; p < NumPits; ++p)
			{
				for (int32 Try = 0; Try < 20; ++Try)
				{
					const double Z = Rng.FRandRange(0.2, 4.0);
					const double Y = Rng.FRandRange(-0.95, 1.05);
					double HW = 0;
					if (!HalfWidth(Y, Z, HW)) { continue; }
					const double Side = Rng.FRand() < 0.5 ? -1.0 : 1.0;
					FPit& Pit = Pits.AddDefaulted_GetRef();
					Pit.Radius = Rng.FRandRange(0.05, 0.17);
					Pit.Centre = FVector(OffsetX(Z) + Side * (HW + Pit.Radius * 0.35), Y, Z);
					break;
				}
			}
		}

		/** Negative inside the rock, positive outside, roughly in metres. */
		double Eval(const FVector& P0) const
		{
			// A slow warp bends the whole form, so nothing is straight or symmetrical.
			const FVector Warp(MN::Noise(P0 * 0.8, NoiseSeed + 91) - 0.5, MN::Noise(P0 * 0.8 + FVector(31.0), NoiseSeed + 92) - 0.5,
							   0.4 * (MN::Noise(P0 * 0.8 + FVector(57.0), NoiseSeed + 93) - 0.5));
			const FVector P = P0 + Warp * 0.28;
			const double Z = FMath::Max(P.Z, 0.01);
			// The E–W half-width, blurred up and down so the peaks taper instead of stepping.
			double HW = 0;
			for (int32 s = -2; s <= 2; ++s)
			{
				double Sample = 0;
				if (HalfWidth(P.Y, FMath::Max(Z + 0.12 * s, 0.01), Sample)) { HW += Sample * 0.2; }
			}
			HW *= 0.8 + 0.4 * MN::Noise(FVector(Z * 0.9, 3.3, 7.7), NoiseSeed + 94);
			const double D2 = SectionDistance(FVector2D(P.Y, Z));
			const double DX = FMath::Abs(P.X - OffsetX(Z)) - HW;
			double F = SMax(D2, DX, 0.12);
			F = FMath::Max(F, -(P.Z + 0.1));   // closed under the ground
			if (Erosion > 0)
			{
				for (int32 h = 0; h < 4; ++h)
				{
					const double Y0 = Hollows[h][0] - CentreY + 0.06 * FMath::Sin(3.0 * P.X + h);
					const double Z0 = Hollows[h][1] + 0.05 * FMath::Cos(2.5 * P.X + h);
					const double A = 0.5 * Hollows[h][2] * FMath::Min(1.0, Erosion), B = 0.5 * Hollows[h][3] * FMath::Min(1.0, Erosion);
					const double T = (FMath::Sqrt(FMath::Square((P.Y - Y0) / A) + FMath::Square((P.Z - Z0) / B)) - 1.0) * FMath::Min(A, B);
					F = SMax(F, -T, 0.07);
				}
				for (const FTunnel& Tunnel : Tunnels)
				{
					const FVector Rel = P - Tunnel.Point;
					const double Dist = (Rel - Tunnel.Dir * FVector::DotProduct(Rel, Tunnel.Dir)).Size() - Tunnel.Radius;
					F = SMax(F, -Dist, 0.06);
				}
				for (const FPit& Pit : Pits)
				{
					const double Dist = FVector::Dist(P, Pit.Centre) - Pit.Radius;
					if (Dist < 0.2) { F = SMax(F, -Dist, 0.05); }
				}
				// Water-worn wrinkles, drawn out vertically, and a fine grain.
				const double Wrinkle = MN::Fbm(FVector(P.X * 1.6, P.Y * 1.6, P.Z * 0.8), 3, NoiseSeed) - 0.5;
				const double Ripple = MN::Fbm(P * 4.0, 2, NoiseSeed + 2) - 0.5;
				const double Grain = MN::Noise(P * 9.0, NoiseSeed + 5) - 0.5;
				F += Erosion * (0.09 * Wrinkle + 0.04 * Ripple + 0.02 * Grain);
			}
			return F;
		}

		FVector Gradient(const FVector& P, double H) const
		{
			return FVector(Eval(P + FVector(H, 0, 0)) - Eval(P - FVector(H, 0, 0)), Eval(P + FVector(0, H, 0)) - Eval(P - FVector(0, H, 0)),
						   Eval(P + FVector(0, 0, H)) - Eval(P - FVector(0, 0, H)));
		}

	private:
		FRandomStream Rng;
		double Erosion;
		int32 NoiseSeed;
		double Twist = 0;
		TArray<FVector2D> Section;
		TArray<FTunnel> Tunnels;
		TArray<FPit> Pits;

		/** The rock twists a little from east to west as it rises. */
		double OffsetX(double Z) const { return 0.08 * FMath::Sin(1.3 * Z + Twist); }

		/** Signed distance to the section (negative inside). */
		double SectionDistance(const FVector2D& Q) const
		{
			bool bIn = false;
			double Best = TNumericLimits<double>::Max();
			for (int32 i = 0, j = Section.Num() - 1; i < Section.Num(); j = i++)
			{
				const FVector2D& A = Section[i];
				const FVector2D& B = Section[j];
				if (((A.Y > Q.Y) != (B.Y > Q.Y)) && (Q.X < (B.X - A.X) * (Q.Y - A.Y) / (B.Y - A.Y) + A.X)) { bIn = !bIn; }
				const FVector2D AB = B - A;
				const double T = FMath::Clamp(FVector2D::DotProduct(Q - A, AB) / FMath::Max(AB.SizeSquared(), 1e-12), 0.0, 1.0);
				Best = FMath::Min(Best, (A + AB * T - Q).Size());
			}
			return bIn ? -Best : Best;
		}

		/**
		 * The E–W half-width at (y, z): across the section's span at height z the rock is round,
		 * as wide as it is deep (× 1.05), as the Swift made it.
		 */
		bool HalfWidth(double Y, double Z, double& OutHW) const
		{
			double Lo = TNumericLimits<double>::Max(), Hi = -TNumericLimits<double>::Max();
			for (int32 i = 0, j = Section.Num() - 1; i < Section.Num(); j = i++)
			{
				const FVector2D& A = Section[i];
				const FVector2D& B = Section[j];
				if (A.Y == B.Y) { continue; }
				if ((Z >= A.Y && Z < B.Y) || (Z >= B.Y && Z < A.Y))
				{
					const double Cross = A.X + (Z - A.Y) * (B.X - A.X) / (B.Y - A.Y);
					Lo = FMath::Min(Lo, Cross);
					Hi = FMath::Max(Hi, Cross);
				}
			}
			if (Hi < Lo || Y < Lo || Y > Hi)
			{
				OutHW = 0;
				return false;
			}
			const double C = 0.5 * (Lo + Hi), R = FMath::Max(0.5 * (Hi - Lo), 1e-4);
			OutHW = 1.05 * R * FMath::Sqrt(FMath::Max(0.0, 1.0 - FMath::Square((Y - C) / R)));
			return true;
		}
	};

	void BuildRock(int32 Seed, double Voxel, double Erosion, bool bSnow, FNatureMesh& Mesh)
	{
		const FRockField Field(Seed, Erosion);
		const FVector Lo(-1.35, -1.15, -0.14), Hi(1.35, 1.25, 4.4);
		const int32 NX = FMath::CeilToInt((Hi.X - Lo.X) / Voxel) + 1;
		const int32 NY = FMath::CeilToInt((Hi.Y - Lo.Y) / Voxel) + 1;
		const int32 NZ = FMath::CeilToInt((Hi.Z - Lo.Z) / Voxel) + 1;
		auto Idx = [NX, NY](int32 I, int32 J, int32 K) { return (K * NY + J) * NX + I; };
		auto At = [&](int32 I, int32 J, int32 K) { return Lo + FVector(I, J, K) * Voxel; };
		TArray<float> Values;
		Values.SetNumUninitialized(NX * NY * NZ);
		ParallelFor(NZ, [&](int32 K)
		{
			for (int32 J = 0; J < NY; ++J)
			{
				for (int32 I = 0; I < NX; ++I) { Values[Idx(I, J, K)] = static_cast<float>(Field.Eval(At(I, J, K))); }
			}
		});

		// Surface nets: a vertex in every cell the surface crosses, at the mean of the edge crossings.
		const int32 Corner[8][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}};
		const int32 Edge[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
		TArray<int32> CellVertex;
		CellVertex.Init(INDEX_NONE, NX * NY * NZ);
		const FLinearColor Pale = MN::Srgb(0xBAB6AC), Blue = MN::Srgb(0xA4A5A0), Warm = MN::Srgb(0xC0B49E), Snow = MN::Srgb(0xF4F6F8);
		const double GradStep = Voxel * 0.5;
		for (int32 K = 0; K + 1 < NZ; ++K)
		{
			for (int32 J = 0; J + 1 < NY; ++J)
			{
				for (int32 I = 0; I + 1 < NX; ++I)
				{
					float V[8];
					int32 Inside = 0;
					for (int32 c = 0; c < 8; ++c)
					{
						V[c] = Values[Idx(I + Corner[c][0], J + Corner[c][1], K + Corner[c][2])];
						Inside += V[c] < 0 ? 1 : 0;
					}
					if (Inside == 0 || Inside == 8) { continue; }
					FVector Sum = FVector::ZeroVector;
					int32 Count = 0;
					for (int32 e = 0; e < 12; ++e)
					{
						const float A = V[Edge[e][0]], B = V[Edge[e][1]];
						if ((A < 0) == (B < 0)) { continue; }
						const double T = A / (A - B);
						const FVector PA = At(I + Corner[Edge[e][0]][0], J + Corner[Edge[e][0]][1], K + Corner[Edge[e][0]][2]);
						const FVector PB = At(I + Corner[Edge[e][1]][0], J + Corner[Edge[e][1]][1], K + Corner[Edge[e][1]][2]);
						Sum += FMath::Lerp(PA, PB, T);
						++Count;
					}
					const FVector P = Sum / FMath::Max(1, Count);
					const FVector N = Field.Gradient(P, GradStep).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
					// How open the point is: the field a little way out along the normal.
					const double Near = Field.Eval(P + N * 0.08), Far = Field.Eval(P + N * 0.25);
					const double Open = 0.5 * FMath::Clamp(Near / 0.08, 0.0, 1.0) + 0.5 * FMath::Clamp(Far / 0.25, 0.0, 1.0);
					const double Occlusion = 0.3 + 0.7 * FMath::Pow(Open, 0.8);
					const double Tone = MN::Fbm(P * 0.9, 3, Seed + 11);
					FLinearColor Colour = Tone < 0.5 ? MN::Mix(Blue, Pale, Tone * 2.0) : MN::Mix(Pale, Warm, (Tone - 0.5) * 2.0);
					if (bSnow && N.Z > 0.45) { Colour = MN::Mix(Colour, Snow, FMath::Clamp((N.Z - 0.45) / 0.3, 0.0, 1.0) * Open); }
					FVector Tangent = FVector::CrossProduct(N, FVector::UpVector);
					if (Tangent.SizeSquared() < 1e-6) { Tangent = FVector::ForwardVector; }
					FNatureWind Still;
					Still.Height = P.Z;
					CellVertex[Idx(I, J, K)] = Mesh.Vertex(P, N, Tangent, FVector2D(P.X + P.Y, P.Z), Still, Colour, Occlusion);
				}
			}
		}
		// A quad across every grid edge the surface crosses, joining the four cells round it.
		auto Cell = [&](int32 I, int32 J, int32 K) { return (I < 0 || J < 0 || K < 0 || I >= NX - 1 || J >= NY - 1 || K >= NZ - 1) ? INDEX_NONE : CellVertex[Idx(I, J, K)]; };
		auto Face = [&](int32 A, int32 B, int32 C, int32 D) { if (A != INDEX_NONE && B != INDEX_NONE && C != INDEX_NONE && D != INDEX_NONE) { Mesh.Quad(A, B, C, D); } };
		for (int32 K = 0; K < NZ; ++K)
		{
			for (int32 J = 0; J < NY; ++J)
			{
				for (int32 I = 0; I < NX; ++I)
				{
					const bool bIn = Values[Idx(I, J, K)] < 0;
					if (I + 1 < NX && (Values[Idx(I + 1, J, K)] < 0) != bIn) { Face(Cell(I, J - 1, K - 1), Cell(I, J, K - 1), Cell(I, J, K), Cell(I, J - 1, K)); }
					if (J + 1 < NY && (Values[Idx(I, J + 1, K)] < 0) != bIn) { Face(Cell(I - 1, J, K - 1), Cell(I, J, K - 1), Cell(I, J, K), Cell(I - 1, J, K)); }
					if (K + 1 < NZ && (Values[Idx(I, J, K + 1)] < 0) != bIn) { Face(Cell(I - 1, J - 1, K), Cell(I, J - 1, K), Cell(I, J, K), Cell(I - 1, J, K)); }
				}
			}
		}
	}
}

AMuseeTaihuRock::AMuseeTaihuRock()
{
	RockCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("RockCollision"));
	RockCollision->SetupAttachment(RootComponent);
	RockCollision->SetMobility(EComponentMobility::Static);
	RockCollision->SetCapsuleSize(95.f, 210.f);
	RockCollision->SetRelativeLocation(FVector(0, 5, 210));
	RockCollision->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RockCollision->SetHiddenInGame(true);
}

void AMuseeTaihuRock::BuildPlant()
{
	FNatureMesh Mesh;
	const int32 Term = EffectiveTerm();
	MuseeRockGen::BuildRock(Seed, FMath::Clamp(double(Voxel), 0.02, 0.1), Erosion, Term >= 19 && Term <= 23, Mesh);
	// (Chenghuai) A full path names the material directly; a bare name is one of the Nature folder's.
	const bool bPath = RockMaterial.StartsWith(TEXT("/"));
	WriteSection(0, Mesh, bPath || RockMaterial.IsEmpty() ? TEXT("MI_Rock_Taihu") : *RockMaterial);
	if (bPath && Plant && !Mesh.IsEmpty())
	{
		if (UMaterialInterface* Material = Cast<UMaterialInterface>(FSoftObjectPath(RockMaterial).TryLoad())) { Plant->SetMaterial(0, Material); }
	}
}
