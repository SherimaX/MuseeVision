#include "LightSky.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sky/SkyPhotometry.h"
#include "TimeSubsystem.h"

namespace Musee::Light
{
	namespace
	{
		/** The Sun's effective temperature, 5772 K (IAU 2015 Resolution B3); moonlight is the same light returned. */
		constexpr float SunTemperatureKelvin = 5772.0f;

		/** The sun lights the atmosphere through civil twilight, until its centre is 6 degrees down. */
		constexpr double CivilTwilightDegrees = -6.0;

		/** The two lights the atmosphere takes: the sun first, the Moon second. */
		constexpr int32 SunAtmosphereIndex = 0;
		constexpr int32 MoonAtmosphereIndex = 1;

		/** Of the two directional lights, the sun lights translucency, water and fog: the higher priority wins. */
		constexpr int32 SunShadingPriority = 1;

		/** The sky light re-emits the sky it captures as captured, in the same physical units. */
		constexpr float CapturedSkyAsIs = 1.0f;

		/** The engine's simple volumetric cloud and its parameter for the fraction of the sky the cloud layer covers. */
		const TCHAR* const CloudMaterialPath = TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst");
		const TCHAR* const CloudCoverParameter = TEXT("Cloud_GlobalCoverage");

		/** The weather place over the museum itself (Decision D-005). */
		const TCHAR* const MuseumWeather = TEXT("museum");

		/** A light shining from a direction points the other way. */
		FRotator ShiningFrom(const FVector& TowardsBody)
		{
			return (-TowardsBody).Rotation();
		}
	}
}

ALightSky::ALightSky()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(RootComponent);

	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	SunLight->SetupAttachment(RootComponent);
	SunLight->SetMobility(EComponentMobility::Movable);
	SunLight->SetAtmosphereSunLight(true);
	SunLight->SetAtmosphereSunLightIndex(Musee::Light::SunAtmosphereIndex);
	SunLight->bUseTemperature = true;
	SunLight->Temperature = Musee::Light::SunTemperatureKelvin;
	SunLight->ForwardShadingPriority = Musee::Light::SunShadingPriority;

	MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	MoonLight->SetupAttachment(RootComponent);
	MoonLight->SetMobility(EComponentMobility::Movable);
	MoonLight->SetAtmosphereSunLight(true);
	MoonLight->SetAtmosphereSunLightIndex(Musee::Light::MoonAtmosphereIndex);
	MoonLight->bUseTemperature = true;
	MoonLight->Temperature = Musee::Light::SunTemperatureKelvin;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetRealTimeCapture(true);
	SkyLight->SetIntensity(Musee::Light::CapturedSkyAsIs);

	CloudLayer = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	CloudLayer->SetupAttachment(RootComponent);

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(RootComponent);
	HeightFog->SetVolumetricFog(true);
	HeightFog->SetFogDensity(0.0f);
}

void ALightSky::BeginPlay()
{
	Super::BeginPlay();
	if (UTimeSubsystem* Time = GetWorld()->GetSubsystem<UTimeSubsystem>())
	{
		Follow(Time->Now());
		MomentHandle = Time->OnMoment().AddUObject(this, &ALightSky::Follow);
	}
}

void ALightSky::EndPlay(EEndPlayReason::Type Reason)
{
	if (UTimeSubsystem* Time = GetWorld() ? GetWorld()->GetSubsystem<UTimeSubsystem>() : nullptr)
	{
		Time->OnMoment().Remove(MomentHandle);
	}
	Super::EndPlay(Reason);
}

void ALightSky::Follow(const FTimeMoment& Moment)
{
	FollowSun(Moment);
	FollowMoon(Moment);
	FollowWeather(Moment);
}

void ALightSky::FollowSun(const FTimeMoment& Moment)
{
	using namespace Musee::Light;
	SunLight->SetWorldRotation(ShiningFrom(Moment.SunDirection));
	// Above the atmosphere; the atmosphere takes its share on the way down to each surface.
	SunLight->SetIntensity(static_cast<float>(SkyPhotometry::SunIlluminance(Moment.SunDistanceAstronomicalUnits).Get()));
	SunLight->SetLightSourceAngle(static_cast<float>(SkyPhotometry::SunAngularDiameter(Moment.SunDistanceAstronomicalUnits).Get()));
	SunLight->SetVisibility(Moment.SunElevationDegrees > CivilTwilightDegrees);
}

void ALightSky::FollowMoon(const FTimeMoment& Moment)
{
	using namespace Musee::Light;
	MoonLight->SetWorldRotation(ShiningFrom(Moment.MoonDirection));
	MoonLight->SetIntensity(static_cast<float>(SkyPhotometry::MoonIlluminance(Musee::Degrees(Moment.MoonPhaseAngleDegrees), Moment.MoonDistanceKilometers).Get()));
	MoonLight->SetLightSourceAngle(static_cast<float>(SkyPhotometry::MoonAngularDiameter(Moment.MoonDistanceKilometers).Get()));
	MoonLight->SetVisibility(Moment.MoonElevationDegrees > 0.0);
}

void ALightSky::FollowWeather(const FTimeMoment& Moment)
{
	using namespace Musee::Light;
	const FTimeWeather* Weather = Moment.WeatherAt(MuseumWeather);
	if (!Weather) { return; }
	if (!CloudMaterial)
	{
		if (UMaterialInterface* Cloud = LoadObject<UMaterialInterface>(nullptr, CloudMaterialPath))
		{
			CloudMaterial = UMaterialInstanceDynamic::Create(Cloud, this);
			CloudLayer->SetMaterial(CloudMaterial);
		}
	}
	if (CloudMaterial)
	{
		CloudMaterial->SetScalarParameterValue(CloudCoverParameter, static_cast<float>(Weather->CloudCover));
	}
	const double Extinction = SkyPhotometry::PrecipitationExtinctionPerMeter(Weather->RainMillimetersPerHour, Weather->SnowfallMillimetersPerHour);
	HeightFog->SetFogDensity(SkyPhotometry::EngineFogDensity(Extinction));
}
