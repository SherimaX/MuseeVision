#pragma once

#include "CoreMinimal.h"
#include "MatterTriple.h"

#include "MatterMedium.generated.h"

/**
 * Light inside a body that lets it through: its absorption and its (reduced) scattering coefficients at the three
 * primaries (Medium.ush, Glass.ush). Glass absorbs and barely scatters; water absorbs, and a dye in it absorbs more;
 * marble scatters millimetres deep.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterMedium
{
	GENERATED_BODY()

	/** The absorption coefficient, Napierian, per millimetre (unit 1/mm), sampled at the primaries. */
	UPROPERTY()
	FMatterTriple Absorption;

	/** The reduced scattering coefficient per millimetre (unit 1/mm), sampled at the primaries. */
	UPROPERTY()
	FMatterTriple Scattering;
};
