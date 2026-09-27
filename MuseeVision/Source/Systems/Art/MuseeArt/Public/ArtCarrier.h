#pragma once

#include "CoreMinimal.h"

#include "ArtCarrier.generated.h"

/**
 * What a work in the round stands on, as the hanging plan gives it (the Classical Hall's casts): its kind (`plinth`,
 * `pedestal`, `niche-pedestal`, `console`, `herm`...), its top [w, d] and height z, and for a niche's carrier how deep
 * its axis stands, all in metres. The carrier's stone is its surface in the finish schedule (the entry's `wall`).
 */
USTRUCT()
struct MUSEEART_API FArtCarrier
{
	GENERATED_BODY()

	UPROPERTY()
	FName Kind;

	/** The top's width and depth [w, d], metres. */
	UPROPERTY()
	TArray<double> Top;

	/** The top's height above the floor, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	float Z = 0.0f;

	/** How far the carrier's axis stands in from the niche's face. Spelt as the plan's key, `axis_depth`. */
	UPROPERTY(meta = (Units = "Meters"))
	float Axis_Depth = 0.0f;
};
