#include "Almanac/AstronomicalTime.h"

namespace Musee::Time::DeltaTRecords
{
	/** TT - TAI, fixed by definition (IAU 1991 Resolution A4, carried into TT by IAU 2000 B1.9). */
	constexpr double TerrestrialMinusAtomicSeconds = 32.184;

	/** TAI - UTC from the first day of each month listed: the leap seconds of IERS Bulletin C (the last in 2016). */
	struct FLeapSecondStep
	{
		int32 Year;
		int32 Month;
		int32 AtomicMinusUtcSeconds;
	};

	constexpr FLeapSecondStep LeapSecondSteps[] = {
		{1972, 1, 10}, {1972, 7, 11}, {1973, 1, 12}, {1974, 1, 13}, {1975, 1, 14}, {1976, 1, 15}, {1977, 1, 16},
		{1978, 1, 17}, {1979, 1, 18}, {1980, 1, 19}, {1981, 7, 20}, {1982, 7, 21}, {1983, 7, 22}, {1985, 7, 23},
		{1988, 1, 24}, {1990, 1, 25}, {1991, 1, 26}, {1992, 7, 27}, {1993, 7, 28}, {1994, 7, 29}, {1996, 1, 30},
		{1997, 7, 31}, {1999, 1, 32}, {2006, 1, 33}, {2009, 1, 34}, {2012, 7, 35}, {2015, 7, 36}, {2017, 1, 37},
	};

	/** The year at the middle of the instant's month, the argument of the Espenak and Meeus fits. */
	double DecimalYear(const FDateTime& Utc)
	{
		constexpr double MonthsPerYear = 12.0;
		return Utc.GetYear() + (Utc.GetMonth() - 0.5) / MonthsPerYear;
	}

	/** Espenak and Meeus (2006) for 1900-1972, and Morrison and Stephenson (2004) before 1900. */
	double FittedDeltaTSeconds(double Year)
	{
		if (Year < 1900.0)
		{
			const double U = (Year - 1820.0) / 100.0;
			return -20.0 + 32.0 * U * U;
		}
		if (Year < 1920.0)
		{
			const double T = Year - 1900.0;
			return -2.79 + 1.494119 * T - 0.0598939 * T * T + 0.0061966 * T * T * T - 0.000197 * T * T * T * T;
		}
		if (Year < 1941.0)
		{
			const double T = Year - 1920.0;
			return 21.20 + 0.84493 * T - 0.076100 * T * T + 0.0020936 * T * T * T;
		}
		if (Year < 1961.0)
		{
			const double T = Year - 1950.0;
			return 29.07 + 0.407 * T - T * T / 233.0 + T * T * T / 2547.0;
		}
		const double T = Year - 1975.0;
		return 45.45 + 1.067 * T - T * T / 260.0 - T * T * T / 718.0;
	}
}

namespace Musee::Time
{
	double JulianDayFromUtc(const FDateTime& Utc)
	{
		return Utc.GetJulianDay();
	}

	FDateTime UtcFromJulianDay(double JulianDay)
	{
		// FDateTime counts 100 ns ticks from 0001-01-01 00:00, which is Julian Day 1721425.5. Whole days and the
		// fraction are converted apart so the tick count keeps the Julian Day's own precision.
		constexpr double DateTimeEpochJulianDay = 1721425.5;
		const double Days = JulianDay - DateTimeEpochJulianDay;
		const double WholeDays = FMath::FloorToDouble(Days);
		const int64 FractionTicks = FMath::RoundToInt64((Days - WholeDays) * static_cast<double>(ETimespan::TicksPerDay));
		return FDateTime(static_cast<int64>(WholeDays) * ETimespan::TicksPerDay + FractionTicks);
	}

	double DeltaTSeconds(const FDateTime& Utc)
	{
		using namespace DeltaTRecords;
		const FDateTime FirstStep(LeapSecondSteps[0].Year, LeapSecondSteps[0].Month, 1);
		if (Utc < FirstStep)
		{
			return FittedDeltaTSeconds(DecimalYear(Utc));
		}
		int32 AtomicMinusUtc = LeapSecondSteps[0].AtomicMinusUtcSeconds;
		for (const FLeapSecondStep& Step : LeapSecondSteps)
		{
			if (Utc < FDateTime(Step.Year, Step.Month, 1)) { break; }
			AtomicMinusUtc = Step.AtomicMinusUtcSeconds;
		}
		return TerrestrialMinusAtomicSeconds + AtomicMinusUtc;
	}

	FInstant FInstant::FromUtc(const FDateTime& Utc)
	{
		return FromUtc(Utc, Musee::Time::DeltaTSeconds(Utc));
	}

	FInstant FInstant::FromUtc(const FDateTime& Utc, double InDeltaTSeconds)
	{
		FInstant Instant;
		Instant.JulianDayUniversal = JulianDayFromUtc(Utc);
		Instant.DeltaTSeconds = InDeltaTSeconds;
		return Instant;
	}

	FInstant FInstant::FromTerrestrial(double JulianEphemerisDay, double InDeltaTSeconds)
	{
		FInstant Instant;
		Instant.JulianDayUniversal = JulianEphemerisDay - InDeltaTSeconds / SecondsPerDay;
		Instant.DeltaTSeconds = InDeltaTSeconds;
		return Instant;
	}

	double FInstant::JulianEphemerisDay() const
	{
		return JulianDayUniversal + DeltaTSeconds / SecondsPerDay;
	}

	FDateTime FInstant::Utc() const
	{
		return UtcFromJulianDay(JulianDayUniversal);
	}
}
