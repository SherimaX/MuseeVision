#pragma once

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Time::Tests
{
	/** Every Time test runs wherever automation runs (editor, commandlet, game) and belongs to the project's tests. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/**
	 * Checks a measured value against its reference within a stated tolerance. Every comparison is reported, passing or
	 * not, so the test report records how far each result lies from its published value.
	 */
	inline bool Near(FAutomationTestBase& Test, const FString& What, double Actual, double Expected, double Tolerance)
	{
		const double Off = Actual - Expected;
		const bool bWithin = FMath::Abs(Off) <= Tolerance;
		const FString Line = FString::Printf(TEXT("%s: %.10g against %.10g, off by %.3g (tolerance %.3g)"),
			*What, Actual, Expected, Off, Tolerance);
		if (bWithin)
		{
			Test.AddInfo(Line);
		}
		else
		{
			Test.AddError(Line);
		}
		return bWithin;
	}

	/** Degrees from degrees, minutes and seconds of arc. */
	inline double FromSexagesimal(double Whole, double Minutes, double Seconds)
	{
		const double Sign = Whole < 0.0 ? -1.0 : 1.0;
		return Sign * (FMath::Abs(Whole) + Minutes / 60.0 + Seconds / 3600.0);
	}

	/** Degrees from hours, minutes and seconds of right ascension or sidereal time (15 degrees an hour). */
	inline double FromHours(double Hours, double Minutes, double Seconds)
	{
		return 15.0 * FromSexagesimal(Hours, Minutes, Seconds);
	}

	/** Seconds between two instants, for comparing event times. */
	inline double SecondsBetween(const FDateTime& Actual, const FDateTime& Expected)
	{
		return (Actual - Expected).GetTotalSeconds();
	}

	/** Checks an event's instant against its reference within a tolerance in seconds. */
	inline bool NearInstant(FAutomationTestBase& Test, const FString& What, const FDateTime& Actual, const FDateTime& Expected,
		double ToleranceSeconds)
	{
		return Near(Test, What + TEXT(" (s from ") + Expected.ToIso8601() + TEXT(")"), SecondsBetween(Actual, Expected), 0.0,
			ToleranceSeconds);
	}
}

#endif
