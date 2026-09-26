#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SalonSky.generated.h"

class URectLightComponent;
class UAudioComponent;
class USalonRainWave;
class UMaterialParameterCollection;
class IHttpRequest;

/**
 * The Salon's sky (the salon-interior proposal, the Light board: "five lanterns, five lights … all of them take their sky
 * from Giverny, live"). The sun's place stays the museum's (AMuseeSky: your hour); the cloud and the rain come from
 * Giverny now: Open-Meteo's current weather for Monet's garden (49.0757 N, 1.5337 E; free, no key, no account), fetched
 * every 15 minutes and eased in. Without a connection it keeps a fair sky and tries again later.
 *
 * What it drives:
 * - MPC_Salon (Scripts/salon_interior.py): Cloud (0 clear … 1 overcast), Rain (0 … 1), the cloud's drift (CloudDriftX, Y,
 *   metres a second): the sky panels in the clear lanterns' shafts (the cloud you see, and its shadow on the floor), the
 *   rain on the lanterns' glass, the velarium's light;
 * - the light the diffusers pass into the room (bay 2's opal glass, bay 4's muslin) and the velarium's, as the daylight
 *   over them: the sun on the roof (its height, through what cloud there is) and the sky; a rect light the size of each;
 * - the sound of the rain on the glass (USalonRainWave, made as it plays): over each lantern and over the velarium.
 *
 * musee.SalonWeather: −1 live (the default), 0 clear, 1 broken cloud, 2 overcast, 3 rain (for the tour's views).
 * Placed at the origin by Scripts/native.py; it has no geometry (musee.nobake).
 */
UCLASS()
class MUSEEVISION_API ASalonSky : public AActor
{
	GENERATED_BODY()

public:
	ASalonSky();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<URectLightComponent> OpalLight;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<URectLightComponent> MuslinLight;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<URectLightComponent> VelariumLight;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialParameterCollection> Parameters;

	/** The rain's loudness at its heaviest (0 … 1). */
	UPROPERTY(EditAnywhere, Category = "Musee|Sound")
	float RainVolume = 0.55f;

	/** The museum's MPC (AMuseeSky writes Daylight there). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialParameterCollection> MuseeParameters;

	/** Direct sun and clear sky on the roof at noon (lux): AMuseeSky's sun, and a clear sky's diffuse share. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float SunLux = 75000.f;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float ClearSkyLux = 16000.f;

	/** An overcast sky's illuminance on the roof at noon (lux). Rain darkens it further (RainDim). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float OvercastLux = 11000.f;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float RainDim = 0.55f;

	/** Diffuse transmittance: flashed opal glass, a single layer of muslin, the velarium's cloth under its glass roof. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float OpalTransmittance = 0.30f;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float MuslinTransmittance = 0.40f;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float VelariumTransmittance = 0.03f;   // the light it passes on; its own glow (M_Salon_Velarium, τ 0.12) reaches the room through Lumen too

	/** At night the rooms keep a little electric light behind the diffusers (a share of a dull day's). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float NightFloorLux = 900.f;

	/** How fast the sky follows the weather (seconds to settle). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float EaseSeconds = 25.f;

	/** The current (eased) sky, for the checks and the log. */
	UPROPERTY(VisibleInstanceOnly, Category = "Musee")
	float Cloud = 0.25f;

	UPROPERTY(VisibleInstanceOnly, Category = "Musee")
	float Rain = 0.f;

private:
	void Fetch();
	void OnWeather(const FString& Body, bool bOk);
	void Apply(float DeltaSeconds);
	/** The sun's height above the horizon (sine), from the museum's sun; 0 at night. */
	double SunHeight() const;
	double Daylight() const;

	// Giverny, as last heard (targets for the easing).
	float LiveCloud = 0.25f;
	float LiveRain = 0.f;
	float LiveWindSpeed = 3.f;     // m/s
	float LiveWindFrom = 240.f;    // degrees, where the wind blows from
	bool bHeard = false;
	double NextFetch = 0.0;
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request;
	float LogTimer = 0.f;
	int32 LastMode = -2;

	// The rain on the glass: one voice over each lantern and one over the velarium.
	void StartRainSound();
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> RainAudio;
	UPROPERTY(Transient)
	TArray<TObjectPtr<USalonRainWave>> RainWaves;
};
