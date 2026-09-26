#pragma once

#include "CoreMinimal.h"

/**
 * Sidereal time: how far the Earth has turned against the stars. The local sidereal time is the right ascension on
 * the observer's meridian, so it sets the star field's rotation and every hour angle.
 */
namespace Musee::Time
{
	struct FNutation;

	/** Greenwich mean sidereal time at a Julian Day on Universal Time (Meeus eq. 12.4; IAU 1982), in [0, 360). */
	double MeanSiderealDegrees(double JulianDayUniversal);

	/** Greenwich apparent sidereal time: the mean, plus the equation of the equinoxes (delta psi cos epsilon). */
	double ApparentSiderealDegrees(double JulianDayUniversal, const FNutation& Nutation);
}
