#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Almanac/Almanac.h"
#include "Almanac/CelestialAngles.h"
#include "Dial/Dial.h"
#include "Sun/SunModel.h"

// The museum's sun (Decision D-003) and the sun clock's gnomonics (D-002).
namespace Musee::Time::Tests
{
	/** The approved idealised sun, as Data/Time/Museum.json declares it. */
	FIdealisedSun ApprovedSun()
	{
		FIdealisedSun Sun;
		Sun.NodeHeightMeters = 20.3;
		Sun.HourRingRadiusMeters = 6.78;
		Sun.FirstHour = 6.0;
		Sun.LastHour = 18.0;
		return Sun;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeIdealisedSunTest, "Musee.Time.IdealisedSun", Flags)
	bool FTimeIdealisedSunTest::RunTest(const FString& Parameters)
	{
		const FIdealisedSun Sun = ApprovedSun();
		Near(*this, TEXT("constant altitude atan(20.3 / 6.78)"), Sun.AltitudeDegrees(), 71.531188, 1e-6);

		// The sun: east at 06:00, south at noon, west at 18:00, 15 degrees an hour.
		Near(*this, TEXT("azimuth at 06:00"), Sun.AzimuthDegrees(6.0), 90.0, 1e-9);
		Near(*this, TEXT("azimuth at 12:00"), Sun.AzimuthDegrees(12.0), 180.0, 1e-9);
		Near(*this, TEXT("azimuth at 18:00"), Sun.AzimuthDegrees(18.0), 270.0, 1e-9);
		TestTrue(TEXT("the sun at noon is in the south (+y), up (+z)"),
			Sun.Direction(12.0).Y > 0.0 && Sun.Direction(12.0).Z > 0.0 && FMath::IsNearlyZero(Sun.Direction(12.0).X, 1e-12));

		// The point of shadow: VI due west, XII due north, VI due east (as windows/.../SunClockDial.cpp lays them out),
		// and at every quarter hour on the hour ring. The shadow found by the dial's general construction, from the node
		// along the sun's direction onto the floor, is the same point.
		TestTrue(TEXT("VI in the west"), Sun.ShadowPoint(6.0).Equals(FVector(-6.78, 0.0, 0.0), 1e-9));
		TestTrue(TEXT("XII in the north"), Sun.ShadowPoint(12.0).Equals(FVector(0.0, -6.78, 0.0), 1e-9));
		TestTrue(TEXT("VI in the east"), Sun.ShadowPoint(18.0).Equals(FVector(6.78, 0.0, 0.0), 1e-9));
		const FDial Floor = FDial::Horizontal(Sun.NodeHeightMeters, 40.3);
		double LargestMiss = 0.0;
		for (double Hour = 6.0; Hour <= 18.0; Hour += 0.25)
		{
			const TOptional<FVector> Cast = DialShadowPoint(Floor, Sun.Direction(Hour));
			LargestMiss = FMath::Max(LargestMiss, (Cast.Get(FVector::ZeroVector) - Sun.ShadowPoint(Hour)).Size());
			LargestMiss = FMath::Max(LargestMiss, FMath::Abs(Sun.ShadowPoint(Hour).Size2D() - Sun.HourRingRadiusMeters));
			LargestMiss = FMath::Max(LargestMiss, FMath::Abs(Sun.ReadHour(Sun.ShadowPoint(Hour)) - Hour) * Sun.HourRingRadiusMeters);
		}
		Near(*this, TEXT("largest miss of the hour ring, 06:00-18:00 (m)"), LargestMiss, 0.0, 1e-9);
		// The ring's minute ticks are 3 cm apart (6.78 m x pi / 720), so a minute of the clock is 3 cm of shadow.
		Near(*this, TEXT("shadow travel in a minute (m)"), (Sun.ShadowPoint(12.0 + 1.0 / 60.0) - Sun.ShadowPoint(12.0)).Size(), 0.0296, 1e-4);

		TestTrue(TEXT("up at 06:00 and 18:00"), Sun.IsUp(6.0) && Sun.IsUp(18.0));
		TestFalse(TEXT("down at 05:59"), Sun.IsUp(5.99));
		TestFalse(TEXT("down at 18:01"), Sun.IsUp(18.01));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeSunModelTest, "Musee.Time.SunModel", Flags)
	bool FTimeSunModelTest::RunTest(const FString& Parameters)
	{
		// The model picks the museum's sun; the idealised one ignores the city, the real one is the almanac's.
		const FIdealisedSun Idealised = ApprovedSun();
		const FInstant Instant = FInstant::FromUtc(FDateTime(2026, 9, 26, 4, 0, 0));   // noon in Beijing
		const FSunPosition Beijing = Sun(Instant, FObserver::At(39.90667, 116.3975, 47.0));
		const FSunPosition NewYork = Sun(Instant, FObserver::At(40.7128, -74.0061, 32.0));
		const FMuseumSun ShownInBeijing = MuseumSun(ESunModel::Idealised, Idealised, 12.0, Beijing);
		const FMuseumSun ShownInNewYork = MuseumSun(ESunModel::Idealised, Idealised, 12.0, NewYork);
		TestTrue(TEXT("the idealised sun does not depend on the city"), ShownInBeijing.Direction.Equals(ShownInNewYork.Direction, 1e-12));
		const FMuseumSun Real = MuseumSun(ESunModel::Ephemeris, Idealised, 12.0, Beijing);
		TestTrue(TEXT("the ephemeris model shows the almanac's Sun"), Real.Direction.Equals(Beijing.Place.Direction, 1e-12));
		TestTrue(TEXT("the real Sun is up at Beijing's noon"), Real.bUp);
		TestFalse(TEXT("the real Sun is down at New York's midnight"), MuseumSun(ESunModel::Ephemeris, Idealised, 0.0, NewYork).bUp);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeDialLinesTest, "Musee.Time.DialLines", Flags)
	bool FTimeDialLinesTest::RunTest(const FString& Parameters)
	{
		// A horizontal dial with its nodus 20.3 m up, cut for 40.3 N (the Rotunda's floor under the real Sun).
		const double Latitude = 40.3;
		const double Height = 20.3;
		const FDial Dial = FDial::Horizontal(Height, Latitude);

		// All hour lines meet where the polar axis through the nodus pierces the floor, Height / tan(latitude) south of
		// the nodus's foot (the style points to the north celestial pole).
		const FVector Centre(0.0, Height / TanDegrees(Latitude), 0.0);
		for (double Hour = 7.0; Hour <= 17.0; Hour += 1.0)
		{
			const TOptional<FDialLine> Line = DialHourLine(Dial, Hour);
			if (!TestTrue(TEXT("each daylight hour has a line"), Line.IsSet())) { continue; }
			const FVector ToCentre = Centre - Line->PointMeters;
			const double OffLine = (ToCentre - Line->Direction * FVector::DotProduct(ToCentre, Line->Direction)).Size();
			Near(*this, FString::Printf(TEXT("hour line %.0f passes the centre (m)"), Hour), OffLine, 0.0, 1e-9);

			// The horizontal dial's classical rule: the line's angle from the noon line obeys tan(x) = sin(phi) tan(H)
			// (Waugh, Sundials: Their Theory and Construction, 1973, ch. 4).
			const double HourAngle = DegreesPerHour * (Hour - 12.0);
			const FVector Along = Line->Direction.Y < 0.0 ? Line->Direction : -Line->Direction;   // from the centre, northwards
			const double Angle = Atan2Degrees(Along.X, -Along.Y);
			const double Expected = FMath::RadiansToDegrees(FMath::Atan(SinDegrees(Latitude) * TanDegrees(HourAngle)));
			Near(*this, FString::Printf(TEXT("hour line %.0f angle from noon"), Hour), Angle, Expected, 1e-9);
		}

		// The shadow of any day falls on its hour's line and reads that hour; at equinox noon it lies Height x tan(phi)
		// north of the foot.
		double LargestError = 0.0;
		for (const double Declination : {-23.44, -10.0, 0.0, 12.0, 23.44})
		{
			for (double Hour = 8.0; Hour <= 16.0; Hour += 0.5)
			{
				const FVector TowardsSun = SunDirectionAtHourAngle(DegreesPerHour * (Hour - 12.0), Declination, Latitude);
				const TOptional<FVector> Shadow = DialShadowPoint(Dial, TowardsSun);
				if (Shadow.IsSet()) { LargestError = FMath::Max(LargestError, FMath::Abs(DialReadApparentHour(Dial, Shadow.GetValue()) - Hour)); }
			}
		}
		Near(*this, TEXT("largest reading error on the dial's own latitude (h)"), LargestError, 0.0, 1e-9);
		const TOptional<FVector> EquinoxNoon = DialShadowPoint(Dial, SunDirectionAtHourAngle(0.0, 0.0, Latitude));
		Near(*this, TEXT("equinox noon shadow north of the foot (m)"), -EquinoxNoon.Get(FVector::ZeroVector).Y, Height * TanDegrees(Latitude), 1e-9);

		// The hour-angle direction agrees with the almanac's horizon formulas.
		const FVector ByAlmanac = DirectionFromHorizontal(HorizontalFromHourAngle(-37.0, 15.0, Latitude));
		TestTrue(TEXT("the dial's sun agrees with the almanac's"), SunDirectionAtHourAngle(-37.0, 15.0, Latitude).Equals(ByAlmanac, 1e-12));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeDialTrueTest, "Musee.Time.DialTrue", Flags)
	bool FTimeDialTrueTest::RunTest(const FString& Parameters)
	{
		// Decision D-002: a dial cut for 40.3 N reads within a fraction of a degree in either city: under a minute of
		// time (a quarter of a degree of hour angle) at any hour and season.
		const FDial Dial = FDial::Horizontal(20.3, 40.3);
		const double Obliquity = 23.44;
		const double AtHome = DialReadingErrorMinutes(Dial, 40.3, Obliquity);
		const double InBeijing = DialReadingErrorMinutes(Dial, 39.90667, Obliquity);
		const double InNewYork = DialReadingErrorMinutes(Dial, 40.7128, Obliquity);
		const double InLondon = DialReadingErrorMinutes(Dial, 51.50722, Obliquity);
		Near(*this, TEXT("reading error at the dial's latitude (min)"), AtHome, 0.0, 1e-6);
		Near(*this, TEXT("reading error in Beijing (min)"), InBeijing, 0.0, 1.0);
		Near(*this, TEXT("reading error in New York (min)"), InNewYork, 0.0, 1.0);
		TestTrue(TEXT("London, 11 degrees off, reads far worse"), InLondon > 5.0 * FMath::Max(InBeijing, InNewYork));
		AddInfo(FString::Printf(TEXT("Reading errors: Beijing %.2f min, New York %.2f min, London %.2f min"), InBeijing, InNewYork, InLondon));
		return true;
	}
}

#endif
