#include "Albion/AlbionSky.h"

#include "Albion/AlbionPlan.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MuseeVision.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sky/MuseeSky.h"

namespace AlbionSkyImpl
{
	TAutoConsoleVariable<int32> CVarAlbionWeather(
		TEXT("musee.AlbionWeather"), -1,
		TEXT("Albion's sky: -1 live from London (Open-Meteo), 0 bright, 1 broken cloud, 2 rain, 3 fog, 4 snow, 5 overcast."),
		ECVF_Default);

	// Westminster.
	constexpr double LondonLatitude = 51.5072, LondonLongitude = -0.1276;
	constexpr double FetchEverySeconds = 15.0 * 60.0, RetrySeconds = 5.0 * 60.0;
	constexpr double CacheHours = 24.0;

	float Ease(float Current, float Target, float DeltaSeconds, float Seconds)
	{
		const float K = 1.f - FMath::Exp(-DeltaSeconds / FMath::Max(Seconds, 0.01f));
		return Current + (Target - Current) * K;
	}

	UDirectionalLightComponent* MakeColourLight(AActor* Owner, const TCHAR* Name, USceneComponent* Root, const FLinearColor& Colour)
	{
		UDirectionalLightComponent* L = Owner->CreateDefaultSubobject<UDirectionalLightComponent>(Name);
		L->SetupAttachment(Root);
		L->SetMobility(EComponentMobility::Movable);
		L->SetLightColor(Colour);
		L->SetIntensity(0.f);
		L->SetCastShadows(true);
		L->SetAtmosphereSunLight(false);
		L->SetVisibility(false);
		L->LightSourceAngle = 0.5357f;
		return L;
	}
}

AAlbionSky::AAlbionSky()
{
	using namespace AlbionSkyImpl;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;   // the coloured lights follow the sun every frame
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	Red = MakeColourLight(this, TEXT("StainedRed"), RootComponent, FLinearColor(1, 0, 0));
	Green = MakeColourLight(this, TEXT("StainedGreen"), RootComponent, FLinearColor(0, 1, 0));
	Blue = MakeColourLight(this, TEXT("StainedBlue"), RootComponent, FLinearColor(0, 0, 1));

	// Fog in the vaults: ellipsoids over the nave and the aisles, their lower edges at the springing (10 m).
	namespace AP = AlbionPlan;
	struct FFogBox { const TCHAR* Name; double X, Y, Z, RX, RY, RZ; };
	const double YC = 0.5 * (AP::Y0 + AP::Y1), RY = 0.5 * (AP::Y1 - AP::Y0) + 1.0;
	const FFogBox Boxes[3] = {{TEXT("FogNave"), 0.0, YC, 14.0, 7.0, RY, 4.2}, {TEXT("FogWest"), -AP::AisleCentreX, YC, 13.0, 4.6, RY, 3.2},
							  {TEXT("FogEast"), AP::AisleCentreX, YC, 13.0, 4.6, RY, 3.2}};
	const float Base = ULocalFogVolumeComponent::GetBaseVolumeSize();
	for (const FFogBox& B : Boxes)
	{
		ULocalFogVolumeComponent* F = CreateDefaultSubobject<ULocalFogVolumeComponent>(B.Name);
		F->SetupAttachment(RootComponent);
		F->SetMobility(EComponentMobility::Movable);
		F->SetRelativeLocation(AP::At(B.X, B.Y, B.Z));
		F->SetRelativeScale3D(FVector(B.RX, B.RY, B.RZ) * 100.0 / Base);
		F->RadialFogExtinction = 0.f;
		F->HeightFogExtinction = 0.f;
		F->FogAlbedo = FLinearColor(0.82, 0.83, 0.84);
		F->FogPhaseG = 0.35f;
		F->SetVisibility(false);
		Fogs.Add(F);
	}

	Parameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/Albion/MPC_Albion.MPC_Albion")));
	for (const TCHAR* Name : {TEXT("MI_Albion_SunLF_White"), TEXT("MI_Albion_SunLF_R"), TEXT("MI_Albion_SunLF_G"), TEXT("MI_Albion_SunLF_B")})
	{
		LightFunctions.Add(TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("/Game/Museum/Materials/Albion/%s.%s"), Name, Name))));
	}
	Tags.AddUnique(FName(TEXT("musee.nobake")));
}

void AAlbionSky::BeginPlay()
{
	Super::BeginPlay();
	for (TActorIterator<AMuseeSky> It(GetWorld()); It; ++It) { MuseeSun = It->Sun.Get(); break; }
	UDirectionalLightComponent* Colours[3] = {Red, Green, Blue};
	for (int32 i = 0; i < 3; ++i)
	{
		if (UMaterialInterface* M = LightFunctions.IsValidIndex(i + 1) ? LightFunctions[i + 1].LoadSynchronous() : nullptr)
		{
			Colours[i]->SetLightFunctionMaterial(M);
			Colours[i]->SetLightFunctionFadeDistance(1.0e7f);
			Colours[i]->SetLightFunctionDisabledBrightness(0.f);
		}
	}
	if (UDirectionalLightComponent* Sun = MuseeSun.Get())
	{
		UMaterialInterface* White = LightFunctions.IsValidIndex(0) ? LightFunctions[0].LoadSynchronous() : nullptr;
		if (White && !Sun->LightFunctionMaterial)
		{
			Sun->SetLightFunctionMaterial(White);
			Sun->SetLightFunctionFadeDistance(1.0e7f);
			Sun->SetLightFunctionDisabledBrightness(1.f);
			bOwnLightFunction = true;
		}
		else if (Sun->LightFunctionMaterial && Sun->LightFunctionMaterial != White)
		{
			UE_LOG(LogMusee, Warning, TEXT("Albion: the sun already has a light function (%s); the stained glass's colours stay off."),
				   *Sun->LightFunctionMaterial->GetName());
		}
	}
	// An offline start: the last sky heard, if recent.
	FString Cached;
	if (FFileHelper::LoadFileToString(Cached, *CachePath()))
	{
		const FDateTime Stamp = IFileManager::Get().GetTimeStamp(*CachePath());
		if ((FDateTime::UtcNow() - Stamp).GetTotalHours() < AlbionSkyImpl::CacheHours) { OnWeather(Cached, true, true); }
	}
	NextFetch = 0.0;
	Apply(1000.f);
}

void AAlbionSky::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Request.IsValid())
	{
		Request->OnProcessRequestComplete().Unbind();
		Request->CancelRequest();
		Request.Reset();
	}
	if (bOwnLightFunction)
	{
		if (UDirectionalLightComponent* Sun = MuseeSun.Get()) { Sun->SetLightFunctionMaterial(nullptr); }
	}
	Super::EndPlay(Reason);
}

FString AAlbionSky::CachePath() const
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Albion"), TEXT("london.json"));
}

void AAlbionSky::Fetch()
{
	using namespace AlbionSkyImpl;
	if (Request.IsValid()) { return; }
	Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(FString::Printf(
		TEXT("https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=cloud_cover,precipitation,rain,showers,snowfall,weather_code,visibility,temperature_2m"),
		LondonLatitude, LondonLongitude));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("User-Agent"), TEXT("MuseeVision (a museum walk-through; Albion's glass roof)"));
	Request->SetTimeout(20.f);
	TWeakObjectPtr<AAlbionSky> Self(this);
	Request->OnProcessRequestComplete().BindLambda([Self](FHttpRequestPtr, FHttpResponsePtr Response, bool bOk)
	{
		if (AAlbionSky* Sky = Self.Get())
		{
			const bool bGood = bOk && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
			Sky->OnWeather(bGood ? Response->GetContentAsString() : FString(), bGood, false);
		}
	});
	if (!Request->ProcessRequest())
	{
		Request.Reset();
		NextFetch = GetWorld()->GetTimeSeconds() + RetrySeconds;
	}
}

void AAlbionSky::OnWeather(const FString& Body, bool bOk, bool bFromCache)
{
	using namespace AlbionSkyImpl;
	if (!bFromCache) { Request.Reset(); }
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	TSharedPtr<FJsonObject> Root;
	const TSharedPtr<FJsonObject>* Current = nullptr;
	if (bOk && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body), Root) && Root.IsValid() &&
		Root->TryGetObjectField(TEXT("current"), Current) && Current && Current->IsValid())
	{
		const FJsonObject& C = **Current;
		double CloudPct = 50.0, RainMm = 0.0, Showers = 0.0, SnowCm = 0.0, Code = 0.0, Vis = 20000.0, Temp = 10.0;
		C.TryGetNumberField(TEXT("cloud_cover"), CloudPct);
		C.TryGetNumberField(TEXT("rain"), RainMm);
		C.TryGetNumberField(TEXT("showers"), Showers);
		C.TryGetNumberField(TEXT("snowfall"), SnowCm);
		C.TryGetNumberField(TEXT("weather_code"), Code);
		C.TryGetNumberField(TEXT("visibility"), Vis);
		C.TryGetNumberField(TEXT("temperature_2m"), Temp);
		const double Mm = RainMm + Showers;
		LiveCloud = float(FMath::Clamp(CloudPct / 100.0, 0.0, 1.0));
		// Rain: a drizzle (0.1 mm/h) beads the glass; 1 mm/h runs down it; 4 mm/h and more streams.
		LiveRain = float(FMath::Clamp(FMath::Sqrt(FMath::Max(Mm, 0.0) / 4.0), 0.0, 1.0));
		if (Mm <= 0.0 && ((Code >= 51 && Code <= 67) || (Code >= 80 && Code <= 82))) { LiveRain = 0.3f; }
		LiveSnow = float(FMath::Clamp(SnowCm / 0.5, 0.0, 1.0));
		if (LiveSnow <= 0.f && ((Code >= 71 && Code <= 77) || Code == 85 || Code == 86)) { LiveSnow = 0.6f; }
		if (Temp > 2.0) { LiveSnow *= 0.3f; }   // wet snow doesn't lie long on warm glass
		LiveFog = (Code == 45 || Code == 48) ? 1.f : float(FMath::Clamp(1.0 - (Vis - 400.0) / 4000.0, 0.0, 1.0));
		bHeard = true;
		if (!bFromCache)
		{
			NextFetch = Now + FetchEverySeconds;
			FFileHelper::SaveStringToFile(Body, *CachePath());
		}
		UE_LOG(LogMusee, Log, TEXT("Albion: London %s: cloud %.0f%%, rain %.1f mm/h, snow %.1f cm/h, visibility %.0f m (code %.0f), %.0f °C."),
			   bFromCache ? TEXT("(as last heard)") : TEXT("now"), CloudPct, Mm, SnowCm, Vis, Code, Temp);
	}
	else if (!bFromCache)
	{
		NextFetch = Now + RetrySeconds;
		UE_LOG(LogMusee, Log, TEXT("Albion: London's weather not reached; the glass keeps %s sky."), bHeard ? TEXT("the last") : TEXT("a plain London"));
	}
}

void AAlbionSky::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (!World) { return; }
	if (AlbionSkyImpl::CVarAlbionWeather.GetValueOnGameThread() < 0 && World->GetTimeSeconds() >= NextFetch && !Request.IsValid()) { Fetch(); }
	Apply(DeltaSeconds);
}

void AAlbionSky::Apply(float DeltaSeconds)
{
	using namespace AlbionSkyImpl;
	// The target sky: London's, or the one asked for.
	float TCloud = LiveCloud, TRain = LiveRain, TSnow = LiveSnow, TFog = LiveFog;
	switch (CVarAlbionWeather.GetValueOnGameThread())
	{
	case 0: TCloud = 0.1f; TRain = 0.f; TSnow = 0.f; TFog = 0.f; break;
	case 1: TCloud = 0.6f; TRain = 0.f; TSnow = 0.f; TFog = 0.f; break;
	case 2: TCloud = 0.95f; TRain = 0.8f; TSnow = 0.f; TFog = 0.1f; break;
	case 3: TCloud = 1.0f; TRain = 0.f; TSnow = 0.f; TFog = 1.f; break;
	case 4: TCloud = 0.9f; TRain = 0.f; TSnow = 0.9f; TFog = 0.2f; break;
	case 5: TCloud = 1.0f; TRain = 0.f; TSnow = 0.f; TFog = 0.f; break;
	default: break;
	}
	const bool bForced = CVarAlbionWeather.GetValueOnGameThread() >= 0;
	const float Settle = bForced ? 0.5f : EaseSeconds;
	Cloud = Ease(Cloud, TCloud, DeltaSeconds, Settle);
	Rain = Ease(Rain, TRain, DeltaSeconds, Settle);
	Snow = Ease(Snow, TSnow, DeltaSeconds, bForced ? 0.5f : 120.f);
	Fog = Ease(Fog, TFog, DeltaSeconds, bForced ? 0.5f : 60.f);
	// The glass wets as fast as it rains, and dries slowly.
	Wet = Rain > Wet ? Ease(Wet, Rain, DeltaSeconds, 5.f) : Ease(Wet, Rain, DeltaSeconds, bForced ? 0.5f : DrySeconds);
	// The sun: out under a clear sky, gone under overcast; under broken cloud the clouds pass (bright hours between).
	CloudClock += DeltaSeconds;
	const double Passing = 0.5 + 0.5 * FMath::Sin(CloudClock / 47.0) * FMath::Cos(CloudClock / 131.0 + 1.3);
	const float Gap = FMath::Clamp((float(Passing) - (Cloud - 0.35f) * 1.6f) * 4.f, 0.f, 1.f);
	const float TSun = Cloud < 0.3f ? 1.f : (Cloud > 0.85f ? 0.f : Gap);
	SunOn = Ease(SunOn, TSun * (1.f - 0.8f * Fog), DeltaSeconds, 3.f);

	if (UMaterialParameterCollection* C = Parameters.LoadSynchronous())
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, C, TEXT("Rain"), Rain);
		UKismetMaterialLibrary::SetScalarParameterValue(this, C, TEXT("Wet"), Wet);
		UKismetMaterialLibrary::SetScalarParameterValue(this, C, TEXT("Snow"), Snow);
		UKismetMaterialLibrary::SetScalarParameterValue(this, C, TEXT("Fog"), Fog);
		UKismetMaterialLibrary::SetScalarParameterValue(this, C, TEXT("Sun"), SunOn);
	}
	for (ULocalFogVolumeComponent* F : Fogs)
	{
		if (!F) { continue; }
		const bool bOn = Fog > 0.01f;
		if (F->IsVisible() != bOn) { F->SetVisibility(bOn); }
		if (bOn) { F->SetRadialFogExtinction(Fog * FogDensity); }
	}
	ApplySun();
	LogTimer += DeltaSeconds;
	if (LogTimer > 60.f)
	{
		LogTimer = 0.f;
		UE_LOG(LogMusee, Verbose, TEXT("Albion: cloud %.2f rain %.2f wet %.2f snow %.2f fog %.2f sun %.2f"), Cloud, Rain, Wet, Snow, Fog, SunOn);
	}
}

void AAlbionSky::ApplySun()
{
	UDirectionalLightComponent* Sun = MuseeSun.Get();
	UDirectionalLightComponent* Colours[3] = {Red, Green, Blue};
	const FVector ToSun = Sun ? -Sun->GetForwardVector() : FVector(0, 0.32, 0.95);
	if (UMaterialParameterCollection* C = Parameters.LoadSynchronous())
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, C, TEXT("SunDir"), FLinearColor(ToSun.X, ToSun.Y, ToSun.Z, 0.f));
	}
	// Burn the coloured lights only while they can be seen: the view in or near the court, the sun up and out.
	bool bNear = false;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			const FVector At = PC->PlayerCameraManager->GetCameraLocation();
			bNear = FBox(AlbionPlan::At(-20.0, 8.0, -2.0), AlbionPlan::At(20.0, 60.0, 30.0)).IsInside(At);
		}
	}
	const bool bSunUp = Sun && Sun->IsVisible() && Sun->Intensity > 0.f && ToSun.Z > 0.02;
	const bool bOn = bOwnLightFunction && bNear && bSunUp && SunOn > 0.02f;
	// Air mass at the sun's height: a little of the blue and green scattered out of the direct beam.
	const FLinearColor SunColour = Sun ? Sun->GetLightColor() : FLinearColor::White;
	const FLinearColor Air(0.97f, 0.95f, 0.91f);
	for (int32 i = 0; i < 3; ++i)
	{
		UDirectionalLightComponent* L = Colours[i];
		if (!L) { continue; }
		if (L->IsVisible() != bOn) { L->SetVisibility(bOn); }
		if (!bOn) { continue; }
		L->SetWorldRotation(Sun->GetComponentRotation());
		const float Channel = i == 0 ? SunColour.R * Air.R : (i == 1 ? SunColour.G * Air.G : SunColour.B * Air.B);
		L->SetIntensity(Sun->Intensity * Channel);
	}
}
