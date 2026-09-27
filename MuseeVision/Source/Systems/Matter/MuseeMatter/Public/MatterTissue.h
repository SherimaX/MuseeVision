#pragma once

#include "CoreMinimal.h"
#include "MatterQuantity.h"

#include "MatterTissue.generated.h"

/**
 * A living plant tissue as light meets it (Living.ush): a leaf, a petal, a stamen or a stem, a thin layer of cells
 * under a waxy cuticle. Light scattered in the cells leaves by both faces, and a leaf transmits nearly the spectrum it
 * reflects; the colour itself is the species' own, which Nature draws in the tissue's vertex colour.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterTissue
{
	GENERATED_BODY()

	/** The Slab roughness of the cuticle (unit 1). */
	UPROPERTY()
	FMatterQuantity Roughness;

	/** What the tissue transmits as a fraction of what it reflects, over the visible (unit 1). */
	UPROPERTY()
	FMatterQuantity TransmittanceRatio;
};
