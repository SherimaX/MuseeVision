#include "ChineseWing/ChineseWingStructure.h"

#include "ChineseWing/ChineseWingGeometry.h"
#include "Components/RectLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Geometry/MuseeBake.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"

/** The actor's side of the Chinese Wing: sections, materials, lights, and the test hooks. A named namespace (unity builds). */
namespace ChineseWingActor
{
	namespace CWB = ChineseWingBuild;
	using ChineseWingKit::FPart;

	/** The material slots, in FallbackMaterials' order. */
	enum ESlot : int32
	{
		SlotWall, SlotVestibule, SlotPaving, SlotGravel, SlotGreyStone, SlotPondBank, SlotEdging, SlotPlinth,
		SlotTimber, SlotSeatRail, SlotCaseBed, SlotTile, SlotMortar, SlotRidge, SlotCount
	};

	/** Every section: its component (0 Structure, 1 Frame, 2 Roof, 3 PondGuard), index, material slot, part and name. */
	struct FSection
	{
		int32 Component;
		int32 Index;
		int32 Slot;
		const FPart* Part;
		const TCHAR* Name;
	};

	TArray<FSection> Sections(const CWB::FWingMeshes& M)
	{
		return {
			{0, 0, SlotWall, &M.Walls, TEXT("Walls")}, {0, 1, SlotVestibule, &M.Plaster, TEXT("Vestibule")},
			{0, 2, SlotPaving, &M.Paving, TEXT("Paving")}, {0, 3, SlotGravel, &M.Gravel, TEXT("Gravel")},
			{0, 4, SlotGreyStone, &M.GreyStone, TEXT("GreyStone")}, {0, 5, SlotPondBank, &M.Bank, TEXT("PondBank")},
			{0, 6, SlotEdging, &M.Edging, TEXT("EdgingStones")}, {0, 7, SlotPlinth, &M.Plinths, TEXT("Plinths")},
			{1, 0, SlotTimber, &M.Timber, TEXT("Timber")}, {1, 1, SlotSeatRail, &M.Seats, TEXT("SeatRails")},
			{1, 2, SlotCaseBed, &M.Beds, TEXT("CaseBeds")},
			{2, 0, SlotTile, &M.Tiles, TEXT("Tiles")}, {2, 1, SlotMortar, &M.Mortar, TEXT("Mortar")}, {2, 2, SlotRidge, &M.Ridges, TEXT("Ridges")},
			{3, 0, INDEX_NONE, &M.Guard, TEXT("PondGuard")},
		};
	}

	TSoftObjectPtr<UMaterialInterface> Soft(const TCHAR* Folder, const TCHAR* Name)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name)));
	}

	/** The rect light's rotation for a strip: X along its facing, Y along its length. */
	FRotator StripRotation(const CWB::FLightStrip& S) { return FRotationMatrix::MakeFromXY(S.Facing, S.Along).Rotator(); }

	void WriteObj(const CWB::FWingMeshes& M, FString& Out)
	{
		Out = TEXT("# AChineseWingStructure geometry: centimetres, x east, y south, z up. Groups: part|assembly|solid|closed.\n");
		int32 Offset = 1;
		for (const FSection& S : Sections(M))
		{
			const SalonKit::FMeshData& D = S.Part->M;
			Out += FString::Printf(TEXT("o %s\n"), S.Name);
			for (int32 i = 0; i < D.Positions.Num(); ++i)
			{
				const FVector& P = D.Positions[i];
				const FVector& N = D.Normals[i];
				const FVector2D& UV = D.UVs[i];
				Out += FString::Printf(TEXT("v %.5f %.5f %.5f\nvn %.5f %.5f %.5f\nvt %.5f %.5f\n"), P.X, P.Y, P.Z, N.X, N.Y, N.Z, UV.X, UV.Y);
			}
			const TArray<ChineseWingKit::FGroup>& Groups = S.Part->Groups;
			for (int32 g = 0; g < Groups.Num(); ++g)
			{
				const int32 First = Groups[g].FirstIndex;
				const int32 End = g + 1 < Groups.Num() ? Groups[g + 1].FirstIndex : D.Indices.Num();
				Out += FString::Printf(TEXT("g %s|%s|%s|%d\n"), S.Name, *Groups[g].Assembly, *Groups[g].Name, Groups[g].bClosed ? 1 : 0);
				for (int32 t = First; t + 2 < End; t += 3)
				{
					const int32 A = D.Indices[t] + Offset, B = D.Indices[t + 1] + Offset, C = D.Indices[t + 2] + Offset;
					Out += FString::Printf(TEXT("f %d/%d/%d %d/%d/%d %d/%d/%d\n"), A, A, A, B, B, B, C, C, C);
				}
			}
			Offset += D.Positions.Num();
		}
	}

	/** musee.ChineseWing.Preview: for a test run, places the wing at the origin (not saved) and hides what it replaces. */
	void Preview(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		AChineseWingStructure* Wing = nullptr;
		for (TActorIterator<AChineseWingStructure> It(World); It; ++It) { Wing = *It; break; }
		if (!Wing)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Wing = World->SpawnActor<AChineseWingStructure>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		}
		const TArray<FString> Prims = AChineseWingStructure::GetReplacedImportPrims();
		int32 Hidden = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			bool bReplaced = false;
			for (const FName& Tag : It->Tags)
			{
				const FString Text = Tag.ToString();
				if (!Text.StartsWith(TEXT("prim:"))) { continue; }
				const FString PrimPath = Text.Mid(5);
				for (const FString& Replaced : Prims)
				{
					if (PrimPath == Replaced || PrimPath.StartsWith(Replaced + TEXT("/"))) { bReplaced = true; }
				}
			}
			if (bReplaced)
			{
				It->SetActorHiddenInGame(true);
				It->SetActorEnableCollision(false);
				++Hidden;
			}
		}
		UE_LOG(LogMusee, Log, TEXT("ChineseWing preview: native wing %s, %d imported actors hidden."), Wing ? TEXT("placed") : TEXT("NOT placed"), Hidden);
	}

	FAutoConsoleCommandWithWorldAndArgs GPreviewCommand(
		TEXT("musee.ChineseWing.Preview"),
		TEXT("Places the native Chinese Wing at the origin for this run (not saved) and hides the imported pieces it replaces."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Preview));

	void WriteObjCommand(const TArray<FString>& Args)
	{
		const FString Path = Args.Num() > 0 ? Args[0] : FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ChineseWing"), TEXT("ChineseWing.obj"));
		const bool bOk = AChineseWingStructure::WriteGeometryObj(Path);
		UE_LOG(LogMusee, Log, TEXT("ChineseWing geometry %s %s."), bOk ? TEXT("written to") : TEXT("NOT written to"), *Path);
	}

	FAutoConsoleCommand GWriteObjCommand(
		TEXT("musee.ChineseWing.WriteObj"),
		TEXT("Writes the native Chinese Wing's geometry as OBJ (default Saved/ChineseWing/ChineseWing.obj) for the checks in Scripts."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&WriteObjCommand));
}

namespace CWA = ChineseWingActor;

AChineseWingStructure::AChineseWingStructure()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* ComponentName, bool bCollision)
	{
		UProceduralMeshComponent* Component = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		Component->SetupAttachment(RootComponent);
		Component->bUseAsyncCooking = true;
		if (bCollision)
		{
			Component->bUseComplexAsSimpleCollision = true;
			Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Component;
	};
	Structure = Make(TEXT("Structure"), true);
	Frame = Make(TEXT("Frame"), true);
	Roof = Make(TEXT("Roof"), false);
	PondGuard = Make(TEXT("PondGuard"), true);
	PondGuard->SetVisibility(false);
	PondGuard->SetHiddenInGame(true);
	PondGuard->SetCastShadow(false);
	PondGuard->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	MuseeBake::NoBake(PondGuard);   // never drawn, collision only: stays procedural

	auto MakeLight = [this](const FString& ComponentName)
	{
		URectLightComponent* L = CreateDefaultSubobject<URectLightComponent>(*ComponentName);
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		return L;
	};
	const int32 Eaves = ChineseWingBuild::EaveStrips().Num(), Paths = ChineseWingBuild::PathStrips().Num();
	for (int32 i = 0; i < Eaves; ++i) { EaveLights.Add(MakeLight(FString::Printf(TEXT("EaveLight%d"), i + 1))); }
	for (int32 i = 0; i < Paths; ++i) { PathLights.Add(MakeLight(FString::Printf(TEXT("PathLight%02d"), i + 1))); }
	VestibuleLight = MakeLight(TEXT("VestibuleLight"));

	const TCHAR* Materials = TEXT("/Game/Museum/Materials");
	const TCHAR* Imported = TEXT("/Game/Museum/Materials/USD");
	WallMaterial = CWA::Soft(Imported, TEXT("MI_whitewash"));
	VestibuleMaterial = CWA::Soft(Imported, TEXT("MI_plaster_vestibule"));
	PavingMaterial = CWA::Soft(Imported, TEXT("MI_stone_slab_EFE8DB"));
	GravelMaterial = CWA::Soft(Imported, TEXT("MI_gravel"));
	GreyStoneMaterial = CWA::Soft(Imported, TEXT("MI_stone_grey"));
	PondBankMaterial = CWA::Soft(Imported, TEXT("MI_pond_bank"));
	EdgingStoneMaterial = CWA::Soft(Imported, TEXT("MI_pond_bank"));
	PlinthMaterial = CWA::Soft(Imported, TEXT("MI_stone_plinth_r75"));
	TimberMaterial = CWA::Soft(Imported, TEXT("MI_timber_dark_r70"));
	SeatRailMaterial = CWA::Soft(Imported, TEXT("MI_seat_rail"));
	CaseBedMaterial = CWA::Soft(Imported, TEXT("MI_silk_bed"));
	TileMaterial = CWA::Soft(Imported, TEXT("MI_roof_tiles"));
	MortarMaterial = CWA::Soft(Imported, TEXT("MI_roof_coping"));
	RidgeMaterial = CWA::Soft(Imported, TEXT("MI_roof_ridge"));
	FallbackMaterials = {
		CWA::Soft(Materials, TEXT("M_Plaster")), CWA::Soft(Imported, TEXT("MI_whitewash")), CWA::Soft(Imported, TEXT("MI_stone_grey")),
		CWA::Soft(Imported, TEXT("MI_stone_grey")), CWA::Soft(Imported, TEXT("MI_stone_plinth_r75")), CWA::Soft(Imported, TEXT("MI_stone_grey")),
		CWA::Soft(Imported, TEXT("MI_stone_grey")), CWA::Soft(Imported, TEXT("MI_stone_grey")), CWA::Soft(Imported, TEXT("MI_timber_dark_r90")),
		CWA::Soft(Imported, TEXT("MI_timber_dark_r70")), CWA::Soft(Imported, TEXT("MI_plinth_cream")), CWA::Soft(Imported, TEXT("MI_roof_coping")),
		CWA::Soft(Imported, TEXT("MI_stone_grey")), CWA::Soft(Imported, TEXT("MI_roof_coping")),
	};
	DaylightParameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/MPC_Musee.MPC_Musee")));

	AddTags();
	PlaceLights();
}

TArray<FString> AChineseWingStructure::GetReplacedImportPrims()
{
	const FString Root = TEXT("/Museum/ChineseWing/");
	TArray<FString> Out;
	for (const TCHAR* Name : {
			 // The vestibule, the walls and their copings, the moon gate's surround, the floors.
			 TEXT("Vestibule"), TEXT("Vestibule_floor"), TEXT("Chinese_Wing_walls"), TEXT("Coping"), TEXT("Coping_ridge"),
			 TEXT("Moon_gate_surround"), TEXT("Cloister_paving"), TEXT("Gravel"),
			 // The cloister: its frame, seats, roofs and boarding.
			 TEXT("Colonnade"), TEXT("Seat_rails"), TEXT("Tile_roofs"), TEXT("Roof_boarding"),
			 // The garden's stonework (the water, the lotus, the rock and the plants stay).
			 TEXT("Pond_bank"), TEXT("Edging_stones"), TEXT("Bamboo_beds"), TEXT("Terrace_benches"),
			 // The display furniture (not the works on it): the picture rail, the cases, the plinths.
			 TEXT("Picture_rail"), TEXT("Handscroll_case"), TEXT("Scroll_bed"), TEXT("Handscroll_glass"), TEXT("Case_trim"),
			 TEXT("Orchid_case"), TEXT("Orchid_case_glass"),
			 TEXT("Ceramic_plinth"), TEXT("Ceramic_plinth_2"), TEXT("Ceramic_plinth_3"), TEXT("Ceramic_plinth_4"), TEXT("Ceramic_plinth_5"),
			 TEXT("Ceramic_case"), TEXT("Ceramic_case_2"), TEXT("Ceramic_case_3"), TEXT("Ceramic_case_4"),
			 TEXT("Case_ring"), TEXT("Case_ring_2"), TEXT("Case_ring_3"), TEXT("Case_ring_4"),
			 // The cloister's lamps in ChineseWing.swift's order: under the south eave, the west walk, the east walk.
			 TEXT("SpotLight"), TEXT("SpotLight_2"), TEXT("SpotLight_3")})
	{
		Out.Add(Root + Name);
	}
	return Out;
}

TArray<FVector> AChineseWingStructure::GetColumnPositions() const
{
	TArray<FVector> Out;
	for (const FVector2D& C : ChineseWingBuild::ColumnCentres()) { Out.Add(GetActorTransform().TransformPosition(MuseePlan::At(C.X, C.Y, 0.0))); }
	return Out;
}

FVector AChineseWingStructure::GetMoonGateCentre() const
{
	namespace CWPlan = MuseePlan::ChineseWing;
	return GetActorTransform().TransformPosition(MuseePlan::At(0.0, CWPlan::Y0 - CWPlan::Wall / 2, CWPlan::MoonGateCentreHeight));
}

bool AChineseWingStructure::WriteGeometryObj(const FString& Path)
{
	const ChineseWingBuild::FWingMeshes Meshes = ChineseWingBuild::BuildMeshes();
	FString Text;
	CWA::WriteObj(Meshes, Text);
	return FFileHelper::SaveStringToFile(Text, *Path);
}

void AChineseWingStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
	PlaceLights();
	AddTags();
}

void AChineseWingStructure::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	PlaceLights();
	AddTags();
}

void AChineseWingStructure::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never shows an older build.
	Build();
	ApplyMaterials();
	LastNight = -1.f;
}

void AChineseWingStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) { return; }
	float Daylight = 0.f;
	if (UMaterialParameterCollection* Collection = DaylightParameters.LoadSynchronous())
	{
		Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, Collection, TEXT("Daylight"));
	}
	// Full at dusk and through the night; out once the sun is well up.
	const float Night = 1.f - FMath::SmoothStep(0.f, 0.2f, Daylight);
	if (FMath::Abs(Night - LastNight) > 0.01f) { UpdateLights(Night); }
}

void AChineseWingStructure::AddTags()
{
	// Part of the building (hidden in the Sphere with the rest). Its lamps follow the sky themselves (no musee.laylight).
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:ChineseWing")));
	// It ticks only to dim its lamps; its meshes never move, so they may be baked (Geometry/MuseeBake.h).
	Tags.AddUnique(MuseeBake::BakeableTag());
}

void AChineseWingStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	const ChineseWingBuild::FWingMeshes Meshes = ChineseWingBuild::BuildMeshes();
	UProceduralMeshComponent* Components[4] = {Structure.Get(), Frame.Get(), Roof.Get(), PondGuard.Get()};
	for (UProceduralMeshComponent* Component : Components)
	{
		if (Component) { Component->ClearAllMeshSections(); }
	}
	for (const CWA::FSection& S : CWA::Sections(Meshes))
	{
		if (UProceduralMeshComponent* Component = Components[S.Component]) { S.Part->M.Write(Component, S.Index, S.Component != 2); }
	}
}

void AChineseWingStructure::ApplyMaterials()
{
	const TSoftObjectPtr<UMaterialInterface>* Wanted[CWA::SlotCount] = {
		&WallMaterial, &VestibuleMaterial, &PavingMaterial, &GravelMaterial, &GreyStoneMaterial, &PondBankMaterial, &EdgingStoneMaterial,
		&PlinthMaterial, &TimberMaterial, &SeatRailMaterial, &CaseBedMaterial, &TileMaterial, &MortarMaterial, &RidgeMaterial};
	UMaterialInterface* Picked[CWA::SlotCount] = {};
	for (int32 i = 0; i < CWA::SlotCount; ++i)
	{
		Picked[i] = Wanted[i]->LoadSynchronous();
		if (!Picked[i] && FallbackMaterials.IsValidIndex(i)) { Picked[i] = FallbackMaterials[i].LoadSynchronous(); }
	}
	UProceduralMeshComponent* Components[4] = {Structure.Get(), Frame.Get(), Roof.Get(), PondGuard.Get()};
	const ChineseWingBuild::FWingMeshes None;
	for (const CWA::FSection& S : CWA::Sections(None))
	{
		if (S.Slot == INDEX_NONE || !Components[S.Component] || !Picked[S.Slot]) { continue; }
		Components[S.Component]->SetMaterial(S.Index, Picked[S.Slot]);
	}
}

void AChineseWingStructure::PlaceLights()
{
	auto Place = [this](URectLightComponent* L, const ChineseWingBuild::FLightStrip& S, bool bShadows, float BarnAngle, float BarnLength, float RangeCm)
	{
		if (!L) { return; }
		L->SetRelativeLocationAndRotation(S.Centre * MuseePlan::Cm, CWA::StripRotation(S));
		L->SetSourceWidth(S.Width * MuseePlan::Cm);
		L->SetSourceHeight(S.Height * MuseePlan::Cm);
		L->SetBarnDoorAngle(BarnAngle);
		L->SetBarnDoorLength(BarnLength);
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetUseTemperature(true);
		L->SetTemperature(LightKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetAttenuationRadius(RangeCm);
		L->SetCastShadows(bShadows);
	};
	const TArray<ChineseWingBuild::FLightStrip> Eaves = ChineseWingBuild::EaveStrips();
	for (int32 i = 0; i < EaveLights.Num() && i < Eaves.Num(); ++i) { Place(EaveLights[i], Eaves[i], true, 80.f, 2.f, 900.f); }
	const TArray<ChineseWingBuild::FLightStrip> Paths = ChineseWingBuild::PathStrips();
	for (int32 i = 0; i < PathLights.Num() && i < Paths.Num(); ++i) { Place(PathLights[i], Paths[i], false, 60.f, 3.f, 400.f); }
	Place(VestibuleLight, ChineseWingBuild::VestibuleStrip(), true, 88.f, 0.f, 900.f);
	if (VestibuleLight) { VestibuleLight->SetIntensity(VestibuleLumens); }
	UpdateLights(1.f);
}

void AChineseWingStructure::UpdateLights(float Night)
{
	LastNight = Night;
	const float Eave = FMath::Lerp(EaveDayFraction, 1.f, Night);
	for (URectLightComponent* L : EaveLights)
	{
		if (!L) { continue; }
		L->SetIntensity(EaveLumensPerMetre * L->SourceWidth / MuseePlan::Cm * Eave);
		L->SetVisibility(Eave > 0.001f);
	}
	for (URectLightComponent* L : PathLights)
	{
		if (!L) { continue; }
		L->SetIntensity(PathLumensPerMetre * L->SourceWidth / MuseePlan::Cm * Night);
		L->SetVisibility(Night > 0.001f);
	}
}
