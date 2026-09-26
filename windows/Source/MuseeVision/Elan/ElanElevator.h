#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Visitor/MuseeInteractable.h"
#include "ElanElevator.generated.h"

class ACubeStructure;
class AElanJourney;
class AElanStructure;
class AMuseeCharacter;
class AMuseeSky;
class UElanCar;
class UElanCubeWay;
class UElanLanding;
class UElanNeck;
class UElanPlatform;
class UStaticMeshComponent;

/**
 * The glass elevator (Shared/Wings/Elan.swift, Elevator and updateElevator). One round glass car on
 * the Atrium's axis joins the Cube (its eyes at the Cube's centre, −22 m), the Atrium (0) and the centre
 * of the Sphere (eyes at 22 m). It runs in real time at 1.5 m/s with soft starts and no skip; the first
 * ride from the Atrium goes down to the Cube before it goes up. When idle it waits just inside the Sphere,
 * over the opening; call it from the bronze ring.
 *
 * Down to the Cube (Cube/CubePlan.h, UElanCubeWay): the car runs down the glass shaft on its rails, the iris in
 * the Cube's ceiling opens ahead of it, and the car stops with the mast's head (riding on its crown) in the iris's
 * plane; the iris closes on the head, and the car sinks on the mast its crown feeds out, slower (1 m/s), until the
 * eyes are at the centre (about 25 s in all). It never lands: its glass clears to its floor and rail, in the room
 * at rest (the journeys, AElanJourney, are parked: not placed, not offered). Up again, the glass comes back, and the
 * way is the same in reverse: to the Atrium, or on through it to the Sphere.
 *
 * Between the building and the Sphere is the neck (UElanNeck), an airlock: the car stops in it, the
 * iris behind closes, the world changes unseen (the building hidden, the sky all round, or back), and
 * the iris ahead opens. At the centre the car stands in a round platform (UElanPlatform): step out,
 * send the car down (its way closes flush behind it), call it back, ride down.
 *
 * The car and the two landing enclosures are built natively (UElanCar, UElanLanding: all glass,
 * slim bronze rings, curved leaves that slide round the cylinder, explicit collision); the imported
 * car, doors, landing screens, glow and mast are retired at BeginPlay (hidden, no collision).
 */
UCLASS()
class MUSEEVISION_API AElanElevator : public AActor, public IMuseeInteractable, public IMuseePromptProvider
{
	GENERATED_BODY()

public:
	AElanElevator();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// IMuseeInteractable: click the car or the Atrium's glass (from the Atrium) to call it.
	virtual bool CanInteract(const AMuseeCharacter* Visitor, const FHitResult& Hit) const override;
	virtual void Interact(AMuseeCharacter* Visitor, const FHitResult& Hit) override;
	virtual FText InteractHint(const AMuseeCharacter* Visitor, const FHitResult& Hit) const override;

	// IMuseePromptProvider: the buttons in the car and on the bronze ring.
	virtual void GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const override;
	virtual void RunPrompt(AMuseeCharacter* Visitor, FName Id) override;

	/** The glass car: it moves; its frame is its floor's level on the axis. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UElanCar> GlassCar;

	/** The glass enclosure round the shaft at the Atrium (0). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UElanLanding> AtriumLanding;

	/** The iris in the Cube's ceiling and the mast the car hangs from below it. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UElanCubeWay> CubeWay;

	/** The airlock through the Sphere's opening, and the platform at the Sphere's centre. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UElanNeck> Neck;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UElanPlatform> Platform;

	/** The bronze mast in the Sphere, from the neck's mouth up to the car. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UStaticMeshComponent> Mast;

	/** Car floor height (m). */
	double Y;

	/** For tests (musee.Journey): the car at the Cube's centre at once, its glass cleared, the visitor in it. */
	void PlaceInCube(AMuseeCharacter* Visitor);
	/** For tests (musee.CarAt): the car stopped at a level (m) of its way below the Atrium, the visitor in it. */
	void PlaceCarAt(double Level, AMuseeCharacter* Visitor);
	/** The car is at rest at the Cube's centre with its glass cleared. */
	bool AtCube() const;
	/** A journey shows in the Cube: the landing, the neck, the platform and the Sphere's mast go with the building. */
	void SetJourneyHidden(bool bHidden);

private:
	/** Hide the imported car, doors, landing screens, glow and mast, and turn off their collision. */
	void RetireImportedParts();
	void HideRetired();
	/** The visitor rides the car: their movement ticks after it. */
	void LinkVisitor(AMuseeCharacter* Visitor);
	bool At(double Level) const { return !bHasTarget && FMath::Abs(Y - Level) < 0.01; }
	bool VisitorInCar(const AMuseeCharacter* Visitor, double Tolerance) const;
	/** Standing where the leaves close, at the car's level: the doors wait. */
	bool VisitorInDoorway(const AMuseeCharacter* Visitor) const;
	void Call(AMuseeCharacter* Visitor);
	/** On the platform at the Sphere's centre (out of the car). */
	bool VisitorOnPlatform(const AMuseeCharacter* Visitor) const;
	void Go(double Level, const FText& Message);
	void SetInSphere(bool bInSphere);
	AMuseeCharacter* Visitor() const;

	double V = 0;
	double Target = 0;
	bool bHasTarget = false;
	float Doors = 0.f;          // 0 closed … 1 open
	float DoorsTarget = 0.f;
	bool bVisitedCube = false;
	float CeilingIris = 0.f;    // the Cube's ceiling iris: 0 shut … CubePlan::IrisClampOpen round the mast … 1 open
	float Dim = 0.f;            // the car's glass: 0 there … 1 cleared (at the Cube's centre)
	uint8 PendingJourney = 3;   // EElanJourney, chosen in the car (3: none)
	bool bCubeTold = false;
	bool bCubeArrivedTold = false;
	float BottomIris = 0.f;     // the neck's irises and the platform's: 0 shut … 1 open
	float TopIris = 0.f;
	float PlatformIris = 0.f;
	bool bSphereSide = false;   // the visitor is on the Sphere's side of the neck
	bool bToldToStepOff = false;
	float GlowAmount = 0.f;     // the canopy a little brighter while it runs
	float LeaveTimer = 0.f;
	bool bInSphere = false;
	bool bLogState = false;    // -MuseeLogElevator: its state once a second
	float LogTimer = 0.f;

	TArray<TWeakObjectPtr<AActor>> Retired;             // the imported elevator parts, kept hidden
	TWeakObjectPtr<AMuseeCharacter> LinkedVisitor;
	TWeakObjectPtr<AElanStructure> Structure;
	TWeakObjectPtr<AMuseeSky> Sky;
	TWeakObjectPtr<ACubeStructure> Cube;
	TWeakObjectPtr<AElanJourney> Journey;
	/** Élan Cube: the world's directional lights, and whether each lit translucency, while the eyes are underground. */
	TArray<TPair<TWeakObjectPtr<class UDirectionalLightComponent>, bool>> SunsHeld;
	bool bSunsCut = false;
	void CutSunsUnderground(bool bUnder);
	TWeakObjectPtr<class ACubeSeaLight> SeaLight;   // the Cube's piece (Cube/SeaLight.h), once played
	TWeakObjectPtr<class ACubeRest> Rest;           // the Cube at rest (Cube/CubeRest.h): the muons' tracks

	/** Down to the Cube with a journey (EElanJourney) to open on arrival. */
	void GoToCube(uint8 Which);
	bool JourneyAvailable(uint8 Which) const;
	/** The car's next stop on the way to Target: the neck or the Cube's clamp level when it must pass them. */
	double NextLeg() const;
};
