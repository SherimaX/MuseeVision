#include "Almanac/Refraction.h"

#include "Almanac/CelestialAngles.h"

namespace Musee::Time::StandardAtmosphere
{
	/** The tropospheric lapse rate of the standard atmosphere (ISO 2533:1975; U.S. Standard Atmosphere 1976). */
	constexpr double LapseRateKelvinPerMeter = 0.0065;
	/** Standard gravity (m s^-2), the molar mass of dry air (kg mol^-1) and the gas constant (J mol^-1 K^-1) of the
	 *  U.S. Standard Atmosphere 1976. */
	constexpr double StandardGravity = 9.80665;
	constexpr double DryAirMolarMass = 0.0289644;
	constexpr double GasConstant = 8.31432;
	constexpr double ZeroCelsiusKelvin = 273.15;
	constexpr double SeaLevelTemperatureKelvin = StandardSeaLevelTemperatureCelsius + ZeroCelsiusKelvin;

	/** The pressure and temperature Saemundsson's and Bennett's formulas are stated for (Meeus ch. 16), and the
	 *  kelvin offset Meeus's correction factor uses. */
	constexpr double FormulaPressureHectopascals = 1010.0;
	constexpr double FormulaTemperatureKelvin = 283.0;
	constexpr double FormulaKelvinOffset = 273.0;
	constexpr double ArcminutesPerDegree = 60.0;
}

namespace Musee::Time
{
	double StandardPressureHectopascals(double ElevationMeters)
	{
		using namespace StandardAtmosphere;
		const double Exponent = StandardGravity * DryAirMolarMass / (GasConstant * LapseRateKelvinPerMeter);
		return StandardSeaLevelPressureHectopascals
			* FMath::Pow(1.0 - LapseRateKelvinPerMeter * ElevationMeters / SeaLevelTemperatureKelvin, Exponent);
	}

	double StandardTemperatureCelsius(double ElevationMeters)
	{
		using namespace StandardAtmosphere;
		return SeaLevelTemperatureKelvin - LapseRateKelvinPerMeter * ElevationMeters - ZeroCelsiusKelvin;
	}

	double RefractionDegrees(double TrueAltitudeDegrees, double PressureHectopascals, double TemperatureCelsius)
	{
		using namespace StandardAtmosphere;
		// Saemundsson: R = 1.02 / tan(h + 10.3 / (h + 5.11)) arcminutes, h in degrees. Stated for altitudes from about
		// -1 degree up; the callers apply it only while the body's disc can still show above the horizon.
		const double Arcminutes = 1.02 / TanDegrees(TrueAltitudeDegrees + 10.3 / (TrueAltitudeDegrees + 5.11));
		const double AirDensityRatio = (PressureHectopascals / FormulaPressureHectopascals)
			* (FormulaTemperatureKelvin / (FormulaKelvinOffset + TemperatureCelsius));
		return AirDensityRatio * Arcminutes / ArcminutesPerDegree;
	}

	double RefractionOfApparentDegrees(double ApparentAltitudeDegrees)
	{
		using namespace StandardAtmosphere;
		// Bennett: R = 1 / tan(h0 + 7.31 / (h0 + 4.4)) arcminutes, h0 the apparent altitude in degrees.
		return 1.0 / TanDegrees(ApparentAltitudeDegrees + 7.31 / (ApparentAltitudeDegrees + 4.4)) / ArcminutesPerDegree;
	}
}
