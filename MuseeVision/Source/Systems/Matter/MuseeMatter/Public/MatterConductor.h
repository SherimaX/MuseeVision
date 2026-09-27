#pragma once

#include "CoreMinimal.h"
#include "MatterTriple.h"

#include "MatterConductor.generated.h"

/**
 * A metal's reflection, from its measured complex index integrated against the CIE observer: its reflectance at normal
 * incidence and at 82 degrees, where a metal's edge colour is fitted (Metal.ush). Gold leaf's lives in the gilt
 * working's record, the alloy's in its substance's.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterConductor
{
	GENERATED_BODY()

	/** Reflectance at normal incidence, linear Rec.709 (unit 1; gold's red may pass 1, out of gamut). */
	UPROPERTY()
	FMatterTriple F0;

	/** Reflectance at the incidence cosine 1/7 (81.8 degrees), linear Rec.709 (unit 1). */
	UPROPERTY()
	FMatterTriple F82;
};
