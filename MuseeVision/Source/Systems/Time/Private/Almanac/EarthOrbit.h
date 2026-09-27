#pragma once

#include "CoreMinimal.h"

/**
 * Where the Earth is on its orbit, seen from the Sun. The Sun's place in the museum's sky is this position turned
 * round: the Earth's heliocentric longitude plus 180 degrees.
 */
namespace Musee::Time
{
	/** Heliocentric ecliptic coordinates, referred to the mean equinox and ecliptic of the date (VSOP87D). */
	struct FHeliocentricPlace
	{
		double LongitudeDegrees = 0.0;
		double LatitudeDegrees = 0.0;
		/** The distance from the Sun's centre, in astronomical units. */
		double RadiusAu = 1.0;
	};

	/**
	 * The Earth at a Julian Ephemeris Day, from the VSOP87 theory (Bretagnon and Francou 1988) as truncated in Meeus,
	 * Astronomical Algorithms, 2nd ed., appendix III: about 1 arcsecond for the Sun's place over 2000 BC to AD 6000.
	 * The NREL SPA (Reda and Andreas 2008, table A4.2) uses the same series.
	 */
	FHeliocentricPlace EarthHeliocentric(double JulianEphemerisDay);
}
