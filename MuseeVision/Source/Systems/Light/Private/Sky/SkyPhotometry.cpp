#include "Sky/SkyPhotometry.h"

namespace Musee::Light::SkyPhotometry
{
	namespace
	{
		/**
		 * The luminous solar constant: the Sun's illuminance at 1 AU above the atmosphere, 133.8 klx (S. Darula,
		 * R. Kittler and C. A. Gueymard, "Reference luminous solar constant and solar luminance for illuminance
		 * calculations", Solar Energy 79 (2005) 559-565).
		 */
		constexpr double LuminousSolarConstantLux = 133800.0;

		/** The Sun's semidiameter at 1 AU, 959.63 arcseconds (J. Meeus, Astronomical Algorithms, 2nd ed., ch. 55). */
		constexpr double SunSemidiameterAtOneAuDegrees = 959.63 / 3600.0;

		/** The Moon's mean radius, 1737.4 km (IAU WG on Cartographic Coordinates and Rotational Elements, 2009). */
		constexpr double MoonRadiusKilometers = 1737.4;

		/** The Moon's mean distance from the Earth, 384,400 km, at which its magnitudes are stated. */
		constexpr double MoonMeanDistanceKilometers = 384400.0;

		/**
		 * The Moon's visual magnitude by phase angle a (degrees) at its mean distance: m = -12.73 + 0.026 |a| + 4e-9 a^4
		 * (K. Krisciunas and B. E. Schaefer, "A model of the brightness of moonlight", PASP 103 (1991) 1033-1039: the phase law of their model).
		 */
		constexpr double FullMoonMagnitude = -12.73;
		constexpr double MagnitudePerPhaseDegree = 0.026;
		constexpr double MagnitudePerPhaseDegreeFourth = 4.0e-9;

		/**
		 * The Sun's visual magnitude, -26.74 (C. W. Allen, Astrophysical Quantities, 4th ed., A. N. Cox ed., 2000), which
		 * ties a magnitude to lux through the luminous solar constant: five magnitudes are a factor of 100.
		 */
		constexpr double SunMagnitude = -26.74;
		constexpr double MagnitudesPerDecade = 2.5;

		/**
		 * Extinction by rain and snow for visible and near-infrared light, as free-space optical links measure it,
		 * in decibels per kilometre for a rate R in mm/h of water: rain 1.076 R^0.67 (Carbonneau); dry snow
		 * (5.42e-5 lambda + 5.4958776) R^1.38 with lambda in nanometres, taken at 550 nm, the peak of the eye's
		 * sensitivity (S. Sheikh Muhammad, P. Kohldorfer and E. Leitgeb, "Channel modeling for terrestrial free space
		 * optical links", Proc. ICTON 2005).
		 */
		constexpr double RainDecibelsPerKilometer = 1.076;
		constexpr double RainExponent = 0.67;
		constexpr double VisibleWavelengthNanometers = 550.0;
		constexpr double SnowDecibelsPerKilometer = 5.42e-5 * VisibleWavelengthNanometers + 5.4958776;
		constexpr double SnowExponent = 1.38;

		/** Ten log10 e decibels are one neper of power: an extinction of 1 per unit length is 4.343 dB per unit. */
		const double DecibelsPerNeper = 10.0 / FMath::Loge(10.0);
		constexpr double MetersPerKilometer = 1000.0;

		/**
		 * The engine keeps height-fog density as an extinction per centimetre times 1000 (the renderer divides it by
		 * 1000 per world unit: Renderer/Private/SceneCore.cpp, FExponentialHeightFogSceneInfo), so a density is ten
		 * times the extinction per metre.
		 */
		constexpr double EngineFogDensityPerExtinctionPerMeter = 1000.0 / 100.0;
	}

	Lux SunIlluminance(double DistanceAstronomicalUnits)
	{
		return Lux(LuminousSolarConstantLux / FMath::Square(DistanceAstronomicalUnits));
	}

	Degrees SunAngularDiameter(double DistanceAstronomicalUnits)
	{
		return Degrees(2.0 * SunSemidiameterAtOneAuDegrees / DistanceAstronomicalUnits);
	}

	Lux MoonIlluminance(Degrees PhaseAngle, double DistanceKilometers)
	{
		const double Angle = FMath::Abs(PhaseAngle.Get());
		const double Magnitude = FullMoonMagnitude + MagnitudePerPhaseDegree * Angle + MagnitudePerPhaseDegreeFourth * FMath::Pow(Angle, 4.0);
		const double AtMeanDistance = LuminousSolarConstantLux * FMath::Pow(10.0, -(Magnitude - SunMagnitude) / MagnitudesPerDecade);
		return Lux(AtMeanDistance * FMath::Square(MoonMeanDistanceKilometers / DistanceKilometers));
	}

	Lux FullMoonIlluminance()
	{
		return MoonIlluminance(Degrees(0.0), MoonMeanDistanceKilometers);
	}

	Degrees MoonAngularDiameter(double DistanceKilometers)
	{
		return ToDegrees(Radians(2.0 * FMath::Asin(MoonRadiusKilometers / DistanceKilometers)));
	}

	double PrecipitationExtinctionPerMeter(double RainMillimetersPerHour, double SnowMillimetersPerHour)
	{
		const double Rain = RainMillimetersPerHour > 0.0 ? RainDecibelsPerKilometer * FMath::Pow(RainMillimetersPerHour, RainExponent) : 0.0;
		const double Snow = SnowMillimetersPerHour > 0.0 ? SnowDecibelsPerKilometer * FMath::Pow(SnowMillimetersPerHour, SnowExponent) : 0.0;
		return (Rain + Snow) / DecibelsPerNeper / MetersPerKilometer;
	}

	float EngineFogDensity(double ExtinctionPerMeter)
	{
		return static_cast<float>(ExtinctionPerMeter * EngineFogDensityPerExtinctionPerMeter);
	}
}
