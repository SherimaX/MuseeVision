#include "Clock/ZoneRule.h"

namespace Musee::Time::DaylightSavingLaw
{
	/** Both laws advance the clocks by one hour. */
	constexpr double ShiftHours = 1.0;
	constexpr int32 DaysPerWeek = 7;

	/** The Nth Sunday (from 1) of a month, or its last Sunday when Nth is 0, at 00:00. */
	FDateTime SundayOfMonth(int32 Year, int32 Month, int32 Nth)
	{
		if (Nth > 0)
		{
			FDateTime Day(Year, Month, 1);
			while (Day.GetDayOfWeek() != EDayOfWeek::Sunday) { Day += FTimespan::FromDays(1); }
			return Day + FTimespan::FromDays(DaysPerWeek * (Nth - 1));
		}
		FDateTime Day(Year, Month, FDateTime::DaysInMonth(Year, Month));
		while (Day.GetDayOfWeek() != EDayOfWeek::Sunday) { Day -= FTimespan::FromDays(1); }
		return Day;
	}

	/** Daylight time in a year, as a span of UTC. */
	struct FSpan
	{
		FDateTime StartUtc;
		FDateTime EndUtc;
	};

	/** The United States rule for a year: the change is at 02:00 local time, standard in spring, daylight in autumn. */
	TOptional<FSpan> UnitedStatesSpan(int32 Year, double StandardOffsetHours)
	{
		struct FRule { int32 FirstYear; int32 StartMonth; int32 StartNth; int32 EndMonth; int32 EndNth; };
		constexpr FRule Rules[] = {{2007, 3, 2, 11, 1}, {1987, 4, 1, 10, 0}, {1967, 4, 0, 10, 0}};
		constexpr double ChangeHourLocal = 2.0;
		for (const FRule& Rule : Rules)
		{
			if (Year < Rule.FirstYear) { continue; }
			FSpan Span;
			Span.StartUtc = SundayOfMonth(Year, Rule.StartMonth, Rule.StartNth)
				+ FTimespan::FromHours(ChangeHourLocal - StandardOffsetHours);
			Span.EndUtc = SundayOfMonth(Year, Rule.EndMonth, Rule.EndNth)
				+ FTimespan::FromHours(ChangeHourLocal - StandardOffsetHours - ShiftHours);
			return Span;
		}
		return {};
	}

	/** The European rule for a year: the change is at 01:00 UTC everywhere it applies. */
	FSpan EuropeanSpan(int32 Year)
	{
		constexpr double ChangeHourUtc = 1.0;
		constexpr int32 March = 3;
		constexpr int32 October = 10;
		FSpan Span;
		Span.StartUtc = SundayOfMonth(Year, March, 0) + FTimespan::FromHours(ChangeHourUtc);
		Span.EndUtc = SundayOfMonth(Year, October, 0) + FTimespan::FromHours(ChangeHourUtc);
		return Span;
	}
}

namespace Musee::Time
{
	bool FZoneRule::IsDaylightSaving(const FDateTime& Utc) const
	{
		using namespace DaylightSavingLaw;
		TOptional<FSpan> Span;
		switch (DaylightSaving)
		{
		case EDaylightSaving::UnitedStates: Span = UnitedStatesSpan(Utc.GetYear(), StandardOffsetHours); break;
		case EDaylightSaving::European: Span = EuropeanSpan(Utc.GetYear()); break;
		case EDaylightSaving::None: break;
		}
		return Span.IsSet() && Utc >= Span->StartUtc && Utc < Span->EndUtc;
	}

	FTimespan FZoneRule::UtcOffset(const FDateTime& Utc) const
	{
		const double Hours = StandardOffsetHours + (IsDaylightSaving(Utc) ? DaylightSavingLaw::ShiftHours : 0.0);
		return FTimespan::FromHours(Hours);
	}

	FDateTime FZoneRule::CivilFromUtc(const FDateTime& Utc) const
	{
		return Utc + UtcOffset(Utc);
	}

	FDateTime FZoneRule::UtcFromCivil(const FDateTime& Civil) const
	{
		const FDateTime ReadAsStandard = Civil - FTimespan::FromHours(StandardOffsetHours);
		const FDateTime ReadAsDaylight = ReadAsStandard - FTimespan::FromHours(DaylightSavingLaw::ShiftHours);
		return IsDaylightSaving(ReadAsDaylight) ? ReadAsDaylight : ReadAsStandard;
	}

	FDateTime FZoneRule::CivilMidnightUtc(const FDateTime& CivilDate) const
	{
		return UtcFromCivil(CivilDate.GetDate());
	}
}
