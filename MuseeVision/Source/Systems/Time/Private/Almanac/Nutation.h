#pragma once

#include "CoreMinimal.h"

/**
 * The wobble of the Earth's axis. The Moon and Sun pull on the Earth's equatorial bulge, so the celestial pole nods
 * (nutation) about its slow precession, and the tilt of the axis to the ecliptic (the obliquity) changes with it.
 * Every apparent position and the apparent sidereal time carry these corrections.
 */
namespace Musee::Time
{
	struct FNutation
	{
		/** Nutation in longitude, delta psi. */
		double LongitudeDegrees = 0.0;
		/** Nutation in obliquity, delta epsilon. */
		double ObliquityDegrees = 0.0;
		/** The mean obliquity of the ecliptic, epsilon zero (Laskar 1986, Meeus eq. 22.3). */
		double MeanObliquityDegrees = 0.0;

		/** The true obliquity, epsilon = epsilon zero + delta epsilon. */
		double TrueObliquityDegrees() const { return MeanObliquityDegrees + ObliquityDegrees; }
	};

	/**
	 * Nutation and obliquity at a Julian Ephemeris Day: the 63 terms of the IAU 1980 theory as Meeus gives them
	 * (Astronomical Algorithms, 2nd ed., ch. 22, table 22.A), good to about 0.0003 arcseconds of the full theory
	 * for the museum's centuries. The NREL SPA (Reda and Andreas 2008, table A4.3) uses the same table.
	 */
	FNutation Nutation(double JulianEphemerisDay);
}
