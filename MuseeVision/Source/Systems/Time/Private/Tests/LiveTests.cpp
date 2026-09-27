#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "Feed/LiveFeed.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MuseeCommand.h"
#include "MuseeSetting.h"
#include "Settings/TimeSettings.h"
#include "TimeSubsystem.h"

// The live side of Time: the feed's answer read into observations, and the world subsystem following its settings.
namespace Musee::Time::Tests
{
	/**
	 * One place of an answer as Open-Meteo gave it on 2026-09-27 for Giverny (current conditions at 04:15 UTC), with a
	 * shower and snowfall written in so the conversions show.
	 */
	FString FeedPlaceAnswer(const TCHAR* TemperatureUnit)
	{
		return FString::Printf(TEXT("{\"latitude\":49.08,\"longitude\":1.54,\"generationtime_ms\":3.61,\"utc_offset_seconds\":0,")
			TEXT("\"timezone\":\"GMT\",\"timezone_abbreviation\":\"GMT\",\"elevation\":25.0,\"location_id\":1,")
			TEXT("\"current_units\":{\"time\":\"unixtime\",\"interval\":\"seconds\",\"temperature_2m\":\"%s\",\"cloud_cover\":\"%%\",")
			TEXT("\"rain\":\"mm\",\"showers\":\"mm\",\"snowfall\":\"cm\",\"wind_speed_10m\":\"m/s\",\"wind_direction_10m\":\"°\",\"snow_depth\":\"m\"},")
			TEXT("\"current\":{\"time\":1790489700,\"interval\":900,\"temperature_2m\":12.4,\"cloud_cover\":55,\"rain\":0.50,")
			TEXT("\"showers\":0.25,\"snowfall\":0.70,\"wind_speed_10m\":1.02,\"wind_direction_10m\":79,\"snow_depth\":null}}"), TemperatureUnit);
	}

	FString WriteFeedFixture(const FString& Name, const FString& Answer)
	{
		const FString Path = FPaths::ConvertRelativePathToFull(FPaths::AutomationTransientDir() / TEXT("MuseeTime") / Name);
		FFileHelper::SaveStringToFile(LiveFeed::AsRecordFile(Answer), *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		return Path;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeFeedTest, "Musee.Time.Feed", Flags)
	bool FTimeFeedTest::RunTest(const FString& Parameters)
	{
		const FString Place = FeedPlaceAnswer(TEXT("°C"));
		const FString Path = WriteFeedFixture(TEXT("Feed.json"), TEXT("[") + Place + TEXT(",") + Place + TEXT("]"));
		TArray<FWeatherObservation> Observations;
		FString Error;
		if (!TestTrue(TEXT("A kept answer reads: ") + Error, LiveFeed::Read(Path, 2, Observations, Error))) { return false; }
		const FWeatherObservation& Reading = Observations[0];
		TestEqual(TEXT("The reading's instant"), Reading.Utc, FDateTime::FromUnixTimestamp(1790489700));
		Near(*this, TEXT("Cloud cover from percent"), Reading.State.CloudCover, 0.55, 1e-9);
		Near(*this, TEXT("Rain and showers over 15 minutes as a rate"), Reading.State.RainMillimetersPerHour, 3.0, 1e-9);
		Near(*this, TEXT("Snow depth as water, 7 cm of snow to 10 mm, as a rate"), Reading.State.SnowfallMillimetersPerHour, 4.0, 1e-9);
		TestFalse(TEXT("No snow depth reported"), Reading.SnowDepthMeters.IsSet());
		TestFalse(TEXT("The wrong number of places is refused"), LiveFeed::Read(Path, 3, Observations, Error));

		const FString Wrong = WriteFeedFixture(TEXT("Fahrenheit.json"), TEXT("[") + FeedPlaceAnswer(TEXT("°F")) + TEXT("]"));
		TestFalse(TEXT("A reading in a unit not asked for is refused"), LiveFeed::Read(Wrong, 1, Observations, Error));
		TestTrue(TEXT("and the reason names the unit"), Error.Contains(TEXT("°F")));

		FObserver Beijing;
		Beijing.LatitudeDegrees = 39.90667;
		Beijing.LongitudeDegrees = 116.3975;
		const FObserver Places[] = { Beijing, Beijing };
		const FString Url = LiveFeed::RequestUrl(TEXT("https://api.open-meteo.com/v1/forecast"), Places);
		TestTrue(TEXT("One request asks for every place"), Url.Contains(TEXT("latitude=39.90667,39.90667")));
		TestTrue(TEXT("in metres a second and Unix time"), Url.Contains(TEXT("wind_speed_unit=ms")) && Url.Contains(TEXT("timeformat=unixtime")));
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Path), false, true);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeSubsystemTest, "Musee.Time.Subsystem", Flags)
	bool FTimeSubsystemTest::RunTest(const FString& Parameters)
	{
		FMuseeSetting* Hour = FMuseeSetting::Find(Settings::HourKey());
		FMuseeSetting* City = FMuseeSetting::Find(Settings::CityKey());
		if (!TestNotNull(TEXT("time.hour is registered"), Hour) || !TestNotNull(TEXT("time.city is registered"), City)) { return false; }
		const FString HourBefore = Hour->ToText();
		const FName CityBefore = City->Choice();

		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		UTimeSubsystem* Time = World->GetSubsystem<UTimeSubsystem>();
		if (TestNotNull(TEXT("Every game world keeps time"), Time))
		{
			int32 Heard = 0;
			double HeardHour = -1.0;
			Time->OnMoment().AddLambda([&](const FTimeMoment& Moment) { ++Heard; HeardHour = Moment.CivilHour; });
			Hour->SetNumber(16.0);
			TestEqual(TEXT("Setting the hour announces a moment at once"), Heard, 1);
			Near(*this, TEXT("at that hour"), HeardHour, 16.0, 1e-3);
			TestTrue(TEXT("The hour key runs"), FMuseeCommand::Run(TEXT("musee.time.advance-hour"), {}));
			Near(*this, TEXT("and moves to the next stop"), Time->Now().CivilHour, 21.0, 1e-3);
			TestTrue(TEXT("The city key runs"), FMuseeCommand::Run(TEXT("musee.time.advance-city"), {}));
			TestNotEqual(TEXT("and moves the museum"), Time->Now().City, CityBefore);
		}
		World->DestroyWorld(false);
		Hour->SetFromText(HourBefore);
		City->SetChoice(CityBefore);
		return true;
	}
}

#endif
