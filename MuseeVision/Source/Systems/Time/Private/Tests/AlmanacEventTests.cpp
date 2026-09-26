#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Almanac/Almanac.h"
#include "Almanac/CelestialAngles.h"
#include "Almanac/SiderealTime.h"
#include "Almanac/SolarDay.h"

// The almanac's events (phases of the Moon, equinoxes and solstices, the Sun's rising and setting) against the U.S.
// Naval Observatory's published times, Meeus's worked examples and the NREL SPA.
namespace Musee::Time::Tests
{
	/** The UTC instant of an ephemeris event, with Delta T at the event. */
	FDateTime UtcOfEvent(double JulianEphemerisDay)
	{
		const FDateTime Estimate = UtcFromJulianDay(JulianEphemerisDay);
		return UtcFromJulianDay(JulianEphemerisDay - DeltaTSeconds(Estimate) / SecondsPerDay);
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeMoonPhasesTest, "Musee.Time.MoonPhases", Flags)
	bool FTimeMoonPhasesTest::RunTest(const FString& Parameters)
	{
		// Meeus example 49.a, the new Moon of 1977 February: 1977 Feb 18 3h37m42s TD. Example 49.b, the first last
		// quarter of 2044: 2044 Jan 21 23h48m17s TD (as PyMeeus reproduces them). Meeus puts the method of ch. 49 within
		// a few seconds; the ELP truncation adds a few more.
		const double NewMoon = LunarPhaseInstant(0.0, JulianDayFromUtc(FDateTime(1977, 2, 15)));
		NearInstant(*this, TEXT("new Moon, 49.a (TD)"), UtcFromJulianDay(NewMoon), FDateTime(1977, 2, 18, 3, 37, 42), 10.0);
		const double LastQuarter = LunarPhaseInstant(270.0, JulianDayFromUtc(FDateTime(2044, 1, 15)));
		NearInstant(*this, TEXT("last quarter, 49.b (TD)"), UtcFromJulianDay(LastQuarter), FDateTime(2044, 1, 21, 23, 48, 17), 10.0);

		// USNO, "Phases of the Moon" for 2026 (UT, to the minute): full Moon 2026 Jan 3 10:03; new Moon 2026 Feb 17
		// 12:01 (the annular eclipse of that day). A minute's rounding allows 30 s; 45 s are allowed.
		const double Full = LunarPhaseInstant(180.0, JulianDayFromUtc(FDateTime(2026, 1, 3)));
		NearInstant(*this, TEXT("full Moon, USNO 2026-01-03 10:03 UT"), UtcOfEvent(Full), FDateTime(2026, 1, 3, 10, 3, 0), 45.0);
		const double New = LunarPhaseInstant(0.0, JulianDayFromUtc(FDateTime(2026, 2, 17)));
		NearInstant(*this, TEXT("new Moon, USNO 2026-02-17 12:01 UT"), UtcOfEvent(New), FDateTime(2026, 2, 17, 12, 1, 0), 45.0);

		// The Moon's age: five days after that new Moon it is five days old, waxing, and a crescent.
		const FInstant FiveDaysOn = FInstant::FromTerrestrial(New + 5.0, DeltaTSeconds(FDateTime(2026, 2, 22)));
		const FMoonPosition Crescent = Moon(FiveDaysOn, FObserver::At(39.90667, 116.3975, 47.0));
		Near(*this, TEXT("age five days after new (days)"), Crescent.AgeDays, 5.0, 1e-5);
		TestTrue(TEXT("waxing after new"), Crescent.Phase.IsWaxing());
		TestTrue(TEXT("a crescent: less than half lit"), Crescent.Phase.IlluminatedFraction < 0.5);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeEquinoxesTest, "Musee.Time.Equinoxes", Flags)
	bool FTimeEquinoxesTest::RunTest(const FString& Parameters)
	{
		// USNO, "Earth's Seasons: Equinoxes, Solstices, Perihelion, and Aphelion", 2024-2026, UT to the minute.
		struct FEvent { int32 Year; int32 Month; int32 Day; int32 Hour; int32 Minute; double Longitude; };
		const FEvent Events[] = {
			{2024, 3, 20, 3, 6, 0.0}, {2024, 6, 20, 20, 51, 90.0}, {2024, 9, 22, 12, 44, 180.0}, {2024, 12, 21, 9, 20, 270.0},
			{2025, 3, 20, 9, 1, 0.0}, {2025, 6, 21, 2, 42, 90.0}, {2025, 9, 22, 18, 19, 180.0}, {2025, 12, 21, 15, 3, 270.0},
			{2026, 3, 20, 14, 46, 0.0}, {2026, 6, 21, 8, 24, 90.0}, {2026, 9, 23, 0, 5, 180.0}, {2026, 12, 21, 20, 50, 270.0},
		};
		for (const FEvent& Event : Events)
		{
			const FDateTime Published(Event.Year, Event.Month, Event.Day, Event.Hour, Event.Minute, 0);
			const double Instant = SolarLongitudeInstant(Event.Longitude, JulianDayFromUtc(FDateTime(Event.Year, Event.Month, Event.Day)));
			NearInstant(*this, FString::Printf(TEXT("Sun at %.0f degrees"), Event.Longitude), UtcOfEvent(Instant), Published, 45.0);
		}
		// The June solstice of 1962 on the full theory: 1962 June 21 21h24m42s TD (the value PyMeeus gives for Meeus's
		// example 27.a, whose own approximate method gives 21h25m08s).
		const double Solstice = SolarLongitudeInstant(90.0, JulianDayFromUtc(FDateTime(1962, 6, 21)));
		NearInstant(*this, TEXT("June solstice 1962 (TD)"), UtcFromJulianDay(Solstice), FDateTime(1962, 6, 21, 21, 24, 42), 10.0);
		return true;
	}

	/** The Sun's geocentric altitude, as rising and setting are defined. */
	double GeocentricSunAltitude(const FDateTime& Utc, const FObserver& Observer)
	{
		const FInstant Instant = FInstant::FromUtc(Utc);
		const FSolarCoordinates Solar = SolarCoordinates(Instant.JulianEphemerisDay());
		const double LocalSidereal = ApparentSiderealDegrees(Instant.JulianDayUniversal, Solar.Nutation) + Observer.LongitudeDegrees;
		return HorizontalFromHourAngle(LocalSidereal - Solar.RightAscensionDegrees, Solar.DeclinationDegrees,
			Observer.LatitudeDegrees).AltitudeDegrees;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeSunriseSunsetTest, "Musee.Time.SunriseSunset", Flags)
	bool FTimeSunriseSunsetTest::RunTest(const FString& Parameters)
	{
		// NREL SPA table A5.1: at Golden on 2003-10-17 (UTC-7), transit 18:46:04.97 UT and sunrise 13:12:43.46 UT. Its
		// sunset, 00:20:19.19 UT on the 17th, is the evening of the local 16th.
		const FObserver Golden = FObserver::At(39.742476, -105.1786, 1830.14);
		const FSolarDay October17 = SolarDay(FDateTime(2003, 10, 17, 7, 0, 0), Golden);
		NearInstant(*this, TEXT("SPA transit"), October17.TransitUtc, FDateTime(2003, 10, 17, 18, 46, 4, 970), 1.0);
		NearInstant(*this, TEXT("SPA sunrise"), October17.Sunrise.MorningUtc.Get(FDateTime()), FDateTime(2003, 10, 17, 13, 12, 43, 460), 1.0);
		const FSolarDay October16 = SolarDay(FDateTime(2003, 10, 16, 7, 0, 0), Golden);
		NearInstant(*this, TEXT("SPA sunset"), October16.Sunrise.EveningUtc.Get(FDateTime()), FDateTime(2003, 10, 17, 0, 20, 19, 190), 1.0);

		// NREL SPA table A2.1: the Astronomical Almanac's times (to the minute) and the SPA's, on the Greenwich meridian.
		struct FCase { int32 Year; int32 Month; int32 Day; double Latitude; int32 RiseH; int32 RiseM; int32 RiseS; int32 RiseMs; int32 SetH; int32 SetM; int32 SetS; int32 SetMs; };
		const FCase Cases[] = {
			{1994, 1, 2, 35.0, 7, 8, 12, 800, 16, 59, 55, 900},
			{1996, 7, 5, -35.0, 7, 8, 15, 400, 17, 1, 4, 500},
			{2004, 12, 4, -35.0, 4, 38, 57, 100, 19, 2, 2, 500},
		};
		for (const FCase& Case : Cases)
		{
			const FDateTime Date(Case.Year, Case.Month, Case.Day);
			const FSolarDay Day = SolarDay(Date, FObserver::At(Case.Latitude, 0.0, 0.0));
			const FDateTime Rise = Date + FTimespan(0, Case.RiseH, Case.RiseM, Case.RiseS, Case.RiseMs * 1000000);
			const FDateTime Set = Date + FTimespan(0, Case.SetH, Case.SetM, Case.SetS, Case.SetMs * 1000000);
			NearInstant(*this, TEXT("sunrise against the SPA"), Day.Sunrise.MorningUtc.Get(FDateTime()), Rise, 1.0);
			NearInstant(*this, TEXT("sunset against the SPA"), Day.Sunrise.EveningUtc.Get(FDateTime()), Set, 1.0);
		}

		// The definition itself: at the computed sunrise over Beijing the Sun's centre stands 50 arcminutes down.
		const FObserver Beijing = FObserver::At(39.90667, 116.3975, 47.0);
		const FSolarDay Solstice = SolarDay(FDateTime(2026, 6, 20, 16, 0, 0), Beijing);
		Near(*this, TEXT("altitude at sunrise"), GeocentricSunAltitude(Solstice.Sunrise.MorningUtc.Get(FDateTime()), Beijing),
			SolarDayAltitudes::SunriseDegrees, 1e-4);
		Near(*this, TEXT("altitude at civil dusk"), GeocentricSunAltitude(Solstice.CivilTwilight.EveningUtc.Get(FDateTime()), Beijing),
			SolarDayAltitudes::CivilTwilightDegrees, 1e-4);
		TestTrue(TEXT("dawn comes in order"), Solstice.AstronomicalTwilight.MorningUtc.Get(FDateTime()) < Solstice.NauticalTwilight.MorningUtc.Get(FDateTime())
			&& Solstice.NauticalTwilight.MorningUtc.Get(FDateTime()) < Solstice.CivilTwilight.MorningUtc.Get(FDateTime())
			&& Solstice.CivilTwilight.MorningUtc.Get(FDateTime()) < Solstice.Sunrise.MorningUtc.Get(FDateTime())
			&& Solstice.Sunrise.MorningUtc.Get(FDateTime()) < Solstice.TransitUtc);
		Near(*this, TEXT("Beijing's longest day (h)"), Solstice.DayLengthHours(), 15.0, 0.1);

		// Polar day and night: at 80 N the Sun never sets at the June solstice and never rises at the December one.
		const FObserver Svalbard = FObserver::At(80.0, 15.0, 0.0);
		const FSolarDay PolarDay = SolarDay(FDateTime(2026, 6, 21), Svalbard);
		const FSolarDay PolarNight = SolarDay(FDateTime(2026, 12, 21), Svalbard);
		TestFalse(TEXT("no sunrise in polar day"), PolarDay.Sunrise.MorningUtc.IsSet());
		Near(*this, TEXT("polar day length (h)"), PolarDay.DayLengthHours(), HoursPerDay, 0.0);
		Near(*this, TEXT("polar night length (h)"), PolarNight.DayLengthHours(), 0.0, 0.0);
		return true;
	}
}

#endif
