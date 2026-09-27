#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "LightSky.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UVolumetricCloudComponent;
struct FTimeMoment;

/**
 * The one sky over the museum: the sun and the Moon as directional lights, the atmosphere they shine through, the sky
 * light that carries the sky's own light indoors, the volumetric clouds and the height fog. It follows Time's moment and
 * nothing else, in real units: the sun's illuminance in lux is the luminous solar constant at the Earth's distance, the
 * atmosphere takes its share on the way down, the Moon's is sunlight returned by its phase. Light places exactly one in
 * every game world.
 */
UCLASS(NotPlaceable)
class MUSEELIGHT_API ALightSky final : public AActor
{
	GENERATED_BODY()

public:
	ALightSky();

	/** Sets the sky to a moment. */
	void Follow(const FTimeMoment& Moment);

	UDirectionalLightComponent* Sun() const { return SunLight; }
	UDirectionalLightComponent* Moon() const { return MoonLight; }
	UExponentialHeightFogComponent* Fog() const { return HeightFog; }
	UVolumetricCloudComponent* Clouds() const { return CloudLayer; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:
	void FollowSun(const FTimeMoment& Moment);
	void FollowMoon(const FTimeMoment& Moment);
	void FollowWeather(const FTimeMoment& Moment);

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<USkyAtmosphereComponent> Atmosphere;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<UDirectionalLightComponent> SunLight;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<UDirectionalLightComponent> MoonLight;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<UVolumetricCloudComponent> CloudLayer;

	UPROPERTY(VisibleAnywhere, Category = "Sky")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> CloudMaterial;

	FDelegateHandle MomentHandle;
};
