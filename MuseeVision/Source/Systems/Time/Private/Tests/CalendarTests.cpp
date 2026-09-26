#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Almanac/SolarCoordinates.h"
#include "Calendar/Season.h"
#include "Calendar/SolarTerms.h"

// The calendar: the 24 solar terms and the seasons, from the real Sun.
namespace Musee::Time::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeSolarTermsTest, "Musee.Time.SolarTerms", Flags)
	bool FTimeSolarTermsTest::RunTest(const FString& Parameters)
	{
		// The terms by the Sun's longitude: Lichun (0) at 315 degrees, Chunfen (3) at the March equinox, Xiazhi (9) at
		// the June solstice, Dongzhi (21) at the December solstice, Dahan (23) ending at 315.
		TestEqual(TEXT("Lichun begins at 315"), SolarTermAt(315.0).Index, 0);
		TestEqual(TEXT("Dahan ends at 315"), SolarTermAt(314.999).Index, 23);
		TestEqual(TEXT("Chunfen at the March equinox"), SolarTermAt(0.0).Index, 3);
		TestEqual(TEXT("Xiazhi at the June solstice"), SolarTermAt(90.0).Index, 9);
		TestEqual(TEXT("Dongzhi at the December solstice"), SolarTermAt(270.0).Index, 21);
		Near(*this, TEXT("halfway through Qingming"), SolarTermAt(22.5).Progress, 0.5, 1e-12);
		for (int32 Index = 0; Index < SolarTermCount; ++Index)
		{
			TestEqual(TEXT("each term starts at its own longitude"), SolarTermAt(SolarTermStartLongitudeDegrees(Index)).Index, Index);
		}

		// An hour after the 2026 March equinox (USNO: 2026-03-20 14:46 UT) the term is Chunfen; it began at the equinox
		// and ends when the Sun reaches 15 degrees, about 15.2 days on.
		const FSolarTermSpan Span = SolarTermSpan(FInstant::FromUtc(FDateTime(2026, 3, 20, 15, 46, 0)));
		TestEqual(TEXT("the term an hour after the equinox"), Span.Place.Index, 3);
		NearInstant(*this, TEXT("Chunfen 2026 began at the USNO equinox"), Span.StartUtc, FDateTime(2026, 3, 20, 14, 46, 0), 45.0);
		Near(*this, TEXT("Chunfen 2026 lasts (days)"), (Span.EndUtc - Span.StartUtc).GetTotalDays(), 15.2, 0.3);
		Near(*this, TEXT("the Sun's longitude when Chunfen ends"),
			SolarCoordinates(FInstant::FromUtc(Span.EndUtc).JulianEphemerisDay()).ApparentLongitudeDegrees, 15.0, 1e-5);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeSeasonsTest, "Musee.Time.Seasons", Flags)
	bool FTimeSeasonsTest::RunTest(const FString& Parameters)
	{
		const FDateTime Date(2026, 2, 10);
		const double Beijing = 39.9;
		// Early February, the Sun near 321 degrees: winter by the equinoxes, spring by the solar terms (after Lichun),
		// winter by the months.
		TestTrue(TEXT("astronomical: winter"), SeasonAt(ESeasonReckoning::Astronomical, 321.0, Date, Beijing).Season == ESeason::Winter);
		TestTrue(TEXT("solar terms: spring"), SeasonAt(ESeasonReckoning::SolarTerms, 321.0, Date, Beijing).Season == ESeason::Spring);
		TestTrue(TEXT("meteorological: winter"), SeasonAt(ESeasonReckoning::Meteorological, 321.0, Date, Beijing).Season == ESeason::Winter);
		// South of the equator the same day is summer, astronomically.
		TestTrue(TEXT("southern hemisphere: summer"), SeasonAt(ESeasonReckoning::Astronomical, 321.0, Date, -33.9).Season == ESeason::Summer);

		// Progress: halfway between the June solstice and the September equinox; mid-January is halfway through the
		// meteorological winter (1 December to 1 March: 45 of 90 days).
		Near(*this, TEXT("astronomical summer halfway"), SeasonAt(ESeasonReckoning::Astronomical, 135.0, Date, Beijing).Progress, 0.5, 1e-12);
		Near(*this, TEXT("meteorological winter on 15 January"),
			SeasonAt(ESeasonReckoning::Meteorological, 0.0, FDateTime(2026, 1, 15), Beijing).Progress, 0.5, 1e-12);
		TestTrue(TEXT("solar terms: summer begins at Lixia (45)"), SeasonAt(ESeasonReckoning::SolarTerms, 45.0, Date, Beijing).Season == ESeason::Summer);
		TestTrue(TEXT("solar terms: winter begins at Lidong (225)"), SeasonAt(ESeasonReckoning::SolarTerms, 225.0, Date, Beijing).Season == ESeason::Winter);
		return true;
	}
}

#endif
