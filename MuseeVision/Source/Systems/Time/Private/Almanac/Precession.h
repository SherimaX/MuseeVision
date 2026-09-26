#pragma once

#include "CoreMinimal.h"

/**
 * Precession and nutation carry a star catalogue's frame to the sky of the date. Star catalogues give places on the
 * equator and equinox of J2000.0; the Earth's axis has turned since (about 0.36 degrees by 2026), so the stars must
 * be carried to the true equator and equinox of the date before the sidereal time turns them over the observer.
 */
namespace Musee::Time
{
	struct FNutation;

	/**
	 * The rotation from the J2000.0 mean equator and equinox to the true equator and equinox of a date: IAU 1976
	 * precession (Lieske et al. 1977; Meeus eqs. 21.3) followed by nutation (the rotation R1(-eps) R3(-dpsi) R1(eps0),
	 * Explanatory Supplement to the Astronomical Almanac, 1992, sec. 3.222). As a row-vector FMatrix: each row is the
	 * image of one J2000 axis, so TransformVector(J2000 unit vector) gives the vector on the true equator of date.
	 */
	FMatrix TrueEquatorOfDateFromJ2000(double JulianEphemerisDay, const FNutation& Nutation);
}
