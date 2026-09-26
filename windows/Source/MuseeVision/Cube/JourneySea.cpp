#include "Cube/JourneySea.h"

#include "MuseeVision.h"
#include "Cube/CubeMesh.h"
#include "Cube/ElanJourney.h"
#include "Cube/JourneySeaGen.h"
#include "Sky/MuseeSky.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Math/RandomStream.h"
#include "Misc/PackageName.h"
#include "ProceduralMeshComponent.h"

namespace
{
	const TCHAR* const SeaThings = TEXT("/Game/Museum/Journeys/Sea/Things");
	const TCHAR* const SeaMats = TEXT("/Game/Museum/Journeys/Sea/Materials");
	constexpr double SeaPi = UE_DOUBLE_PI;
}

bool UJourneySea::IsAvailable() const
{
	return FPackageName::DoesPackageExist(TEXT("/Game/Museum/Journeys/Sea/Ready"));
}

UStaticMesh* UJourneySea::Thing(const TCHAR* Name) const
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), SeaThings, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

void UJourneySea::Setup(AElanJourney* InDirector)
{
	Super::Setup(InDirector);
	BuildSurface();
	Caustics = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s/M_RA_Caustics.M_RA_Caustics"), SeaMats));
	Underwater = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s/M_RA_Underwater.M_RA_Underwater"), SeaMats));
	if (Underwater && Director->Post) { Director->Post->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, Underwater)); }
	// The shoal that closes in between places (a veil of fusiliers round the car).
	School(TEXT("SM_RA_Fish_Fusilier"), 900, 6, FVector::ZeroVector, 3.2, 1.2, 0.0, 4.5, 1.6, 77, true);
}

void UJourneySea::BuildSurface()
{
	// A disc of water 4 km across: rings closer near the eyes (the waves' displacement needs vertices there).
	CubeMesh::FMesh M;
	constexpr int32 Around = 256;
	TArray<double> Rings = {0.0};
	for (double R = 0.25; R < 2000.0; R *= 1.06) { Rings.Add(R); }
	const FVector Up(0, 0, 1);
	TArray<int32> Prev;
	for (int32 k = 0; k < Rings.Num(); ++k)
	{
		TArray<int32> Ring;
		if (k == 0) { Ring.Add(M.V(FVector::ZeroVector, Up, FVector2D::ZeroVector)); }
		else
		{
			for (int32 a = 0; a < Around; ++a)
			{
				const double A = 2 * SeaPi * a / Around;
				Ring.Add(M.V(FVector(Rings[k] * FMath::Cos(A), Rings[k] * FMath::Sin(A), 0.0), Up, FVector2D(Rings[k] * FMath::Cos(A), Rings[k] * FMath::Sin(A))));
			}
		}
		if (k == 1) { for (int32 a = 0; a < Around; ++a) { M.Tri(Prev[0], Ring[a], Ring[(a + 1) % Around]); } }
		else if (k > 1) { for (int32 a = 0; a < Around; ++a) { M.Quad(Prev[a], Prev[(a + 1) % Around], Ring[(a + 1) % Around], Ring[a]); } }
		Prev = Ring;
	}
	Surface = Make<UProceduralMeshComponent>(TEXT("SeaSurface"), true);
	Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Surface->SetCastShadow(false);
	M.Write(Surface, 0, false);
	Surface->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s/M_RA_Surface.M_RA_Surface"), SeaMats)));
	Surface->SetTranslucentSortPriority(-2);
}

UInstancedStaticMeshComponent* UJourneySea::Scatter(const TCHAR* MeshName, int32 Count, double RMin, double RMax, double ScaleMin, double ScaleMax,
													 TFunctionRef<bool(const FVector&)> Accept, uint32 Seed, bool bOnWall)
{
	UStaticMesh* Mesh = Thing(MeshName);
	if (!Mesh) { return nullptr; }
	UInstancedStaticMeshComponent* I = Instances(Mesh);
	FRandomStream R(Seed);
	const int32 Place = Current + 1;
	int32 Placed = 0;
	for (int32 Tries = 0; Tries < Count * 12 && Placed < Count; ++Tries)
	{
		FTransform T;
		const double S = R.FRandRange(ScaleMin, ScaleMax);
		if (bOnWall)
		{
			// On the wall's face: (y, z) near the car, the thing growing out of it (its +z along the wall's normal, +x).
			const double Y = R.FRandRange(-RMax, RMax), Z = R.FRandRange(-RMax * 0.8, RMax * 0.5);
			if (FMath::Abs(Y) < RMin && FMath::Abs(Z) < RMin) { continue; }
			const FVector P(SeaGen::WallX(Y, Z) - 0.05, Y, Z);
			if (!Accept(P)) { continue; }
			const FRotator Rot(-90.0 + R.FRandRange(-15.0, 15.0), R.FRandRange(-10.0, 10.0), R.FRandRange(0.0, 360.0));
			T = FTransform(Rot, P * 100.0, FVector(S));
		}
		else
		{
			const double A = R.FRandRange(0.0, 2 * SeaPi);
			// Denser near the car (where the eye can tell), thinning out to RMax.
			const double Rr = RMin + (RMax - RMin) * FMath::Pow(R.FRand(), 1.5);
			const double X = Rr * FMath::Cos(A), Y = Rr * FMath::Sin(A);
			const FVector P(X, Y, SeaGen::BedHeight(Place, X, Y) - 0.03);
			if (!Accept(P)) { continue; }
			// Tilted a little with the bottom.
			const double Hx = SeaGen::BedHeight(Place, X + 0.5, Y) - SeaGen::BedHeight(Place, X - 0.5, Y);
			const double Hy = SeaGen::BedHeight(Place, X, Y + 0.5) - SeaGen::BedHeight(Place, X, Y - 0.5);
			const FRotator Rot(FMath::RadiansToDegrees(FMath::Atan(Hx)) * 0.6, R.FRandRange(0.0, 360.0), -FMath::RadiansToDegrees(FMath::Atan(Hy)) * 0.6);
			T = FTransform(Rot, P * 100.0, FVector(S));
		}
		I->AddInstance(T, false);
		++Placed;
	}
	return I;
}

UJourneySea::FGroup& UJourneySea::School(const TCHAR* Species, int32 Count, int32 Kind, const FVector& Centre, double Radius, double RadiusSpread,
										 double Height, double HeightSpread, double Speed, uint32 Seed, bool bForJourney)
{
	FGroup G;
	G.Kind = Kind;
	G.Centre = Centre;
	G.Radius = Radius;
	G.RadiusSpread = RadiusSpread;
	G.Height = Height;
	G.HeightSpread = HeightSpread;
	G.Speed = Speed;
	if (UStaticMesh* Mesh = Thing(Species))
	{
		G.Mesh = Make<UInstancedStaticMeshComponent>(TEXT("Fish"), bForJourney);
		G.Mesh->SetStaticMesh(Mesh);
		G.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		G.Mesh->SetCastShadow(Kind == 3 || Kind == 5);
		FRandomStream R(Seed);
		TArray<FTransform> Start;
		for (int32 i = 0; i < Count; ++i)
		{
			G.Fish.Add(FVector4(R.FRandRange(-1.0, 1.0), R.FRandRange(-1.0, 1.0), R.FRandRange(0.0, 2 * SeaPi), R.FRandRange(0.8, 1.2)));
			Start.Add(FTransform(Centre * 100.0));
		}
		G.Mesh->AddInstances(Start, false);
	}
	FGroup& Out = bForJourney ? JourneyGroups.Add_GetRef(MoveTemp(G)) : Groups.Add_GetRef(MoveTemp(G));
	return Out;
}

void UJourneySea::ClosePlace()
{
	Groups.Reset();
	Super::ClosePlace();
}

void UJourneySea::OpenPlace(int32 Index)
{
	Current = Index;
	Clock = 0;
	const TArray<FJourneyPlace>& All = AElanJourney::Places(EElanJourney::Sea);
	const FJourneyPlace& Pl = All[FMath::Clamp(Index, 0, All.Num() - 1)];
	const int32 Place = Index + 1;
	if (Surface) { Surface->SetRelativeLocation(FVector(0, 0, Pl.WaterSurface * 100.0)); }
	if (UMaterialParameterCollection* MPC = Director->Parameters())
	{
		UKismetMaterialLibrary::SetScalarParameterValue(Director, MPC, TEXT("WaterLevel"), float(Director->GetActorLocation().Z + Pl.WaterSurface * 100.0));
		UKismetMaterialLibrary::SetScalarParameterValue(Director, MPC, TEXT("WaterOn"), 1.f);
		UKismetMaterialLibrary::SetScalarParameterValue(Director, MPC, TEXT("Plankton"), 0.f);
	}
	// The bottom.
	MeshAt(Thing(*FString::Printf(TEXT("SM_RA_Bed_%d"), Place)), FTransform::Identity);

	auto Rock = [Place](const FVector& P) { const FVector4 W = SeaGen::BedMaterial(Place, P.X, P.Y); return W.Z > 0.35; };
	auto Any = [](const FVector&) { return true; };
	auto Under = [&Pl, Rock](const FVector& P) { return P.Z < Pl.WaterSurface - 0.8 && Rock(P); };

	switch (Index)
	{
	case 0:   // Piaynemo, the surface: a reef flat under the car, karst islets round the lagoon.
	case 5:   // … and at night.
	{
		const double Keep = Index == 0 ? 1.0 : 0.6;
		Scatter(TEXT("SM_RA_Coral_Massive_0"), int32(40 * Keep), 3.0, 60.0, 0.6, 1.2, Under, 11 + Index);
		Scatter(TEXT("SM_RA_Coral_Massive_1"), int32(15 * Keep), 5.0, 70.0, 0.6, 1.1, Under, 12 + Index);
		Scatter(TEXT("SM_RA_Coral_Table_0"), int32(25 * Keep), 3.0, 50.0, 0.7, 1.3, Under, 13 + Index);
		Scatter(TEXT("SM_RA_Coral_Table_1"), int32(10 * Keep), 5.0, 60.0, 0.7, 1.2, Under, 14 + Index);
		Scatter(TEXT("SM_RA_Coral_Staghorn_0"), int32(20 * Keep), 3.0, 40.0, 0.7, 1.3, Under, 15 + Index);
		Scatter(TEXT("SM_RA_Coral_Staghorn_1"), int32(15 * Keep), 3.0, 40.0, 0.7, 1.3, Under, 16 + Index);
		Scatter(TEXT("SM_RA_SoftCoral_0"), int32(30 * Keep), 2.5, 30.0, 0.8, 1.4, Under, 17 + Index);
		Scatter(TEXT("SM_RA_SoftCoral_2"), int32(20 * Keep), 2.5, 30.0, 0.8, 1.4, Under, 18 + Index);
		Scatter(TEXT("SM_RA_BarrelSponge_0"), int32(6 * Keep), 5.0, 40.0, 0.7, 1.1, Under, 19 + Index);
		Scatter(TEXT("SM_RA_Rock_0"), 25, 4.0, 80.0, 0.6, 1.5, Any, 20 + Index);
		// The islets: one close to the north, the rest round the lagoon.
		struct FIsle { const TCHAR* Mesh; double X, Y, Yaw, S; };
		const FIsle Isles[] = {{TEXT("SM_RA_Karst_1"), 20, -125, 30, 1.0}, {TEXT("SM_RA_Karst_0"), -140, -60, 80, 1.1}, {TEXT("SM_RA_Karst_2"), 95, -70, 10, 1.3},
							   {TEXT("SM_RA_Karst_3"), -420, -520, 60, 1.0}, {TEXT("SM_RA_Karst_1"), 380, -330, 150, 0.9}, {TEXT("SM_RA_Karst_0"), 610, 120, 200, 1.4},
							   {TEXT("SM_RA_Karst_2"), -260, 240, 120, 1.2}, {TEXT("SM_RA_Karst_3"), 900, -900, 20, 1.3}, {TEXT("SM_RA_Karst_1"), -900, 300, 300, 1.2},
							   {TEXT("SM_RA_Karst_0"), 250, 480, 250, 0.8}, {TEXT("SM_RA_Karst_2"), -60, -300, 70, 1.5}};
		for (const FIsle& S : Isles)
		{
			MeshAt(Thing(S.Mesh), FTransform(FRotator(0, S.Yaw, 0), FVector(S.X, S.Y, Pl.WaterSurface), FVector(S.S)));
		}
		if (Index == 0)
		{
			School(TEXT("SM_RA_Fish_Fusilier"), 160, 0, FVector(3, -4, -3.5), 11.0, 3.0, 0.0, 1.2, 1.4, 101);
			School(TEXT("SM_RA_Fish_Chromis"), 120, 2, FVector(0, 0, -3.2), 7.0, 5.0, 0.0, 0.8, 0.2, 102);
			School(TEXT("SM_RA_Fish_Butterfly"), 12, 2, FVector(0, 0, -3.0), 9.0, 7.0, 0.0, 1.0, 0.3, 103);
			School(TEXT("SM_RA_Fish_Parrot"), 6, 3, FVector(4, 6, -3.8), 9.0, 3.0, 0.0, 0.6, 0.6, 104);
		}
		else
		{
			School(TEXT("SM_RA_Fish_WalkingShark"), 2, 5, FVector(0, 0, 0), 4.5, 1.5, 0.0, 0.0, 0.15, 105);
			// The plankton: motes in the dark water round the car, flashing.
			Plankton = Make<UInstancedStaticMeshComponent>(TEXT("Plankton"));
			Plankton->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
			Plankton->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s/M_RA_Plankton.M_RA_Plankton"), SeaMats)));
			Plankton->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Plankton->SetCastShadow(false);
			FRandomStream R(606);
			TArray<FTransform> Motes;
			for (int32 i = 0; i < 5000; ++i)
			{
				const double A = R.FRandRange(0.0, 2 * SeaPi), Rr = FMath::Sqrt(R.FRandRange(2.6 * 2.6, 12.0 * 12.0));
				Motes.Add(FTransform(FRotator::ZeroRotator, FVector(Rr * FMath::Cos(A), Rr * FMath::Sin(A), -R.FRandRange(0.1, 2.5)) * 100.0, FVector(0.006)));
			}
			Plankton->AddInstances(Motes, false);
		}
		break;
	}
	case 1:   // The mangroves: prop roots all round, corals on them, young blacktips slipping between.
	{
		struct FTree { const TCHAR* Mesh; double X, Y, Yaw; };
		const FTree Trees[] = {{TEXT("SM_RA_Mangrove_0"), 6.5, -3.0, 20}, {TEXT("SM_RA_Mangrove_1"), -5.5, -5.0, 140}, {TEXT("SM_RA_Mangrove_0"), -7.0, 4.5, 260},
							   {TEXT("SM_RA_Mangrove_1"), 3.5, 8.0, 80}, {TEXT("SM_RA_Mangrove_0"), 12.0, 5.0, 200}, {TEXT("SM_RA_Mangrove_1"), -1.0, -11.0, 320},
							   {TEXT("SM_RA_Mangrove_0"), -12.5, -2.0, 30}, {TEXT("SM_RA_Mangrove_1"), 9.0, -12.0, 110}, {TEXT("SM_RA_Mangrove_0"), -9.0, 12.0, 170},
							   {TEXT("SM_RA_Mangrove_1"), 16.0, -6.0, 60}, {TEXT("SM_RA_Mangrove_0"), 1.0, 16.0, 290}, {TEXT("SM_RA_Mangrove_1"), -16.0, 8.0, 10}};
		for (const FTree& T : Trees)
		{
			MeshAt(Thing(T.Mesh), FTransform(FRotator(0, T.Yaw, 0), FVector(T.X, T.Y, Pl.WaterSurface)));
		}
		Scatter(TEXT("SM_RA_SoftCoral_1"), 25, 3.0, 18.0, 0.6, 1.2, Any, 31);
		Scatter(TEXT("SM_RA_SoftCoral_3"), 20, 3.0, 18.0, 0.6, 1.2, Any, 32);
		Scatter(TEXT("SM_RA_Coral_Massive_0"), 8, 4.0, 20.0, 0.4, 0.8, Any, 33);
		Scatter(TEXT("SM_RA_Rock_1"), 12, 4.0, 25.0, 0.4, 0.9, Any, 34);
		School(TEXT("SM_RA_Fish_BlacktipShark"), 3, 3, FVector(0, 0, -1.4), 7.5, 2.5, 0.0, 0.4, 0.7, 201);
		School(TEXT("SM_RA_Fish_Chromis"), 140, 2, FVector(0, 0, -1.0), 9.0, 7.0, 0.0, 0.8, 0.2, 202);
		School(TEXT("SM_RA_Fish_Fusilier"), 60, 0, FVector(0, 0, -0.6), 12.0, 2.0, 0.0, 0.4, 1.0, 203);
		break;
	}
	case 2:   // Cape Kri: the richest reef; a shoal of jacks rings the car and turns.
	{
		// Coral cover over most of the reef (rock and rubble alike), thinning only on the sand channels.
		auto Reef = [Place](const FVector& P) { const FVector4 W = SeaGen::BedMaterial(Place, P.X, P.Y); return W.Y + W.Z > 0.3; };
		Scatter(TEXT("SM_RA_Coral_Table_0"), 135, 3.0, 45.0, 0.7, 1.2, Reef, 41);
		Scatter(TEXT("SM_RA_Coral_Table_1"), 90, 3.0, 45.0, 0.7, 1.2, Reef, 42);
		Scatter(TEXT("SM_RA_Coral_Table_2"), 36, 4.0, 45.0, 0.7, 1.1, Reef, 43);
		Scatter(TEXT("SM_RA_Coral_Staghorn_0"), 90, 3.0, 40.0, 0.7, 1.3, Reef, 44);
		Scatter(TEXT("SM_RA_Coral_Staghorn_1"), 90, 3.0, 40.0, 0.7, 1.3, Reef, 45);
		Scatter(TEXT("SM_RA_Coral_Staghorn_2"), 75, 3.0, 40.0, 0.7, 1.3, Reef, 46);
		Scatter(TEXT("SM_RA_Coral_Massive_0"), 120, 3.0, 50.0, 0.6, 1.2, Reef, 47);
		Scatter(TEXT("SM_RA_Coral_Massive_1"), 75, 4.0, 50.0, 0.6, 1.2, Reef, 48);
		Scatter(TEXT("SM_RA_Coral_Massive_2"), 24, 6.0, 50.0, 0.7, 1.1, Reef, 49);
		Scatter(TEXT("SM_RA_SoftCoral_0"), 120, 2.5, 35.0, 0.8, 1.5, Reef, 50);
		Scatter(TEXT("SM_RA_SoftCoral_1"), 90, 2.5, 35.0, 0.8, 1.5, Reef, 51);
		Scatter(TEXT("SM_RA_SoftCoral_2"), 90, 2.5, 35.0, 0.8, 1.5, Reef, 52);
		Scatter(TEXT("SM_RA_SeaFan_0"), 30, 4.0, 40.0, 0.8, 1.2, Reef, 53);
		Scatter(TEXT("SM_RA_SeaFan_1"), 18, 5.0, 40.0, 0.8, 1.2, Reef, 54);
		Scatter(TEXT("SM_RA_BarrelSponge_0"), 30, 4.0, 45.0, 0.7, 1.2, Reef, 55);
		Scatter(TEXT("SM_RA_BarrelSponge_1"), 15, 5.0, 45.0, 0.7, 1.1, Reef, 56);
		Scatter(TEXT("SM_RA_LeatherCoral_0"), 60, 3.0, 35.0, 0.7, 1.3, Reef, 57);
		Scatter(TEXT("SM_RA_LeatherCoral_1"), 36, 3.0, 35.0, 0.7, 1.3, Reef, 58);
		Scatter(TEXT("SM_RA_Rock_2"), 20, 4.0, 60.0, 0.6, 1.4, Any, 59);
		School(TEXT("SM_RA_Fish_Trevally"), 240, 1, FVector::ZeroVector, 5.6, 1.4, 0.0, 2.4, 0.9, 301);
		School(TEXT("SM_RA_Fish_Fusilier"), 320, 0, FVector(2, -3, 6.0), 12.0, 4.0, 0.0, 2.5, 1.6, 302);
		School(TEXT("SM_RA_Fish_Anthias"), 450, 2, FVector(0, -12, 3.0), 10.0, 8.0, 0.0, 2.0, 0.2, 303);
		School(TEXT("SM_RA_Fish_Chromis"), 200, 2, FVector(4, 6, -3.5), 8.0, 6.0, 0.0, 1.0, 0.2, 304);
		School(TEXT("SM_RA_Fish_Sweetlips"), 10, 2, FVector(-6, 3, -2.5), 3.0, 2.0, 0.0, 0.5, 0.1, 305);
		School(TEXT("SM_RA_Fish_Batfish"), 7, 3, FVector(0, 0, 1.5), 9.0, 2.0, 0.0, 1.0, 0.4, 306);
		School(TEXT("SM_RA_Fish_Butterfly"), 24, 2, FVector(0, -6, -1.0), 10.0, 7.0, 0.0, 1.5, 0.3, 307);
		School(TEXT("SM_RA_Fish_Parrot"), 12, 3, FVector(0, -8, -0.5), 11.0, 4.0, 0.0, 1.5, 0.6, 308);
		break;
	}
	case 3:   // Manta Sandy: the cleaning station, mantas circling over, under and beside the car.
	{
		Scatter(TEXT("SM_RA_Coral_Massive_1"), 6, 0.0, 3.0, 0.6, 1.0, Any, 61);   // (the bommie's heads: near its centre, below)
		Scatter(TEXT("SM_RA_Coral_Massive_0"), 20, 5.0, 40.0, 0.5, 1.0, Any, 62);
		Scatter(TEXT("SM_RA_Coral_Table_0"), 10, 5.0, 40.0, 0.6, 1.0, Any, 63);
		Scatter(TEXT("SM_RA_Rock_3"), 18, 5.0, 60.0, 0.5, 1.2, Any, 64);
		School(TEXT("SM_RA_Manta"), 1, 3, FVector(0, -8, 2.6), 7.0, 0.0, 0.0, 0.0, 0.9, 401);
		School(TEXT("SM_RA_Manta"), 1, 3, FVector(0, -6, -2.3), 6.5, 0.0, 0.0, 0.0, 0.8, 402);
		School(TEXT("SM_RA_Manta"), 1, 3, FVector(0, -9, 0.3), 9.5, 0.0, 0.0, 0.0, 1.0, 403);
		School(TEXT("SM_RA_Fish_Fusilier"), 120, 0, FVector(10, -18, 3.0), 9.0, 3.0, 0.0, 2.0, 1.4, 404);
		School(TEXT("SM_RA_Fish_Chromis"), 80, 2, FVector(0, -8, -1.0), 3.0, 2.5, 0.0, 0.8, 0.2, 405);
		break;
	}
	case 4:   // The Wall: sea fans into the blue; a pygmy seahorse beside the car; far out, a column of barracuda.
	{
		// The fan beside the car: on the wall at the eyes' height, grown out to 1.2 m from the glass.
		const double WallHere = SeaGen::WallX(0.8, -0.4);
		MeshAt(Thing(TEXT("SM_RA_SeaFan_1")), FTransform(FRotator(-90.0, 0.0, 90.0), FVector(WallHere - 0.05, 0.8, -0.4), FVector(1.0)));
		// The pygmy seahorse on it, 1.1 m out from the wall, at the fan's plane, facing the car.
		MeshAt(Thing(TEXT("SM_RA_PygmySeahorse")), FTransform(FRotator(0.0, -90.0, 0.0), FVector(-3.45, 0.8, 0.05), FVector(1.0)));
		Scatter(TEXT("SM_RA_SeaFan_0"), 14, 3.0, 25.0, 0.8, 1.3, Any, 71, true);
		Scatter(TEXT("SM_RA_SeaFan_2"), 8, 4.0, 30.0, 0.8, 1.2, Any, 72, true);
		Scatter(TEXT("SM_RA_SoftCoral_0"), 40, 2.5, 25.0, 0.8, 1.4, Any, 73, true);
		Scatter(TEXT("SM_RA_SoftCoral_3"), 30, 2.5, 25.0, 0.8, 1.4, Any, 74, true);
		Scatter(TEXT("SM_RA_BarrelSponge_1"), 6, 4.0, 25.0, 0.6, 1.0, Any, 75, true);
		School(TEXT("SM_RA_Fish_Barracuda"), 190, 4, FVector(22, -10, -2.0), 3.4, 1.2, 0.0, 7.0, 0.6, 501);
		School(TEXT("SM_RA_Fish_Anthias"), 300, 2, FVector(-6, 0, 0.0), 6.0, 12.0, 0.0, 6.0, 0.2, 502);
		break;
	}
	default: break;
	}

	// The water's light: god rays in a thin volumetric fog under water; none over the surface places.
	if (UExponentialHeightFogComponent* Fog = Director->Fog)
	{
		// Only its volumetric part (the shafts, near the car): the water's colour with distance is the post-process's
		// (M_RA_Underwater); the plain height fog would go to one flat colour beyond the volume, and lie over the sky.
		const bool bUnder = Pl.Depth > 0.5;
		Fog->SetVisibility(bUnder);
		Fog->SetFogMaxOpacity(0.f);
		Fog->SetVolumetricFog(true);
		Fog->SetFogDensity(0.035f);
		Fog->SetFogHeightFalloff(0.00001f);
		Fog->SetVolumetricFogAlbedo(FColor(90, 190, 215));
		Fog->SetVolumetricFogScatteringDistribution(0.75f);
		Fog->SetVolumetricFogExtinctionScale(0.35f);
		Fog->SetFogInscatteringColor(FLinearColor(0.004f, 0.02f, 0.035f));
		Fog->SetStartDistance(0.f);
	}
}

void UJourneySea::AdjustSky(FMuseeSkyOverride& Sky, int32 Place)
{
	// The caustics under the surface (the light function passes the light unchanged above it); a brighter sun to make up
	// for the caustics' mean (0.62), where they fall.
	Sky.LightFunction = Caustics;
	Sky.SunLux *= 1.3f;
}

void UJourneySea::MoveFish(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	const double T = Clock;
	const int32 Place = Current + 1;
	auto Move = [&](FGroup& G)
	{
		if (!G.Mesh || G.Fish.Num() == 0) { return; }
		TArray<FTransform> Out;
		Out.SetNum(G.Fish.Num());
		for (int32 i = 0; i < G.Fish.Num(); ++i)
		{
			const FVector4& F = G.Fish[i];
			FVector P, Dir;
			const double R = FMath::Max(0.5, G.Radius + F.X * G.RadiusSpread);
			switch (G.Kind)
			{
			case 2:   // hovering: a slow wander about a home over the bottom
			{
				const FVector Home = G.Centre + FVector(F.X * G.RadiusSpread, F.Y * G.RadiusSpread, 0) + FVector(0, 0, G.HeightSpread * (0.5 + 0.5 * FMath::Sin(F.Z)));
				const double W = 0.3 * F.W;
				P = Home + FVector(0.35 * FMath::Sin(W * T + F.Z), 0.35 * FMath::Cos(0.7 * W * T + F.Z * 1.3), 0.15 * FMath::Sin(0.5 * W * T + F.Z * 2.1));
				Dir = FVector(0.35 * W * FMath::Cos(W * T + F.Z), -0.35 * 0.7 * W * FMath::Sin(0.7 * W * T + F.Z * 1.3), 0.0);
				break;
			}
			case 3:   // cruising: an ellipse round the centre, rising and falling a little
			{
				const double A = F.Z + G.Speed * F.W * T / FMath::Max(R, 0.5);
				P = G.Centre + FVector(R * FMath::Cos(A), 0.75 * R * FMath::Sin(A), G.Height + F.Y * G.HeightSpread + 0.4 * FMath::Sin(A * 2.0));
				Dir = FVector(-R * FMath::Sin(A), 0.75 * R * FMath::Cos(A), 0.8 * FMath::Cos(A * 2.0));
				break;
			}
			case 4:   // the column: fish round a vertical axis, stacked in height, turning slowly
			{
				const double A = F.Z + G.Speed * F.W * T / FMath::Max(R, 0.5);
				P = G.Centre + FVector(R * FMath::Cos(A), R * FMath::Sin(A), G.Height + F.Y * G.HeightSpread);
				Dir = FVector(-FMath::Sin(A), FMath::Cos(A), 0.0);
				break;
			}
			case 5:   // walking on the bottom: slow, round a circle, the body close over the sand
			{
				const double A = F.Z + G.Speed * T / FMath::Max(R, 0.5);
				const double X = G.Centre.X + R * FMath::Cos(A), Y = G.Centre.Y + R * FMath::Sin(A);
				P = FVector(X, Y, SeaGen::BedHeight(Place, X, Y) + 0.08);
				Dir = FVector(-FMath::Sin(A), FMath::Cos(A), 0.0);
				break;
			}
			case 6:   // the veil: a thick shoal round the car, drawing in as the veil thickens
			{
				const double Rv = FMath::Lerp(18.0, R, double(CurrentVeil));
				const double A = F.Z + G.Speed * F.W * T / FMath::Max(Rv, 0.5);
				P = FVector(Rv * FMath::Cos(A), Rv * FMath::Sin(A), F.Y * G.HeightSpread);
				Dir = FVector(-FMath::Sin(A), FMath::Cos(A), 0.0);
				break;
			}
			default:  // 0 a circling school, 1 a ring round the car
			{
				const double A = F.Z + G.Speed * F.W * T / FMath::Max(R, 0.5);
				const double Wobble = 0.3 * FMath::Sin(T * 0.7 + F.Z * 3.0);
				P = G.Centre + FVector((R + Wobble) * FMath::Cos(A), (R + Wobble) * FMath::Sin(A), G.Height + F.Y * G.HeightSpread + 0.25 * FMath::Sin(T * 0.5 + F.Z));
				Dir = FVector(-FMath::Sin(A), FMath::Cos(A), 0.08 * FMath::Cos(T * 0.5 + F.Z));
				break;
			}
			}
			if (G.Speed < 0) { Dir = -Dir; }
			Out[i] = FTransform(Dir.GetSafeNormal().Rotation(), P * 100.0);
		}
		G.Mesh->BatchUpdateInstancesTransforms(0, Out, false, true, true);
		G.Mesh->SetVisibility(G.Kind != 6 || CurrentVeil > 0.01f);
	};
	for (FGroup& G : Groups) { Move(G); }
	for (FGroup& G : JourneyGroups) { Move(G); }
}

void UJourneySea::TickPlace(float DeltaSeconds, double LocalHour, float Fraction, float Veil)
{
	CurrentVeil = Veil;
	MoveFish(DeltaSeconds);
	if (UMaterialParameterCollection* MPC = Director->Parameters())
	{
		// The sea's glow follows the sun (an idealised day: sunrise 6:05, sunset 18:15 at Piaynemo).
		const double Day = FMath::Clamp(FMath::Sin(SeaPi * (LocalHour - 6.1) / 12.1), 0.0, 1.0);
		UKismetMaterialLibrary::SetScalarParameterValue(Director, MPC, TEXT("SunNits"), float(1600.0 * FMath::Pow(Day, 0.7) + 0.02));
		// The plankton at night; the lights go out one by one at the journey's end.
		const float Night = float(FMath::Clamp((LocalHour - 19.2) / 0.5, 0.0, 1.0)) * (1.f - FMath::Clamp((Fraction - 0.85f) / 0.12f, 0.f, 1.f));
		UKismetMaterialLibrary::SetScalarParameterValue(Director, MPC, TEXT("Plankton"), Current == 5 ? Night : 0.f);
	}
}

void UJourneySea::Teardown()
{
	if (Director && Director->Post) { Director->Post->Settings.WeightedBlendables.Array.RemoveAll([this](const FWeightedBlendable& B) { return B.Object == Underwater.Get(); }); }
	if (UMaterialParameterCollection* MPC = Director ? Director->Parameters() : nullptr)
	{
		UKismetMaterialLibrary::SetScalarParameterValue(Director, MPC, TEXT("WaterOn"), 0.f);
		UKismetMaterialLibrary::SetScalarParameterValue(Director, MPC, TEXT("Plankton"), 0.f);
	}
	Groups.Reset();
	JourneyGroups.Reset();
	Super::Teardown();
	Surface = nullptr;
	Plankton = nullptr;
}
