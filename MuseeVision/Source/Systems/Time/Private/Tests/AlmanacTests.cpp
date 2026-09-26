#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Almanac/Almanac.h"
#include "Almanac/CelestialAngles.h"
#include "Almanac/SiderealTime.h"

// The almanac against published worked examples: Meeus, Astronomical Algorithms, 2nd ed. (1998), and Reda and
// Andreas, "Solar Position Algorithm for Solar Radiation Applications", NREL/TP-560-34302, revised January 2008.
namespace Musee::Time::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeJulianDayTest, "Musee.Time.JulianDay", Flags)
	bool FTimeJulianDayTest::RunTest(const FString& Parameters)
	{
		// NREL SPA table A4.1 (after Meeus ch. 7): the Gregorian dates of the table.
		struct FCase { int32 Year; int32 Month; int32 Day; int32 Hour; double JulianDay; };
		const FCase Cases[] = {
			{2000, 1, 1, 12, 2451545.0}, {1999, 1, 1, 0, 2451179.5}, {1987, 1, 27, 0, 2446822.5}, {1987, 6, 19, 12, 2446966.0},
			{1988, 1, 27, 0, 2447187.5}, {1988, 6, 19, 12, 2447332.0}, {1900, 1, 1, 0, 2415020.5}, {1600, 1, 1, 0, 2305447.5},
			{1600, 12, 31, 0, 2305812.5},
		};
		for (const FCase& Case : Cases)
		{
			const FDateTime Utc(Case.Year, Case.Month, Case.Day, Case.Hour);
			Near(*this, TEXT("Julian Day of ") + Utc.ToIso8601(), JulianDayFromUtc(Utc), Case.JulianDay, 1e-9);
			NearInstant(*this, TEXT("back from the Julian Day"), UtcFromJulianDay(Case.JulianDay), Utc, 1e-4);
		}

		// The SPA example's instant, 2003-10-17 12:30:30 at UTC-7, with its Delta T of 67 s.
		const FInstant Instant = FInstant::FromUtc(FDateTime(2003, 10, 17, 19, 30, 30), 67.0);
		Near(*this, TEXT("SPA example JD"), Instant.JulianDayUniversal, 2452930.312847, 5e-7);
		Near(*this, TEXT("SPA example JDE"), Instant.JulianEphemerisDay(), Instant.JulianDayUniversal + 67.0 / SecondsPerDay, 1e-9);
		const FInstant Terrestrial = FInstant::FromTerrestrial(Instant.JulianEphemerisDay(), 67.0);
		Near(*this, TEXT("TT round trip (days)"), Terrestrial.JulianDayUniversal, Instant.JulianDayUniversal, 1e-9);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeDeltaTTest, "Musee.Time.DeltaT", Flags)
	bool FTimeDeltaTTest::RunTest(const FString& Parameters)
	{
		// From 1972: 32.184 s + TAI - UTC (IERS Bulletin C). The last leap second came at the end of 2016.
		Near(*this, TEXT("Delta T 2026-09-26"), DeltaTSeconds(FDateTime(2026, 9, 26)), 69.184, 1e-9);
		Near(*this, TEXT("Delta T 2016-12-31"), DeltaTSeconds(FDateTime(2016, 12, 31, 23, 59, 59)), 68.184, 1e-9);
		Near(*this, TEXT("Delta T 2017-01-01"), DeltaTSeconds(FDateTime(2017, 1, 1)), 69.184, 1e-9);
		// Observed Delta T (IERS, as in Meeus table 10.A): the model is within the 0.9 s that UT1 - UTC may reach.
		Near(*this, TEXT("Delta T 1990.0 against the observed 56.86 s"), DeltaTSeconds(FDateTime(1990, 1, 1)), 56.86, 0.9);
		Near(*this, TEXT("Delta T 2000.0 against the observed 63.83 s"), DeltaTSeconds(FDateTime(2000, 1, 1)), 63.83, 0.9);
		// Before 1972, the Espenak and Meeus fits, against Meeus table 10.A (to 0.1 s).
		Near(*this, TEXT("Delta T 1950.0 against 29.1 s"), DeltaTSeconds(FDateTime(1950, 1, 1)), 29.1, 0.3);
		Near(*this, TEXT("Delta T 1960.0 against 33.1 s"), DeltaTSeconds(FDateTime(1960, 1, 1)), 33.1, 0.3);
		Near(*this, TEXT("Delta T 1970.0 against 40.2 s"), DeltaTSeconds(FDateTime(1970, 1, 1)), 40.2, 0.3);
		// The fit and the leap-second model meet at 1972 without a jump the sky could show.
		Near(*this, TEXT("Delta T step at 1972"), DeltaTSeconds(FDateTime(1972, 1, 1)) - DeltaTSeconds(FDateTime(1971, 12, 31)), 0.0, 0.1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeNutationTest, "Musee.Time.Nutation", Flags)
	bool FTimeNutationTest::RunTest(const FString& Parameters)
	{
		// Meeus example 22.a: 1987 April 10, 0h TD.
		const FNutation Result = Nutation(2446895.5);
		Near(*this, TEXT("delta psi (arcsec)"), Result.LongitudeDegrees * ArcsecondsPerDegree, -3.788, 0.002);
		Near(*this, TEXT("delta epsilon (arcsec)"), Result.ObliquityDegrees * ArcsecondsPerDegree, 9.443, 0.002);
		Near(*this, TEXT("mean obliquity"), Result.MeanObliquityDegrees, FromSexagesimal(23, 26, 27.407), 0.001 / ArcsecondsPerDegree);
		Near(*this, TEXT("true obliquity"), Result.TrueObliquityDegrees(), FromSexagesimal(23, 26, 36.850), 0.002 / ArcsecondsPerDegree);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeSiderealTimeTest, "Musee.Time.SiderealTime", Flags)
	bool FTimeSiderealTimeTest::RunTest(const FString& Parameters)
	{
		// Meeus example 12.a: 1987 April 10, 0h UT; 12.b: 19h21m00s UT the same day.
		const double JulianDay = 2446895.5;
		Near(*this, TEXT("mean sidereal time, 12.a"), MeanSiderealDegrees(JulianDay), FromHours(13, 10, 46.3668), 1e-6);
		Near(*this, TEXT("apparent sidereal time, 12.a"), ApparentSiderealDegrees(JulianDay, Nutation(JulianDay)),
			FromHours(13, 10, 46.1351), 1e-5);
		Near(*this, TEXT("mean sidereal time, 12.b"), MeanSiderealDegrees(JulianDay + (19.0 + 21.0 / 60.0) / HoursPerDay),
			128.7378734, 1e-6);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeSunAgainstSpaTest, "Musee.Time.SunAgainstSpa", Flags)
	bool FTimeSunAgainstSpaTest::RunTest(const FString& Parameters)
	{
		// NREL SPA, appendix A.5, table A5.1: Golden, Colorado, 2003-10-17 12:30:30 at UTC-7, Delta T 67 s,
		// 820 hPa and 11 C. The SPA states its uncertainty as 0.0003 degrees.
		const FInstant Instant = FInstant::FromUtc(FDateTime(2003, 10, 17, 19, 30, 30), 67.0);
		FObserver Observer;
		Observer.LatitudeDegrees = 39.742476;
		Observer.LongitudeDegrees = -105.1786;
		Observer.ElevationMeters = 1830.14;
		Observer.PressureHectopascals = 820.0;
		Observer.TemperatureCelsius = 11.0;
		const FSunPosition Seen = Sun(Instant, Observer);
		const FSolarCoordinates& C = Seen.Coordinates;

		// The heliocentric series: the revised report's table A4.2 differs from Meeus appendix III in five small terms
		// (misprints of 2.458, 75, 6286.6, 65 and 18849.23); together they move L by 2e-6 degree and R by 2e-7 au.
		Near(*this, TEXT("Earth heliocentric longitude L"), C.Earth.LongitudeDegrees, 24.0182635175, 5e-6);
		Near(*this, TEXT("Earth heliocentric latitude B"), C.Earth.LatitudeDegrees, -0.0001011219, 1e-9);
		Near(*this, TEXT("Earth radius vector R (au)"), C.Earth.RadiusAu, 0.9965421031, 5e-7);
		Near(*this, TEXT("nutation in longitude"), C.Nutation.LongitudeDegrees, -0.00399840, 1e-8);
		Near(*this, TEXT("nutation in obliquity"), C.Nutation.ObliquityDegrees, 0.00166657, 1e-8);
		Near(*this, TEXT("true obliquity"), C.Nutation.TrueObliquityDegrees(), 23.440465, 1e-6);
		// The almanac adds Meeus's FK5 frame correction (-0.09033 arcsecond), which the SPA leaves out.
		Near(*this, TEXT("apparent longitude"), C.ApparentLongitudeDegrees, 204.0085537528, 1e-4);
		Near(*this, TEXT("right ascension"), C.RightAscensionDegrees, 202.22741, 1e-4);
		Near(*this, TEXT("declination"), C.DeclinationDegrees, -9.31434, 1e-4);
		Near(*this, TEXT("mean longitude M"), C.MeanLongitudeDegrees, 205.8971722516, 1e-8);
		Near(*this, TEXT("equation of time (min)"), C.EquationOfTimeMinutes, 14.641503, 1e-3);
		Near(*this, TEXT("topocentric right ascension"), Seen.Place.Topocentric.RightAscensionDegrees, 202.22704, 1e-4);
		Near(*this, TEXT("topocentric declination"), Seen.Place.Topocentric.DeclinationDegrees, -9.316179, 1e-4);
		Near(*this, TEXT("topocentric hour angle"), Seen.Place.Topocentric.HourAngleDegrees, 11.10629, 1e-4);
		Near(*this, TEXT("zenith angle"), 90.0 - Seen.Place.Apparent.AltitudeDegrees, 50.11162, 1e-4);
		Near(*this, TEXT("azimuth"), Seen.Place.Apparent.AzimuthDegrees, 194.34024, 1e-4);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeSunAgainstMeeusTest, "Musee.Time.SunAgainstMeeus", Flags)
	bool FTimeSunAgainstMeeusTest::RunTest(const FString& Parameters)
	{
		// Meeus example 25.b (the full VSOP87 theory): 1992 October 13.0 TD.
		const FSolarCoordinates Solar = SolarCoordinates(2448908.5);
		Near(*this, TEXT("apparent right ascension, 25.b"), Solar.RightAscensionDegrees, 198.378178, 2e-5);
		Near(*this, TEXT("apparent declination, 25.b"), Solar.DeclinationDegrees, -7.783871, 2e-5);
		// Meeus example 28.a: the equation of time at the same instant, +13m42.6s.
		Near(*this, TEXT("equation of time (s), 28.a"), Solar.EquationOfTimeMinutes * MinutesPerHour, 13.0 * 60.0 + 42.6, 0.1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeMoonAgainstMeeusTest, "Musee.Time.MoonAgainstMeeus", Flags)
	bool FTimeMoonAgainstMeeusTest::RunTest(const FString& Parameters)
	{
		// Meeus example 47.a: 1992 April 12, 0h TD. Matching lambda, beta and the distance to the example's last digit
		// checks every term of tables 47.A and 47.B that the example's sums hold.
		const double JulianEphemerisDay = 2448724.5;
		const FLunarCoordinates Lunar = LunarCoordinates(JulianEphemerisDay);
		Near(*this, TEXT("geocentric longitude"), Lunar.LongitudeDegrees, 133.162655, 1e-6);
		Near(*this, TEXT("geocentric latitude"), Lunar.LatitudeDegrees, -3.229126, 1e-6);
		Near(*this, TEXT("distance (km)"), Lunar.DistanceKilometers, 368409.7, 0.1);
		Near(*this, TEXT("equatorial horizontal parallax"), Lunar.HorizontalParallaxDegrees, 0.991990, 1e-6);
		Near(*this, TEXT("apparent longitude"), Lunar.ApparentLongitudeDegrees, 133.167265, 2e-6);
		Near(*this, TEXT("apparent right ascension"), Lunar.RightAscensionDegrees, 134.688470, 3e-6);
		Near(*this, TEXT("apparent declination"), Lunar.DeclinationDegrees, 13.768368, 3e-6);

		// Meeus example 48.a, the same instant: phase angle 69.0756 degrees, lit fraction 0.6786, bright limb at 285.0.
		const FLunarPhase Phase = LunarPhase(Lunar, SolarCoordinates(JulianEphemerisDay));
		Near(*this, TEXT("phase angle"), Phase.PhaseAngleDegrees, 69.0756, 1e-3);
		Near(*this, TEXT("illuminated fraction"), Phase.IlluminatedFraction, 0.6786, 1e-4);
		Near(*this, TEXT("bright limb position angle"), Phase.BrightLimbAngleDegrees, 285.0, 0.05);
		TestTrue(TEXT("a gibbous Moon four days before full is waxing"), Phase.IsWaxing());
		return true;
	}
}

#endif
