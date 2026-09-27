#pragma once

#include "CoreMinimal.h"
#include "MatterQuantity.h"

#include "MatterFibre.generated.h"

/**
 * What a textile's fibres do to light at its face (Cloth.ush): each fibre is a smooth dielectric cylinder with its own
 * specular highlight, and where fibre ends stand up from the cloth (a velvet's pile, a felt's matted surface, a silk's
 * few loose filaments) they scatter light that grazes the cloth back toward the eye: the cloth's sheen. The fibre's
 * dye colour is its diffuse albedo (FMatterDielectric) and its yarn's slubs are its structure (FMatterStructure).
 */
USTRUCT()
struct MUSEEMATTER_API FMatterFibre
{
	GENERATED_BODY()

	/** The Slab roughness of the fibres' specular highlight (unit 1). */
	UPROPERTY()
	FMatterQuantity Roughness;

	/** The weight of the sheen the standing fibre ends give, Substrate's fuzz amount (unit 1). */
	UPROPERTY()
	FMatterQuantity Sheen;

	/** How widely the sheen spreads about grazing, Substrate's fuzz roughness (unit 1). */
	UPROPERTY()
	FMatterQuantity SheenRoughness;
};
