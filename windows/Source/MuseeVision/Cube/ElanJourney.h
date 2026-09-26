#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sky/Ephemeris.h"
#include "ElanJourney.generated.h"

class ACubeStructure;
class AMuseeSky;
class UExponentialHeightFogComponent;
class UMaterialParameterCollection;
class UPostProcessComponent;
class UJourneyScene;

/** What the Cube shows (plan/boards/ElanCubeJourneys): chosen in the car before it goes down. */
UENUM()
enum class EElanJourney : uint8
{
	Matterhorn,   // I · a day on the mountain, dawn to night, six places
	Sea,          // II · Raja Ampat, a day under the sea, morning to night, six places
	Later,        // new art for the field: to be planned (the room at rest)
	None
};

/** One of a journey's six places: where, when (local hours, time-lapse from Start to End), and how it is lit. */
struct FJourneyPlace
{
	FText Title;           // "Dawn · Riffelsee · 2,757 m"
	FText Line;            // the board's line, shown as the place opens
	double Latitude = 0, Longitude = 0;
	double UtcOffsetHours = 0;
	int32 Year = 2026, Month = 7, Day = 14;
	double StartHour = 6, EndHour = 7;
	double Seconds = 120;  // how long the place lasts
	double Altitude = 0;   // the eye's height above the sea (m): the atmosphere's ground is set under it
	double YawDegrees = 0; // the scene's turn about the car (so its view lies north, as on the board); the sun turns with it
	float MinEV = 4.f, MaxEV = 15.f, ExposureBias = 0.f;
	bool bUnderwater = false;
	double WaterSurface = 0; // for the sea: the surface's height relative to the eye (m); below it, water
	double Depth = 0;        // the eye's depth under the surface (m), for the light's colour
};

/**
 * The director of the Cube's journeys (Cube/CubePlan.h; plan/boards/ElanCubeJourneys). The car never moves: the
 * world is rebuilt round it. Each journey is a day told in six places over about twelve minutes; between places a
 * cloud (the mountain) or a shoal of fish (the sea) fills the Cube and clears on the next.
 *
 * In Unreal the light field is the real scene: when a journey begins the panels dissolve (MPC_Cube Reveal, panel by
 * panel, from the car outward) and the place stands round the car on every side, below too; the museum above is
 * hidden (as in the Sphere) and the sky (AMuseeSky) shows the place's own sun, for its latitude and date, turned with
 * the scene. Each place's scene is made when it opens (UJourneyScene: meshes loaded as the journey starts, components
 * made and destroyed with the place) and its light, fog and exposure set by a post-process and a height fog of the
 * director's own.
 *
 * Console: musee.Journey <1|2|3> [place 1…6] [fraction 0…1] puts the car at the Cube's centre (the visitor in it) and
 * plays that journey from that place; musee.Journey 0 ends it.
 */
UCLASS()
class MUSEEVISION_API AElanJourney : public AActor
{
	GENERATED_BODY()

public:
	AElanJourney();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** The world's director (spawned if the map has none). */
	static AElanJourney* Get(UWorld* World);

	/** Whether a journey's scenes are built (its meshes and materials exist): the car offers only those. */
	bool IsAvailable(EElanJourney Which) const;

	/** Open a journey (the car is at the Cube's centre, its glass cleared). */
	void Begin(EElanJourney Which, int32 Place = 0, float Fraction = 0.f);
	/** Close it: the panels come back, the museum returns. */
	void End();
	/** On to the next place (through the cloud or the shoal). */
	void NextPlace();

	/** A journey is showing, opening or closing. */
	bool IsActive() const { return State != EState::Idle; }
	/** It is closing, or already closed: the car may move once this is false and IsActive is false. */
	bool IsBusy() const { return State == EState::Opening || State == EState::Closing; }
	/** It has played its last place: the car should rise (to the Sphere). */
	bool IsFinished() const { return bFinished; }
	EElanJourney Current() const { return Journey; }
	int32 CurrentPlace() const { return Place; }

	static const TArray<FJourneyPlace>& Places(EElanJourney Which);

	/** MPC_Cube: the panels' reveal, the ice's glow, the water's level… (the scenes set their own). */
	UMaterialParameterCollection* Parameters() const { return CubeParameters; }

	/** Seconds a journey's reveal (the panels dissolving) and its close take. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float RevealSeconds = 5.f;

	/** Seconds of each place given to the cloud or shoal at either end. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float VeilSeconds = 6.f;

	/** The Cube at rest: the exposure's range while the eye is in it (a dark-adapted eye). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float RestMinEV = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float RestMaxEV = 6.f;

	/** Speed-up for tests (1 = the real twelve minutes). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float TimeScale = 1.f;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UPostProcessComponent> Post;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UExponentialHeightFogComponent> Fog;

private:
	enum class EState : uint8 { Idle, Opening, Playing, Closing };

	void OpenPlace(int32 Index);
	void ClosePlace();
	void ApplyPlace(float DeltaSeconds);
	void SetMuseumHidden(bool bHidden);
	void SetReveal(float Reveal);
	/** The place's local time now (hours), from its start and end by how far through it we are. */
	double LocalHour() const;
	FDateTime PlaceUtc() const;
	/** 0 … 1 … 0: how thick the cloud or the shoal is (at the start and end of a place). */
	float Veil() const;

	EState State = EState::Idle;
	EElanJourney Journey = EElanJourney::None;
	int32 Place = -1;
	double PlaceTime = 0;     // seconds into the place (scaled)
	double BeganAt = -1e9;    // world seconds at Begin (the camera follows the car a frame late)
	float Reveal = 0.f;       // 0 the panels … 1 the scene
	bool bFinished = false;
	bool bMuseumHidden = false;
	bool bAdvance = false;    // NextPlace asked: run the veil in now

	UPROPERTY(Transient) TObjectPtr<UJourneyScene> Scene;
	UPROPERTY(Transient) TObjectPtr<UMaterialParameterCollection> CubeParameters;
	TArray<TWeakObjectPtr<AActor>> HiddenActors;
	TWeakObjectPtr<ACubeStructure> Cube;
	TWeakObjectPtr<AMuseeSky> Sky;
};
