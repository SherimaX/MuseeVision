#pragma once

#include "CoreMinimal.h"
#include "MatterQuantity.h"

#include "MatterStructure.generated.h"

/**
 * The structure a body's face shows, from its finest grain to its coarsest band, as Stone.ush and White.ush draw it:
 * the scales, how the structure is drawn out along a bedding, and how much tone and roughness vary across it.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterStructure
{
	GENERATED_BODY()

	/** The finest structure the face shows: its grain, crystal or aggregate size (unit mm). */
	UPROPERTY()
	FMatterQuantity Grain;

	/** The coarsest: a stone's bed thickness, or the reach of the tool that laid a white body (unit mm). */
	UPROPERTY()
	FMatterQuantity Bed;

	/** How many times finer the structure is across the bedding than along it (unit 1; 1 for an unbedded body). */
	UPROPERTY()
	FMatterQuantity BedAspect;

	/** The face's self-affinity: each halving of scale divides the relief by 2^H (unit 1). */
	UPROPERTY()
	FMatterQuantity Hurst;

	/** Half-range of ln(albedo) over the face (unit 1). */
	UPROPERTY()
	FMatterQuantity ToneSpread;

	/** Half-range of ln(roughness) over the face (unit 1). */
	UPROPERTY()
	FMatterQuantity RoughnessSpread;
};
