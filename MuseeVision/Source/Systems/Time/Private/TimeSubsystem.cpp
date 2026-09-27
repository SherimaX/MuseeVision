#include "TimeSubsystem.h"

#include "Design/TimeDesign.h"
#include "Engine/World.h"
#include "Feed/LiveFeed.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Keeper/TimeKeeper.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/FileHelper.h"
#include "MuseeSetting.h"
#include "Settings/TimeSettings.h"
#include "TimeLog.h"
#include "TimeMomentParameters.h"

namespace Musee::Time
{
	namespace
	{
		/**
		 * How often a running clock makes a new moment. The sun turns 15 degrees an hour, 0.004 degrees a second, so in
		 * one second the shadow of the Rotunda's node 20.3 m up moves 1.5 mm across the dial: far below what the eye sees.
		 */
		constexpr double MomentPeriodSeconds = 1.0;

		/** HTTP status 200: the feed answered. */
		constexpr int32 HttpOk = 200;

		/** Time's places as the stage contract names them for MPC_Moment. */
		const TCHAR* const MuseumPlace = TEXT("museum");
		const TCHAR* const GivernyPlace = TEXT("giverny");
		const TCHAR* const LondonPlace = TEXT("london");
	}
}

bool UTimeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && (World->IsGameWorld() || World->WorldType == EWorldType::Editor) && Musee::Time::Design();
}

void UTimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	using namespace Musee::Time;
	Keeper = MakePimpl<FTimeKeeper>(*Design(), Settings::ChosenCity());
	Keeper->SetHour(Settings::ChosenHour(), FDateTime::UtcNow());
	MomentCollection = LoadObject<UMaterialParameterCollection>(nullptr, MomentParameters::CollectionPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!MomentCollection)
	{
		UE_LOG(LogMuseeTime, Log, TEXT("%s isn't made yet, so shaders don't see the moment."), MomentParameters::CollectionPath);
	}
	FollowSettings();
	TakeLiveWeather(LiveFeed::CachePath(Keeper->City()));
}

void UTimeSubsystem::Deinitialize()
{
	using namespace Musee::Time;
	if (FMuseeSetting* Hour = FMuseeSetting::Find(Settings::HourKey())) { Hour->OnChanged().Remove(HourHandle); }
	if (FMuseeSetting* City = FMuseeSetting::Find(Settings::CityKey())) { City->OnChanged().Remove(CityHandle); }
	MomentEvent.Clear();
	Keeper.Reset();
	Super::Deinitialize();
}

void UTimeSubsystem::FollowSettings()
{
	using namespace Musee::Time;
	if (FMuseeSetting* Hour = FMuseeSetting::Find(Settings::HourKey()))
	{
		HourHandle = Hour->OnChanged().AddWeakLambda(this, [this](const FMuseeSetting&) { ApplyHour(); });
	}
	if (FMuseeSetting* City = FMuseeSetting::Find(Settings::CityKey()))
	{
		CityHandle = City->OnChanged().AddWeakLambda(this, [this](const FMuseeSetting&) { ApplyCity(); });
	}
}

void UTimeSubsystem::ApplyHour()
{
	Keeper->SetHour(Musee::Time::Settings::ChosenHour(), FDateTime::UtcNow());
	Refresh();
}

void UTimeSubsystem::ApplyCity()
{
	using namespace Musee::Time;
	if (!Keeper->SetCity(Settings::ChosenCity(), FDateTime::UtcNow())) { return; }
	TakeLiveWeather(LiveFeed::CachePath(Keeper->City()));
	SecondsSinceFetch = 0.0;
	if (GetWorld()->IsGameWorld()) { RequestLiveWeather(); }
	Refresh();
}

void UTimeSubsystem::Refresh()
{
	SecondsSinceMoment = 0.0;
	Announce();
}

void UTimeSubsystem::Announce()
{
	Moment = Keeper->MomentAt(FDateTime::UtcNow());
	WriteParameters();
	MomentEvent.Broadcast(Moment);
}

void UTimeSubsystem::Tick(float DeltaTime)
{
	using namespace Musee::Time;
	SecondsSinceMoment += DeltaTime;
	if (SecondsSinceMoment >= MomentPeriodSeconds || Moment.City.IsNone()) { Refresh(); }
	if (!GetWorld()->IsGameWorld() || bFetching) { return; }
	SecondsSinceFetch += DeltaTime;
	const bool bFirst = SecondsSinceFetch <= DeltaTime;
	if (bFirst || SecondsSinceFetch >= Design()->LiveRefreshMinutes * 60.0)
	{
		SecondsSinceFetch = DeltaTime;
		RequestLiveWeather();
	}
}

TStatId UTimeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTimeSubsystem, STATGROUP_MuseeTime);
}

void UTimeSubsystem::WriteParameters() const
{
	using namespace Musee::Time;
	UWorld* World = GetWorld();
	UMaterialParameterCollectionInstance* Instance = MomentCollection && World ? World->GetParameterCollectionInstance(MomentCollection) : nullptr;
	if (!Instance) { return; }
	const auto Set = [Instance](const TCHAR* Name, double Value) { Instance->SetScalarParameterValue(FName(Name), static_cast<float>(Value)); };
	Set(MomentParameters::SunElevationDeg, Moment.SunElevationDegrees);
	Set(MomentParameters::SunAzimuthDeg, Moment.SunAzimuthDegrees);
	Set(MomentParameters::Daylight, Moment.Daylight);
	Set(MomentParameters::SolarTerm, Moment.SolarTerm);
	if (const FTimeWeather* Museum = Moment.WeatherAt(MuseumPlace))
	{
		Set(MomentParameters::RainRateMmH, Museum->RainMillimetersPerHour);
		Set(MomentParameters::Wetness, Museum->Wetness);
		Set(MomentParameters::SnowCover, Museum->SnowCover);
		Set(MomentParameters::TemperatureC, Museum->AirTemperatureCelsius);
		Set(MomentParameters::WindSpeedMS, Museum->WindSpeedMetersPerSecond);
	}
	if (const FTimeWeather* Giverny = Moment.WeatherAt(GivernyPlace)) { Set(MomentParameters::GivernyWetness, Giverny->Wetness); }
	if (const FTimeWeather* London = Moment.WeatherAt(LondonPlace)) { Set(MomentParameters::LondonWetness, London->Wetness); }
}

void UTimeSubsystem::RequestLiveWeather()
{
	using namespace Musee::Time;
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(LiveFeed::RequestUrl(Design()->LiveEndpoint, Keeper->FeedObservers()));
	Request->SetVerb(TEXT("GET"));
	const FName City = Keeper->City();
	Request->OnProcessRequestComplete().BindWeakLambda(this, [this, City](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
	{
		bFetching = false;
		if (!bConnected || !Response.IsValid() || Response->GetResponseCode() != HttpOk)
		{
			UE_LOG(LogMuseeTime, Log, TEXT("The live weather feed can't be reached; each place's generator keeps its weather."));
			return;
		}
		const FString Path = LiveFeed::CachePath(City);
		if (!FFileHelper::SaveStringToFile(LiveFeed::AsRecordFile(Response->GetContentAsString()), *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			UE_LOG(LogMuseeTime, Warning, TEXT("The live weather can't be kept at %s."), *Path);
			return;
		}
		if (Keeper.IsValid() && Keeper->City() == City)
		{
			TakeLiveWeather(Path);
			Refresh();
		}
	});
	bFetching = Request->ProcessRequest();
}

void UTimeSubsystem::TakeLiveWeather(const FString& CachePath)
{
	using namespace Musee::Time;
	if (!FPaths::FileExists(CachePath)) { return; }
	const TArray<FName> Places = Keeper->FeedPlaces();
	TArray<FWeatherObservation> Observations;
	FString Error;
	if (!LiveFeed::Read(CachePath, Places.Num(), Observations, Error))
	{
		UE_LOG(LogMuseeTime, Warning, TEXT("The live weather can't be read, so each place's generator keeps its weather: %s"), *Error);
		return;
	}
	for (int32 Index = 0; Index < Places.Num(); ++Index)
	{
		Keeper->Observe(Places[Index], Observations[Index]);
	}
}
