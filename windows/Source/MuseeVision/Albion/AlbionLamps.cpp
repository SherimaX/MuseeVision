#include "Albion/AlbionLamps.h"

#include "Albion/AlbionKit.h"
#include "Albion/AlbionPlan.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SpotLight.h"
#include "Components/SpotLightComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "ProceduralMeshComponent.h"

namespace AlbionLampsImpl
{
	using namespace AlbionKit;
	namespace AP = AlbionPlan;

	/** One electrolier: where it hangs (plan metres), from what height, its body's height, its arms. */
	struct FFitting
	{
		double X, Y;
		double TopZ;        // the clamp's underside (the arch's soffit flange, or the porch ceiling)
		double BodyZ;       // the turned body's rim, where the arms spring
		int32 Arms;
		bool bChain;        // a chain down to the stem (the porch's hangs by its stem alone)
	};

	constexpr double GlobeR = 0.085, NeckR = 0.45 * GlobeR;
	constexpr double ArmReach = 0.505;           // the galleries' axes from the fitting's
	constexpr double ArmRise = 0.12;             // the arm's end (the gallery's top) above the rim

	/** The arcade arch's soffit at its crown: the arch's inner flange (AlbionIron ArcadeBay: 0.142 … 0.16 m under the arc). */
	constexpr double ArchSoffit = AP::ArcadeCrown - 0.16;

	TArray<FFitting> Fittings()
	{
		TArray<FFitting> Out;
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			for (int32 B = 0; B < 6; ++B) { Out.Add({Side * AP::ColumnX, AP::BayCentreY(B), ArchSoffit, 5.85, 6, true}); }
		}
		// The porch: under its oak ceiling, midway between the Rotunda's door and the court's.
		Out.Add({0.0, 0.5 * (AP::DrumOuter + AP::Y0 - AP::Wall), AP::PorchCeiling, 5.30, 4, false});
		return Out;
	}

	/** The globes' centre height for a fitting (the gallery at the arm's end holds the neck, the ball hangs below). */
	double GlobeZ(const FFitting& F)
	{
		const double G = F.BodyZ + ArmRise;                 // the gallery's top
		const double NeckTop = G - 0.035;
		const double ZCut = NeckTop - 0.12 * GlobeR;
		return ZCut - FMath::Sqrt(GlobeR * GlobeR - NeckR * NeckR);
	}

	/** The cased opal globe: a ball with a neck and a rolled lip. UV0 = (R, 0) on the ball, (R, 1) on the neck (M_GlobeLamp). */
	void Globe(FMeshData& M, const FVector2D& C, double Zg)
	{
		const double R = GlobeR, Rn = NeckR;
		const double ZCut = Zg + FMath::Sqrt(R * R - Rn * Rn), NeckTop = ZCut + 0.12 * R;
		const double ThetaNeck = FMath::Asin(Rn / R);
		TArray<FVector2D> Prof, Nrm, UV;
		constexpr int32 Rings = 24;
		for (int32 J = 0; J <= Rings; ++J)
		{
			const double Th = kPi - (kPi - ThetaNeck) * J / Rings;
			const FVector2D D(FMath::Sin(Th), FMath::Cos(Th));
			Prof.Add(FVector2D(R * D.X, Zg + R * D.Y));
			Nrm.Add(D);
			UV.Add(FVector2D(R, 0));
		}
		const TArray<FVector2D> Neck = {FVector2D(Rn, ZCut), FVector2D(Rn, NeckTop - 0.004), FVector2D(Rn + 0.003, NeckTop - 0.002), FVector2D(Rn - 0.003, NeckTop)};
		const TArray<FVector2D> NeckN = {FVector2D(1, 0), FVector2D(1, 0), FVector2D(0.7, 0.7), FVector2D(0, 1)};
		for (int32 K = 0; K < Neck.Num(); ++K)
		{
			Prof.Add(Neck[K]);
			Nrm.Add(NeckN[K]);
			UV.Add(FVector2D(R, 1));
		}
		constexpr int32 Seg = 32;
		const int32 Base = M.Positions.Num();
		for (int32 J = 0; J < Prof.Num(); ++J)
		{
			for (int32 K = 0; K <= Seg; ++K)
			{
				const double A = 2.0 * kPi * (K % Seg) / Seg;
				const FVector Radial(FMath::Cos(A), FMath::Sin(A), 0.0);
				M.Vertex(FVector(C.X, C.Y, 0.0) + Radial * Prof[J].X + FVector(0, 0, Prof[J].Y), Radial * Nrm[J].X + FVector(0, 0, Nrm[J].Y), UV[J]);
			}
		}
		for (int32 J = 0; J + 1 < Prof.Num(); ++J)
		{
			for (int32 K = 0; K < Seg; ++K)
			{
				const int32 A = Base + J * (Seg + 1) + K;
				M.Quad(A, A + 1, A + Seg + 2, A + Seg + 1);
			}
		}
	}

	/** A round bar Ø 2r along a path in the vertical plane through the fitting's axis at angle Theta: (r, z) points. */
	void Arm(FMeshData& M, const FVector2D& C, double Theta, const TArray<FVector2D>& Path, double Radius)
	{
		const FVector Radial(FMath::Cos(Theta), FMath::Sin(Theta), 0.0);
		const FVector Across(-FMath::Sin(Theta), FMath::Cos(Theta), 0.0);
		TArray<FVector2D> Round;
		for (int32 k = 0; k < 10; ++k) { const double A = 2.0 * kPi * k / 10.0; Round.Add(FVector2D(Radius * FMath::Cos(A), Radius * FMath::Sin(A))); }
		TArray<FStation> St;
		for (int32 i = 0; i < Path.Num(); ++i)
		{
			const FVector2D D = Path[FMath::Min(i + 1, Path.Num() - 1)] - Path[FMath::Max(i - 1, 0)];
			const FVector T = (Radial * D.X + FVector(0, 0, D.Y)).GetSafeNormal();
			const FVector A = Across;
			const FVector B = FVector::CrossProduct(T, A);
			St.Add({FVector(C.X, C.Y, 0.0) + Radial * Path[i].X + FVector(0, 0, Path[i].Y), A, B});
		}
		SweepStations(M, St, Round);
	}

	/** A smooth path through control points (Catmull–Rom), Steps between each pair. */
	TArray<FVector2D> Smooth(const TArray<FVector2D>& P, int32 Steps)
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i + 1 < P.Num(); ++i)
		{
			const FVector2D P0 = P[FMath::Max(i - 1, 0)], P1 = P[i], P2 = P[i + 1], P3 = P[FMath::Min(i + 2, P.Num() - 1)];
			for (int32 s = 0; s < Steps; ++s)
			{
				const double T = double(s) / Steps, T2 = T * T, T3 = T2 * T;
				Out.Add(0.5 * ((2.0 * P1) + (-P0 + P2) * T + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * T2 + (-P0 + 3.0 * P1 - 3.0 * P2 + P3) * T3));
			}
		}
		Out.Add(P.Last());
		return Out;
	}

	/** One electrolier into the hangers (chain, clamp, stem) and bodies (body, arms, ring, galleries) and globes. */
	void Electrolier(FMeshData& Hang, FMeshData& Body, FMeshData& Glass, const FFitting& F)
	{
		const FVector2D C(F.X, F.Y);
		const FVector Base(F.X, F.Y, 0.0);
		const double B = F.BodyZ;
		// The clamp on the arch's inner flange (a block and two cheeks) and an eye under it, and the chain: round links of
		// 7 mm iron, each turned a quarter to the last, down to the loop on the stem's canopy; or the porch's ceiling rose.
		double StemTop = F.TopZ - 0.06;
		if (F.bChain)
		{
			constexpr double Major = 0.019, Minor = 0.0035, Pitch = 2.0 * (Major - Minor), Eye = 0.014;
			Box(Hang, FVector(F.X - 0.030, F.Y - 0.055, F.TopZ - 0.004), FVector(F.X + 0.030, F.Y + 0.055, F.TopZ + 0.022));
			for (int32 s = -1; s <= 1; s += 2)
			{
				const double X0 = s > 0 ? F.X + 0.030 : F.X - 0.038;
				Box(Hang, FVector(X0, F.Y - 0.04, F.TopZ - 0.004), FVector(X0 + 0.008, F.Y + 0.04, F.TopZ + 0.04));
			}
			Lathe(Hang, Base, {{0.0, F.TopZ - 0.030, false}, {0.012, F.TopZ - 0.030, false}, {0.016, F.TopZ - 0.018, true}, {0.012, F.TopZ - 0.004, false},
							   {0.0, F.TopZ - 0.004, false}}, 16);
			const double Ze = F.TopZ - 0.030 - Eye;
			Torus(Hang, FVector(F.X, F.Y, Ze), FVector(1, 0, 0), Eye, 0.0045, 14, 6);
			const double Z1 = Ze - Eye - Major + 2.0 * Minor;
			const double Nominal = B + 1.30 + 0.016;
			const int32 N = FMath::Max(1, FMath::RoundToInt32((Z1 - (Nominal + Eye + Major - 2.0 * Minor)) / Pitch) + 1);
			for (int32 K = 0; K < N; ++K)
			{
				Torus(Hang, FVector(F.X, F.Y, Z1 - K * Pitch), (K % 2) ? FVector(1, 0, 0) : FVector(0, 1, 0), Major, Minor, 14, 6);
			}
			const double Zl = Z1 - (N - 1) * Pitch - Major - Eye + 2.0 * Minor;
			Torus(Hang, FVector(F.X, F.Y, Zl), (N % 2) ? FVector(1, 0, 0) : FVector(0, 1, 0), Eye, 0.0045, 14, 6);
			StemTop = Zl - Eye - 0.002;
		}
		else
		{
			Lathe(Hang, Base, {{0.0, F.TopZ - 0.03, false}, {0.03, F.TopZ - 0.03, false}, {0.07, F.TopZ - 0.012, true}, {0.10, F.TopZ - 0.004, false},
							   {0.10, F.TopZ, false}}, 32);
		}
		// The stem: a tube with a knop and a vase, from its canopy to the body's dome.
		const TArray<FLathePoint> Stem = {
			{0.0, B + 0.07, false}, {0.014, B + 0.07, false}, {0.014, B + 0.38, false}, {0.024, B + 0.41, true}, {0.034, B + 0.47, true},
			{0.030, B + 0.53, true}, {0.018, B + 0.57, true}, {0.014, B + 0.60, false}, {0.014, StemTop - 0.12, false}, {0.022, StemTop - 0.10, true},
			{0.026, StemTop - 0.085, true}, {0.022, StemTop - 0.07, true}, {0.014, StemTop - 0.06, false}, {0.014, StemTop - 0.045, false},
			{0.038, StemTop - 0.03, true}, {0.042, StemTop - 0.012, true}, {0.020, StemTop, false}, {0.0, StemTop, false}};
		Lathe(Hang, Base, Stem, 24);

		// The body: an acorn drop, a bowl flaring to a beaded rim, a dome under the stem.
		const TArray<FLathePoint> Bowl = {
			{0.0, B - 0.30, false}, {0.010, B - 0.292, true}, {0.020, B - 0.27, true}, {0.024, B - 0.245, true}, {0.020, B - 0.215, true},
			{0.013, B - 0.20, false}, {0.011, B - 0.18, false}, {0.018, B - 0.165, true}, {0.011, B - 0.15, false}, {0.012, B - 0.13, false},
			{0.030, B - 0.11, true}, {0.052, B - 0.075, true}, {0.068, B - 0.04, true}, {0.076, B - 0.012, true}, {0.082, B - 0.004, true},
			{0.082, B + 0.006, true}, {0.074, B + 0.014, false}, {0.066, B + 0.02, false}, {0.052, B + 0.045, true}, {0.030, B + 0.066, true},
			{0.014, B + 0.074, false}, {0.0, B + 0.074, false}};
		Lathe(Body, Base, Bowl, 32);

		// The arms: out from the rim, down a little, then up and over in a scroll to the gallery.
		const TArray<FVector2D> Ctrl = {FVector2D(0.06, B), FVector2D(0.16, B - 0.025), FVector2D(0.27, B - 0.005), FVector2D(0.35, B + 0.05),
										FVector2D(0.39, B + 0.13), FVector2D(0.425, B + 0.185), FVector2D(0.47, B + 0.20), FVector2D(0.498, B + 0.175),
										FVector2D(ArmReach, B + ArmRise + 0.012), FVector2D(ArmReach, B + ArmRise)};
		const TArray<FVector2D> Path = Smooth(Ctrl, 6);
		const double Zg = GlobeZ(F);
		for (int32 a = 0; a < F.Arms; ++a)
		{
			const double Theta = 2.0 * kPi * a / F.Arms + (F.Arms == 4 ? 0.25 * kPi : 0.0);
			Arm(Body, C, Theta, Path, 0.0085);
			// A small scroll under each arm, from the rim to the arm's rise.
			Arm(Body, C, Theta, Smooth({FVector2D(0.07, B - 0.03), FVector2D(0.17, B - 0.06), FVector2D(0.26, B - 0.045), FVector2D(0.30, B - 0.01),
										FVector2D(0.29, B + 0.015), FVector2D(0.27, B + 0.02)}, 5), 0.005);
			// The gallery at the arm's end: a band round the globe's neck and a beaded skirt over its shoulder.
			const FVector2D G = C + FVector2D(FMath::Cos(Theta), FMath::Sin(Theta)) * ArmReach;
			const double GTop = B + ArmRise;
			const double ZCut = Zg + FMath::Sqrt(GlobeR * GlobeR - NeckR * NeckR);
			Lathe(Body, FVector(G.X, G.Y, 0.0),
				  {{NeckR, ZCut - 0.010, false}, {0.060, ZCut - 0.010, false}, {0.064, ZCut - 0.006, true}, {0.059, ZCut - 0.002, false},
				   {0.047, ZCut + 0.006, false}, {0.045, ZCut + 0.030, false}, {0.037, ZCut + 0.036, true}, {0.016, GTop - 0.004, false},
				   {0.016, GTop, false}, {0.0, GTop, false}}, 24);
			// Three thumb screws through the band.
			for (int32 k = 0; k < 3; ++k)
			{
				const double A = Theta + 2.0 * kPi * k / 3.0 + kPi / 3.0;
				const FVector D(FMath::Cos(A), FMath::Sin(A), 0.0);
				const FVector P0 = FVector(G.X, G.Y, ZCut + 0.018) + D * 0.045;
				Ball(Body, P0 + D * 0.006, 0.006, 10, 0.8);
			}
			Globe(Glass, G, Zg);
		}
		// The corona: a ring tying the arms where they run level.
		Torus(Body, FVector(F.X, F.Y, B - 0.012), FVector(0, 0, 1), 0.27, 0.0075, 64, 8);
		Torus(Body, FVector(F.X, F.Y, B - 0.012), FVector(0, 0, 1), 0.30, 0.004, 64, 6);
	}

	/** The vaults' glass over plan x (the transverse section; AlbionPlan). */
	double GlassZ(double X)
	{
		const double AX = FMath::Abs(X);
		if (AX <= AP::NaveHalf) { return AP::Spring + FMath::Sqrt(FMath::Max(0.0, FMath::Square(AP::NaveRadius) - FMath::Square(AX + AP::NaveCentreOffset))); }
		const double D = FMath::Abs(AX - AP::AisleCentreX);
		return AP::Spring + FMath::Sqrt(FMath::Max(0.0, FMath::Square(AP::AisleRadius) - FMath::Square(D + AP::AisleCentreOffset)));
	}

	/** A round sleeve along an axis (A0 … A1 along Dir from P), radius R, capped. */
	void Sleeve(FMeshData& M, const FVector& P, const FVector& Dir, double A0, double A1, double R, int32 Seg = 20)
	{
		FVector U, V;
		Dir.FindBestAxisVectors(U, V);
		if (FVector::DotProduct(FVector::CrossProduct(U, V), Dir) < 0.0) { V = -V; }
		TArray<FVector2D> Round;
		for (int32 k = 0; k < Seg; ++k) { const double A = 2.0 * kPi * k / Seg; Round.Add(FVector2D(R * FMath::Cos(A), R * FMath::Sin(A))); }
		SweepStations(M, {{P + Dir * A0, U, V}, {P + Dir * A1, U, V}}, Round);
	}

	/**
	 * An accent projector where gallery_lights.py put an Albion spot (Scripts/albion_place.py mount): a turned can with a
	 * bezel and a finned back, pivoting in a yoke whose stem rises to the iron: a clamp under the arcade girder's bottom
	 * flange (the side walls' works, the cases), or to a main rib's inner flange (the end walls' works). Plan metres.
	 */
	void Projector(FMeshData& M, const FVector& P, const FVector& Dir)
	{
		const FVector D = Dir.GetSafeNormal();
		FVector Across = FVector::CrossProduct(D, FVector::UpVector);
		if (!Across.Normalize()) { Across = FVector(1, 0, 0); }
		// The can: its lens behind the light's origin by more than the light's source radius (gallery_lights.py: 10 cm), so
		// no part of the fitting sits inside its own light.
		Sleeve(M, P, D, -0.300, -0.135, 0.043);
		Sleeve(M, P, D, -0.135, -0.118, 0.048);                    // the bezel
		Sleeve(M, P, D, -0.315, -0.300, 0.036);                    // the back cap
		for (int32 f = 0; f < 4; ++f) { Sleeve(M, P, D, -0.29 + 0.035 * f, -0.278 + 0.035 * f, 0.049, 16); }   // cooling fins
		// The yoke: two flat arms from the pivot, up to a cross bar, and the stem from its middle.
		const FVector Pivot = P - D * 0.215;
		const double YokeTop = Pivot.Z + 0.10;
		for (int32 s = -1; s <= 1; s += 2)
		{
			const FVector Arm0 = Pivot + Across * (s * 0.056);
			Beam(M, Arm0 - FVector(0, 0, 0.012), FVector(Arm0.X, Arm0.Y, YokeTop), Across, 0.004, 0.012);
			Sleeve(M, Arm0, Across * s, -0.004, 0.010, 0.011, 12);   // the pivot's knob
		}
		Beam(M, FVector(Pivot.X, Pivot.Y, YokeTop) - Across * 0.06, FVector(Pivot.X, Pivot.Y, YokeTop) + Across * 0.06, FVector::UpVector, 0.006, 0.012);
		// Up to the iron.
		const bool bGirder = FMath::Abs(FMath::Abs(P.X) - AP::ColumnX) < 0.8 && P.Z < AP::GirderBottom;
		const double Top = bGirder ? AP::GirderBottom - 0.004 : GlassZ(P.X) - 0.41;
		Sleeve(M, FVector(Pivot.X, Pivot.Y, YokeTop), FVector::UpVector, 0.0, Top - YokeTop - 0.012, 0.011, 12);
		if (bGirder)
		{
			// A clamp plate from the stem under the flange's edge, bolted through it.
			const double Side = P.X > 0 ? 1.0 : -1.0;
			const double X0 = Side * (AP::ColumnX + 0.25), X1 = Pivot.X + Side * 0.03;
			Box(M, FVector(FMath::Min(X0, X1), Pivot.Y - 0.035, Top - 0.012), FVector(FMath::Max(X0, X1), Pivot.Y + 0.035, Top));
			Sleeve(M, FVector(X0 + Side * 0.02, Pivot.Y, Top - 0.018), FVector::UpVector, 0.0, 0.006, 0.012, 6);
		}
		else
		{
			Box(M, FVector(Pivot.X - 0.05, Pivot.Y - 0.03, Top - 0.012), FVector(Pivot.X + 0.05, Pivot.Y + 0.03, Top + 0.004));
		}
	}
}

AAlbionLamps::AAlbionLamps()
{
	using namespace AlbionLampsImpl;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	auto Make = [this](const TCHAR* Name, bool bShadow)
	{
		UProceduralMeshComponent* C = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		C->SetupAttachment(RootComponent);
		C->SetMobility(EComponentMobility::Static);
		C->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);   // out of reach
		C->SetCastShadow(bShadow);
		return C;
	};
	Hangers = Make(TEXT("Hangers"), true);
	Bodies = Make(TEXT("Bodies"), false);
	Globes = Make(TEXT("Globes"), false);

	const TArray<FFitting> List = Fittings();
	for (int32 i = 0; i < List.Num(); ++i)
	{
		const FFitting& F = List[i];
		UPointLightComponent* L = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("Lamp%02d"), i));
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		L->SetRelativeLocation(AlbionPlan::At(F.X, F.Y, GlobeZ(F)));
		L->IntensityUnits = ELightUnits::Lumens;
		L->Intensity = 0.f;
		L->bUseTemperature = true;
		L->Temperature = LampKelvin;
		L->SourceRadius = 12.f;   // (about a globe's: a sphere the size of the ring made one broad streak in the polished shafts)
		L->SoftSourceRadius = 0.f;
		L->AttenuationRadius = 2600.f;
		L->CastShadows = true;
		L->SetVisibility(false);
		Lights.Add(L);
	}

	Court = CreateDefaultSubobject<UBoxComponent>(TEXT("Court"));
	Court->SetupAttachment(RootComponent);
	Court->SetMobility(EComponentMobility::Static);
	Court->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Court->SetGenerateOverlapEvents(false);
	const double YN = AP::DrumOuter - 0.5, YS = AP::Y1 + AP::Wall;
	Court->SetRelativeLocation(AP::At(0.0, 0.5 * (YN + YS), 9.0));
	Court->SetBoxExtent(FVector(AP::X1 + AP::Wall, 0.5 * (YS - YN), 9.0) * 100.0);
	NightBalance = CreateDefaultSubobject<UPostProcessComponent>(TEXT("NightBalance"));
	NightBalance->SetupAttachment(Court);
	NightBalance->bUnbound = false;
	NightBalance->Priority = 12.f;
	NightBalance->BlendRadius = 300.f;
	NightBalance->BlendWeight = 0.f;
	NightBalance->Settings.bOverride_WhiteTemp = true;
	NightBalance->Settings.WhiteTemp = NightWhiteTemp;
	NightBalance->Settings.bOverride_AutoExposureBias = true;
	NightBalance->Settings.AutoExposureBias = NightExposureBias;

	BronzeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Bronze_Brushed.M_Bronze_Brushed")));
	GlobeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_GlobeLamp.M_GlobeLamp")));
	DaylightParameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/MPC_Musee.MPC_Musee")));
	Tags.AddUnique(FName(TEXT("musee.nobake")));
}

TArray<FVector> AAlbionLamps::GetLampPositions() const
{
	TArray<FVector> Out;
	for (const UPointLightComponent* L : Lights) { if (L) { Out.Add(L->GetComponentLocation()); } }
	return Out;
}

void AAlbionLamps::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AAlbionLamps::BeginPlay()
{
	Super::BeginPlay();
	Build();
	if (UMaterialInterface* G = GlobeMaterial.LoadSynchronous())
	{
		GlobeInstance = UMaterialInstanceDynamic::Create(G, this);
		Globes->SetMaterial(0, GlobeInstance);
	}
	for (UPointLightComponent* L : Lights)
	{
		if (L)
		{
			L->SetTemperature(LampKelvin);
		}
	}
	NightBalance->Settings.WhiteTemp = NightWhiteTemp;
	NightBalance->Settings.AutoExposureBias = NightExposureBias;
	LastNight = -1.f;
	bLastNear = true;
	SetNight(1.f);
}

void AAlbionLamps::Build()
{
	using namespace AlbionLampsImpl;
	FMeshData Hang, Body, Glass;
	for (const FFitting& F : Fittings()) { Electrolier(Hang, Body, Glass, F); }
	// The accent spots' projectors, where gallery_lights.py put them (in a game; the editor's map has them too).
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ASpotLight> It(World); It; ++It)
		{
			bool bAccent = false, bAlbion = false;
			for (const FName& Tag : It->Tags)
			{
				bAccent |= Tag == FName(TEXT("musee.accent"));
				bAlbion |= Tag == FName(TEXT("musee.wing:Albion"));
			}
			if (!bAccent || !bAlbion) { continue; }
			const USpotLightComponent* L = It->SpotLightComponent;
			if (!L) { continue; }
			Projector(Body, L->GetComponentLocation() / 100.0, L->GetForwardVector());
		}
	}
	Hangers->ClearAllMeshSections();
	Bodies->ClearAllMeshSections();
	Globes->ClearAllMeshSections();
	Hang.Write(Hangers, 0, false);
	Body.Write(Bodies, 0, false);
	Glass.Write(Globes, 0, false);
	if (UMaterialInterface* M = BronzeMaterial.LoadSynchronous())
	{
		Hangers->SetMaterial(0, M);
		Bodies->SetMaterial(0, M);
	}
	if (UMaterialInterface* G = GlobeMaterial.LoadSynchronous()) { Globes->SetMaterial(0, G); }
}

void AAlbionLamps::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	float Daylight = 1.f;
	if (UMaterialParameterCollection* Collection = DaylightParameters.LoadSynchronous())
	{
		Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, Collection, TEXT("Daylight"));
	}
	const float Night = 1.f - FMath::SmoothStep(0.f, 0.2f, Daylight);
	if (FMath::Abs(Night - LastNight) > 0.01f) { SetNight(Night); }
	// The lights burn only while they can be seen (the court, the porch, the Rotunda looking in, the garden outside).
	bool bNear = true;
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager)
		{
			bNear = FBox(AlbionPlan::At(-40.0, -15.0, -5.0), AlbionPlan::At(40.0, 90.0, 60.0)).IsInside(PC->PlayerCameraManager->GetCameraLocation());
		}
	}
	if (bNear != bLastNear)
	{
		bLastNear = bNear;
		LastNight = -1.f;
		SetNight(Night);
	}
}

void AAlbionLamps::SetNight(float Night)
{
	using namespace AlbionLampsImpl;
	LastNight = Night;
	const TArray<FFitting> List = Fittings();
	for (int32 i = 0; i < Lights.Num() && i < List.Num(); ++i)
	{
		UPointLightComponent* L = Lights[i];
		if (!L) { continue; }
		const bool bOn = Night > 0.001f && bLastNear;
		if (L->IsVisible() != bOn) { L->SetVisibility(bOn); }
		L->SetIntensity(LampLumens * List[i].Arms * Night);
	}
	if (GlobeInstance) { GlobeInstance->SetScalarParameterValue(TEXT("LampScale"), Night); }
	if (NightBalance) { NightBalance->BlendWeight = Night; }
}
