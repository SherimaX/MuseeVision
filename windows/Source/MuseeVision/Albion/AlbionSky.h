#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlbionSky.generated.h"

class UDirectionalLightComponent;
class ULocalFogVolumeComponent;
class UMaterialInterface;
class UMaterialParameterCollection;
class IHttpRequest;

/**
 * Albion's sky (the Main and Outside boards: "London keeps the weather", "the sun at the end"). The sun's place stays the
 * museum's (AMuseeSky: your hour, 15° an hour); the weather over Albion is London's now: Open-Meteo's current conditions
 * for Westminster (51.5072 N, 0.1276 W; free, no key, no account), fetched every 15 minutes, eased in, and kept in
 * Saved/Albion/london.json so an offline start keeps the last sky it heard (if it is under a day old) or a plain London
 * one (broken cloud, dry). As ASalonSky does for Giverny.
 *
 * What it drives:
 * - MPC_Albion (Scripts/albion_materials.py): Rain (beads and rivulets on the glass), Wet (the glass stays wet a while
 *   after), Snow (lying on the panes that face up), Fog (the vaults), Sun (0 when London's cloud hides it) and SunDir;
 * - fog in the vaults: three local fog volumes above the iron springs (10 m), so the paintings stay clear below;
 * - the sun through the stained glass: the museum's sun gets MI_Albion_SunLF_White as its light function (the white
 *   share: everything but the rays that pass through the Tristram band and the lancets, and none on Albion under cloud),
 *   and three directional lights of this actor's, pure red, green and blue, follow it, each with its channel of the glass
 *   (MI_Albion_SunLF_R, G, B): together they lay the glass's colours on the floor, shadowed by the ray-traced iron. They
 *   burn only while the visitor is in or near Albion and London's sun is out (they cost three shadowed lights).
 *
 * musee.AlbionWeather: −1 live (the default), 0 bright, 1 broken cloud, 2 rain, 3 fog, 4 snow, 5 overcast.
 * Placed at the origin by Scripts/native.py (musee.nobake: no geometry).
 */
UCLASS()
class MUSEEVISION_API AAlbionSky : public AActor
{
	GENERATED_BODY()

public:
	AAlbionSky();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UDirectionalLightComponent> Red;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UDirectionalLightComponent> Green;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UDirectionalLightComponent> Blue;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<ULocalFogVolumeComponent>> Fogs;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialParameterCollection> Parameters;

	/** The light functions: the white sun's, and the three colours'. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TArray<TSoftObjectPtr<UMaterialInterface>> LightFunctions;

	/** How fast the sky follows London (seconds to settle); the glass dries more slowly. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float EaseSeconds = 25.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float DrySeconds = 900.f;

	/** The fog's density in the vaults at Fog 1 (the volumes' radial extinction). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float FogDensity = 0.9f;

	/** The eased sky (for the checks and the log). */
	UPROPERTY(VisibleInstanceOnly, Category = "Musee") float Cloud = 0.4f;
	UPROPERTY(VisibleInstanceOnly, Category = "Musee") float Rain = 0.f;
	UPROPERTY(VisibleInstanceOnly, Category = "Musee") float Wet = 0.f;
	UPROPERTY(VisibleInstanceOnly, Category = "Musee") float Snow = 0.f;
	UPROPERTY(VisibleInstanceOnly, Category = "Musee") float Fog = 0.f;
	UPROPERTY(VisibleInstanceOnly, Category = "Musee") float SunOn = 1.f;

private:
	void Fetch();
	void OnWeather(const FString& Body, bool bOk, bool bFromCache);
	void Apply(float DeltaSeconds);
	void ApplySun();
	FString CachePath() const;

	// London, as last heard (targets for the easing).
	float LiveCloud = 0.55f, LiveRain = 0.f, LiveSnow = 0.f, LiveFog = 0.f;
	bool bHeard = false;
	double NextFetch = 0.0;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request;
	TWeakObjectPtr<UDirectionalLightComponent> MuseeSun;
	bool bOwnLightFunction = false;
	float LogTimer = 0.f;
	double CloudClock = 0.0;
};
