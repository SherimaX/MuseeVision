#include "Elan/ElanElevator.h"

#include "MuseeVision.h"
#include "Elan/ElanCar.h"
#include "Cube/CubePlan.h"
#include "Cube/CubeStructure.h"
#include "Cube/CubeWay.h"
#include "Cube/ElanJourney.h"
#include "Cube/CubeRest.h"
#include "Cube/SeaLight.h"
#include "Elan/ElanKit.h"
#include "Elan/ElanSphereTop.h"
#include "Elan/ElanStructure.h"
#include "Geometry/MuseeBake.h"
#include "Plan/MuseePlan.h"
#include "Sky/MuseeSky.h"
#include "Visitor/MuseeCharacter.h"
#include "Visitor/MuseeWorld.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UObjectIterator.h"

#define LOCTEXT_NAMESPACE "Elan"

namespace
{
	namespace E = MuseePlan::Elan;
	constexpr double DoorSeconds = 1.5;
	constexpr double IrisSeconds = 4.0;     // an iris opening or closing: slow, a mechanism you watch (was 1.6)
	constexpr double LandingHeight = 3.0;   // the landing enclosures' glass, floor to head ring
	constexpr double DimSeconds = 2.5;      // the car's glass clearing at the Cube's centre (or coming back)
	// Below the Cube's ceiling the car sinks on its mast, slower.
	constexpr double CubeSpeed = 1.0, CubeAccel = 0.5;
	/** The car's floor may pass under this only with the Cube's ceiling iris open (its collar then reaches the blades). */
	constexpr double CeilingGate = CubePlan::Ceiling + 0.25;

	FVector2D PlanOf(const FVector& World) { return FVector2D(World.X / MuseePlan::Cm, World.Y / MuseePlan::Cm); }
	double FromAxis(const FVector& World) { return (PlanOf(World) - FVector2D(E::CentreX, E::CentreY)).Size(); }

	void Collect(AActor* Actor, TArray<TWeakObjectPtr<AActor>>& Out)
	{
		Out.Add(Actor);
		TArray<AActor*> Children;
		Actor->GetAttachedActors(Children);
		for (AActor* Child : Children) { Collect(Child, Out); }
	}

	float Toward(float Value, float Goal, float DeltaSeconds, double Seconds)
	{
		return FMath::FInterpConstantTo(Value, Goal, DeltaSeconds, static_cast<float>(1.0 / Seconds));
	}
}

AElanElevator::AElanElevator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;   // the car moves before the visitor riding it
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Y = GK::NeckStop;

	GlassCar = CreateDefaultSubobject<UElanCar>(TEXT("GlassCar"));
	GlassCar->SetupAttachment(RootComponent);
	AtriumLanding = CreateDefaultSubobject<UElanLanding>(TEXT("AtriumLanding"));
	AtriumLanding->SetupAttachment(RootComponent);
	CubeWay = CreateDefaultSubobject<UElanCubeWay>(TEXT("CubeWay"));
	CubeWay->SetupAttachment(RootComponent);
	Neck = CreateDefaultSubobject<UElanNeck>(TEXT("Neck"));
	Neck->SetupAttachment(RootComponent);
	Platform = CreateDefaultSubobject<UElanPlatform>(TEXT("Platform"));
	Platform->SetupAttachment(RootComponent);
	Platform->SetRelativeLocation(FVector(0, 0, GK::PlatformTop * MuseePlan::Cm));
	// They move, open and close, and are lit at run time: never baked (Geometry/MuseeBake.h; this actor
	// ticks, so the bake skips it anyway).
	MuseeBake::NoBake(Neck);
	MuseeBake::NoBake(Platform);
	MuseeBake::NoBake(GlassCar);
	MuseeBake::NoBake(AtriumLanding);
	MuseeBake::NoBake(CubeWay);

	Mast = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mast"));
	Mast->SetupAttachment(RootComponent);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded()) { Mast->SetStaticMesh(Cylinder.Object); }
	Mast->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mast->SetVisibility(false);
}

void AElanElevator::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(E::Centre());
	bLogState = FParse::Param(FCommandLine::Get(), TEXT("MuseeLogElevator"));
	const TSoftObjectPtr<UMaterialInterface> Gilt(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Gilt.M_Gilt")));
	if (UMaterialInterface* M = Gilt.LoadSynchronous()) { Mast->SetMaterial(0, M); }

	RetireImportedParts();
	GlassCar->Build();
	// The glass tube: at the Atrium up to a gilt collar 3 cm under the ring round the Sphere's opening
	// (reaching out under the ring). (Under the Atrium the Cube has no landing: the car never lands there.)
	AtriumLanding->Build(LandingHeight, GK::CollarTop(), GK::IrisStoreR);   // wide enough to hold the iris's blades
	CubeWay->Build();
	Neck->Build();
	Platform->Build();
	// The car waits in the neck, both its irises shut.
	Y = GK::NeckStop;
	GlassCar->Place(Y, 0.0);

	UWorld* World = GetWorld();
	for (TActorIterator<AElanStructure> It(World); It; ++It) { Structure = *It; break; }
	for (TActorIterator<AMuseeSky> It(World); It; ++It) { Sky = *It; break; }
	for (TActorIterator<ACubeStructure> It(World); It; ++It) { Cube = *It; break; }
	// Élan Cube: the journeys (AElanJourney) are parked: not placed, not offered. The Cube is a room at rest.
	Journey.Reset();
	if (AElanStructure* S = Structure.Get()) { S->SetIrisOpen(true); }   // retired: the neck seals the opening
	UE_LOG(LogMusee, Log, TEXT("Elevator: native glass car (%d colliders), the Atrium's landing (%d colliders), the neck, the platform, the Cube's iris and mast (%s); %d imported parts retired."),
		GlassCar->ColliderCount(), AtriumLanding->ColliderCount(), Cube.IsValid() ? TEXT("the Cube placed") : TEXT("no Cube in the map"), Retired.Num());
}

void AElanElevator::RetireImportedParts()
{
	UWorld* World = GetWorld();
	static const TCHAR* const PartTags[] = {
		TEXT("elevator_car"), TEXT("elevator_car_door"), TEXT("elevator_landing_door"), TEXT("elevator_car_glow"), TEXT("elevator_mast")};
	TArray<AActor*> Found;
	for (const TCHAR* PartTag : PartTags)
	{
		Found.Reset();
		MuseeWorld::FindParts(World, PartTag, Found);
		for (AActor* A : Found) { Collect(A, Retired); }   // with everything attached (the car's glass, frame, floor, rail…)
	}
	bool bSquareGlass = false;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		for (const FName& Tag : It->Tags)
		{
			const FString S = Tag.ToString();
			if (S.StartsWith(TEXT("prim:/Museum/Elan/Landing_screen"))) { Retired.Add(*It); }
			// The Square's glass floor over the strata pit lies 2 cm under the car's floor when the car is
			// down: drawn first, always, so the two never swap; and no diffuse, as the car's glass.
			else if (S.StartsWith(TEXT("prim:/Museum/Elan/")) && S.EndsWith(TEXT("Glass_floor")))
			{
				TArray<UPrimitiveComponent*> Prims;
				It->GetComponents(Prims);
				for (UPrimitiveComponent* P : Prims)
				{
					P->SetTranslucentSortPriority(-1);
					for (int32 m = 0; m < P->GetNumMaterials(); ++m)
					{
						if (UMaterialInterface* Base = P->GetMaterial(m))
						{
							UMaterialInstanceDynamic* Clear = UMaterialInstanceDynamic::Create(Base, P);
							Clear->SetVectorParameterValue(TEXT("Tint"), GK::LowIron);
							Clear->SetScalarParameterValue(TEXT("Diffuse"), 0.f);
							P->SetMaterial(m, Clear);
						}
					}
				}
				bSquareGlass = true;
			}
		}
	}
	HideRetired();
	if (!bSquareGlass) { UE_LOG(LogMusee, Log, TEXT("Elevator: the Square's glass floor was not found (its sort order is unchanged).")); }
}

void AElanElevator::HideRetired()
{
	for (const TWeakObjectPtr<AActor>& Weak : Retired)
	{
		if (AActor* A = Weak.Get())
		{
			A->SetActorHiddenInGame(true);
			A->SetActorEnableCollision(false);
		}
	}
}

AMuseeCharacter* AElanElevator::Visitor() const
{
	return Cast<AMuseeCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void AElanElevator::LinkVisitor(AMuseeCharacter* V_)
{
	if (!V_ || LinkedVisitor.Get() == V_) { return; }
	LinkedVisitor = V_;
	// The car moves first each frame; the visitor standing on its floor then follows it (its moving
	// base) and never sinks into it on the way up.
	if (UCharacterMovementComponent* Move = V_->GetCharacterMovement()) { Move->AddTickPrerequisiteActor(this); }
}

bool AElanElevator::VisitorInCar(const AMuseeCharacter* V_, double Tolerance) const
{
	if (!V_) { return false; }
	const FVector Feet = V_->FeetLocation();
	return FromAxis(Feet) < E::CarRadius && FMath::Abs(Feet.Z / MuseePlan::Cm - Y) < Tolerance;
}

bool AElanElevator::VisitorOnPlatform(const AMuseeCharacter* V_) const
{
	if (!V_ || VisitorInCar(V_, 0.6)) { return false; }
	return FMath::Abs(V_->FeetLocation().Z / MuseePlan::Cm - GK::PlatformTop) < 0.6;
}

bool AElanElevator::VisitorInDoorway(const AMuseeCharacter* V_) const
{
	if (!V_) { return false; }
	const FVector Feet = V_->FeetLocation();
	if (FMath::Abs(Feet.Z / MuseePlan::Cm - Y) > 0.5) { return false; }
	// West of the axis, within a body's width of the leaves' path (the car's at 2.15 m, the landing's
	// at 2.39 m, the doorway 1.8 m across).
	const FVector2D P = PlanOf(Feet) - FVector2D(E::CentreX, E::CentreY);
	const double D = P.Size();
	// On the platform, anywhere close round the car: its way's guard would close on them as it left.
	if (VisitorOnPlatform(V_) && D < 2.8) { return true; }
	return P.X < 0 && FMath::Abs(P.Y) < 1.25 && D > 1.85 && D < 2.8;
}

void AElanElevator::Go(double Level, const FText& Message)
{
	Target = Level;
	bHasTarget = true;
	if (AMuseeCharacter* V_ = Visitor()) { V_->SetMessage(Message); }
}

void AElanElevator::Call(AMuseeCharacter* V_)
{
	if (!V_) { return; }
	// From the platform at the Sphere's centre: back up out of the neck.
	if (VisitorOnPlatform(V_))
	{
		if (!At(E::TopFloor)) { Go(E::TopFloor, LOCTEXT("CalledUp", "The car rises out of the neck to meet you.")); }
		return;
	}
	if (FMath::Abs(V_->FeetLocation().Z / MuseePlan::Cm) > 0.5 || At(0)) { return; }
	Go(0, LOCTEXT("Called", "The car glows down through the misty glass. The wait is part of the visit."));
}

bool AElanElevator::CanInteract(const AMuseeCharacter* V_, const FHitResult& Hit) const
{
	return V_ && !At(0) && !(bHasTarget && Target == 0) && FMath::Abs(V_->FeetLocation().Z / MuseePlan::Cm) < 0.5;
}

void AElanElevator::Interact(AMuseeCharacter* V_, const FHitResult& Hit)
{
	Call(V_);
}

FText AElanElevator::InteractHint(const AMuseeCharacter* V_, const FHitResult& Hit) const
{
	return LOCTEXT("CallHint", "Call the car");
}

bool AElanElevator::JourneyAvailable(uint8 Which) const
{
	const AElanJourney* J = Journey.Get();
	return Cube.IsValid() && J && J->IsAvailable(static_cast<EElanJourney>(Which));
}

bool AElanElevator::AtCube() const
{
	return At(CubePlan::CarStop) && Dim >= 0.999f;
}

void AElanElevator::GoToCube(uint8 Which)
{
	if (!Cube.IsValid()) { return; }
	PendingJourney = Which;
	bCubeTold = false;
	static const FText Lines[] = {
		LOCTEXT("ToMatterhorn", "Down through the ground to the Cube: a day on the Matterhorn."),
		LOCTEXT("ToSea", "Down through the ground to the Cube: a day under the sea at Raja Ampat."),
		LOCTEXT("ToLater", "Down through the ground to the Cube. New art for it is to be planned: the room is at rest."),
		LOCTEXT("ToCube", "Down through the ground to the Cube: soil, clay, then chalk, then the room 22 m down."),
	};
	Go(CubePlan::CarStop, Lines[FMath::Min<uint8>(Which, 3)]);
}

void AElanElevator::GetPrompts(const AMuseeCharacter* V_, TArray<FMuseeActionPrompt>& Out) const
{
	if (!V_) { return; }
	const bool bMoving = bHasTarget;
	if (VisitorInCar(V_, 0.3))
	{
		if (bMoving) { return; }
		if (At(0))
		{
			if (Doors < 0.95f) { return; }
			// The first ride goes down to the earth before it goes up to the stars.
			if (bVisitedCube) { Out.Add({TEXT("elevator.sphere"), LOCTEXT("Up", "Up to the Sphere")}); }
			if (!Cube.IsValid()) { return; }
			const bool bMountain = JourneyAvailable(uint8(EElanJourney::Matterhorn)), bSea = JourneyAvailable(uint8(EElanJourney::Sea));
			if (bMountain) { Out.Add({TEXT("elevator.matterhorn"), LOCTEXT("DownMatterhorn", "Down: I · the Matterhorn")}); }
			if (bSea) { Out.Add({TEXT("elevator.sea"), LOCTEXT("DownSea", "Down: II · Raja Ampat, the sea")}); }
			if (bMountain || bSea) { Out.Add({TEXT("elevator.later"), LOCTEXT("DownLater", "Down: later, new art")}); }
			else { Out.Add({TEXT("elevator.cube"), LOCTEXT("DownCube", "Down to the Cube")}); }
		}
		else if (At(CubePlan::CarStop))
		{
			if (Dim < 0.999f) { return; }
			// The Cube's pieces (Cube/SeaLight.h): while one plays, only its end; at rest, the piece or the way up.
			const ACubeSeaLight* Piece = SeaLight.Get();
			if (Piece && Piece->IsPlaying())
			{
				Out.Add({TEXT("elevator.endpiece"), LOCTEXT("EndPiece", "End the piece")});
				return;
			}
			if (ACubeSeaLight::IsAvailable()) { Out.Add({TEXT("elevator.sealight"), LOCTEXT("SeaLight", "II · Sea Light, 8 min")}); }
			Out.Add({TEXT("elevator.atrium"), LOCTEXT("UpToAtrium", "Up to the Atrium")});
			Out.Add({TEXT("elevator.sphere"), LOCTEXT("UpToSphere", "Up to the Sphere")});
		}
		else if (At(E::TopFloor) && Doors > 0.95f) { Out.Add({TEXT("elevator.atrium"), LOCTEXT("DownAtrium", "Down to the Atrium")}); }
		return;
	}
	// On the platform at the Sphere's centre: send the car away, or call it back.
	if (VisitorOnPlatform(V_))
	{
		if (At(E::TopFloor) && Doors > 0.95f) { Out.Add({TEXT("elevator.away"), LOCTEXT("Away", "Send the car down")}); }
		else if (!At(E::TopFloor) && !(bMoving && Target >= E::TopFloor - 0.01)) { Out.Add({TEXT("elevator.call"), LOCTEXT("CallUp", "Call the car")}); }
		return;
	}
	// Near the car on the Atrium floor (the bronze ring and round it, outside the glass enclosure),
	// with no car here: call it.
	const double D = FromAxis(V_->FeetLocation());
	if (FMath::Abs(V_->FeetLocation().Z / MuseePlan::Cm) < 0.3 && D < 8.0 && D > 2.35 && !At(0) && !(bMoving && Target == 0))
	{
		Out.Add({TEXT("elevator.call"), LOCTEXT("Call", "Call the car")});
	}
}

void AElanElevator::RunPrompt(AMuseeCharacter* V_, FName Id)
{
	const bool bInCar = VisitorInCar(V_, 0.6);
	if (Id == TEXT("elevator.call")) { Call(V_); }
	else if (Id == TEXT("elevator.matterhorn") && At(0)) { GoToCube(uint8(EElanJourney::Matterhorn)); }
	else if (Id == TEXT("elevator.sea") && At(0)) { GoToCube(uint8(EElanJourney::Sea)); }
	else if (Id == TEXT("elevator.later") && At(0)) { GoToCube(uint8(EElanJourney::Later)); }
	else if ((Id == TEXT("elevator.cube") || Id == TEXT("elevator.square")) && At(0)) { GoToCube(uint8(EElanJourney::None)); }
	else if (Id == TEXT("elevator.next"))
	{
		if (AElanJourney* J = Journey.Get()) { J->NextPlace(); }
	}
	else if (Id == TEXT("elevator.sealight") && At(CubePlan::CarStop) && Dim >= 0.999f)
	{
		if (ACubeSeaLight* Piece = ACubeSeaLight::Get(GetWorld())) { SeaLight = Piece; Piece->TimeScale = 1.f; Piece->Begin(0.0); }
	}
	else if (Id == TEXT("elevator.endpiece"))
	{
		if (ACubeSeaLight* Piece = SeaLight.Get()) { Piece->End(); }
	}
	else if (Id == TEXT("elevator.atrium")) { Go(0, At(CubePlan::CarStop) ? LOCTEXT("CubeToAtrium", "Up through the ground to the Atrium.") : FText::GetEmpty()); }
	else if (Id == TEXT("elevator.sphere"))
	{
		Go(E::TopFloor, At(CubePlan::CarStop) ? LOCTEXT("CubeToSphere", "Up through the earth, then the Atrium, to the stars.")
											   : LOCTEXT("ToSphere", "…then up to the stars. A steady 1.5 metres a second."));
	}
	else if (Id == TEXT("elevator.away") && At(E::TopFloor) && !bInCar)
	{
		Go(GK::NeckStop, LOCTEXT("SentAway", "The car sinks back into the neck, and the Sphere is yours."));
	}
}

void AElanElevator::SetInSphere(bool bNow)
{
	if (bInSphere == bNow) { return; }
	bInSphere = bNow;
	// Inside the Sphere the building is gone; only the sky, the neck's mouth, the platform, the mast
	// and the car remain.
	MuseeWorld::SetBuildingHidden(GetWorld(), bNow);
	if (!bNow) { HideRetired(); }   // the building comes back; its old elevator parts stay retired
	AtriumLanding->SetVisibility(!bNow, true);
	CubeWay->SetShown(!bNow);
	Platform->SetLit(bNow);
	if (AMuseeSky* S = Sky.Get()) { S->SetSphereMode(bNow); }
}

void AElanElevator::PlaceInCube(AMuseeCharacter* V_)
{
	if (!Cube.IsValid()) { return; }
	bHasTarget = false;
	V = 0;
	Y = CubePlan::CarStop;
	Doors = DoorsTarget = 0.f;
	CeilingIris = CubePlan::IrisClampOpen;
	BottomIris = TopIris = PlatformIris = 0.f;
	Dim = 1.f;
	bVisitedCube = true;
	bCubeTold = true;
	PendingJourney = uint8(EElanJourney::None);
	GlassCar->Place(Y, 0.0);
	GlassCar->SetDoors(0.f);
	GlassCar->SetDim(1.f);
	CubeWay->Apply(Y, CeilingIris);
	if (V_)
	{
		const float HalfHeight = V_->GetSimpleCollisionHalfHeight();
		const FVector Feet = E::Centre(Y + GK::FloorTop) + FVector(0.6 * MuseePlan::Cm, 0, 0);
		V_->TeleportTo(Feet + FVector(0, 0, HalfHeight + 2), V_->GetActorRotation());
		bSphereSide = false;
	}
}

void AElanElevator::SetJourneyHidden(bool bHide)
{
	AtriumLanding->SetVisibility(!bHide, true);
	Neck->SetVisibility(!bHide, true);
	Platform->SetVisibility(!bHide, true);
	if (bHide) { Mast->SetVisibility(false); }
	if (!bHide) { HideRetired(); }
}

void AElanElevator::PlaceCarAt(double Level, AMuseeCharacter* V_)
{
	if (!Cube.IsValid() || Level > 0.0 || Level < CubePlan::CarStop) { return; }
	PlaceInCube(V_);
	const double Clamp = UElanCubeWay::ClampLevel();
	Y = Level;
	Dim = FMath::Abs(Level - CubePlan::CarStop) < 0.01 ? 1.f : 0.f;
	CeilingIris = Level < Clamp + 0.005 ? CubePlan::IrisClampOpen : (Level < CeilingGate + 0.3 ? 1.f : 0.f);
	GlassCar->Place(Y, 0.0);
	GlassCar->SetDim(Dim);
	CubeWay->Apply(Y, CeilingIris);
	if (V_)
	{
		const float HalfHeight = V_->GetSimpleCollisionHalfHeight();
		V_->TeleportTo(E::Centre(Y + GK::FloorTop) + FVector(0.6 * MuseePlan::Cm, 0, HalfHeight + 2), V_->GetActorRotation());
	}
}

double AElanElevator::NextLeg() const
{
	// Through the neck, stop in it first; past the Cube's clamp level, stop there first (the iris takes the mast's
	// head, or lets it go). Whichever comes first on the way.
	double Leg = Target;
	auto Crosses = [this](double Level) { return (Target > Level + 0.005 && Y < Level - 0.005) || (Target < Level - 0.005 && Y > Level + 0.005); };
	const double Clamp = UElanCubeWay::ClampLevel();
	for (const double Stop : {GK::NeckStop, Clamp})
	{
		if (Crosses(Stop) && FMath::Abs(Stop - Y) < FMath::Abs(Leg - Y)) { Leg = Stop; }
	}
	return Leg;
}

void AElanElevator::CutSunsUnderground(bool bUnder)
{
	if (bUnder == bSunsCut) { return; }
	bSunsCut = bUnder;
	if (bUnder)
	{
		SunsHeld.Reset();
		for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
		{
			UDirectionalLightComponent* L = *It;
			if (!L || L->GetWorld() != GetWorld() || !L->bAffectTranslucentLighting) { continue; }
			SunsHeld.Add({L, true});
			L->SetAffectTranslucentLighting(false);
		}
	}
	else
	{
		for (const TPair<TWeakObjectPtr<UDirectionalLightComponent>, bool>& Held : SunsHeld)
		{
			if (UDirectionalLightComponent* L = Held.Key.Get()) { L->SetAffectTranslucentLighting(Held.Value); }
		}
		SunsHeld.Reset();
	}
}

void AElanElevator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Élan Cube: with the eyes under the Atrium floor (the shaft, the Cube) no sun reaches any glass they see; forward-lit
	// glass is not shadowed through the ground, so the sun is kept off translucency there.
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector Eye = Camera->GetCameraLocation() / MuseePlan::Cm;
		const double Z = Eye.Z - GetActorLocation().Z / MuseePlan::Cm;
		const bool bUnder = Z < CubePlan::SlabBottom - 0.3 && FromAxis(Camera->GetCameraLocation()) < CubePlan::BoxOut + 1.0;
		CutSunsUnderground(bUnder);
	}
	AMuseeCharacter* V_ = Visitor();
	LinkVisitor(V_);
	const bool bInCar = VisitorInCar(V_, 0.6);
	const FVector Feet = V_ ? V_->FeetLocation() / MuseePlan::Cm : FVector::ZeroVector;
	const double Clamp = UElanCubeWay::ClampLevel();
	AElanJourney* J = Journey.Get();

	// Doors first: close before moving; but they wait while someone stands in the doorway.
	if (bHasTarget) { DoorsTarget = Doors > 0.f && VisitorInDoorway(V_) ? 1.f : 0.f; }
	if (Doors != DoorsTarget)
	{
		const float Step = DeltaSeconds / DoorSeconds;
		Doors = DoorsTarget > Doors ? FMath::Min(DoorsTarget, Doors + Step) : FMath::Max(DoorsTarget, Doors - Step);
	}

	// At the Cube's centre the glass clears; leaving, the journey closes first, then the glass comes back.
	const bool bAtCubeStop = FMath::Abs(Y - CubePlan::CarStop) < 0.005;
	const bool bLeavingCube = bAtCubeStop && bHasTarget && FMath::Abs(Target - CubePlan::CarStop) > 0.01;
	if (bLeavingCube && J && J->IsActive() && !J->IsBusy()) { J->End(); }
	// A piece playing holds the car (and leaving, it ends first).
	if (!SeaLight.IsValid()) { for (TActorIterator<ACubeSeaLight> It(GetWorld()); It; ++It) { SeaLight = *It; break; } }
	ACubeSeaLight* Piece = SeaLight.Get();
	if (bLeavingCube && Piece && Piece->IsPlaying()) { Piece->End(); }
	const bool bJourneyHolds = (J && J->IsActive()) || (Piece && Piece->IsPlaying());
	const float DimGoal = (bAtCubeStop && !bHasTarget) || (bLeavingCube && bJourneyHolds) ? 1.f : 0.f;
	Dim = Toward(Dim, DimGoal, DeltaSeconds, DimSeconds);
	if (ACubeStructure* Room = Cube.Get()) { Room->SetRest(bAtCubeStop ? Dim : 0.f); }   // Élan Cube: the eye at rest
	if (bAtCubeStop && !bHasTarget && Dim >= 0.999f && PendingJourney != uint8(EElanJourney::None))
	{
		// Arrived, the glass cleared: the chosen journey opens.
		const EElanJourney Which = static_cast<EElanJourney>(PendingJourney);
		PendingJourney = uint8(EElanJourney::None);
		if (J) { J->Begin(Which); }
	}
	// Its last place played: the car rises straight through the Atrium to the Sphere.
	if (J && J->IsFinished() && bAtCubeStop && !bHasTarget && bInCar)
	{
		Go(E::TopFloor, LOCTEXT("JourneyDone", "The car rises out of the earth, straight through the Atrium, to the Sphere and the real sky."));
	}

	// The irises. Each opens for the car's way and shuts behind it: the neck's lower one while the car is
	// below its stop, the upper one while it is above. The neck is an airlock: passing through, the car
	// stops between them, the way behind closes, then (with the visitor aboard, sealed in) the world
	// outside changes, and the way ahead opens.
	const bool bAtNeck = FMath::Abs(Y - GK::NeckStop) < 0.005;
	float BottomGoal = Y < GK::NeckStop - 0.005 ? 1.f : 0.f;
	float TopGoal = Y > GK::NeckStop + 0.005 ? 1.f : 0.f;
	bool bMayGo = Doors <= 0.001f && Dim <= 0.001f && !bJourneyHolds;
	if (bHasTarget && bAtNeck && FMath::Abs(Target - GK::NeckStop) > 0.01 && Doors <= 0.001f)
	{
		const bool bUp = Target > Y;
		const float Behind = bUp ? BottomIris : TopIris;
		if (Behind <= 0.001f)
		{
			if (bInCar) { bSphereSide = bUp; }
			(bUp ? TopGoal : BottomGoal) = 1.f;
		}
		bMayGo = bMayGo && (bUp ? TopIris : BottomIris) >= 0.999f;
	}
	// The Cube's ceiling iris: shut while the car is up; open ahead of it coming down; closed on the mast's head at the
	// clamp level and below (the car hangs from the mast); open again to let the head go on the way up.
	const bool bHeadingDown = bHasTarget && Target < Y - 0.005, bHeadingUp = bHasTarget && Target > Y + 0.005;
	float CeilingGoal = 0.f;
	if (Y < Clamp - 0.005) { CeilingGoal = CubePlan::IrisClampOpen; }
	else if (Y < Clamp + 0.005) { CeilingGoal = bHeadingUp ? 1.f : CubePlan::IrisClampOpen; }
	else if (Y < CeilingGate + 0.3) { CeilingGoal = 1.f; }
	else { CeilingGoal = bHeadingDown && Target < CubePlan::Ceiling ? 1.f : 0.f; }
	if (bHasTarget && FMath::Abs(Y - Clamp) < 0.005 && FMath::Abs(Target - Clamp) > 0.01)
	{
		bMayGo = bMayGo && (bHeadingUp ? CeilingIris >= 0.999f : FMath::Abs(CeilingIris - CubePlan::IrisClampOpen) < 0.002f);
	}
	// The platform's: open while the car is bound for the top or near it, shut once it has gone; never
	// opening under a visitor standing on it.
	const bool bOnPlatform = VisitorOnPlatform(V_);
	const bool bOnPlatformIris = bOnPlatform && FromAxis(V_->FeetLocation()) < GK::PlatformIrisR + 0.35;
	const bool bBoundUp = bHasTarget && Target >= E::TopFloor - 0.01 && Y > GK::NeckStop - 0.01;
	float PlatformGoal = Y > GK::PlatformGate - 0.05 || bBoundUp ? 1.f : 0.f;
	if (PlatformIris <= 0.001f && bOnPlatformIris) { PlatformGoal = 0.f; }
	BottomIris = Toward(BottomIris, BottomGoal, DeltaSeconds, IrisSeconds);
	TopIris = Toward(TopIris, TopGoal, DeltaSeconds, IrisSeconds);
	PlatformIris = Toward(PlatformIris, PlatformGoal, DeltaSeconds, IrisSeconds);
	CeilingIris = Toward(CeilingIris, CeilingGoal, DeltaSeconds, IrisSeconds);

	if (bHasTarget && bMayGo)
	{
		double Leg = NextLeg();
		if (Leg > Y && Leg > GK::PlatformGate && PlatformIris < 0.999f)
		{
			Leg = FMath::Max(Y, GK::PlatformGate);
			if (bOnPlatformIris && !bToldToStepOff && V_)
			{
				bToldToStepOff = true;
				V_->SetMessage(LOCTEXT("StepOff", "Step off the centre: the car is coming up."));
			}
		}
		// Coming down to the Cube: not through its ceiling until the iris is open.
		if (Leg < Y && Y >= CeilingGate - 0.005 && Leg < CeilingGate && CeilingIris < 0.999f) { Leg = CeilingGate; }
		const double Dist = Leg - Y;
		// On the mast (below the clamp level) the car goes slower.
		const bool bOnMast = FMath::Max(Y, Leg) <= Clamp + 0.005;
		const double Speed = bOnMast ? CubeSpeed : E::Speed, Accel = bOnMast ? CubeAccel : E::Accel;
		if (FMath::Abs(Dist) >= 0.005)
		{
			const double Dir = Dist > 0 ? 1.0 : -1.0;
			const double Brake = FMath::Sqrt(2 * Accel * FMath::Abs(Dist));
			V = FMath::Min(Speed, FMath::Min(Brake, FMath::Abs(V) + Accel * DeltaSeconds)) * Dir;
			double Step = V * DeltaSeconds;
			if (FMath::Abs(Step) >= FMath::Abs(Dist)) { Step = Dist; }
			Y += Step;
		}
		if (FMath::Abs(Leg - Y) < 0.005)
		{
			Y = Leg;
			V = 0;
			if (FMath::Abs(Leg - Target) < 0.005)
			{
				bHasTarget = false;
				bToldToStepOff = false;
				if (Target == CubePlan::CarStop) { bVisitedCube = true; }
				if (Target == 0 || Target == E::TopFloor) { DoorsTarget = 1.f; }
				if (V_ && bInCar)
				{
					if (Target == E::TopFloor) { V_->SetMessage(LOCTEXT("Centre", "The centre of the Sphere: the sky over you, now. Step out onto the platform.")); }
					else { V_->SetMessage(FText::GetEmpty()); }
				}
			}
		}
	}
	// Arrived, the glass cleared: a word on the room.
	if (V_ && bInCar && bAtCubeStop && !bHasTarget && Dim >= 0.999f && !bCubeArrivedTold)
	{
		bCubeArrivedTold = true;
		V_->SetMessage(LOCTEXT("InCube", "The Cube: 28 m each way, its six faces light-field panels, at rest. Its first piece: II · Sea Light."));
	}
	if (bHasTarget && !bAtCubeStop) { bCubeArrivedTold = false; }
	// On the way down: a word as the iris closes on the mast and the car passes into the Cube.
	if (V_ && bInCar && bHasTarget && Target == CubePlan::CarStop && !bCubeTold && Y < Clamp - 0.05)
	{
		bCubeTold = true;
		V_->SetMessage(LOCTEXT("IntoCube", "The iris has closed round the mast. The car sinks to the Cube's centre, 22 m under the Atrium floor."));
	}
	// Left behind at the Atrium: the car closes and goes home, into the neck.
	if (At(0) && Doors > 0.95f && !bInCar && V_ && (FromAxis(V_->FeetLocation()) > E::WaitRing + 0.5 || FMath::Abs(Feet.Z) > 0.5))
	{
		LeaveTimer += DeltaSeconds;
		if (LeaveTimer > 12.f) { Target = GK::NeckStop; bHasTarget = true; LeaveTimer = 0.f; }   // time to walk over
	}
	else { LeaveTimer = 0.f; }

	// Which side of the neck the visitor is on: out of the car, where they stand; in it, only the
	// airlock changes it.
	if (V_ && !bInCar) { bSphereSide = Feet.Z > GK::NeckTop; }

	if (bLogState)
	{
		LogTimer += DeltaSeconds;
		if (LogTimer >= 1.f)
		{
			LogTimer = 0.f;
			const UObject* Base = V_ ? V_->GetMovementBaseObject() : nullptr;
			const FString BaseName = !Base ? FString(TEXT("nothing")) : Base == GlassCar->FloorBody() ? FString(TEXT("the car floor")) : Base->GetName();
			UE_LOG(LogMusee, Log, TEXT("Elevator: y %.2f v %.2f target %s doors %.2f (to %.0f) irises below %.2f above %.2f platform %.2f ceiling %.2f dim %.2f; %s; visitor feet (%.2f, %.2f, %.2f), %.2f m from the axis, %s, on %s"),
				Y, V, bHasTarget ? *FString::Printf(TEXT("%.2f"), Target) : TEXT("-"), Doors, DoorsTarget, BottomIris, TopIris, PlatformIris, CeilingIris, Dim,
				bSphereSide ? TEXT("Sphere side") : TEXT("building side"), Feet.X, Feet.Y, Feet.Z,
				V_ ? FromAxis(V_->FeetLocation()) : 0.0, bInCar ? TEXT("in the car") : bOnPlatform ? TEXT("on the platform") : TEXT("outside"), *BaseName);
		}
	}

	// Apply: the car to its height, both sets of leaves (a landing's open only with the car there), the
	// irises, the platform's guard round the car's way, the canopy's glow, the Cube's iris and mast, the glass.
	GlassCar->Place(Y, bHasTarget ? V : 0.0);
	const float Open = Doors * Doors * (3 - 2 * Doors);
	GlassCar->SetDoors(Open);
	AtriumLanding->SetDoors(FMath::Abs(Y) < 0.01 ? Open : 0.f);
	Neck->SetIrises(BottomIris, TopIris);
	Platform->SetIris(PlatformIris);
	Platform->SetGuard(PlatformIris > 0.001f && !(At(E::TopFloor) && Doors > 0.95f));
	CubeWay->Apply(Y, CeilingIris);
	CubeWay->SetMastOnly(J && J->IsActive() && J->Current() != EElanJourney::Later);
	// Élan Cube: while a piece plays the mast's lights go out; the room at rest shows what reaches the earth (its muon
	// tracks, spawned the first time the car is down in the room).
	CubeWay->SetDark(Piece && Piece->IsPlaying());
	GlassCar->SetGlimmer(Piece && Piece->IsPlaying() ? 0.002f : 0.03f);   // (the piece's light is the sea's own)
	if (Y < CubePlan::Ceiling && !Rest.IsValid()) { Rest = ACubeRest::Get(GetWorld()); }
	GlassCar->SetDim(Dim * Dim * (3 - 2 * Dim));
	GlowAmount = FMath::FInterpConstantTo(GlowAmount, bHasTarget ? 1.f : 0.f, DeltaSeconds, 1.f);
	GlassCar->SetGlow(GlowAmount);
	// Clicking the Atrium's glass calls the car when there is something to call (from outside it).
	AtriumLanding->SetCallable(!At(0) && !(bHasTarget && Target == 0) && !bInCar);
	SetInSphere(bSphereSide);
	// The bronze mast rises out of the neck's mouth to the collar under the car's floor, while the car
	// is above it: the car climbs on it through the middle of the Sphere.
	const double MastBottom = GK::NeckTop - GK::FlangeDepth, MastTop = Y + GK::CollarBottom;
	const bool bMast = MastTop > GK::NeckTop + 0.05;
	Mast->SetVisibility(bMast);
	if (bMast)
	{
		const double MastHeight = MastTop - MastBottom;
		Mast->SetRelativeScale3D(FVector(0.4, 0.4, MastHeight));
		Mast->SetRelativeLocation(FVector(0, 0, (MastBottom + MastHeight / 2) * MuseePlan::Cm));
	}
}

#undef LOCTEXT_NAMESPACE
