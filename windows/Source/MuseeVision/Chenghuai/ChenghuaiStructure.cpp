#include "Chenghuai/ChenghuaiStructure.h"

#include "Chenghuai/ChenghuaiGeometry.h"
#include "Chenghuai/ChenghuaiPlan.h"
#include "Chenghuai/ChenghuaiWater.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Curves/CurveFloat.h"
#include "Kismet/GameplayStatics.h"
#include "Nature/MuseeNatureActor.h"
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

/** The actor's side of Chenghuai: components, sections, materials, lights. A named namespace (unity builds). */
namespace ChenghuaiActor
{
	namespace CB = ChenghuaiBuild;

	const TCHAR* PartNames[CB::PartCount] = {
		TEXT("BrickFine"), TEXT("BrickHairline"), TEXT("BrickPointed"), TEXT("PlasterWhite"), TEXT("PlasterRoom"), TEXT("StoneKerb"),
		TEXT("FloorLarge"), TEXT("FloorSmall"), TEXT("CourtPaving"), TEXT("CourtPath"), TEXT("GardenPebble"), TEXT("GardenGround"),
		TEXT("Rockery"), TEXT("PondBed"), TEXT("BlueStone"), TEXT("Engraved"),
		TEXT("RedLacquer"), TEXT("RafterRed"), TEXT("PaintedBeam"), TEXT("GreenLacquer"), TEXT("BlackLacquer"), TEXT("Nanmu"), TEXT("Chestnut"),
		TEXT("Paper"), TEXT("Brass"), TEXT("Plaques"), TEXT("CaseSilk"), TEXT("CaseLamp"), TEXT("LanternSilk"),
		TEXT("Tiles"), TEXT("RoofMortar"), TEXT("Ridges"), TEXT("Glass"), TEXT("Guard")};

	/** Which component a part goes in: 0 Structure, 1 Frame, 2 Roof, 3 Glass, 5 Guard. */
	int32 ComponentOf(int32 Part)
	{
		if (Part <= CB::Engraved) { return 0; }
		if (Part <= CB::LanternSilk) { return 1; }
		if (Part <= CB::Ridges) { return 2; }
		if (Part == CB::Glass) { return 3; }
		return 5;
	}

	/** Its section index in that component. */
	int32 SectionOf(int32 Part)
	{
		const int32 C = ComponentOf(Part);
		int32 First = 0;
		for (int32 p = 0; p < Part; ++p) { if (ComponentOf(p) == C) { ++First; } }
		return First;
	}

	TSoftObjectPtr<UMaterialInterface> Soft(const FString& Path)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path));
	}

	FString MaterialPath(const TCHAR* Folder, const TCHAR* Name) { return FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name); }

	void WriteObj(const CB::FChMeshes& M, FString& Out)
	{
		Out = TEXT("# AChenghuaiStructure geometry: centimetres, x east, y south, z up. Groups: part|assembly|solid|closed.\n");
		int32 Offset = 1;
		for (int32 p = 0; p < CB::PartCount; ++p)
		{
			const SalonKit::FMeshData& D = M[p].M;
			Out += FString::Printf(TEXT("o %s\n"), PartNames[p]);
			for (int32 i = 0; i < D.Positions.Num(); ++i)
			{
				const FVector& P = D.Positions[i];
				const FVector& N = D.Normals[i];
				Out += FString::Printf(TEXT("v %.4f %.4f %.4f\nvn %.4f %.4f %.4f\nvt %.4f %.4f\n"), P.X, P.Y, P.Z, N.X, N.Y, N.Z, D.UVs[i].X, D.UVs[i].Y);
			}
			const TArray<ChenghuaiKit::FGroup>& Groups = M[p].Groups;
			for (int32 g = 0; g < Groups.Num(); ++g)
			{
				const int32 First = Groups[g].FirstIndex;
				const int32 End = g + 1 < Groups.Num() ? Groups[g + 1].FirstIndex : D.Indices.Num();
				Out += FString::Printf(TEXT("g %s|%s|%s|%d\n"), PartNames[p], *Groups[g].Assembly, *Groups[g].Name, Groups[g].bClosed ? 1 : 0);
				for (int32 t = First; t + 2 < End; t += 3)
				{
					const int32 A = D.Indices[t] + Offset, B = D.Indices[t + 1] + Offset, C = D.Indices[t + 2] + Offset;
					Out += FString::Printf(TEXT("f %d/%d/%d %d/%d/%d %d/%d/%d\n"), A, A, A, B, B, B, C, C, C);
				}
			}
			Offset += D.Positions.Num();
		}
	}

	void WriteObjCommand(const TArray<FString>& Args)
	{
		const FString Path = Args.Num() > 0 ? Args[0] : FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Chenghuai"), TEXT("Chenghuai.obj"));
		const bool bOk = AChenghuaiStructure::WriteGeometryObj(Path);
		UE_LOG(LogMusee, Log, TEXT("Chenghuai geometry %s %s."), bOk ? TEXT("written to") : TEXT("NOT written to"), *Path);
	}

	FAutoConsoleCommand GWriteObjCommand(
		TEXT("musee.Chenghuai.WriteObj"),
		TEXT("Writes Chenghuai's geometry as OBJ (default Saved/Chenghuai/Chenghuai.obj) for the checks."),
		FConsoleCommandWithArgsDelegate::CreateStatic(&WriteObjCommand));

	/** musee.Chenghuai.Stats: triangles per part. */
	void StatsCommand(const TArray<FString>& Args)
	{
		const CB::FChMeshes M = CB::BuildMeshes();
		int32 Total = 0;
		for (int32 p = 0; p < CB::PartCount; ++p)
		{
			const int32 T = M[p].M.Indices.Num() / 3;
			Total += T;
			UE_LOG(LogMusee, Log, TEXT("Chenghuai %-14s %8d triangles, %6d solids"), PartNames[p], T, M[p].Groups.Num());
		}
		UE_LOG(LogMusee, Log, TEXT("Chenghuai total %d triangles"), Total);
	}

	/** musee.Chenghuai.Preview: for a test run, places the wing at the origin (not saved) and hides the Sculpture Hall. */
	void Preview(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		AChenghuaiStructure* Wing = nullptr;
		for (TActorIterator<AChenghuaiStructure> It(World); It; ++It) { Wing = *It; break; }
		if (!Wing)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Wing = World->SpawnActor<AChenghuaiStructure>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
		}
		int32 Hidden = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			const FString Cls = It->GetClass()->GetName();
			bool bSculpture = Cls == TEXT("SculptureHallStructure");
			for (const FName& Tag : It->Tags)
			{
				const FString T = Tag.ToString();
				if (T == TEXT("musee.wing:SculptureHall") || T.StartsWith(TEXT("prim:/Museum/SculptureHall"))) { bSculpture = true; }
			}
			if (bSculpture)
			{
				It->SetActorHiddenInGame(true);
				It->SetActorEnableCollision(false);
				++Hidden;
			}
		}
		UE_LOG(LogMusee, Log, TEXT("Chenghuai preview: native wing %s, %d Sculpture Hall actors hidden."), Wing ? TEXT("placed") : TEXT("NOT placed"), Hidden);
	}

	FAutoConsoleCommandWithWorldAndArgs GPreviewCommand(
		TEXT("musee.Chenghuai.Preview"),
		TEXT("Places Chenghuai at the origin for this run (not saved) and hides the Sculpture Hall."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Preview));

	FAutoConsoleCommand GStatsCommand(TEXT("musee.Chenghuai.Stats"), TEXT("Chenghuai's triangles per part."),
									  FConsoleCommandWithArgsDelegate::CreateStatic(&StatsCommand));
}

namespace CHA = ChenghuaiActor;
namespace CB = ChenghuaiBuild;

AChenghuaiStructure::AChenghuaiStructure()
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
	Glass = Make(TEXT("Glass"), false);
	Glass->SetCastShadow(false);
	Water = Make(TEXT("Water"), false);
	Water->SetCastShadow(false);
	MuseeBake::NoBake(Water);   // Single Layer Water can't be Nanite
	Guard = Make(TEXT("Guard"), true);
	Guard->SetVisibility(false);
	Guard->SetHiddenInGame(true);
	Guard->SetCastShadow(false);
	Guard->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	MuseeBake::NoBake(Guard);

	// Outdoors in the courts and the garden: the grounds' exposure curve (AMuseeLandscape's), blended in by Tick.
	CourtExposureCurve = CreateDefaultSubobject<UCurveFloat>(TEXT("CourtExposureCurve"));
	// (A sunlit court of grey brick and white lime meters EV100 14-15: exposed as a photographer would, the lime just
	// short of clipping and the paving mid-grey, about a stop under the grounds' curve, which is set for lawn.)
	// (2026-09-26: a stop lower in sun: sunlit grey brick reads mid-grey (sRGB ~125-140), the lime just off white, as
	// Gongwangfu's courts do in the reference photographs; the night keys expose a lantern-lit court as a night
	// photograph would, the paving in the lanterns' pools legible, the moonlit roofs dim.)
	for (const FVector2f Key : {FVector2f(-4.f, -1.1f), FVector2f(-1.f, -0.8f), FVector2f(3.f, 0.0f), FVector2f(6.f, 0.4f), FVector2f(11.5f, 0.1f), FVector2f(12.5f, -0.2f),
								FVector2f(13.5f, -0.7f), FVector2f(14.5f, -1.1f), FVector2f(15.5f, -1.3f), FVector2f(17.f, -1.4f)})
	{
		CourtExposureCurve->FloatCurve.AddKey(Key.X, Key.Y);
	}
	CourtExposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("CourtExposure"));
	CourtExposure->SetupAttachment(RootComponent);
	CourtExposure->bUnbound = true;
	CourtExposure->Priority = 6.f;   // over the grounds' (5, whose trace doesn't find this wing), under the wings' zones (10)
	CourtExposure->BlendWeight = 0.f;
	CourtExposure->Settings.bOverride_AutoExposureBias = true;
	CourtExposure->Settings.AutoExposureBias = 0.f;
	CourtExposure->Settings.bOverride_AutoExposureBiasCurve = true;
	CourtExposure->Settings.AutoExposureBiasCurve = CourtExposureCurve;
	// At night the museum's floor (EV100 5.5: a lamplit gallery) would leave a court lit by lanterns and the moon black;
	// out here the eye (and a photographer) adapts further.
	CourtExposure->Settings.bOverride_AutoExposureMinBrightness = true;
	CourtExposure->Settings.AutoExposureMinBrightness = -3.0f;   // a full moon's court meters about EV100 -2 to -3: a night photograph's exposure
	// (the museum's histogram starts at EV100 0, which pinned a moonlit court's metering there: 3 stops under)
	CourtExposure->Settings.bOverride_HistogramLogMin = true;
	CourtExposure->Settings.HistogramLogMin = -8.f;
	RoomExposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("RoomExposure"));
	RoomExposure->SetupAttachment(RootComponent);
	RoomExposure->bUnbound = true;
	RoomExposure->Priority = 6.f;
	RoomExposure->BlendWeight = 0.f;
	RoomExposure->Settings.bOverride_AutoExposureBias = true;
	RoomExposure->Settings.AutoExposureBias = 0.9f - 0.7f;   // the museum's 0.9 (setup_project.EXPOSURE_BIAS) less 0.7 stop
	// The rooms' own white: daylight through old paper (about 5000 K) and the cases' 3500 K lamps, balanced as a
	// photographer would in them (the museum's 6600 K left them peach under the red timber).
	RoomExposure->Settings.bOverride_WhiteTemp = true;
	RoomExposure->Settings.WhiteTemp = 5300.f;

	const TArray<CB::FChLight> L = CB::Lights();
	for (int32 i = 0; i < L.Num(); ++i)
	{
		URectLightComponent* Light = CreateDefaultSubobject<URectLightComponent>(*FString::Printf(TEXT("Lamp%03d"), i));
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Lamps.Add(Light);
	}

	const TCHAR* Folder = TEXT("/Game/Museum/Materials/Chenghuai");
	const TCHAR* Materials = TEXT("/Game/Museum/Materials");
	const TCHAR* Usd = TEXT("/Game/Museum/Materials/USD");
	for (int32 p = 0; p < CB::PartCount; ++p)
	{
		PartMaterials.Add(CHA::Soft(CHA::MaterialPath(Folder, *FString::Printf(TEXT("MI_Ch_%s"), CHA::PartNames[p]))));
	}
	// Until Scripts/chenghuai.py has made them: the museum's own nearest materials.
	const FString Stone = CHA::MaterialPath(Usd, TEXT("MI_stone_grey")), Plaster = CHA::MaterialPath(Materials, TEXT("M_Plaster"));
	const FString Timber = CHA::MaterialPath(Usd, TEXT("MI_timber_dark_r70")), Tile = CHA::MaterialPath(Usd, TEXT("MI_roof_tiles"));
	const FString Coping = CHA::MaterialPath(Usd, TEXT("MI_roof_coping")), Glass0 = CHA::MaterialPath(Materials, TEXT("M_Glass"));
	for (int32 p = 0; p < CB::PartCount; ++p)
	{
		FString F = Stone;
		if (p == CB::PlasterWhite || p == CB::PlasterRoom || p == CB::Paper) { F = Plaster; }
		else if (p >= CB::RedLacquer && p <= CB::LanternSilk) { F = Timber; }
		else if (p == CB::Tiles) { F = Tile; }
		else if (p == CB::RoofMortar || p == CB::Ridges) { F = Coping; }
		else if (p == CB::Glass) { F = Glass0; }
		Fallbacks.Add(CHA::Soft(F));
	}
	WaterMaterial = CHA::Soft(CHA::MaterialPath(Folder, TEXT("MI_Ch_Pond")));   // (Scripts/chenghuai.py: a still, green-brown Suzhou pond)
	DaylightParameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/MPC_Musee.MPC_Musee")));

	AddTags();
	PlaceLights();
}

TArray<FString> AChenghuaiStructure::GetReplacedImportPrims()
{
	// The Sculpture Hall's imported pieces (its court, passage, laylight, bench, plinths and the retired scans).
	return {TEXT("/Museum/SculptureHall")};
}

TArray<FString> AChenghuaiStructure::GetPartNames()
{
	TArray<FString> Out;
	for (int32 p = 0; p < CB::PartCount; ++p) { Out.Add(CHA::PartNames[p]); }
	return Out;
}

TArray<FVector2D> AChenghuaiStructure::GetPondOutline()
{
	const TArray<FVector2D> Edge = ChenghuaiWater::PondOutline();
	FVector2D C = FVector2D::ZeroVector;
	for (const FVector2D& P : Edge) { C += P; }
	C /= FMath::Max(1, Edge.Num());
	TArray<FVector2D> Out;
	for (const FVector2D& P : Edge) { Out.Add(P + (C - P).GetSafeNormal() * 0.12); }
	return Out;
}

bool AChenghuaiStructure::WriteGeometryObj(const FString& Path)
{
	const CB::FChMeshes Meshes = CB::BuildMeshes();
	FString Text;
	CHA::WriteObj(Meshes, Text);
	return FFileHelper::SaveStringToFile(Text, *Path);
}

void AChenghuaiStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
	PlaceLights();
	AddTags();
}

void AChenghuaiStructure::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	PlaceLights();
	AddTags();
}

void AChenghuaiStructure::BeginPlay()
{
	Super::BeginPlay();
	Build();
	ApplyMaterials();
	LastDaylight = -1.f;
}

void AChenghuaiStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) { return; }
	float Daylight = 0.f;
	if (UMaterialParameterCollection* Collection = DaylightParameters.LoadSynchronous())
	{
		Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, Collection, TEXT("Daylight"));
	}
	if (FMath::Abs(Daylight - LastDaylight) > 0.01f) { UpdateLights(Daylight); }
	UpdateCourtExposure(DeltaSeconds);
}

void AChenghuaiStructure::UpdateCourtExposure(float DeltaSeconds)
{
	if (!CourtExposure) { return; }
	float Target = 0.f, RoomTarget = 0.f;
	UWorld* World = GetWorld();
	APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(this, 0) : nullptr;
	if (Camera)
	{
		const FVector At = Camera->GetCameraLocation();
		const FVector Plan = At / MuseePlan::Cm;   // this actor stands at the origin in plan metres (x east, y south)
		// The site, and the open porch from the Rotunda's north door to the gate (a court too, under the sky).
		const bool bOnSite = (Plan.X > Chenghuai::SiteW && Plan.X < Chenghuai::SiteE && Plan.Y > Chenghuai::SiteN && Plan.Y < Chenghuai::SiteS - 0.3)
			|| (FMath::Abs(Plan.X) < Chenghuai::PorchWallOut && Plan.Y < -Chenghuai::DrumOuter - 0.3 && Plan.Y >= Chenghuai::SiteS - 0.3);
		// Indoors are the rooms (their plan rectangles inside the walls); the courts, the garden and the open structures
		// in them (the gate, the verandas and galleries, the walk, the pavilions) are outdoors, as a photographer would
		// expose there. (A roof trace alone put the walk and the pavilions indoors.)
		const double X = Plan.X, Y = Plan.Y;
		auto In = [X, Y](double X0, double X1, double Y0, double Y1) { return X > X0 && X < X1 && Y > Y0 && Y < Y1; };
		const double M = 2.0 * Chenghuai::Axis;   // the east side's mirror
		const bool bRoom = In(Chenghuai::InW, Chenghuai::FrontRowX1, Chenghuai::FrontRowFace, Chenghuai::InS)
			|| In(Chenghuai::InW, Chenghuai::SideHallFace, Chenghuai::SideHallN, Chenghuai::SideHallS)
			|| In(M - Chenghuai::SideHallFace, M - Chenghuai::InW, Chenghuai::SideHallN, Chenghuai::SideHallS)
			|| In(Chenghuai::HallX0, Chenghuai::HallX1, Chenghuai::HallBack, Chenghuai::HallFace)
			|| In(Chenghuai::EarX0, Chenghuai::EarX1, Chenghuai::EarN, Chenghuai::EarS)
			|| In(M - Chenghuai::EarX1, M - Chenghuai::EarX0, Chenghuai::EarN, Chenghuai::EarS)
			|| In(Chenghuai::InW, Chenghuai::HouseInE, Chenghuai::InN, Chenghuai::RearRowFace)
			|| In(Chenghuai::FlowerHallX0, Chenghuai::FlowerHallX1, Chenghuai::FlowerHallN, Chenghuai::FlowerHallS);
		RoomTarget = bOnSite && bRoom && Plan.Z > -1.5 ? 1.f : 0.f;
		if (bOnSite && !bRoom && Plan.Z > -1.5)
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ChenghuaiOutdoors), false);
			Params.AddIgnoredActor(Camera->GetViewTarget());
			FHitResult Down;
			const bool bOnWing = World->LineTraceSingleByChannel(Down, At, At - FVector(0.0, 0.0, 1000.0), ECC_Visibility, Params) && Down.GetActor() == this;
			Target = bOnWing ? 1.f : 0.f;
		}
	}
	if ((Target > 0.5f) != bWasOutdoor)
	{
		bWasOutdoor = Target > 0.5f;
		UE_LOG(LogMusee, Log, TEXT("Chenghuai: the courts' exposure %s."), bWasOutdoor ? TEXT("on (open sky)") : TEXT("off"));
	}
	OutdoorWeight = FMath::FInterpConstantTo(OutdoorWeight, Target, DeltaSeconds, 1.5f);
	CourtExposure->BlendWeight = OutdoorWeight;
	RoomWeight = FMath::FInterpConstantTo(RoomWeight, RoomTarget, DeltaSeconds, 1.5f);
	if (RoomExposure)
	{
		// By day the rooms are hushed under their paper windows; after dark they are lamplit galleries (the cases the
		// light), metered as the museum's rooms at night are.
		RoomExposure->Settings.AutoExposureBias = 0.9f + RoomExposureStops * FMath::Clamp((LastDaylight - 0.05f) / 0.3f, 0.f, 1.f);
		RoomExposure->BlendWeight = RoomWeight;
	}
}

void AChenghuaiStructure::AddTags()
{
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Chenghuai")));
	Tags.AddUnique(MuseeBake::BakeableTag());
}

void AChenghuaiStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }
	const CB::FChMeshes Meshes = CB::BuildMeshes();
	UProceduralMeshComponent* Components[6] = {Structure.Get(), Frame.Get(), Roof.Get(), Glass.Get(), Water.Get(), Guard.Get()};
	for (UProceduralMeshComponent* Component : Components)
	{
		if (Component) { Component->ClearAllMeshSections(); }
	}
	for (int32 p = 0; p < CB::PartCount; ++p)
	{
		const int32 C = CHA::ComponentOf(p);
		if (UProceduralMeshComponent* Component = Components[C]) { Meshes[p].M.Write(Component, CHA::SectionOf(p), C == 0 || C == 1 || C == 5); }
	}
	if (Water) { ChenghuaiWater::BuildWater().Write(Water, 0, false); }
}

void AChenghuaiStructure::ApplyMaterials()
{
	UProceduralMeshComponent* Components[6] = {Structure.Get(), Frame.Get(), Roof.Get(), Glass.Get(), Water.Get(), Guard.Get()};
	UMaterialInterface* Last = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Plaster.M_Plaster"))).LoadSynchronous();
	for (int32 p = 0; p < CB::PartCount; ++p)
	{
		UMaterialInterface* M = PartMaterials.IsValidIndex(p) ? PartMaterials[p].LoadSynchronous() : nullptr;
		if (!M && Fallbacks.IsValidIndex(p)) { M = Fallbacks[p].LoadSynchronous(); }
		if (!M) { M = Last; }
		const int32 C = CHA::ComponentOf(p);
		if (Components[C] && M) { Components[C]->SetMaterial(CHA::SectionOf(p), M); }
	}
	if (Water)
	{
		UMaterialInterface* W = WaterMaterial.LoadSynchronous();
		if (!W) { W = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/MI_Water_Garden.MI_Water_Garden"))).LoadSynchronous(); }
		if (W) { Water->SetMaterial(0, W); }
	}
}

void AChenghuaiStructure::PlaceLights()
{
	const TArray<CB::FChLight> L = CB::Lights();
	for (int32 i = 0; i < Lamps.Num() && i < L.Num(); ++i)
	{
		URectLightComponent* Light = Lamps[i];
		if (!Light) { continue; }
		const CB::FChLight& S = L[i];
		Light->SetRelativeLocationAndRotation(S.Centre * MuseePlan::Cm, FRotationMatrix::MakeFromXY(S.Facing, S.Along).Rotator());
		Light->SetSourceWidth(S.Width * MuseePlan::Cm);
		Light->SetSourceHeight(S.Height * MuseePlan::Cm);
		Light->SetBarnDoorAngle(S.bDaylit || S.bNight ? 88.f : 60.f);
		Light->SetBarnDoorLength(S.bDaylit || S.bNight ? 0.f : 3.f);
		Light->SetIntensityUnits(ELightUnits::Lumens);
		Light->SetUseTemperature(true);
		Light->SetTemperature(S.bDaylit ? WindowKelvin : (S.bNight ? 2500.f : LampKelvin));
		Light->SetLightColor(FLinearColor::White);
		Light->SetAttenuationRadius(S.bDaylit ? 900.f : 700.f);
		Light->SetCastShadows(S.bShadows);
		// No light shafts in the cases or the rooms (a fog elsewhere in the map scattered the case lamps into glowing cones).
		Light->SetVolumetricScatteringIntensity(0.f);
	}
	UpdateLights(0.6f);
}

void AChenghuaiStructure::UpdateLights(float Daylight)
{
	LastDaylight = Daylight;
	const TArray<CB::FChLight> L = CB::Lights();
	for (int32 i = 0; i < Lamps.Num() && i < L.Num(); ++i)
	{
		URectLightComponent* Light = Lamps[i];
		if (!Light) { continue; }
		const float Night = FMath::Clamp((0.25f - Daylight) / 0.2f, 0.f, 1.f);
		const float Level = L[i].bDaylit ? WindowScale * FMath::Clamp(Daylight, 0.f, 1.f) : (L[i].bNight ? LampScale * Night : LampScale);
		Light->SetIntensity(L[i].Lumens * Level);
		Light->SetVisibility(Level > 0.001f);
	}
}
