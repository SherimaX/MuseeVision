#pragma once

#include "CoreMinimal.h"
#include "MatterMasterRecipe.h"

/**
 * The stone probe: Matter's stone master with its history set by hand.
 *
 * It is the stone master whole: a stone's face from its substance and bedding (Stone.ush), marked by the surface law
 * (History.ush), on a Substrate Slab with haze. Where the museum's stone master will read its history from the museum
 * Moment and from Form's made facts, the probe takes every input as a parameter. That makes it the specimen stand's
 * material, which shows one substance at chosen states (dry, wet, puddled, snowed on, soiled, worn) side by side, and
 * the proof that a master can be built from code, compiled and made reproducibly.
 *
 * Every parameter defaults to a new face: dry, clean, unworn and sheltered, with the Slab's own neutral optics until an
 * instance gives it a substance's record.
 */
namespace Musee::Matter
{
	/** The stone probe's recipe, made as /Game/Generated/Matter/M_Matter_Stone_Probe. */
	FMasterRecipe StoneProbeRecipe();
}
