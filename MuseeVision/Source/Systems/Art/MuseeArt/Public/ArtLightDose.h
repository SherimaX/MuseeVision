#pragma once

#include "CoreMinimal.h"
#include "MuseeUnits.h"

/**
 * The light a hung work receives, as Light measures it on the work's face: the illuminance at a moment, the exposure it
 * adds up to in a year (lux-hours), and the ultraviolet in it per lumen (microwatts per lumen). Care.json's limits are
 * written in the same three.
 */
struct FArtLightDose
{
	Musee::Lux Illuminance;
	double AnnualLuxHours = 0.0;
	double UvMicrowattsPerLumen = 0.0;
};
