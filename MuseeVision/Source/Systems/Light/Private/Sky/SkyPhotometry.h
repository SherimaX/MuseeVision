#pragma once

#include "CoreMinimal.h"
#include "MuseeUnits.h"

/**
 * The light of the sky's bodies and of the air, as numbers the renderer takes: pure functions of Time's moment.
 */
namespace Musee::Light::SkyPhotometry
{
	/** The sun's illuminance on a surface facing it above the atmosphere, at the Earth's distance in astronomical units. */
	Lux SunIlluminance(double DistanceAstronomicalUnits);

	/** The sun's angular diameter at that distance. */
	Degrees SunAngularDiameter(double DistanceAstronomicalUnits);

	/** The full or partial Moon's illuminance above the atmosphere, by its phase angle and distance. */
	Lux MoonIlluminance(Degrees PhaseAngle, double DistanceKilometers);

	/** The full Moon's illuminance at its mean distance: about a third of a lux. */
	Lux FullMoonIlluminance();

	/** The Moon's angular diameter at a distance. */
	Degrees MoonAngularDiameter(double DistanceKilometers);

	/**
	 * The extinction coefficient of falling rain and snow for visible light, per metre: the haze the precipitation adds
	 * to the clear air, which the atmosphere already carries. Rates are millimetres of water an hour.
	 */
	double PrecipitationExtinctionPerMeter(double RainMillimetersPerHour, double SnowMillimetersPerHour);

	/** The engine's height-fog density for an extinction coefficient per metre. */
	float EngineFogDensity(double ExtinctionPerMeter);
}
