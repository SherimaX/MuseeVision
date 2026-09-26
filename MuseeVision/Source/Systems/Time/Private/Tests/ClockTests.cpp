#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Clock/MuseumClock.h"
#include "Clock/ZoneRule.h"

// Civil time in the museum's cities and the museum's one clock.
namespace Musee::Time::Tests
{
	FZoneRule BeijingZone()
	{
		FZoneRule Zone;
		Zone.StandardOffsetHours = 8.0;
		return Zone;
	}

	FZoneRule NewYorkZone()
	{
		FZoneRule Zone;
		Zone.StandardOffsetHours = -5.0;
		Zone.DaylightSaving = EDaylightSaving::UnitedStates;
		return Zone;
	}

	FZoneRule LondonZone()
	{
		FZoneRule Zone;
		Zone.DaylightSaving = EDaylightSaving::European;
		return Zone;
	}

	double OffsetHours(const FZoneRule& Zone, const FDateTime& Utc)
	{
		return Zone.UtcOffset(Utc).GetTotalHours();
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeZoneRulesTest, "Musee.Time.ZoneRules", Flags)
	bool FTimeZoneRulesTest::RunTest(const FString& Parameters)
	{
		// Beijing keeps UTC+8 all year.
		Near(*this, TEXT("Beijing in January"), OffsetHours(BeijingZone(), FDateTime(2026, 1, 15)), 8.0, 0.0);
		Near(*this, TEXT("Beijing in July"), OffsetHours(BeijingZone(), FDateTime(2026, 7, 15)), 8.0, 0.0);

		// New York, 2026: EDT from 02:00 EST on Sunday 8 March (07:00 UTC) to 02:00 EDT on Sunday 1 November (06:00 UTC).
		const FZoneRule NewYork = NewYorkZone();
		Near(*this, TEXT("New York just before spring forward"), OffsetHours(NewYork, FDateTime(2026, 3, 8, 6, 59, 59)), -5.0, 0.0);
		Near(*this, TEXT("New York at spring forward"), OffsetHours(NewYork, FDateTime(2026, 3, 8, 7, 0, 0)), -4.0, 0.0);
		Near(*this, TEXT("New York just before fall back"), OffsetHours(NewYork, FDateTime(2026, 11, 1, 5, 59, 59)), -4.0, 0.0);
		Near(*this, TEXT("New York at fall back"), OffsetHours(NewYork, FDateTime(2026, 11, 1, 6, 0, 0)), -5.0, 0.0);
		// 2003, under the 1987-2006 rule: the first Sunday of April (6 April) to the last Sunday of October (26 October).
		Near(*this, TEXT("New York on 2003-04-06 06:59 UTC"), OffsetHours(NewYork, FDateTime(2003, 4, 6, 6, 59, 0)), -5.0, 0.0);
		Near(*this, TEXT("New York on 2003-04-06 07:00 UTC"), OffsetHours(NewYork, FDateTime(2003, 4, 6, 7, 0, 0)), -4.0, 0.0);
		Near(*this, TEXT("New York on 2003-10-26 06:00 UTC"), OffsetHours(NewYork, FDateTime(2003, 10, 26, 6, 0, 0)), -5.0, 0.0);

		// London, 2026: BST from 01:00 UTC on Sunday 29 March to 01:00 UTC on Sunday 25 October.
		const FZoneRule London = LondonZone();
		Near(*this, TEXT("London before summer time"), OffsetHours(London, FDateTime(2026, 3, 29, 0, 59, 59)), 0.0, 0.0);
		Near(*this, TEXT("London in summer time"), OffsetHours(London, FDateTime(2026, 3, 29, 1, 0, 0)), 1.0, 0.0);
		Near(*this, TEXT("London after summer time"), OffsetHours(London, FDateTime(2026, 10, 25, 1, 0, 0)), 0.0, 0.0);

		// Civil to UTC: the spring gap is read on standard time; the doubled autumn hour takes its first occurrence.
		NearInstant(*this, TEXT("New York 02:30 on the spring-forward day"), NewYork.UtcFromCivil(FDateTime(2026, 3, 8, 2, 30, 0)),
			FDateTime(2026, 3, 8, 7, 30, 0), 0.0);
		NearInstant(*this, TEXT("New York 01:30 on the fall-back day"), NewYork.UtcFromCivil(FDateTime(2026, 11, 1, 1, 30, 0)),
			FDateTime(2026, 11, 1, 5, 30, 0), 0.0);
		NearInstant(*this, TEXT("Beijing noon"), BeijingZone().UtcFromCivil(FDateTime(2026, 9, 26, 12, 0, 0)), FDateTime(2026, 9, 26, 4, 0, 0), 0.0);
		NearInstant(*this, TEXT("New York civil midnight in summer"), NewYork.CivilMidnightUtc(FDateTime(2026, 7, 4, 15, 0, 0)),
			FDateTime(2026, 7, 4, 4, 0, 0), 0.0);
		NearInstant(*this, TEXT("round trip through civil time"), NewYork.UtcFromCivil(NewYork.CivilFromUtc(FDateTime(2026, 7, 4, 18, 45, 0))),
			FDateTime(2026, 7, 4, 18, 45, 0), 0.0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeMuseumClockTest, "Musee.Time.MuseumClock", Flags)
	bool FTimeMuseumClockTest::RunTest(const FString& Parameters)
	{
		const FDateTime Real(2026, 9, 26, 3, 0, 0);   // 11:00 in Beijing
		FMuseumClock Clock(BeijingZone());
		TestTrue(TEXT("a new clock is live"), Clock.IsLive());
		NearInstant(*this, TEXT("live is the real time"), Clock.MuseumUtc(Real), Real, 0.0);

		// Set to 16:00, the clock stands at 16:00 of today's civil date in Beijing and runs on at real speed.
		Clock.SetHour(16.0, Real);
		NearInstant(*this, TEXT("set to 16:00"), Clock.MuseumCivil(Real), FDateTime(2026, 9, 26, 16, 0, 0), 0.0);
		NearInstant(*this, TEXT("half an hour later"), Clock.MuseumCivil(Real + FTimespan::FromMinutes(30)), FDateTime(2026, 9, 26, 16, 30, 0), 1e-3);

		// A rate of 0 holds it; a rate of 60 runs a minute a second.
		Clock.SetRate(0.0, Real + FTimespan::FromMinutes(30));
		NearInstant(*this, TEXT("held"), Clock.MuseumCivil(Real + FTimespan::FromHours(5)), FDateTime(2026, 9, 26, 16, 30, 0), 1e-3);
		Clock.SetRate(60.0, Real + FTimespan::FromHours(5));
		NearInstant(*this, TEXT("a minute a second"), Clock.MuseumCivil(Real + FTimespan::FromHours(5) + FTimespan::FromSeconds(10)),
			FDateTime(2026, 9, 26, 16, 40, 0), 1e-3);

		// The museum moves to New York: the set clock keeps its civil reading, so New York's sky shows the same hour.
		const FDateTime Moved = Real + FTimespan::FromHours(6);
		const FDateTime Before = Clock.MuseumCivil(Moved);
		Clock.SetZone(NewYorkZone(), Moved);
		NearInstant(*this, TEXT("the civil reading survives the move"), Clock.MuseumCivil(Moved), Before, 1e-3);

		// The hour key: live, then 10, 16, 21 and back to live (Data/Time/Museum.json).
		const double Stops[] = {10.0, 16.0, 21.0};
		FMuseumClock Key(BeijingZone());
		Key.AdvanceHour(Stops, Real);
		Near(*this, TEXT("first press"), Key.ChosenHour().Get(-1.0), 10.0, 0.0);
		Key.AdvanceHour(Stops, Real);
		Near(*this, TEXT("second press"), Key.ChosenHour().Get(-1.0), 16.0, 0.0);
		Key.AdvanceHour(Stops, Real);
		Near(*this, TEXT("third press"), Key.ChosenHour().Get(-1.0), 21.0, 0.0);
		Key.AdvanceHour(Stops, Real);
		TestTrue(TEXT("fourth press is live again"), Key.IsLive());
		Key.SetHour(13.0, Real);
		Key.AdvanceHour(Stops, Real);
		Near(*this, TEXT("from a console hour, the next stop after it"), Key.ChosenHour().Get(-1.0), 16.0, 0.0);
		return true;
	}
}

#endif
