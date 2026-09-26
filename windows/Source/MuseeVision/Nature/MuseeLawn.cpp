#include "Nature/MuseeLawn.h"

#include "Chenghuai/ChenghuaiPlan.h"

#include "CollisionQueryParams.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Geometry/MuseeBake.h"
#include "Ground/MuseeGround.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreMisc.h"
#include "MuseeVision.h"
#include "Nature/MuseeNatureActor.h"
#include "Nature/NatureMesh.h"
#include "WorldCollision.h"

namespace MuseeLawn
{
	namespace MN = MuseeNature;

	void DecodeMeshIndex(int32 Index, int32& OutLevel, int32& OutSparse, int32& OutVariant)
	{
		if (Index >= Levels * Variants)
		{
			const int32 K = Index - Levels * Variants;
			OutLevel = 0;
			OutSparse = 1 + K / Variants;
			OutVariant = K % Variants;
		}
		else
		{
			OutLevel = Index / Variants;
			OutSparse = 0;
			OutVariant = Index % Variants;
		}
	}

	FString MeshName(int32 Index)
	{
		int32 L, S, V;
		DecodeMeshIndex(Index, L, S, V);
		return S > 0 ? FString::Printf(TEXT("SM_Lawn_Sparse%d_V%d"), S, V) : FString::Printf(TEXT("SM_Lawn_L%d_V%d"), L, V);
	}

	FString MeshPath(int32 Index)
	{
		const FString Name = MeshName(Index);
		return FString::Printf(TEXT("%s/%s.%s"), Folder(), *Name, *Name);
	}

	/** The sward's colours (sRGB): the blades' upper parts; the sheaths down in the turf; the torn tips the mower leaves. */
	struct FPalette
	{
		FLinearColor Root, Mid, Tip, Cut, DryMid, DryTip;
	};

	const FPalette& Palette()
	{
		// A sunlit mown sward reads about (0.05–0.08, 0.10–0.14, 0.02–0.04) linear: the tips a lively green, the sheaths
		// deep and dark below them.
		static const FPalette P{MN::Srgb(0x19260F), MN::Srgb(0x345620), MN::Srgb(0x436A26), MN::Srgb(0x7C7C5A), MN::Srgb(0x68683F),
								MN::Srgb(0x877F55)};
		return P;
	}

	/** How far the mower's pass lays the blades over (added to each blade's direction, along the patch's +x). */
	constexpr double LeanBias = 0.16;
	/** Blades per tiller (one to four), on average. */
	constexpr double BladesPerTiller = 2.5;

	FLinearColor Shift(const FLinearColor& C, double Hue, double Sat, double Val)
	{
		FLinearColor HSV = C.LinearRGBToHSV();
		HSV.R = static_cast<float>(FMath::Fmod(double(HSV.R) + Hue + 360.0, 360.0));
		HSV.G = static_cast<float>(FMath::Clamp(double(HSV.G) * Sat, 0.0, 1.0));
		HSV.B = static_cast<float>(FMath::Max(0.0, double(HSV.B) * Val));
		FLinearColor Out = HSV.HSVToLinearRGB();
		Out.A = 1.f;
		return Out;
	}

	void Triangle(FMuseeLawnPatch& Out, int32 A, int32 B, int32 C, const FVector3f& Facing)
	{
		const FVector3f& PA = Out.Positions[A];
		const FVector3f& PB = Out.Positions[B];
		const FVector3f& PC = Out.Positions[C];
		// The project's front-face rule (FNatureMesh::Triangle): cross(b − a, c − a) points away from the face's viewer.
		if (FVector3f::DotProduct(FVector3f::CrossProduct(PB - PA, PC - PA), Facing) < 0) { Out.Indices.Append({A, B, C}); }
		else { Out.Indices.Append({A, C, B}); }
	}

	/**
	 * One blade (metres): from Root along Dir0, arching over by Bend (radians) away from the upright, twisting by Twist
	 * (degrees), its face turned by Spin about it; tapered, with a blunt cut tip (the mower's) or a young pointed one.
	 */
	void Blade(FMuseeLawnPatch& Out, const FVector& Root, const FVector& Dir0, double Length, double Width, double Bend, double Twist, double Spin,
			   bool bCut, const FLinearColor& Low, const FLinearColor& Mid, const FLinearColor& High, const FLinearColor& Torn, FRandomStream& Rng)
	{
		const int32 NY = Length > 0.042 ? 3 : 2;
		const FVector H = FVector(Dir0.X, Dir0.Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const FVector K = FVector::CrossProduct(FVector::UpVector, H).GetSafeNormal();   // the arching's axis
		FVector D = Dir0;
		FVector Side = MN::RotateAbout(K, D, Spin);
		const double Step = Length / NY;
		TArray<FVector, TInlineAllocator<4>> Centre, Sides, Faces;
		FVector P = Root;
		for (int32 j = 0; j <= NY; ++j)
		{
			Centre.Add(P);
			Sides.Add(Side);
			Faces.Add(FVector::CrossProduct(Side, D).GetSafeNormal());
			const FVector Before = D;
			const double Arch = Bend / NY * (0.6 + 0.8 * double(j) / NY);   // more towards the tip
			D = MN::RotateAbout(D, K, Arch);
			Side = MN::RotateAbout(Side, K, Arch);
			Side = MN::RotateAbout(Side, D, FMath::DegreesToRadians(Twist / NY));
			Side = (Side - D * FVector::DotProduct(Side, D)).GetSafeNormal(UE_SMALL_NUMBER, Sides.Last());
			P += (Before + D).GetSafeNormal() * Step;
		}
		// Widths along the blade (fractions of Width): a narrower sheath, the widest a third up; cut square, or pointed.
		static const double Cut3[4] = {0.72, 1.0, 0.96, 0.86};
		static const double Cut2[3] = {0.72, 1.0, 0.86};
		static const double Point3[4] = {0.72, 1.0, 0.72, 0.0};
		static const double Point2[3] = {0.72, 0.88, 0.0};
		const double* Widths = bCut ? (NY == 3 ? Cut3 : Cut2) : (NY == 3 ? Point3 : Point2);
		// The mower tears as it cuts: the cut edge a little askew.
		const double Ragged = bCut ? Rng.FRandRange(-0.25, 0.25) * Width : 0.0;
		// The sward shades itself: the blades are kept out of the ray-traced scene, so the shadow line the neighbouring
		// blades cast on this one (higher or lower on each) is in its colour, as is the dimmer sky down in the turf.
		const double ShadeLine = Rng.FRandRange(0.15, 0.85);
		TArray<int32, TInlineAllocator<8>> Left, Right;
		for (int32 j = 0; j <= NY; ++j)
		{
			const double S = double(j) / NY;
			FLinearColor C = S < 0.4 ? MN::Mix(Low, Mid, S / 0.4) : MN::Mix(Mid, High, (S - 0.4) / 0.6);
			if (bCut && j == NY) { C = MN::Mix(C, Torn, 0.08); }
			C *= static_cast<float>(FMath::Lerp(0.5, 1.0, FMath::SmoothStep(ShadeLine - 0.2, ShadeLine + 0.2, S)));
			C.A = static_cast<float>(0.15 + 0.85 * FMath::Pow(S, 0.9));   // occlusion down in the sward
			const FVector Face = Faces[j];
			const FVector Across = Sides[j];
			const double Half = 0.5 * Width * Widths[j];
			const FVector3f Tangent(Across);
			auto Add = [&](const FVector& At, const FVector& Normal, float U)
			{
				const int32 Index = Out.Positions.Num();
				Out.Positions.Add(FVector3f(At * 100.0));
				Out.Normals.Add(FVector3f(Normal.GetSafeNormal()));
				Out.Tangents.Add(Tangent);
				Out.UVs.Add(FVector2f(U, float(S)));
				Out.Colours.Add(C);
				return Index;
			};
			if (Half <= 0.0)
			{
				const int32 Tip = Add(Centre[j], Face, 0.5f);
				Left.Add(Tip);
				Right.Add(Tip);
				continue;
			}
			// The two halves' normals lean out from the midrib: a rounded blade in the light.
			const FVector Lift = (j == NY && bCut) ? Dir0 * Ragged : FVector::ZeroVector;
			Left.Add(Add(Centre[j] - Across * Half - Lift, Face - Across * 0.3, 0.f));
			Right.Add(Add(Centre[j] + Across * Half + Lift, Face + Across * 0.3, 1.f));
		}
		for (int32 j = 0; j < NY; ++j)
		{
			const FVector3f Facing(Faces[j]);
			if (Left[j + 1] == Right[j + 1])
			{
				Triangle(Out, Left[j], Right[j], Left[j + 1], Facing);
			}
			else
			{
				Triangle(Out, Left[j], Right[j], Right[j + 1], Facing);
				Triangle(Out, Left[j], Right[j + 1], Left[j + 1], Facing);
			}
		}
		++Out.Blades;
	}

	void BuildPatch(int32 Level, int32 Sparse, int32 Variant, FMuseeLawnPatch& Out)
	{
		Out = FMuseeLawnPatch();
		const double Side = TileSize(Level);
		const double Density = BladesPerSquareMetre * (Sparse == 1 ? 0.5 : Sparse == 2 ? 0.25 : 1.0);
		// Tillers on a jittered grid: even, as a sown and mown sward is, without a pattern.
		const int32 N = FMath::Max(1, FMath::RoundToInt(Side * FMath::Sqrt(Density / BladesPerTiller)));
		const double Cell = Side / N;
		FRandomStream Rng(9173 + Level * 1009 + Sparse * 331 + Variant * 97);
		const int32 Expect = FMath::RoundToInt(N * N * BladesPerTiller);
		Out.Positions.Reserve(Expect * 8);
		Out.Normals.Reserve(Expect * 8);
		Out.Tangents.Reserve(Expect * 8);
		Out.UVs.Reserve(Expect * 8);
		Out.Colours.Reserve(Expect * 8);
		Out.Indices.Reserve(Expect * 18);
		const FPalette& Pal = Palette();
		for (int32 i = 0; i < N; ++i)
		{
			for (int32 j = 0; j < N; ++j)
			{
				const FVector2D At(-0.5 * Side + (i + Rng.FRand()) * Cell, -0.5 * Side + (j + Rng.FRand()) * Cell);
				const int32 Count = Rng.RandRange(1, 4);
				const double Az0 = Rng.FRandRange(0.0, MN::Tau);
				// A tiller's blades share its colour, more or less.
				const double TillerHue = Rng.FRandRange(-4.0, 4.0), TillerVal = Rng.FRandRange(0.88, 1.12);
				for (int32 b = 0; b < Count; ++b)
				{
					const double Az = Az0 + MN::Tau * b / Count + Rng.FRandRange(-0.5, 0.5);
					const double E = FMath::DegreesToRadians(Rng.FRandRange(58.0, 86.0));
					FVector Dir(FMath::Cos(Az) * FMath::Cos(E), FMath::Sin(Az) * FMath::Cos(E), FMath::Sin(E));
					Dir = (Dir + FVector(LeanBias * Rng.FRandRange(0.6, 1.3), 0.0, 0.0)).GetSafeNormal();
					const bool bYoung = Rng.FRand() < 0.22;
					const bool bDry = !bYoung && Rng.FRand() < 0.035;
					double Length = 0.030 + 0.028 * FMath::Pow(Rng.FRand(), 0.85);
					if (bYoung) { Length *= Rng.FRandRange(0.75, 1.05); }
					const double Width = Rng.FRandRange(0.0022, 0.0036) * (bYoung ? 0.85 : 1.0);
					const double Bend = FMath::DegreesToRadians(Rng.FRandRange(6.0, 30.0));
					const double Twist = Rng.FRandRange(-30.0, 30.0);
					const double Spin = FMath::DegreesToRadians(Rng.FRandRange(-35.0, 35.0));
					const double Hue = TillerHue + Rng.FRandRange(-2.5, 2.5);
					const double Sat = Rng.FRandRange(0.88, 1.12);
					const double Val = TillerVal * Rng.FRandRange(0.92, 1.08) * (bYoung ? 1.08 : 1.0);
					const FLinearColor Low = Shift(Pal.Root, Hue, Sat, Val);
					const FLinearColor Mid = Shift(bDry ? Pal.DryMid : Pal.Mid, Hue, Sat, Val);
					const FLinearColor High = Shift(bDry ? Pal.DryTip : Pal.Tip, Hue, Sat, Val);
					const FVector Root(At.X + Rng.FRandRange(-0.002, 0.002), At.Y + Rng.FRandRange(-0.002, 0.002), -0.004);
					Blade(Out, Root, Dir, Length, Width, Bend, Twist, Spin, !bYoung, Low, Mid, High, Pal.Cut, Rng);
				}
			}
		}
	}

	TArray<int32>& TriangleCounts()
	{
		static TArray<int32> Counts;
		if (Counts.Num() != NumMeshes)
		{
			Counts.SetNumZeroed(NumMeshes);
			for (int32 m = 0; m < NumMeshes; ++m)
			{
				int32 L, S, V;
				DecodeMeshIndex(m, L, S, V);
				FMuseeLawnPatch Patch;
				BuildPatch(L, S, V, Patch);
				Counts[m] = Patch.NumTriangles();
			}
		}
		return Counts;
	}

	/** Every instance the survey gives, with its patch mesh. */
	void Enumerate(const AMuseeLawn& Lawn, TFunctionRef<double(const FVector2D&)> WeightAt, TFunctionRef<void(int32, const FTransform&)> Visit)
	{
		for (int32 L = 0; L < Lawn.Tiles.Num() && L < Levels; ++L)
		{
			const double S = TileSize(L);
			for (const int32 Packed : Lawn.Tiles[L].Cells)
			{
				const int32 IX = Packed & 0xFFFF, IY = int32((uint32(Packed) >> 16) & 0xFFFFu);
				const FVector2D C = Lawn.GridOrigin + FVector2D((IX + 0.5) * S, (IY + 0.5) * S);
				const double W = WeightAt(C);
				if (W <= 0.0) { continue; }
				const double R = MN::Hash(IX, IY, L, 71);
				int32 Sparse = 0;
				if (L == 0)
				{
					// Dithered between the full, half and quarter densities (and none) by the weight.
					if (W >= 0.5) { Sparse = R < (W - 0.5) / 0.5 ? 0 : 1; }
					else if (W >= 0.25) { Sparse = R < (W - 0.25) / 0.25 ? 1 : 2; }
					else if (R < W / 0.25) { Sparse = 2; }
					else { continue; }
				}
				else if (R > W) { continue; }
				const int32 V = FMath::Min(Variants - 1, int32(MN::Hash(IX, IY, L, 13) * Variants));
				const int32 Band = FMath::FloorToInt32(C.X / Lawn.StripeWidth);
				const double Yaw = (Band & 1) ? -90.0 : 90.0;   // the patch's lean (+x) south, or north
				const double Drift = 1.0 + Lawn.HeightVariation * 2.0 * (MN::Noise(FVector(C.X / 4.3, C.Y / 4.3, 0.5), 17) - 0.5);
				const double Height = FMath::Lerp(double(Lawn.FringeHeight), 1.0, W) * Drift;
				Visit(MeshIndex(L, Sparse, V), FTransform(FRotator(0.0, Yaw, 0.0), FVector(C.X * 100.0, C.Y * 100.0, Lawn.GroundZ),
														FVector(1.0, 1.0, Height)));
			}
		}
	}
}

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GLawnCommand(
		TEXT("musee.Lawn"), TEXT("musee.Lawn 0|1: hide or show the grounds' grass blades; musee.Lawn info: how many there are."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) { return; }
			for (TActorIterator<AMuseeLawn> It(World); It; ++It)
			{
				if (Args.Num() == 0 || Args[0] == TEXT("info")) { UE_LOG(LogMusee, Log, TEXT("Lawn: %s"), *It->Describe()); }
				else { It->SetShown(FCString::Atoi(*Args[0]) != 0); }
			}
		}));
}

AMuseeLawn::AMuseeLawn()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	Tags.Add(MuseeBake::NoBakeTag());
	Tags.Add(FName(TEXT("musee.exterior")));
	Tags.Add(FName(TEXT("musee.lawn")));
	for (int32 m = 0; m < MuseeLawn::NumMeshes; ++m) { Meshes.Add(TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(MuseeLawn::MeshPath(m)))); }
	// The Chinese Wing's court (its walls' outer faces), the Hall of Light's meadow and orchard to their hedges.
	Exclusions.Add(FVector4(-10.6, 12.9, 10.6, 34.1));
	Exclusions.Add(FVector4(10.9, -14.25, 41.1, 14.25));
	if (Chenghuai::bNorthDoorOpen)
	{
		Exclusions.Add(FVector4(-20.8, -62.0, 17.0, -16.9));   // Chenghuai: its courts are paved, its garden has its own ground
	}
}

void AMuseeLawn::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UWorld* World = GetWorld();
	if (bPreviewInEditor && GIsEditor && !IsRunningCommandlet() && World && !World->IsGameWorld()) { Plant(); }
}

void AMuseeLawn::BeginPlay()
{
	Super::BeginPlay();
	Plant();
}

void AMuseeLawn::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearPlanted();
	Super::EndPlay(Reason);
}

double AMuseeLawn::Weight(const FVector2D& At) const
{
	const double DX = FMath::Max3(RegionMin.X - At.X, 0.0, At.X - RegionMax.X);
	const double DY = FMath::Max3(RegionMin.Y - At.Y, 0.0, At.Y - RegionMax.Y);
	double D = FMath::Sqrt(DX * DX + DY * DY);
	if (D <= 0.0) { return 1.0; }
	// A wandering edge, not a ruled one.
	D += (MuseeNature::Noise(FVector(At.X / 11.0, At.Y / 11.0, 3.3), 23) - 0.5) * 0.4 * Fringe;
	return 1.0 - FMath::SmoothStep(0.0, double(Fringe), D);
}

void AMuseeLawn::ClearPlanted()
{
	for (UInstancedStaticMeshComponent* C : Planted)
	{
		if (IsValid(C)) { C->DestroyComponent(); }
	}
	Planted.Reset();
	PlantedCounts.Reset();
}

void AMuseeLawn::Plant()
{
	ClearPlanted();
	int32 Cells = 0;
	for (const FMuseeLawnCells& T : Tiles) { Cells += T.Cells.Num(); }
	if (Cells == 0) { return; }
	TArray<TArray<FTransform>> Per;
	Per.SetNum(MuseeLawn::NumMeshes);
	MuseeLawn::Enumerate(*this, [this](const FVector2D& C) { return Weight(C); }, [&Per](int32 M, const FTransform& T) { Per[M].Add(T); });
	const double Start = FPlatformTime::Seconds();
	int32 Total = 0;
	PlantedCounts.SetNumZeroed(MuseeLawn::NumMeshes);
	for (int32 m = 0; m < MuseeLawn::NumMeshes; ++m)
	{
		if (Per[m].Num() == 0 || !Meshes.IsValidIndex(m)) { continue; }
		UStaticMesh* Mesh = Meshes[m].LoadSynchronous();
		if (!Mesh)
		{
			UE_LOG(LogMusee, Warning, TEXT("Lawn: %s is missing (run Scripts/apply_all.py lawn)."), *Meshes[m].ToString());
			continue;
		}
		const FName Name = MakeUniqueObjectName(this, UInstancedStaticMeshComponent::StaticClass(), FName(*(TEXT("Lawn_") + MuseeLawn::MeshName(m))));
		UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this, Name, RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
		ISM->SetupAttachment(RootComponent);
		ISM->SetMobility(EComponentMobility::Static);
		ISM->SetStaticMesh(Mesh);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ISM->SetCanEverAffectNavigation(false);
		ISM->SetGenerateOverlapEvents(false);
		// Lit and shadowed by the sun's ray-traced shadows, but not in the ray-traced or Lumen scene, and casting none:
		// a hundred thousand instances would weigh on both, for contact shadows the sward's colour already has.
		ISM->SetCastShadow(false);
		ISM->bVisibleInRayTracing = false;
		ISM->bAffectDistanceFieldLighting = false;
		ISM->bAffectDynamicIndirectLighting = false;
		ISM->bVisibleInReflectionCaptures = false;
		ISM->SetReceivesDecals(false);
		ISM->SetLightingChannels(true, true, false);   // the sun, and the exterior's lamps and floodlights
		ISM->ComponentTags.Add(MuseeBake::NoBakeTag());
		ISM->RegisterComponent();
		ISM->AddInstances(Per[m], false, false, false);
		Planted.Add(ISM);
		PlantedCounts[m] = Per[m].Num();
		Total += Per[m].Num();
	}
	UE_LOG(LogMusee, Log, TEXT("Lawn: %d patches planted in %d components (%.0f ms)."), Total, Planted.Num(), (FPlatformTime::Seconds() - Start) * 1000.0);
}

void AMuseeLawn::SetShown(bool bShow)
{
	for (UInstancedStaticMeshComponent* C : Planted)
	{
		if (IsValid(C)) { C->SetVisibility(bShow); }
	}
}

FString AMuseeLawn::Describe() const
{
	TArray<int32> Count;
	Count.SetNumZeroed(MuseeLawn::NumMeshes);
	MuseeLawn::Enumerate(*this, [this](const FVector2D& C) { return Weight(C); }, [&Count](int32 M, const FTransform&) { ++Count[M]; });
	const TArray<int32>& Tris = MuseeLawn::TriangleCounts();
	int64 Instances = 0, Triangles = 0;
	FString PerSize;
	for (int32 L = 0; L < MuseeLawn::Levels; ++L)
	{
		int64 N = 0;
		for (int32 m = 0; m < MuseeLawn::NumMeshes; ++m)
		{
			int32 ML, MS, MV;
			MuseeLawn::DecodeMeshIndex(m, ML, MS, MV);
			if (ML == L) { N += Count[m]; }
		}
		PerSize += FString::Printf(TEXT("%s%.4g m: %lld"), L ? TEXT(", ") : TEXT(""), MuseeLawn::TileSize(L), N);
	}
	for (int32 m = 0; m < MuseeLawn::NumMeshes; ++m)
	{
		Instances += Count[m];
		Triangles += int64(Count[m]) * Tris[m];
	}
	int32 Cells = 0;
	for (const FMuseeLawnCells& T : Tiles) { Cells += T.Cells.Num(); }
	return FString::Printf(TEXT("%lld instances (%s) of %d cells; %.1f M triangles in all (before Nanite's LOD); patch meshes 1 m: %d triangles."),
						   Instances, *PerSize, Cells, Triangles / 1.0e6, Tris[0]);
}

int32 AMuseeLawn::Survey()
{
	UWorld* World = GetWorld();
	if (!World) { return 0; }
	const double Start = FPlatformTime::Seconds();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MuseeLawnSurvey), false, this);
	GroundZ = -3.f;
	for (TActorIterator<AMuseeGround> It(World); It; ++It)
	{
		Params.AddIgnoredActor(*It);
		GroundZ = static_cast<float>(It->Height + It->GetActorLocation().Z);
	}
	// The plants stand in the lawn: the grass grows up to the trunks and culms (inside them it is hidden).
	for (TActorIterator<AMuseeNatureActor> It(World); It; ++It) { Params.AddIgnoredActor(*It); }
	Tiles.Reset();
	Tiles.SetNum(MuseeLawn::Levels);
	GridOrigin = FVector2D(FMath::FloorToDouble(RegionMin.X - Fringe - 1.0), FMath::FloorToDouble(RegionMin.Y - Fringe - 1.0));
	const int32 NX = FMath::CeilToInt32(RegionMax.X + Fringe + 1.0 - GridOrigin.X);
	const int32 NY = FMath::CeilToInt32(RegionMax.Y + Fringe + 1.0 - GridOrigin.Y);
	check(NX << (MuseeLawn::Levels - 1) < 65536 && NY << (MuseeLawn::Levels - 1) < 65536);

	// The ground's own holes (the sun clock's shaft, the Élan's, the pond stair's; under floors, but to be sure).
	struct FHole
	{
		FVector2D C;
		double R;
	};
	const FHole Circles[] = {{FVector2D::ZeroVector, 7.62 + 0.05}, {FVector2D(54.0, 0.0), 2.6 + 0.05}};
	const FBox2D PondHole(FVector2D(-90.45 - 0.05, -1.25 - 0.05), FVector2D(-82.85 + 0.05, 1.25 + 0.05));

	const double ZBottom = GroundZ + 0.6, ZTop = GroundZ + 12.0;   // cm: from just over the ground to over the blades
	const FVector ZCentre(0, 0, 0.5 * (ZBottom + ZTop));
	const double ZHalf = 0.5 * (ZTop - ZBottom);
	int64 Queries = 0, HedgeRejects = 0;
	auto Clear = [&](const FVector2D& C, double S) -> bool
	{
		const double H = 0.5 * S + Margin;
		const FBox2D Box(C - FVector2D(H, H), C + FVector2D(H, H));
		for (const FVector4& E : Exclusions)
		{
			if (Box.Max.X > E.X && Box.Min.X < E.Z && Box.Max.Y > E.Y && Box.Min.Y < E.W) { return false; }
		}
		if (Box.Intersect(PondHole)) { return false; }
		for (const FHole& Hole : Circles)
		{
			const FVector2D Near(FMath::Clamp(Hole.C.X, Box.Min.X, Box.Max.X), FMath::Clamp(Hole.C.Y, Box.Min.Y, Box.Max.Y));
			if (FVector2D::DistSquared(Near, Hole.C) < Hole.R * Hole.R) { return false; }
		}
		const FVector At(C.X * 100.0, C.Y * 100.0, ZCentre.Z);
		// Anything the visitor would bump into, with the hedges' reach round their cores.
		const double HH = 0.5 * S + FMath::Max(Margin, HedgeMargin);
		TArray<FOverlapResult> Hits;
		++Queries;
		World->OverlapMultiByChannel(Hits, At, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeBox(FVector(HH * 100.0, HH * 100.0, ZHalf)), Params);
		bool bAny = false;
		for (const FOverlapResult& Hit : Hits)
		{
			if (!Hit.bBlockingHit) { continue; }
			bAny = true;
			const UPrimitiveComponent* Component = Hit.GetComponent();
			if (Component && Component->GetName().Contains(TEXT("Hedge"))) { ++HedgeRejects; return false; }
		}
		if (!bAny) { return true; }
		++Queries;
		return !World->OverlapBlockingTestByChannel(At, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeBox(FVector(H * 100.0, H * 100.0, ZHalf)), Params);
	};
	auto Visit = [&](auto& Self, int32 L, int32 IX, int32 IY) -> void
	{
		const double S = MuseeLawn::TileSize(L);
		const FVector2D C = GridOrigin + FVector2D((IX + 0.5) * S, (IY + 0.5) * S);
		if (Clear(C, S))
		{
			Tiles[L].Cells.Add(IX | (IY << 16));
			return;
		}
		if (L + 1 >= MuseeLawn::Levels) { return; }
		for (int32 d = 0; d < 4; ++d) { Self(Self, L + 1, IX * 2 + (d & 1), IY * 2 + (d >> 1)); }
	};
	for (int32 IX = 0; IX < NX; ++IX)
	{
		for (int32 IY = 0; IY < NY; ++IY)
		{
			const FVector2D C = GridOrigin + FVector2D(IX + 0.5, IY + 0.5);
			if (Weight(C) <= 0.0) { continue; }
			Visit(Visit, 0, IX, IY);
		}
	}
	int32 Total = 0;
	FString Per;
	for (int32 L = 0; L < MuseeLawn::Levels; ++L)
	{
		Total += Tiles[L].Cells.Num();
		Per += FString::Printf(TEXT("%s%d"), L ? TEXT(" / ") : TEXT(""), Tiles[L].Cells.Num());
	}
	UE_LOG(LogMusee, Log, TEXT("Lawn: surveyed %d patches (1 m / 50 / 25 / 12.5 / 6.25 cm: %s), %lld queries (%lld against hedges), ground at %.1f cm, in %.1f s."),
		   Total, *Per, Queries, HedgeRejects, GroundZ, FPlatformTime::Seconds() - Start);
	Modify();
	return Total;
}
