#pragma once

#include "CoreMinimal.h"
#include "Almanac/Nutation.h"

/**
 * The Moon's place as seen from the Earth's centre. The museum's moon, its phase and its light always come from
 * here, whatever the sun model (Decision D-003).
 */
namespace Musee::Time
{
	struct FLunarCoordinates
	{
		/** Nutation and obliquity at the instant. */
		FNutation Nutation;
		/** Geocentric ecliptic longitude and latitude, mean equinox of the date (Meeus ch. 47). */
		double LongitudeDegrees = 0.0;
		double LatitudeDegrees = 0.0;
		/** The distance between the centres of the Earth and the Moon. */
		double DistanceKilometers = 0.0;
		/** Equatorial horizontal parallax, sin(pi) = 6378.14 km / distance (Meeus ch. 47). */
		double HorizontalParallaxDegrees = 0.0;
		/** Apparent longitude: the longitude plus the nutation in longitude (Meeus ch. 47). */
		double ApparentLongitudeDegrees = 0.0;
		/** Apparent geocentric right ascension and declination, on the true equator and equinox of the date. */
		double RightAscensionDegrees = 0.0;
		double DeclinationDegrees = 0.0;

		/** The geocentric semidiameter: 358473400 / distance arcseconds (Meeus ch. 55). */
		double SemidiameterDegrees() const;
	};

	/**
	 * The Moon at a Julian Ephemeris Day, from the ELP-2000/82 theory (Chapront-Touze and Chapront 1983) as
	 * truncated by Meeus (Astronomical Algorithms, 2nd ed., ch. 47, tables 47.A and 47.B): about 10 arcseconds in
	 * longitude and 4 arcseconds in latitude.
	 */
	FLunarCoordinates LunarCoordinates(double JulianEphemerisDay);
}
