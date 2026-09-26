#include "Salon/SalonSky.h"

#include "Salon/SalonRain.h"
#include "Salon/SalonStructure.h"
#include "Components/AudioComponent.h"
#include "Sky/MuseeSky.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogSalonSky, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarSalonWeather(
		TEXT("musee.SalonWeather"), -1,
		TEXT("The Salon's sky: -1 live from Giverny (Open-Meteo), 0 clear, 1 broken cloud, 2 overcast, 3 rain."),
		ECVF_Default);

	// Monet's garden at Giverny.
	constexpr double GivernyLatitude = 49.0757, GivernyLongitude = 1.5337;
	constexpr double FetchEverySeconds = 15.0 * 60.0, RetrySeconds = 5.0 * 60.0;

	/** A rect light facing down at plan (x, y, z) metres, W × H metres. */
	URectLightComponent* MakeLight(AActor* Owner, const TCHAR* Name, USceneComponent* Root)
	{
		URectLightComponent* L = Owner->CreateDefaultSubobject<URectLightComponent>(Name);
		L->SetupAttachment(Root);
		L->SetMobility(EComponentMobility::Movable);
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(0.f);
		L->SetCastShadows(true);
		L->bUseTemperature = true;
		L->SetBarnDoorAngle(88.f);
		L->SetBarnDoorLength(0.f);
		return L;
	}

	void Place(URectLightComponent* L, double X, double Y, double Z, double W, double H)
	{
		// A rect light emits along its +X: pointed down.
		L->SetRelativeLocation(FVector(X, Y, Z) * 100.0);
		L->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
		L->SetSourceWidth(float(W * 100.0));
		L->SetSourceHeight(float(H * 100.0));
		L->SetAttenuationRadius(2400.f);
	}
}

ASalonSky::ASalonSky()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);

	OpalLight = MakeLight(this, TEXT("OpalLight"), RootComponent);
	MuslinLight = MakeLight(this, TEXT("MuslinLight"), RootComponent);
	VelariumLight = MakeLight(this, TEXT("VelariumLight"), RootComponent);
	// Daylight through a diffuser: the sky's colour and the sun's, about 5600 K; the muslin a breath warmer (unbleached).
	OpalLight->SetTemperature(5900.f);
	MuslinLight->SetTemperature(5000.f);
	VelariumLight->SetTemperature(6200.f);

	double X0, Y0, X1, Y1, GZ;
	ASalonStructure::LanternRect(1, X0, Y0, X1, Y1, GZ);
	Place(OpalLight, 0.5 * (X0 + X1), 0.5 * (Y0 + Y1), ASalonStructure::DiffuserZ(1) - 0.02, X1 - X0, Y1 - Y0);
	ASalonStructure::LanternRect(3, X0, Y0, X1, Y1, GZ);
	Place(MuslinLight, 0.5 * (X0 + X1), 0.5 * (Y0 + Y1), ASalonStructure::DiffuserZ(3) - 0.02, X1 - X0, Y1 - Y0);
	double CX, HX, HY, VZ;
	ASalonStructure::VelariumRect(CX, HX, HY, VZ);
	// The velarium's light: an area a little inside its ellipse (π/4 of the box), just under it.
	Place(VelariumLight, CX, 0.0, VZ - 0.05, 2.0 * HX * 0.886, 2.0 * HY * 0.886);
	VelariumLight->SetAttenuationRadius(1800.f);

	Parameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/Salon/MPC_Salon.MPC_Salon")));
	MuseeParameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/MPC_Musee.MPC_Musee")));
	Tags.AddUnique(FName(TEXT("musee.nobake")));
}

void ASalonSky::BeginPlay()
{
	Super::BeginPlay();
	StartRainSound();
	NextFetch = 0.0;
	Apply(1000.f);   // settle at once
}

void ASalonSky::StartRainSound()
{
	TArray<FVector> Where;
	for (int32 Bay = 0; Bay < 5; ++Bay)
	{
		double X0, Y0, X1, Y1, GZ;
		ASalonStructure::LanternRect(Bay, X0, Y0, X1, Y1, GZ);
		Where.Add(FVector(0.5 * (X0 + X1), 0.5 * (Y0 + Y1), GZ + 0.1));
	}
	double CX, HX, HY, VZ;
	ASalonStructure::VelariumRect(CX, HX, HY, VZ);
	Where.Add(FVector(CX, 0.0, VZ + 0.6));
	for (const FVector& P : Where)
	{
		USalonRainWave* Wave = NewObject<USalonRainWave>(this);
		UAudioComponent* Audio = NewObject<UAudioComponent>(this);
		Audio->SetupAttachment(RootComponent);
		Audio->bAutoActivate = false;
		Audio->bAllowSpatialization = true;
		Audio->bOverrideAttenuation = true;
		Audio->AttenuationOverrides.bAttenuate = true;
		Audio->AttenuationOverrides.bSpatialize = true;
		Audio->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
		Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(250.f, 0.f, 0.f);   // the pane's own size
		Audio->AttenuationOverrides.FalloffDistance = 2200.f;                             // heard across a bay
		Audio->SetSound(Wave);
		Audio->RegisterComponent();
		Audio->SetRelativeLocation(P * 100.0);
		RainAudio.Add(Audio);
		RainWaves.Add(Wave);
	}
}

void ASalonSky::EndPlay(const EEndPlayReason::Type Reason)
{
	for (UAudioComponent* Audio : RainAudio)
	{
		if (Audio) { Audio->Stop(); }
	}
	if (Request.IsValid())
	{
		Request->OnProcessRequestComplete().Unbind();
		Request->CancelRequest();
		Request.Reset();
	}
	Super::EndPlay(Reason);
}

void ASalonSky::Fetch()
{
	if (Request.IsValid()) { return; }
	Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(FString::Printf(
		TEXT("https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=cloud_cover,precipitation,rain,showers,weather_code,wind_speed_10m,wind_direction_10m"),
		GivernyLatitude, GivernyLongitude));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("MuseeVision (a museum walk-through; the Salon's lanterns)"));
	Request->SetTimeout(20.f);
	TWeakObjectPtr<ASalonSky> Self(this);
	Request->OnProcessRequestComplete().BindLambda([Self](FHttpRequestPtr, FHttpResponsePtr Response, bool bOk)
	{
		if (ASalonSky* Sky = Self.Get())
		{
			const bool bGood = bOk && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
			Sky->OnWeather(bGood ? Response->GetContentAsString() : FString(), bGood);
		}
	});
	if (!Request->ProcessRequest())
	{
		Request.Reset();
		NextFetch = GetWorld()->GetTimeSeconds() + RetrySeconds;
	}
}

void ASalonSky::OnWeather(const FString& Body, bool bOk)
{
	Request.Reset();
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	TSharedPtr<FJsonObject> Root;
	const TSharedPtr<FJsonObject>* Current = nullptr;
	if (bOk && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root) && Root.IsValid() &&
		Root->TryGetObjectField(TEXT("current"), Current) && Current && Current->IsValid())
	{
		const FJsonObject& C = **Current;
		double CloudPct = 25.0, Precip = 0.0, RainMm = 0.0, Showers = 0.0, Wind = 10.0, WindFrom = 240.0, Code = 0.0;
		C.TryGetNumberField(TEXT("cloud_cover"), CloudPct);
		C.TryGetNumberField(TEXT("precipitation"), Precip);
		C.TryGetNumberField(TEXT("rain"), RainMm);
		C.TryGetNumberField(TEXT("showers"), Showers);
		C.TryGetNumberField(TEXT("wind_speed_10m"), Wind);
		C.TryGetNumberField(TEXT("wind_direction_10m"), WindFrom);
		C.TryGetNumberField(TEXT("weather_code"), Code);
		LiveCloud = float(FMath::Clamp(CloudPct / 100.0, 0.0, 1.0));
		// Rain in mm an hour: a drizzle 0.2, steady rain 2, a downpour 8 and more. WMO codes 51–67 and 80–82 are rain.
		const double Mm = FMath::Max3(Precip, RainMm, Showers);
		const bool bRainCode = (Code >= 51 && Code <= 67) || (Code >= 80 && Code <= 82) || (Code >= 95);
		LiveRain = float(FMath::Clamp(Mm > 0.0 ? 0.25 + 0.75 * FMath::Sqrt(FMath::Min(1.0, Mm / 6.0)) : (bRainCode ? 0.2 : 0.0), 0.0, 1.0));
		if (LiveRain > 0.f) { LiveCloud = FMath::Max(LiveCloud, 0.85f); }
		LiveWindSpeed = float(Wind / 3.6);
		LiveWindFrom = float(WindFrom);
		bHeard = true;
		NextFetch = Now + FetchEverySeconds;
		UE_LOG(LogSalonSky, Log, TEXT("Giverny: cloud %.0f%%, rain %.1f mm/h (code %.0f), wind %.0f km/h from %.0f°."), CloudPct, Mm, Code, Wind, WindFrom);
	}
	else
	{
		// No word from Giverny: keep the sky as it is (a fair one at first) and ask again later.
		NextFetch = Now + RetrySeconds;
		UE_LOG(LogSalonSky, Log, TEXT("Giverny's weather not reached (%s); the Salon keeps %s sky."),
			   bOk ? TEXT("unreadable") : TEXT("offline"), bHeard ? TEXT("the last") : TEXT("a fair"));
	}
}

double ASalonSky::Daylight() const
{
	UMaterialParameterCollection* Mpc = MuseeParameters.IsNull() ? nullptr : MuseeParameters.LoadSynchronous();
	return Mpc ? FMath::Clamp(double(UKismetMaterialLibrary::GetScalarParameterValue(const_cast<ASalonSky*>(this), Mpc, TEXT("Daylight"))), 0.0, 1.0) : 1.0;
}

double ASalonSky::SunHeight() const
{
	for (TActorIterator<AMuseeSky> It(GetWorld()); It; ++It)
	{
		const UDirectionalLightComponent* Sun = It->Sun;
		if (!Sun || !Sun->IsVisible() || It->bSphereMode) { return 0.0; }
		// The light travels along the component's forward vector: the sun is up the other way.
		return FMath::Max(0.0, -Sun->GetForwardVector().Z);
	}
	return 0.0;
}

void ASalonSky::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (!World) { return; }
	if (CVarSalonWeather.GetValueOnGameThread() < 0 && World->GetTimeSeconds() >= NextFetch && !Request.IsValid()) { Fetch(); }
	Apply(DeltaSeconds);
}

void ASalonSky::Apply(float DeltaSeconds)
{
	// The target sky: Giverny's, or the one asked for.
	float WantCloud = LiveCloud, WantRain = LiveRain, WindSpeed = LiveWindSpeed, WindFrom = LiveWindFrom;
	switch (CVarSalonWeather.GetValueOnGameThread())
	{
	case 0: WantCloud = 0.f; WantRain = 0.f; break;
	case 1: WantCloud = 0.55f; WantRain = 0.f; WindSpeed = 5.f; break;
	case 2: WantCloud = 1.f; WantRain = 0.f; break;
	case 3: WantCloud = 1.f; WantRain = 0.7f; WindSpeed = 6.f; break;
	default: break;
	}
	// Eased as a real sky changes; a sky asked for (musee.SalonWeather, the tour's views) is there at once.
	const int32 Mode = CVarSalonWeather.GetValueOnGameThread();
	const bool bJump = Mode != LastMode;
	LastMode = Mode;
	const float K = bJump ? 1.f : 1.f - FMath::Exp(-DeltaSeconds * 3.f / FMath::Max(1.f, EaseSeconds));
	Cloud = FMath::Lerp(Cloud, WantCloud, K);
	Rain = FMath::Lerp(Rain, WantRain, K);

	// Clouds drift with the wind, at their height a good deal faster than at the ground (about twice).
	const float To = FMath::DegreesToRadians(WindFrom + 180.f);
	const float Drift = 2.f * FMath::Max(1.f, WindSpeed);
	if (UMaterialParameterCollection* Mpc = Parameters.IsNull() ? nullptr : Parameters.LoadSynchronous())
	{
		// Plan x east, y south; a bearing is from north towards east.
		UKismetMaterialLibrary::SetScalarParameterValue(this, Mpc, TEXT("Cloud"), Cloud);
		UKismetMaterialLibrary::SetScalarParameterValue(this, Mpc, TEXT("Rain"), Rain);
		UKismetMaterialLibrary::SetScalarParameterValue(this, Mpc, TEXT("CloudDriftX"), Drift * FMath::Sin(To));
		UKismetMaterialLibrary::SetScalarParameterValue(this, Mpc, TEXT("CloudDriftY"), -Drift * FMath::Cos(To));
	}

	// The light on the roof: the sun through what cloud there is (broken cloud lets it through between the clouds, on
	// average), the sky (brighter and whiter as it clouds over, up to a point, then duller; rain duller still).
	const double Day = Daylight();
	const double Sun = SunHeight() * SunLux * FMath::Square(1.0 - FMath::SmoothStep(0.15, 0.98, double(Cloud)));
	const double Sky = FMath::Lerp(double(ClearSkyLux), double(OvercastLux), FMath::SmoothStep(0.3, 1.0, double(Cloud))) *
					   FMath::Lerp(1.0, double(RainDim), double(Rain)) * Day;
	const double Roof = Sun + Sky;
	if (UMaterialParameterCollection* Mpc = Parameters.IsNull() ? nullptr : Parameters.LoadSynchronous())
	{
		// For the diffusers' and the velarium's glow (they show what they pass on: τ · E / π nits), and the share of it
		// that is direct sun (the steel grid's shadow on the muslin, the brighter patch where the sun stands).
		UKismetMaterialLibrary::SetScalarParameterValue(this, Mpc, TEXT("RoofLux"), float(FMath::Max(Roof, double(NightFloorLux))));
		UKismetMaterialLibrary::SetScalarParameterValue(this, Mpc, TEXT("SunShare"), float(Roof > 1.0 ? Sun / Roof : 0.0));
	}
	auto Lumens = [&](URectLightComponent* L, double Transmittance, double Area)
	{
		if (!L) { return; }
		// A diffuser passes τ of the light on it, spread evenly: lumens = τ · E · A (and never less than the night floor).
		L->SetIntensity(float(Transmittance * FMath::Max(Roof, double(NightFloorLux)) * Area));
	};
	double X0, Y0, X1, Y1, GZ;
	ASalonStructure::LanternRect(1, X0, Y0, X1, Y1, GZ);
	const double LanternArea = (X1 - X0) * (Y1 - Y0);
	Lumens(OpalLight, OpalTransmittance, LanternArea);
	Lumens(MuslinLight, MuslinTransmittance, LanternArea);
	double CX, HX, HY, VZ;
	ASalonStructure::VelariumRect(CX, HX, HY, VZ);
	Lumens(VelariumLight, VelariumTransmittance, UE_DOUBLE_PI * HX * HY);   // the velarium's ellipse

	// The rain on the glass: heard while it rains.
	for (int32 i = 0; i < RainAudio.Num(); ++i)
	{
		UAudioComponent* Audio = RainAudio[i];
		USalonRainWave* Wave = RainWaves.IsValidIndex(i) ? RainWaves[i].Get() : nullptr;
		if (!Audio || !Wave) { continue; }
		Wave->Intensity.store(Rain, std::memory_order_relaxed);
		if (Rain > 0.02f && !Audio->IsPlaying()) { Audio->SetVolumeMultiplier(RainVolume); Audio->Play(); }
		else if (Rain < 0.01f && Audio->IsPlaying()) { Audio->Stop(); }
	}

	LogTimer -= DeltaSeconds;
	if (LogTimer <= 0.f)
	{
		LogTimer = 20.f;
		UE_LOG(LogSalonSky, Log, TEXT("Salon sky: cloud %.2f, rain %.2f, roof %.0f lux (sun %.0f at height %.2f, sky %.0f, daylight %.2f)."), Cloud, Rain, Roof, Sun, SunHeight(), Sky, Day);
	}
}
