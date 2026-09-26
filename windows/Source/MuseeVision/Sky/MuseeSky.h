#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sky/Ephemeris.h"
#include "MuseeSky.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialParameterCollection;

/**
 * Élan Cube journeys (Cube/ElanJourney): while a journey plays in the Cube, the sky is the place's own. Its real sun,
 * moon and stars for the place's latitude, longitude and moment; the whole sky turned about the vertical with the
 * scene (YawDegrees), so the sun keeps its bearing on the land; the atmosphere's ground put under the visitor at the
 * place's altitude (PlanetTop, world cm), so the air is as thin, the horizon as far and the haze as the mountain's.
 */
struct FMuseeSkyOverride
{
	FDateTime Utc;
	FMuseeObserver Observer;
	double YawDegrees = 0;
	FVector PlanetTop = FVector::ZeroVector;
	float SunLux = 120000.f;
	FLinearColor SunTint = FLinearColor::White;   // under water: what the sea leaves of the sun
	float SkyLightScale = 1.f;
	FLinearColor SkyTint = FLinearColor::White;
	bool bAtmosphere = true;
	UMaterialInterface* LightFunction = nullptr;  // under water: the caustics
};

/**
 * The sky over the museum (Shared/Wings/SkyDome.swift, Shared/Core/Sky.swift, Rotunda.updateSun).
 *
 * The sun is one directional light for the whole museum. As on the iPhone it follows the
 * Rotunda's idealised sun, 15° an hour from VI in the west through XII in the north to VI in the
 * east, always shining from the eye's bronze node onto the hour on the clock, so the clock keeps
 * local time. It is up from 06:00 to 18:00. The moon and the stars are the real ones for the
 * observer. Inside the Sphere there is no sun: the sky is all round, below your feet too.
 */
UCLASS()
class MUSEEVISION_API AMuseeSky : public AActor
{
	GENERATED_BODY()

public:
	AMuseeSky();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Inside the Sphere: sun off, stars and moon all round. */
	UFUNCTION(BlueprintCallable, Category = "Musee")
	void SetSphereMode(bool bInSphere);

	/** Élan Cube journeys: the place's sky (see FMuseeSkyOverride), updated every frame while set; Clear gives the museum's back. */
	void SetJourneySky(const FMuseeSkyOverride& Sky);
	void ClearJourneySky();
	bool HasJourneySky() const { return bJourneySky; }

	/** Where the visitor is on Earth, until the next Refresh (which sets the museum's city, MuseeClock). */
	UFUNCTION(BlueprintCallable, Category = "Musee")
	void SetObserver(double Latitude, double Longitude);

	/** Recompute the sun, moon and stars now. */
	UFUNCTION(BlueprintCallable, Category = "Musee")
	void Refresh();

	/** Local time the sky shows (the live clock unless musee.Hour is set). */
	FDateTime LocalNow() const;
	FDateTime UtcNow() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Musee")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Musee")
	TObjectPtr<UDirectionalLightComponent> MoonLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Musee")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Musee")
	TObjectPtr<USkyLightComponent> SkyLight;

	/** The Yale Bright Star Catalogue, at infinity: it follows the camera, only its rotation changes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Stars;

	/** Unlit, additive; emissive = vertex colour × a soft round sprite (made by Scripts/setup_project.py). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> StarMaterial;

	/** The Milky Way (NASA SVS, celestial coordinates) on the star field's sphere, section 1: unlit, additive. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> MilkyWayMaterial;

	/**
	 * "Daylight" (0 at night … 1 at noon) for the surfaces lit by the sky: the Salon's skylights and
	 * velarium, the laylights, the misty glass (made by Scripts/setup_project.py).
	 */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialParameterCollection> Parameters;

	/** Direct sun at the museum, in lux. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float SunLux = 75000.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float MoonLux = 0.3f;

	/** Radius of the star field around the camera (cm); well inside the far plane. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float StarRadius = 500000.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Musee")
	bool bSphereMode = false;

	FMuseeObserver Observer;

	/** The idealised sun's direction of travel for a local hour (towards the clock). */
	static FVector SunClockDirection(double LocalHour);

private:
	void BuildStars();
	void UpdateSun(const FDateTime& Local);
	void UpdateMoonAndStars(const FDateTime& Utc);
	/** The area lights under the laylights and the velarium (tag musee.laylight) follow the daylight. */
	void UpdateLaylights(double Daylight);

	struct FLaylight
	{
		TWeakObjectPtr<class ULocalLightComponent> Light;
		float Full = 0.f;        // intensity at full daylight
		float NightFloor = 0.25f;
	};
	TArray<FLaylight> Laylights;
	bool bLaylightsGathered = false;
	double LastLaylightDaylight = -1.0;

	/**
	 * Under the Élan (the Square, level −1, and its pit) the sky light fades out. The only way the sky gets down there
	 * is the car's shaft into 5, but Lumen's radiance cache carries the open sky over the corner rooms (outside the
	 * Atrium's footprint, under 1.5 m of slab and ground) through the slab, so the dark rooms read as lit from outside
	 * by day. The Atrium's roof lights and the sun still come down the shaft.
	 */
	void UpdateUnderground(float DeltaSeconds);
	float SkyLightFull = -1.f;   // the sky light's own intensity (as placed)
	float SkyLightScale = 1.f;

	float Timer = 0.f;
	int32 LastCity = -1;

	// Élan Cube journeys.
	void UpdateJourneySky();
	bool bJourneySky = false;
	FMuseeSkyOverride JourneySky;
	FLinearColor SkyLightColourAsPlaced = FLinearColor::White;   // the sky refreshes at once when the city changes (musee.City, C)
};
