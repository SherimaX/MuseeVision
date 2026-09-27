#include "Almanac/SiderealTime.h"

#include "Almanac/AstronomicalTime.h"
#include "Almanac/CelestialAngles.h"
#include "Almanac/Nutation.h"

namespace Musee::Time
{
	double MeanSiderealDegrees(double JulianDayUniversal)
	{
		const double Days = JulianDayUniversal - J2000JulianDay;
		const double T = Days / DaysPerJulianCentury;
		return ReduceDegrees(280.46061837 + 360.98564736629 * Days + 0.000387933 * T * T - T * T * T / 38710000.0);
	}

	double ApparentSiderealDegrees(double JulianDayUniversal, const FNutation& Nutation)
	{
		return ReduceDegrees(MeanSiderealDegrees(JulianDayUniversal)
			+ Nutation.LongitudeDegrees * CosDegrees(Nutation.TrueObliquityDegrees()));
	}
}
