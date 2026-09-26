#pragma once

#include "CoreMinimal.h"

/**
 * The almanac's clocks. Astronomy counts time in Julian Days and keeps two scales: Universal Time, which follows the
 * turning Earth and so sets the sky's rotation, and Terrestrial Time (TT), the uniform clock the ephemerides of the
 * sun and moon run on. Their difference is Delta T = TT - UT, which grows as the Earth's rotation slows.
 *
 * The museum reads UTC. UTC is kept within 0.9 s of UT1 by leap seconds (IERS), so UTC stands for UT here: at 0.9 s
 * the sky is turned by 0.004 degrees, well inside the almanac's 0.01 degree.
 */
namespace Musee::Time
{
	/** The J2000.0 epoch, 2000 January 1 at 12h TT, as a Julian Day (Meeus, Astronomical Algorithms, 2nd ed., ch. 21). */
	inline constexpr double J2000JulianDay = 2451545.0;
	/** Days in a Julian century and in a Julian millennium (Meeus ch. 21 and 32). */
	inline constexpr double DaysPerJulianCentury = 36525.0;
	inline constexpr double DaysPerJulianMillennium = 365250.0;
	/** The day of 24 hours of 60 minutes of 60 SI seconds. */
	inline constexpr double HoursPerDay = 24.0;
	inline constexpr double MinutesPerHour = 60.0;
	inline constexpr double SecondsPerHour = 3600.0;
	inline constexpr double SecondsPerDay = 86400.0;

	/** The Julian Day of a UTC instant (proleptic Gregorian calendar; Meeus ch. 7). */
	double JulianDayFromUtc(const FDateTime& Utc);

	/** The UTC instant of a Julian Day, to the nearest 100 ns tick. */
	FDateTime UtcFromJulianDay(double JulianDay);

	/**
	 * Delta T = TT - UT in seconds at a UTC instant.
	 *  - From 1972: 32.184 s (TT - TAI, by definition) plus TAI - UTC from the IERS leap-second table (Bulletin C).
	 *    The error is UT1 - UTC, which the IERS keeps within 0.9 s.
	 *  - 1900 to 1972: the polynomial fits to the observed Delta T of Espenak and Meeus (NASA, "Polynomial Expressions
	 *    for Delta T", 2006), good to about a second.
	 *  - Before 1900: the long-term parabola of Morrison and Stephenson (2004), -20 + 32 u^2 s with u in centuries
	 *    from 1820, a mean good to about a minute over recent centuries.
	 */
	double DeltaTSeconds(const FDateTime& Utc);

	/** One instant on both of the almanac's clocks. */
	struct FInstant
	{
		/** The Julian Day on Universal Time. */
		double JulianDayUniversal = J2000JulianDay;
		/** TT - UT at the instant. */
		double DeltaTSeconds = 0.0;

		/** A UTC instant, with Delta T from the model above. */
		static FInstant FromUtc(const FDateTime& Utc);
		/** A UTC instant with a given Delta T (a published example states its own). */
		static FInstant FromUtc(const FDateTime& Utc, double DeltaTSeconds);
		/** An instant given on Terrestrial Time (the scale Meeus's worked examples are stated in). */
		static FInstant FromTerrestrial(double JulianEphemerisDay, double DeltaTSeconds);

		/** The Julian Ephemeris Day: the same instant on Terrestrial Time. */
		double JulianEphemerisDay() const;
		/** The UTC instant, to the nearest tick. */
		FDateTime Utc() const;
	};
}
