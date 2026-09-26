#include "Cube/ElanJourney.h"

#include "MuseeVision.h"
#include "Cube/CubePlan.h"
#include "Cube/CubeStructure.h"
#include "Cube/JourneyScene.h"
#include "Cube/JourneyMatterhorn.h"
#include "Cube/JourneySea.h"
#include "Elan/ElanElevator.h"
#include "Sky/MuseeSky.h"
#include "Visitor/MuseeCharacter.h"
#include "Visitor/MuseeWorld.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/LightComponentBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Info.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

#define LOCTEXT_NAMESPACE "Journey"

namespace
{
	FJourneyPlace P(const FText& Title, const FText& Line, double Lat, double Lon, double Utc, double Start, double End, double Alt, double Yaw,
					float MinEV, float MaxEV, float Bias = 0.f)
	{
		FJourneyPlace Out;
		Out.Title = Title;
		Out.Line = Line;
		Out.Latitude = Lat;
		Out.Longitude = Lon;
		Out.UtcOffsetHours = Utc;
		Out.StartHour = Start;
		Out.EndHour = End;
		Out.Altitude = Alt;
		Out.YawDegrees = Yaw;
		Out.MinEV = MinEV;
		Out.MaxEV = MaxEV;
		Out.ExposureBias = Bias;
		return Out;
	}

	FJourneyPlace Sea(const FJourneyPlace& In, double Depth)
	{
		FJourneyPlace Out = In;
		Out.bUnderwater = Depth > 0.0;
		Out.Depth = Depth;
		Out.WaterSurface = Depth;
		return Out;
	}

	// The Matterhorn: 14 July (Whymper's day, 1865), Central European Summer Time. The scenes turn so each place's
	// view lies north of the car, as on the board (the sun turns with them).
	constexpr double ZLat = 45.985, ZLon = 7.70;
	// Raja Ampat, Piaynemo: the same day, Eastern Indonesia Time.
	constexpr double RLat = -0.567, RLon = 130.27;
}

const TArray<FJourneyPlace>& AElanJourney::Places(EElanJourney Which)
{
	static TArray<FJourneyPlace> Mountain, Water, None;
	if (Mountain.IsEmpty())
	{
		Mountain = {
			P(LOCTEXT("M1", "I · Dawn · Riffelsee · 2,757 m"), LOCTEXT("M1L", "The mountain in the lake. Its summit turns pink before the rest; the first photograph of it was made on this shore, for John Ruskin, in 1849."),
			  ZLat, ZLon, 2, 5.25, 6.35, 2758.5, 95.8, -4.f, 15.f),
			P(LOCTEXT("M2", "II · Morning · Theodul Glacier · c. 3,000 m"), LOCTEXT("M2L", "Inside the glacier: blue ice a few metres either side, a slit of sky above, blue dark below."),
			  ZLat, ZLon, 2, 8.0, 9.5, 3041.3, 0.0, 2.f, 15.f),
			P(LOCTEXT("M3", "III · Late morning · Hörnli Hut · 3,260 m"), LOCTEXT("M3L", "The foot of the ridge. The Hörnli rises 1,200 m above you; find the rope teams strung along it."),
			  ZLat, ZLon, 2, 10.5, 12.0, 3259.9, 113.6, 6.f, 16.f),
			P(LOCTEXT("M4", "IV · Afternoon · Solvay Hut · 4,003 m"), LOCTEXT("M4L", "On the ridge. The east face falls away under your feet to the glacier; a fixed rope runs within reach."),
			  ZLat, ZLon, 2, 14.0, 16.5, 4003.1, 126.0, 6.f, 16.f),
			P(LOCTEXT("M5", "V · Sunset · The summit · 4,478 m"), LOCTEXT("M5L", "Above the clouds. Monte Rosa to the east, the Weisshorn to the north, Mont Blanc far to the west where the sun goes down. Whymper's party stood here first, on 14 July 1865."),
			  ZLat, ZLon, 2, 20.5, 21.6, 4479.2, 0.0, -2.f, 15.f),
			P(LOCTEXT("M6", "VI · Night · The summit · 4,478 m"), LOCTEXT("M6L", "Under the stars: the Milky Way overhead, Zermatt's lights 2,870 m below. Then it snows inside the Cube."),
			  ZLat, ZLon, 2, 22.5, 25.0, 4479.2, 0.0, -6.f, 8.f),
		};
		Water = {
			Sea(P(LOCTEXT("S1", "I · Morning · Piaynemo · the surface"), LOCTEXT("S1L", "Half in, half out: limestone islands and sky above, the reef below. Look down and you are under water; look up and you are not."),
				  RLat, RLon, 9, 7.0, 8.25, 0.0, 0.0, 4.f, 15.f), 0.0),
			Sea(P(LOCTEXT("S2", "II · Morning · the mangroves · 0–3 m"), LOCTEXT("S2L", "Among the roots: corals grow on them, one of the few places the two live together. Young blacktip reef sharks slip between them."),
				  RLat, RLon, 9, 8.5, 10.0, 0.0, 0.0, 3.f, 14.f), 1.2),
			Sea(P(LOCTEXT("S3", "III · Noon · Cape Kri · 5–25 m"), LOCTEXT("S3L", "The richest reef. In the current off Kri Island a shoal of jacks closes into a ring round the car and turns."),
				  RLat, RLon, 9, 11.5, 13.0, 0.0, 0.0, 3.f, 14.f), 12.0),
			Sea(P(LOCTEXT("S4", "IV · Afternoon · Manta Sandy · c. 15 m"), LOCTEXT("S4L", "Mantas, 3–4 m across, circle a coral mound where small wrasse clean them; they pass over the car, under it and beside it."),
				  RLat, RLon, 9, 14.5, 16.0, 0.0, 0.0, 2.f, 13.f), 15.0),
			Sea(P(LOCTEXT("S5", "V · Late afternoon · the Wall · 30–40 m"), LOCTEXT("S5L", "Into the blue. A wall of sea fans drops into the deep; on the fan beside you a pygmy seahorse 2 cm long; far out, a column of barracuda."),
				  RLat, RLon, 9, 16.5, 18.0, 0.0, 0.0, 0.f, 12.f), 34.0),
			Sea(P(LOCTEXT("S6", "VI · Night · the surface"), LOCTEXT("S6L", "The sea's own stars: stars over the islands, and below, the plankton flashing blue wherever the water moves. Then the lights go out one by one."),
				  RLat, RLon, 9, 19.5, 21.5, 0.0, 0.0, -6.f, 6.f), 0.0),
		};
		for (FJourneyPlace& Pl : Mountain) { Pl.Seconds = 120.0; }
		for (FJourneyPlace& Pl : Water) { Pl.Seconds = 120.0; }
	}
	return Which == EElanJourney::Matterhorn ? Mountain : Which == EElanJourney::Sea ? Water : None;
}

AElanJourney::AElanJourney()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	Post = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Post"));
	Post->SetupAttachment(RootComponent);
	Post->bUnbound = true;
	Post->Priority = 50.f;
	Post->bEnabled = false;
	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(RootComponent);
	Fog->SetVisibility(false);
	Tags.AddUnique(FName(TEXT("musee.nobake")));
}

AElanJourney* AElanJourney::Get(UWorld* World)
{
	if (!World) { return nullptr; }
	for (TActorIterator<AElanJourney> It(World); It; ++It) { return *It; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AElanJourney>(AElanJourney::StaticClass(), FTransform(CubePlan::WorldCentre()), Params);
}

void AElanJourney::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(CubePlan::WorldCentre());
	for (TActorIterator<ACubeStructure> It(GetWorld()); It; ++It) { Cube = *It; break; }
	for (TActorIterator<AMuseeSky> It(GetWorld()); It; ++It) { Sky = *It; break; }
	CubeParameters = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Museum/Journeys/Materials/MPC_Cube.MPC_Cube"));
	SetReveal(0.f);
}

void AElanJourney::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Scene) { Scene->Teardown(); }
	Super::EndPlay(Reason);
}

bool AElanJourney::IsAvailable(EElanJourney Which) const
{
	switch (Which)
	{
	case EElanJourney::Matterhorn: return GetDefault<UJourneyMatterhorn>()->IsAvailable();
	case EElanJourney::Sea: return GetDefault<UJourneySea>()->IsAvailable();
	case EElanJourney::Later: return true;
	default: return false;
	}
}

void AElanJourney::SetReveal(float Value)
{
	Reveal = Value;
	if (CubeParameters) { UKismetMaterialLibrary::SetScalarParameterValue(this, CubeParameters, TEXT("Reveal"), Value); }
}

void AElanJourney::SetMuseumHidden(bool bHide)
{
	if (bMuseumHidden == bHide) { return; }
	bMuseumHidden = bHide;
	UWorld* World = GetWorld();
	// Everything of the museum goes (the building, the grounds, the lawn, the gardens, their lamps, a fog), but the
	// car, its way, the sky and the Cube's panels; and comes back as it was.
	if (bHide)
	{
		HiddenActors.Reset();
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* A = *It;
			if (A == this || A->IsHidden() || A->IsA<AElanElevator>() || A->IsA<AMuseeSky>() || A->IsA<ACubeStructure>() || A->IsA<APawn>()
				|| A->IsA<AController>() || A->IsA<AHUD>() || A->IsA<APlayerCameraManager>() || A->IsA<AWorldSettings>())
			{
				continue;
			}
			bool bShows = false;
			for (UActorComponent* C : A->GetComponents())
			{
				if (C && (C->IsA<UPrimitiveComponent>() || C->IsA<ULightComponentBase>() || C->IsA<UExponentialHeightFogComponent>())) { bShows = true; break; }
			}
			if (!bShows) { continue; }
			A->SetActorHiddenInGame(true);
			HiddenActors.Add(A);
		}
	}
	else
	{
		for (const TWeakObjectPtr<AActor>& A : HiddenActors) { if (A.IsValid()) { A->SetActorHiddenInGame(false); } }
		HiddenActors.Reset();
		MuseeWorld::SetBuildingHidden(World, false);
	}
	for (TActorIterator<AElanElevator> It(World); It; ++It) { It->SetJourneyHidden(bHide); }
	if (ACubeStructure* C = Cube.Get()) { C->SetJourneyMode(bHide); }
	UE_LOG(LogMusee, Log, TEXT("Journey: the museum %s (%d actors)."), bHide ? TEXT("hidden") : TEXT("back"), HiddenActors.Num());
}

void AElanJourney::Begin(EElanJourney Which, int32 FirstPlace, float Fraction)
{
	if (IsActive()) { End(); ClosePlace(); if (Scene) { Scene->Teardown(); Scene = nullptr; } State = EState::Idle; SetMuseumHidden(false); }
	Journey = Which;
	BeganAt = GetWorld()->GetTimeSeconds();
	bFinished = false;
	bAdvance = false;
	if (Which == EElanJourney::Later || Which == EElanJourney::None || Places(Which).IsEmpty())
	{
		// The room at rest: its grid only.
		State = EState::Playing;
		Place = -1;
		return;
	}
	Scene = Which == EElanJourney::Matterhorn ? static_cast<UJourneyScene*>(NewObject<UJourneyMatterhorn>(this)) : NewObject<UJourneySea>(this);
	Scene->Setup(this);
	SetMuseumHidden(true);
	Post->bEnabled = true;
	OpenPlace(FMath::Clamp(FirstPlace, 0, Places(Which).Num() - 1));
	PlaceTime = FMath::Clamp<double>(Fraction, 0.0, 1.0) * Places(Which)[Place].Seconds;
	State = EState::Opening;
	UE_LOG(LogMusee, Log, TEXT("Journey: %s begins at place %d (%.0f%%)."), Which == EElanJourney::Matterhorn ? TEXT("the Matterhorn") : TEXT("Raja Ampat"), Place + 1, Fraction * 100);
}

void AElanJourney::End()
{
	if (State == EState::Idle || State == EState::Closing) { return; }
	if (Journey == EElanJourney::Later || Journey == EElanJourney::None || !Scene)
	{
		State = EState::Idle;
		Journey = EElanJourney::None;
		return;
	}
	State = EState::Closing;
}

void AElanJourney::NextPlace()
{
	if (State != EState::Playing || !Scene || Place < 0) { return; }
	const FJourneyPlace& Pl = Places(Journey)[Place];
	PlaceTime = FMath::Max(PlaceTime, Pl.Seconds - VeilSeconds);
	bAdvance = true;
}

void AElanJourney::OpenPlace(int32 Index)
{
	ClosePlace();
	Place = Index;
	PlaceTime = 0;
	const FJourneyPlace& Pl = Places(Journey)[Place];
	SetActorRotation(FRotator(0, Pl.YawDegrees, 0));
	if (Scene) { Scene->OpenPlace(Index); }
	if (AMuseeCharacter* V = Cast<AMuseeCharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) { V->SetMessage(Pl.Title); }
	ApplyPlace(0.f);
}

void AElanJourney::ClosePlace()
{
	if (Scene) { Scene->ClosePlace(); }
}

double AElanJourney::LocalHour() const
{
	if (Place < 0) { return 12.0; }
	const FJourneyPlace& Pl = Places(Journey)[Place];
	const double F = FMath::Clamp(PlaceTime / Pl.Seconds, 0.0, 1.0);
	return FMath::Lerp(Pl.StartHour, Pl.EndHour, F);
}

FDateTime AElanJourney::PlaceUtc() const
{
	const FJourneyPlace& Pl = Places(Journey)[FMath::Max(Place, 0)];
	const FDateTime Midnight(Pl.Year, Pl.Month, Pl.Day);
	return Midnight + FTimespan::FromHours(LocalHour() - Pl.UtcOffsetHours);
}

float AElanJourney::Veil() const
{
	if (Place < 0) { return 0.f; }
	const TArray<FJourneyPlace>& All = Places(Journey);
	const FJourneyPlace& Pl = All[Place];
	float V = 0.f;
	// Clearing at the start (not the first place: it opens with the panels), thickening at the end (the last place
	// too: the snow thickens to white before the car rises).
	if (Place > 0) { V = FMath::Max(V, 1.f - float(PlaceTime / VeilSeconds)); }
	V = FMath::Max(V, float((PlaceTime - (Pl.Seconds - VeilSeconds)) / VeilSeconds));
	return FMath::Clamp(V, 0.f, 1.f);
}

void AElanJourney::ApplyPlace(float DeltaSeconds)
{
	if (Place < 0 || !Scene) { return; }
	const FJourneyPlace& Pl = Places(Journey)[Place];
	const FVector Eyes = GetActorLocation();
	if (AMuseeSky* S = Sky.Get())
	{
		FMuseeSkyOverride O;
		O.Utc = PlaceUtc();
		O.Observer.Latitude = Pl.Latitude;
		O.Observer.Longitude = Pl.Longitude;
		O.YawDegrees = Pl.YawDegrees;
		O.PlanetTop = Eyes - FVector(0, 0, Pl.Altitude * 100.0);
		O.SunLux = 120000.f;
		if (Pl.bUnderwater || Pl.Depth > 0)
		{
			// What the sea leaves of the light at the eye's depth (clear tropical water, Jerlov I–II: Kd ≈ 0.35, 0.065, 0.028 /m
			// for red, green, blue): the surface is WaterSurface metres above the eyes.
			const double D = Pl.Depth;
			O.SunTint = FLinearColor(FMath::Exp(-0.35 * D), FMath::Exp(-0.065 * D), FMath::Exp(-0.028 * D));
			O.SkyTint = FLinearColor(0.35f, 0.75f, 1.0f) * O.SunTint;
			O.SkyLightScale = 0.6f;
			O.PlanetTop = Eyes + FVector(0, 0, Pl.WaterSurface * 100.0);
		}
		Scene->AdjustSky(O, Place);
		S->SetJourneySky(O);
	}
	// Exposure: the place's own range; the museum's compensation curve off.
	FPostProcessSettings& PP = Post->Settings;
	PP.bOverride_AutoExposureMinBrightness = true;
	PP.bOverride_AutoExposureMaxBrightness = true;
	PP.bOverride_AutoExposureBias = true;
	PP.bOverride_AutoExposureBiasCurve = true;
	PP.AutoExposureMinBrightness = Pl.MinEV;
	PP.AutoExposureMaxBrightness = Pl.MaxEV;
	PP.AutoExposureBias = Pl.ExposureBias;
	PP.AutoExposureBiasCurve = nullptr;
	const float F = float(FMath::Clamp(PlaceTime / Pl.Seconds, 0.0, 1.0));
	Scene->TickPlace(DeltaSeconds, LocalHour(), F, Veil());
}

void AElanJourney::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (State == EState::Idle || !Scene)
	{
		if (Reveal != 0.f) { SetReveal(0.f); }
		// The Cube at rest: a black room the eye adapts to, where only the panels' grid, the car's floor and its rail show.
		const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
		const FVector Eye = Camera ? (Camera->GetCameraLocation() - CubePlan::WorldCentre()) / 100.0 : FVector(1e9);
		const bool bInCube = FMath::Abs(Eye.X) < CubePlan::Half && FMath::Abs(Eye.Y) < CubePlan::Half && FMath::Abs(Eye.Z) < CubePlan::Half;
		Post->bEnabled = bInCube;
		if (bInCube)
		{
			FPostProcessSettings& PP = Post->Settings;
			PP.bOverride_AutoExposureMinBrightness = PP.bOverride_AutoExposureMaxBrightness = true;
			PP.bOverride_AutoExposureBias = PP.bOverride_AutoExposureBiasCurve = true;
			PP.AutoExposureMinBrightness = RestMinEV;
			PP.AutoExposureMaxBrightness = RestMaxEV;
			PP.AutoExposureBias = 0.f;
			PP.AutoExposureBiasCurve = nullptr;
		}
		return;
	}
	const TArray<FJourneyPlace>& All = Places(Journey);
	// The visitor gone from the Cube (a teleport, a test tour): the journey closes and the museum comes back.
	if (State != EState::Closing && GetWorld()->GetTimeSeconds() - BeganAt > 1.0)
	{
		const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
		const FVector Eye = Camera ? (Camera->GetCameraLocation() - CubePlan::WorldCentre()) / 100.0 : FVector::ZeroVector;
		if (FMath::Abs(Eye.X) > CubePlan::Half || FMath::Abs(Eye.Y) > CubePlan::Half || FMath::Abs(Eye.Z) > CubePlan::Half) { End(); }
	}
	if (State == EState::Opening)
	{
		SetReveal(FMath::Min(1.f, Reveal + DeltaSeconds / RevealSeconds));
		if (Reveal >= 1.f) { State = EState::Playing; }
	}
	else if (State == EState::Closing)
	{
		SetReveal(FMath::Max(0.f, Reveal - DeltaSeconds / RevealSeconds));
		if (Reveal <= 0.f)
		{
			ClosePlace();
			Scene->Teardown();
			Scene = nullptr;
			if (AMuseeSky* S = Sky.Get()) { S->ClearJourneySky(); }
			Post->bEnabled = false;
			Fog->SetVisibility(false);
			SetMuseumHidden(false);
			State = EState::Idle;
			Journey = EElanJourney::None;
			Place = -1;
			return;
		}
	}
	// The day goes on (not while closing: the last moment holds).
	if (State != EState::Closing)
	{
		PlaceTime += DeltaSeconds * TimeScale;
		if (PlaceTime >= All[Place].Seconds)
		{
			if (Place + 1 < All.Num()) { OpenPlace(Place + 1); }
			else { PlaceTime = All[Place].Seconds; bFinished = true; }
		}
	}
	ApplyPlace(DeltaSeconds);
}

// ---------------------------------------------------------------------------------------------
// Console: musee.Journey <1 Matterhorn | 2 Sea | 3 at rest | 0 end> [place 1…6] [fraction 0…1] [time scale]

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GJourneyCommand(
		TEXT("musee.Journey"),
		TEXT("musee.Journey <1 Matterhorn | 2 Raja Ampat | 3 the Cube at rest | 0 end> [place 1-6] [fraction 0-1] [time scale]: put the car at the Cube's centre with the visitor in it and play from there."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) { return; }
			// Parked (the Cube is a room at rest; its content is being redesigned): the journeys are not placed.
			static bool bParked = true;
			if (bParked) { UE_LOG(LogMusee, Warning, TEXT("musee.Journey: the Cube's journeys are parked.")); return; }
			AElanJourney* J = AElanJourney::Get(World);
			const int32 Which = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 1;
			if (!J) { return; }
			if (Which <= 0) { J->End(); return; }
			AElanElevator* Elevator = nullptr;
			for (TActorIterator<AElanElevator> It(World); It; ++It) { Elevator = *It; break; }
			AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
			if (Elevator) { Elevator->PlaceInCube(Visitor); }
			const EElanJourney Journey = Which == 1 ? EElanJourney::Matterhorn : Which == 2 ? EElanJourney::Sea : EElanJourney::Later;
			const int32 Place = Args.Num() > 1 ? FMath::Max(1, FCString::Atoi(*Args[1])) - 1 : 0;
			const float Fraction = Args.Num() > 2 ? FCString::Atof(*Args[2]) : 0.f;
			if (Args.Num() > 3) { J->TimeScale = FMath::Max(0.f, FCString::Atof(*Args[3])); }
			J->Begin(Journey, Place, Fraction);
			J->RevealSeconds = 0.5f;   // a test: straight in
		}));
}

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GCarAtCommand(
		TEXT("musee.CarAt"),
		TEXT("musee.CarAt <level m>: the Élan car stopped at that level of its way down to the Cube (0 … -23.6), the visitor in it (tests and tour views)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || Args.Num() < 1) { return; }
			for (TActorIterator<AElanElevator> It(World); It; ++It)
			{
				It->PlaceCarAt(FCString::Atod(*Args[0]), Cast<AMuseeCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)));
				break;
			}
		}));
}

#undef LOCTEXT_NAMESPACE
