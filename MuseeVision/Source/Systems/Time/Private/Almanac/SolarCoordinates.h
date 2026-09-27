#pragma once

#include "CoreMinimal.h"
#include "Almanac/EarthOrbit.h"
#include "Almanac/Nutation.h"

/**
 * The Sun's place as seen from the Earth's centre: where it stands on the ecliptic and on the celestial sphere, and
 * how far apparent solar time runs ahead of mean time. This is the real Sun. Whichever sun model the museum shows
 * (Decision D-003), the solar terms, the seasons and the Moon's phase always follow this one.
 */
namespace Musee::Time
{
	/** The tropical year, 365.242189 days (Meeus ch. 27), and so the Sun's mean motion in longitude. */
	inline constexpr double TropicalYearDays = 365.242189;
	inline constexpr double SolarMeanMotionDegreesPerDay = 360.0 / TropicalYearDays;

	struct FSolarCoordinates
	{
		/** The Earth's heliocentric place (VSOP87, Meeus appendix III). */
		FHeliocentricPlace Earth;
		/** Nutation and obliquity at the instant. */
		FNutation Nutation;
		/** Geometric longitude and latitude of the Sun, referred to the FK5 system (Meeus eq. 25.9). */
		double GeometricLongitudeDegrees = 0.0;
		double LatitudeDegrees = 0.0;
		/** Apparent longitude: corrected for nutation and aberration (Meeus eq. 25.10). */
		double ApparentLongitudeDegrees = 0.0;
		/** Apparent geocentric right ascension and declination, on the true equator and equinox of the date. */
		double RightAscensionDegrees = 0.0;
		double DeclinationDegrees = 0.0;
		/** The Sun's mean longitude L0 (Meeus eq. 28.2). */
		double MeanLongitudeDegrees = 0.0;
		/** Apparent minus mean solar time, in minutes (Meeus eq. 28.3; NREL SPA eq. A1). */
		double EquationOfTimeMinutes = 0.0;

		double DistanceAu() const { return Earth.RadiusAu; }
		/** The Sun's equatorial horizontal parallax: 8.794 arcseconds at 1 au (Meeus ch. 40; NREL SPA eq. 33). */
		double HorizontalParallaxDegrees() const;
		/** The angular semidiameter: 959.63 arcseconds at 1 au (the Astronomical Almanac; Meeus ch. 55). */
		double SemidiameterDegrees() const;
	};

	/** The Sun at a Julian Ephemeris Day: full precision, about 1 arcsecond (Meeus ch. 25, "higher accuracy"). */
	FSolarCoordinates SolarCoordinates(double JulianEphemerisDay);

	/**
	 * The Julian Ephemeris Day at which the Sun's apparent longitude reaches TargetDegrees, the crossing nearest the
	 * guess (within half a year of it). Equinoxes and solstices are the crossings of 0, 90, 180 and 270 degrees
	 * (Meeus ch. 27); the solar terms begin every 15 degrees.
	 */
	double SolarLongitudeInstant(double TargetDegrees, double JulianEphemerisDayGuess);
}
