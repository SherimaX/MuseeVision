#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Almanac/Almanac.h"
#include "Almanac/CelestialAngles.h"
#include "Almanac/Precession.h"
#include "Almanac/Refraction.h"

// The sky's frames and the air: refraction, the observer's horizon frame, and the star catalogue carried to the date.
namespace Musee::Time::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeRefractionTest, "Musee.Time.Refraction", Flags)
	bool FTimeRefractionTest::RunTest(const FString& Parameters)
	{
		// Saemundsson's formula (true to apparent) and Bennett's (apparent to true) agree within 4 arcseconds from the
		// horizon to the zenith (Meeus ch. 16).
		const double ApparentAltitudes[] = {0.0, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 45.0, 80.0};
		for (const double Apparent : ApparentAltitudes)
		{
			const double Bennett = RefractionOfApparentDegrees(Apparent);
			const double Saemundsson = RefractionDegrees(Apparent - Bennett, 1010.0, 10.0);
			Near(*this, FString::Printf(TEXT("the two formulas at %.1f degrees (arcsec)"), Apparent),
				(Saemundsson - Bennett) * ArcsecondsPerDegree, 0.0, 4.0);
		}
		// At the horizon the standard air lifts a body by about 34 arcminutes, the 0.5667 degree that rising and setting
		// assume (Meeus ch. 15).
		Near(*this, TEXT("refraction at the horizon (arcmin)"), RefractionOfApparentDegrees(0.0) * 60.0, 34.0, 0.6);

		// The standard atmosphere (ISO 2533): 898.76 hPa at 1000 m and 795.01 hPa at 2000 m, 8.5 C at 1000 m.
		Near(*this, TEXT("pressure at sea level (hPa)"), StandardPressureHectopascals(0.0), 1013.25, 1e-9);
		Near(*this, TEXT("pressure at 1000 m (hPa)"), StandardPressureHectopascals(1000.0), 898.76, 0.05);
		Near(*this, TEXT("pressure at 2000 m (hPa)"), StandardPressureHectopascals(2000.0), 795.01, 0.1);
		Near(*this, TEXT("temperature at 1000 m (C)"), StandardTemperatureCelsius(1000.0), 8.5, 1e-9);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeHorizonFrameTest, "Musee.Time.HorizonFrame", Flags)
	bool FTimeHorizonFrameTest::RunTest(const FString& Parameters)
	{
		// The museum's frame: x east, y south, z up; azimuth from true north through east.
		auto Place = [](double Azimuth, double Altitude) { FHorizontalPlace P; P.AzimuthDegrees = Azimuth; P.AltitudeDegrees = Altitude; return P; };
		TestTrue(TEXT("north is -y"), DirectionFromHorizontal(Place(0, 0)).Equals(FVector(0, -1, 0), 1e-12));
		TestTrue(TEXT("east is +x"), DirectionFromHorizontal(Place(90, 0)).Equals(FVector(1, 0, 0), 1e-12));
		TestTrue(TEXT("south is +y"), DirectionFromHorizontal(Place(180, 0)).Equals(FVector(0, 1, 0), 1e-12));
		TestTrue(TEXT("the zenith is +z"), DirectionFromHorizontal(Place(0, 90)).Equals(FVector(0, 0, 1), 1e-12));
		const FHorizontalPlace Back = HorizontalFromDirection(DirectionFromHorizontal(Place(231.5, 17.25)));
		Near(*this, TEXT("azimuth round trip"), Back.AzimuthDegrees, 231.5, 1e-9);
		Near(*this, TEXT("altitude round trip"), Back.AltitudeDegrees, 17.25, 1e-9);

		// The equatorial map agrees with the hour-angle formulas for any star, is orthogonal, and mirrors (the museum
		// frame is left-handed).
		const double Latitude = 40.3;
		const double Sidereal = 123.4;
		const FMatrix Map = EquatorialToHorizonFrame(Sidereal, Latitude);
		Near(*this, TEXT("determinant of the equatorial map"), Map.Determinant(), -1.0, 1e-12);
		const double Stars[][2] = {{10.0, 20.0}, {200.0, -35.0}, {123.4, 0.0}, {300.0, 80.0}};
		for (const auto& Star : Stars)
		{
			const FVector Equatorial(CosDegrees(Star[1]) * CosDegrees(Star[0]), CosDegrees(Star[1]) * SinDegrees(Star[0]), SinDegrees(Star[1]));
			const FVector ByMap = Map.TransformVector(Equatorial);
			const FVector ByHourAngle = DirectionFromHorizontal(HorizontalFromHourAngle(Sidereal - Star[0], Star[1], Latitude));
			Near(*this, FString::Printf(TEXT("star at %.0f, %.0f: map against hour angle"), Star[0], Star[1]),
				(ByMap - ByHourAngle).Size(), 0.0, 1e-12);
		}
		const FHorizontalPlace Pole = HorizontalFromDirection(Map.TransformVector(FVector::ZAxisVector));
		Near(*this, TEXT("the celestial pole's altitude is the latitude"), Pole.AltitudeDegrees, Latitude, 1e-9);
		Near(*this, TEXT("the celestial pole is due north"), SignedDegrees(Pole.AzimuthDegrees), 0.0, 1e-9);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeStarFieldTest, "Musee.Time.StarField", Flags)
	bool FTimeStarFieldTest::RunTest(const FString& Parameters)
	{
		// Meeus examples 21.b and 23.a: theta Persei, J2000 place 2h44m11.986s +49 13 42.48 with proper motion
		// +0.03425 s and -0.0895 arcsec a year, carried to 2028 November 13.19 TD (JDE 2462088.69). Precession gives
		// 2h46m11.331s +49 20 54.54 (21.b); nutation then adds +15.843 and +6.218 arcseconds (23.a).
		const double JulianEphemerisDay = 2462088.69;
		const double Years = (JulianEphemerisDay - J2000JulianDay) / 365.25;
		const double RightAscension = FromHours(2, 44, 11.986) + FromHours(0, 0, 0.03425 * Years);
		const double Declination = FromSexagesimal(49, 13, 42.48) - 0.0895 * Years / ArcsecondsPerDegree;
		const FVector J2000(CosDegrees(Declination) * CosDegrees(RightAscension), CosDegrees(Declination) * SinDegrees(RightAscension),
			SinDegrees(Declination));

		FNutation NoNutation = Nutation(JulianEphemerisDay);
		NoNutation.LongitudeDegrees = 0.0;
		NoNutation.ObliquityDegrees = 0.0;
		const FVector Precessed = TrueEquatorOfDateFromJ2000(JulianEphemerisDay, NoNutation).TransformVector(J2000);
		Near(*this, TEXT("precessed right ascension, 21.b"), ReduceDegrees(Atan2Degrees(Precessed.Y, Precessed.X)), FromHours(2, 46, 11.331), 5e-6);
		Near(*this, TEXT("precessed declination, 21.b"), AsinDegrees(Precessed.Z), FromSexagesimal(49, 20, 54.54), 5e-6);

		const FVector OfDate = TrueEquatorOfDateFromJ2000(JulianEphemerisDay, Nutation(JulianEphemerisDay)).TransformVector(J2000);
		Near(*this, TEXT("nutation in right ascension, 23.a (arcsec)"),
			(Atan2Degrees(OfDate.Y, OfDate.X) - Atan2Degrees(Precessed.Y, Precessed.X)) * ArcsecondsPerDegree, 15.843, 0.005);
		Near(*this, TEXT("nutation in declination, 23.a (arcsec)"),
			(AsinDegrees(OfDate.Z) - AsinDegrees(Precessed.Z)) * ArcsecondsPerDegree, 6.218, 0.005);

		// Over Beijing, Polaris (J2000 2h31m49.09s +89 15 51) stands within a degree of the pole: due north, as high as
		// the latitude.
		const FObserver Beijing = FObserver::At(39.90667, 116.3975, 47.0);
		const FMatrix Orientation = StarFieldOrientation(FInstant::FromUtc(FDateTime(2026, 9, 26, 12, 0, 0)), Beijing);
		Near(*this, TEXT("determinant of the star field's orientation"), Orientation.Determinant(), -1.0, 1e-9);
		const double PolarisRa = FromHours(2, 31, 49.09);
		const double PolarisDec = FromSexagesimal(89, 15, 51.0);
		const FVector Polaris(CosDegrees(PolarisDec) * CosDegrees(PolarisRa), CosDegrees(PolarisDec) * SinDegrees(PolarisRa), SinDegrees(PolarisDec));
		const FHorizontalPlace Seen = HorizontalFromDirection(Orientation.TransformVector(Polaris));
		Near(*this, TEXT("Polaris's altitude over Beijing"), Seen.AltitudeDegrees, Beijing.LatitudeDegrees, 1.0);
		Near(*this, TEXT("Polaris's azimuth over Beijing"), SignedDegrees(Seen.AzimuthDegrees), 0.0, 1.5);
		return true;
	}
}

#endif
