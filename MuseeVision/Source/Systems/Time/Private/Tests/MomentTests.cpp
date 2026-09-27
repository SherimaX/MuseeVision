#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Calendar/SolarTerms.h"
#include "Design/TimeDesign.h"
#include "Keeper/TimeKeeper.h"
#include "TimeMomentParameters.h"

// Time's public side: its records read whole, and the moment the keeper makes from them.
namespace Musee::Time::Tests
{
	/** An instant for the moment tests: 2026 September 27, the week of the autumn equinox (Qiufen). */
	FDateTime MomentTestInstant()
	{
		return FDateTime(2026, 9, 27, 4, 0, 0);
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeRecordsTest, "Musee.Time.Records", Flags)
	bool FTimeRecordsTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FTimeDesign, FString> Read = ReadDesign();
		if (!TestFalse(TEXT("Data/Time reads whole: ") + (Read.HasError() ? Read.GetError() : FString()), Read.HasError()))
		{
			return false;
		}
		const FTimeDesign& Design = Read.GetValue();
		TestEqual(TEXT("The museum stands in Beijing by default (D-002)"), Design.DefaultCity, FName(TEXT("beijing")));
		TestTrue(TEXT("New York is a city the dial reads true for"), Design.MuseumCities.Contains(FName(TEXT("new-york"))));
		TestFalse(TEXT("Giverny is a weather place, not a museum city"), Design.MuseumCities.Contains(FName(TEXT("giverny"))));
		TestEqual(TEXT("Three weather places (D-005)"), Design.WeatherPlaces.Num(), 3);
		TestEqual(TEXT("The idealised sun is the default (D-003)"), Design.SunModel, ESunModel::Idealised);
		TestTrue(TEXT("Every solar term has its key"), !Design.SolarTermKeys.Contains(NAME_None) && Design.SolarTermKeys.Num() == SolarTermCount);
		TestTrue(TEXT("The live feed has an endpoint and a period"), !Design.LiveEndpoint.IsEmpty() && Design.LiveRefreshMinutes > 0.0);
		TestTrue(TEXT("Wetness and snow cover have their measures"), Design.WetFilmMillimeters > 0.0 && Design.FullSnowCoverMeters > 0.0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeMomentSunTest, "Musee.Time.Moment.Sun", Flags)
	bool FTimeMomentSunTest::RunTest(const FString& Parameters)
	{
		const FTimeDesign* Design = Time::Design();
		if (!TestNotNull(TEXT("The design"), Design)) { return false; }
		FTimeKeeper Keeper(*Design, TEXT("beijing"));
		const FDateTime Real = MomentTestInstant();

		Keeper.SetHour(12.0, Real);
		const FTimeMoment Noon = Keeper.MomentAt(Real);
		Near(*this, TEXT("Set to noon, the clock reads noon"), Noon.CivilHour, 12.0, 1e-6);
		TestFalse(TEXT("A set clock isn't live"), Noon.bLive);
		TestTrue(TEXT("The sun is up at noon"), Noon.bSunUp);
		Near(*this, TEXT("The idealised sun stands due south at noon"), Noon.SunAzimuthDegrees, 180.0, 1e-6);
		Near(*this, TEXT("at its constant altitude, atan(20.3 / 6.78)"), Noon.SunElevationDegrees, 71.53, 0.01);
		Near(*this, TEXT("Full daylight"), Noon.Daylight, 1.0, 0.0);
		Near(*this, TEXT("The direction points up at that altitude"), Noon.SunDirection.Z, FMath::Sin(FMath::DegreesToRadians(71.53)), 1e-3);
		TestTrue(TEXT("It shines from the south (+y)"), Noon.SunDirection.Y > 0.0);

		Keeper.SetHour(18.2, Real);
		const FTimeMoment Dusk = Keeper.MomentAt(Real);
		TestFalse(TEXT("After 18:00 the idealised sun is down"), Dusk.bSunUp);
		Near(*this, TEXT("12 minutes after its last hour it is 3 degrees down"), Dusk.SunElevationDegrees, -3.0, 1e-6);
		Near(*this, TEXT("halfway through civil twilight"), Dusk.Daylight, 0.5, 1e-6);

		Keeper.SetHour(3.0, Real);
		const FTimeMoment Night = Keeper.MomentAt(Real);
		Near(*this, TEXT("At 03:00 there is no daylight"), Night.Daylight, 0.0, 0.0);
		TestTrue(TEXT("and the sun is below the horizon"), Night.SunDirection.Z < 0.0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeMomentSkyTest, "Musee.Time.Moment.Sky", Flags)
	bool FTimeMomentSkyTest::RunTest(const FString& Parameters)
	{
		const FTimeDesign* Design = Time::Design();
		if (!TestNotNull(TEXT("The design"), Design)) { return false; }
		FTimeKeeper Keeper(*Design, TEXT("beijing"));
		const FTimeMoment Moment = Keeper.MomentAt(MomentTestInstant());
		TestTrue(TEXT("A new keeper is live"), Moment.bLive);
		TestEqual(TEXT("Four days after the equinox, the term is Qiufen"), Moment.SolarTermKey, FName(TEXT("qiufen")));
		TestEqual(TEXT("which is term 15"), Moment.SolarTerm, 15);
		TestEqual(TEXT("and the season by the solar terms is autumn"), Moment.Season, FName(TEXT("autumn")));
		Near(*this, TEXT("The Earth is about 1 AU from the Sun"), Moment.SunDistanceAstronomicalUnits, 1.0, 0.02);
		TestTrue(TEXT("The Moon's lit fraction is a fraction"), Moment.MoonIlluminatedFraction >= 0.0 && Moment.MoonIlluminatedFraction <= 1.0);
		Near(*this, TEXT("The Moon is about 384,400 km away"), Moment.MoonDistanceKilometers, 384400.0, 30000.0);
		Near(*this, TEXT("The star field turns rigidly, into a left-handed frame"), Moment.StarFieldOrientation.Determinant(), -1.0, 1e-6);
		TestEqual(TEXT("Weather at the three places"), Moment.Weather.Num(), 3);
		for (const FTimeWeather& Weather : Moment.Weather)
		{
			TestTrue(Weather.Place.ToString() + TEXT(": cloud, wetness and snow cover are fractions"),
				Weather.CloudCover >= 0.0 && Weather.CloudCover <= 1.0 && Weather.Wetness >= 0.0 && Weather.Wetness <= 1.0
				&& Weather.SnowCover >= 0.0 && Weather.SnowCover <= 1.0);
			TestFalse(Weather.Place.ToString() + TEXT(": with no feed, the generator gives the weather"), Weather.bLive);
		}
		TestNotNull(TEXT("The museum's own weather"), Moment.WeatherAt(TEXT("museum")));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeMomentCityTest, "Musee.Time.Moment.City", Flags)
	bool FTimeMomentCityTest::RunTest(const FString& Parameters)
	{
		const FTimeDesign* Design = Time::Design();
		if (!TestNotNull(TEXT("The design"), Design)) { return false; }
		FTimeKeeper Keeper(*Design, TEXT("beijing"));
		const FDateTime Real = MomentTestInstant();
		Keeper.SetHour(10.0, Real);
		TestTrue(TEXT("The museum moves to New York"), Keeper.SetCity(TEXT("new-york"), Real));
		const FTimeMoment Moment = Keeper.MomentAt(Real);
		TestEqual(TEXT("The moment is New York's"), Moment.City, FName(TEXT("new-york")));
		Near(*this, TEXT("A set clock keeps its reading in the new city"), Moment.CivilHour, 10.0, 1e-6);
		TestFalse(TEXT("The museum can't stand in London (the dial reads false there)"), Keeper.SetCity(TEXT("london"), Real));
		TestEqual(TEXT("and stays where it was"), Keeper.City(), FName(TEXT("new-york")));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeHourKeyTest, "Musee.Time.HourKey", Flags)
	bool FTimeHourKeyTest::RunTest(const FString& Parameters)
	{
		const double Stops[] = { 10.0, 16.0, 21.0 };
		TestEqual(TEXT("From live to the first stop"), FTimeKeeper::NextHourStop(Stops, {}).Get(-1.0), 10.0);
		TestEqual(TEXT("From a stop to the next"), FTimeKeeper::NextHourStop(Stops, 10.0).Get(-1.0), 16.0);
		TestEqual(TEXT("From an hour between stops to the next later one"), FTimeKeeper::NextHourStop(Stops, 12.5).Get(-1.0), 16.0);
		TestFalse(TEXT("After the last, back to live"), FTimeKeeper::NextHourStop(Stops, 21.0).IsSet());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeParametersTest, "Musee.Time.Parameters", Flags)
	bool FTimeParametersTest::RunTest(const FString& Parameters)
	{
		TSet<FString> Names;
		for (const TCHAR* Name : MomentParameters::All) { Names.Add(Name); }
		TestEqual(TEXT("MPC_Moment has the stage contract's eleven parameters, each once"), Names.Num(), 11);
		TestTrue(TEXT("under /Game/Generated/Time"), FString(MomentParameters::CollectionPath).StartsWith(TEXT("/Game/Generated/Time/MPC_Moment")));
		return true;
	}
}

#endif
