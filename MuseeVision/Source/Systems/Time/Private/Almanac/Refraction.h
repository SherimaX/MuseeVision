#pragma once

#include "CoreMinimal.h"

/**
 * Atmospheric refraction: the air bends light from a body near the horizon downwards, so the body is seen higher
 * than it geometrically stands (about 34 arcminutes at the horizon, nothing at the zenith). Denser, colder air bends
 * it more.
 */
namespace Musee::Time
{
	/** Sea-level pressure and temperature of the standard atmosphere (ISO 2533:1975; U.S. Standard Atmosphere 1976). */
	inline constexpr double StandardSeaLevelPressureHectopascals = 1013.25;
	inline constexpr double StandardSeaLevelTemperatureCelsius = 15.0;

	/** The standard atmosphere's pressure at an elevation (ISO 2533:1975, the ICAO standard atmosphere). */
	double StandardPressureHectopascals(double ElevationMeters);

	/** The standard atmosphere's temperature at an elevation: 15 C at sea level, falling 6.5 K per km (ISO 2533). */
	double StandardTemperatureCelsius(double ElevationMeters);

	/**
	 * The refraction for a true (airless) altitude: Saemundsson's formula (Sky and Telescope 72, 1986; Meeus eq. 16.4),
	 * scaled for pressure and temperature by (P / 1010 hPa)(283 K / T) as Meeus and the NREL SPA (eq. 42) do.
	 */
	double RefractionDegrees(double TrueAltitudeDegrees, double PressureHectopascals, double TemperatureCelsius);

	/** The refraction for an apparent (observed) altitude at 1010 hPa and 10 C: Bennett's formula (Meeus eq. 16.3). */
	double RefractionOfApparentDegrees(double ApparentAltitudeDegrees);
}
