#pragma once

#include "CoreMinimal.h"
#include "MatterQuantity.h"

#include "MatterWood.generated.h"

/**
 * The figure a sawn board of wood shows, beyond the fibre structure every body has (FMatterStructure): the growth
 * rings, each a pale porous earlywood band closed by a dense latewood band, and the rays, the ribbons of cells that run
 * from the pith outward and show on a quarter-sawn face as the flakes called ray fleck (Wood.ush). A ring's width is
 * the structure's coarsest scale (FMatterStructure::Bed).
 */
USTRUCT()
struct MUSEEMATTER_API FMatterWood
{
	GENERATED_BODY()

	/** The fraction of each ring that is latewood (unit 1). */
	UPROPERTY()
	FMatterQuantity LatewoodFraction;

	/** The latewood's albedo as a fraction of the earlywood's (unit 1; under 1: latewood is darker). */
	UPROPERTY()
	FMatterQuantity LatewoodTone;

	/** The fraction of a quarter-sawn face the ray fleck covers (unit 1). */
	UPROPERTY()
	FMatterQuantity RayFleck;

	/** The rays' albedo as a fraction of the wood around them (unit 1). */
	UPROPERTY()
	FMatterQuantity RayTone;

	/** A fleck's length along the grain (unit mm). */
	UPROPERTY()
	FMatterQuantity RayLength;

	/** A fleck's width across the grain (unit mm). */
	UPROPERTY()
	FMatterQuantity RayWidth;
};
