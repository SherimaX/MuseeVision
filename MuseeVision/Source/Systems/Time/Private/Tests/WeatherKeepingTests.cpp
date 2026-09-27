#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Almanac/CelestialAngles.h"
#include "Tests/WeatherFixtures.h"
#include "Weather/PrecipitationHistory.h"
#include "Weather/WeatherPlaces.h"
#include "Weather/WeatherSmoother.h"

// Keeping the weather: easing towards each target, the ground's history, and the keyed places of Decision D-005.
namespace Musee::Time::Tests
{
	FWeatherState WeatherOf(double Temperature, double Cloud, double Rain, double Snowfall, double Wind, double WindFrom)
	{
		FWeatherState State;
		State.AirTemperatureCelsius = Temperature;
		State.CloudCover = Cloud;
		State.RainMillimetersPerHour = Rain;
		State.SnowfallMillimetersPerHour = Snowfall;
		State.WindSpeedMetersPerSecond = Wind;
		State.WindFromDegrees = WindFrom;
		return State;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeWeatherSmoothingTest, "Musee.Time.WeatherSmoothing", Flags)
	bool FTimeWeatherSmoothingTest::RunTest(const FString& Parameters)
	{
		const FWeatherSmoothing Smoothing = MuseumWeatherDesign().Smoothing;
		FWeatherSmoother Smoother;
		const FWeatherState Start = WeatherOf(10.0, 0.2, 0.0, 0.0, 5.0, 350.0);
		const FWeatherState Target = WeatherOf(20.0, 0.8, 4.0, 0.0, 5.0, 10.0);
		Smoother.Advance(Start, 1.0, Smoothing);
		TestTrue(TEXT("the first reading is taken at once"), Smoother.State().AirTemperatureCelsius == 10.0);

		// After one time constant, 1 - 1/e of each step is made up, however the time is cut.
		for (int32 Step = 0; Step < 30; ++Step) { Smoother.Advance(Target, Smoothing.TemperatureSeconds / 30.0, Smoothing); }
		const double Made = 1.0 - FMath::Exp(-1.0);
		Near(*this, TEXT("temperature after one time constant"), Smoother.State().AirTemperatureCelsius, 10.0 + 10.0 * Made, 1e-9);
		Near(*this, TEXT("cloud after one time constant"), Smoother.State().CloudCover, 0.2 + 0.6 * Made, 1e-9);
		Near(*this, TEXT("rain after 2.5 time constants"), Smoother.State().RainMillimetersPerHour, 4.0 * (1.0 - FMath::Exp(-2.5)), 1e-9);

		// The wind veers the short way, from 350 through north to 10, never through south.
		TestTrue(TEXT("the wind passes through north"), FMath::Abs(SignedDegrees(Smoother.State().WindFromDegrees)) < 10.0);
		FWeatherSmoother Calm;
		Calm.Reset(WeatherOf(10.0, 0.0, 0.0, 0.0, 0.0, 123.0));
		Calm.Advance(WeatherOf(10.0, 0.0, 0.0, 0.0, 0.0, 123.0), 60.0, Smoothing);
		Near(*this, TEXT("a calm keeps a direction"), Calm.State().WindFromDegrees, 123.0, 1e-9);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimePrecipitationHistoryTest, "Musee.Time.PrecipitationHistory", Flags)
	bool FTimePrecipitationHistoryTest::RunTest(const FString& Parameters)
	{
		FPrecipitationHistory History(MuseumWeatherDesign().History);
		const double Memories[] = {1.0, 6.0, 48.0};

		// Three hours of 2 mm/h fill each record to 2 tau (1 - exp(-3 / tau)), whatever the step.
		for (int32 Minute = 0; Minute < 180; ++Minute) { History.Advance(WeatherOf(12.0, 1.0, 2.0, 0.0, 3.0, 200.0), 60.0); }
		for (int32 Record = 0; Record < 3; ++Record)
		{
			const double Tau = Memories[Record];
			Near(*this, FString::Printf(TEXT("record of %.0f h after 3 h of rain (mm)"), Tau), History.RainRecordMillimeters(Record),
				2.0 * Tau * (1.0 - FMath::Exp(-3.0 / Tau)), 1e-9);
		}
		Near(*this, TEXT("hours since wet, while raining"), History.HoursSinceWet().Get(-1.0), 0.0, 0.0);
		const double AfterRain = History.RainRecordMillimeters(0);
		History.Advance(WeatherOf(12.0, 0.3, 0.0, 0.0, 3.0, 200.0), 2.0 * 3600.0);
		Near(*this, TEXT("the one-hour record fades over two dry hours"), History.RainRecordMillimeters(0), AfterRain * FMath::Exp(-2.0), 1e-9);
		Near(*this, TEXT("hours since wet"), History.HoursSinceWet().Get(-1.0), 2.0, 1e-9);

		// Snow at -10 C: 10 mm of water falls at Hedstrom and Pomeroy's 69 kg/m3 and settles towards 300 kg/m3.
		for (int32 Minute = 0; Minute < 600; ++Minute) { History.Advance(WeatherOf(-10.0, 1.0, 0.0, 1.0, 2.0, 0.0), 60.0); }
		Near(*this, TEXT("snow water after 10 h of 1 mm/h (mm)"), History.SnowWaterMillimeters(), 10.0, 1e-9);
		TestTrue(TEXT("fresh snow is light, settling"), History.SnowDensityKilogramsPerCubicMeter() > 69.0
			&& History.SnowDensityKilogramsPerCubicMeter() < 100.0);
		History.Advance(WeatherOf(-5.0, 0.0, 0.0, 0.0, 2.0, 0.0), 500.0 * 3600.0);
		Near(*this, TEXT("lying snow settles to 300 kg/m3 (after 5 e-folding times)"),
			History.SnowDensityKilogramsPerCubicMeter(), 300.0, (300.0 - 69.0) * FMath::Exp(-5.0));
		// At +2 C, 3 mm a day per degree melts 6 mm in a day; the meltwater reaches the ground.
		const double WetBeforeThaw = History.RainRecordMillimeters(2);
		for (int32 Hour = 0; Hour < 24; ++Hour) { History.Advance(WeatherOf(2.0, 0.5, 0.0, 0.0, 2.0, 0.0), 3600.0); }
		Near(*this, TEXT("snow water after a day at +2 C (mm)"), History.SnowWaterMillimeters(), 4.0, 1e-9);
		TestTrue(TEXT("meltwater enters the ground's record"), History.RainRecordMillimeters(2) > WetBeforeThaw);
		for (int32 Hour = 0; Hour < 24; ++Hour) { History.Advance(WeatherOf(8.0, 0.5, 0.0, 0.0, 2.0, 0.0), 3600.0); }
		Near(*this, TEXT("all the snow gone at +8 C"), History.SnowDepthMeters(), 0.0, 0.0);

		// A reported depth replaces the modelled one; unmodelled snow is taken as settled.
		History.ObserveSnowDepth(0.2);
		Near(*this, TEXT("reported depth (m)"), History.SnowDepthMeters(), 0.2, 1e-12);
		Near(*this, TEXT("its water at 300 kg/m3 (mm)"), History.SnowWaterMillimeters(), 60.0, 1e-9);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeWeatherPlacesTest, "Musee.Time.WeatherPlaces", Flags)
	bool FTimeWeatherPlacesTest::RunTest(const FString& Parameters)
	{
		const FWeatherDesign Design = MuseumWeatherDesign();
		FWeatherPlaces Places(Design);
		const FName Museum(TEXT("museum"));
		const FName London(TEXT("london"));
		Places.SetPlace(Museum, ContinentalPlace(1));
		Places.SetPlace(London, MaritimePlace(2));
		TestTrue(TEXT("no weather before the first advance"), Places.Weather(Museum) == nullptr);
		TestTrue(TEXT("no weather for an unknown key"), Places.Weather(FName(TEXT("giverny"))) == nullptr);

		// The first advance builds the ground's history from the generator and shows its weather at once.
		const FDateTime Start(2026, 1, 20, 6, 0, 0);
		Places.Advance(Start);
		const FWeatherGenerator Reference(ContinentalPlace(1), Design.Generator);
		Near(*this, TEXT("the museum shows its generated weather"), Places.Weather(Museum)->AirTemperatureCelsius,
			Reference.StateAt(Start).AirTemperatureCelsius, 1e-9);
		TestFalse(TEXT("no live reading yet"), Places.IsLive(Museum));

		// A live reading takes over and the shown weather eases to it; London keeps its own.
		FWeatherObservation Reading;
		Reading.Utc = Start + FTimespan::FromMinutes(5);
		Reading.State = WeatherOf(3.0, 1.0, 1.5, 0.0, 6.0, 90.0);
		Reading.SnowDepthMeters = 0.05;
		Places.Observe(Museum, Reading);
		Near(*this, TEXT("the reported snow is taken at once (m)"), Places.History(Museum)->SnowDepthMeters(), 0.05, 1e-12);
		for (int32 Minute = 1; Minute <= 40; ++Minute) { Places.Advance(Start + FTimespan::FromMinutes(Minute)); }
		TestTrue(TEXT("the museum follows its live reading"), Places.IsLive(Museum));
		TestFalse(TEXT("London keeps its generated weather"), Places.IsLive(London));
		// Forty minutes are eight of the cloud's time constants and twenty of the rain's.
		Near(*this, TEXT("cloud eased to the reading"), Places.Weather(Museum)->CloudCover, 1.0, 1e-3);
		Near(*this, TEXT("rain eased to the reading (mm/h)"), Places.Weather(Museum)->RainMillimetersPerHour, 1.5, 1e-6);

		// A clock set three hours on is outside the reading's 90 minutes: the generator's weather of that hour shows.
		const FDateTime Later = Start + FTimespan::FromHours(3);
		Places.Advance(Later);
		TestFalse(TEXT("an old reading no longer stands"), Places.IsLive(Museum));
		Near(*this, TEXT("the weather of the set hour"), Places.Weather(Museum)->AirTemperatureCelsius,
			Reference.StateAt(Later).AirTemperatureCelsius, 1e-9);
		TestEqual(TEXT("two places kept"), Places.Keys().Num(), 2);
		return true;
	}
}

#endif
