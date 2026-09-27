#pragma once

#include "CoreMinimal.h"

#include "MotionStop.generated.h"

/**
 * A named place on a mechanism's travel where its body comes to rest, as a lift's landings are: the Élan car's stops at
 * the Cube, the clamp, the Atrium, the neck and the Sphere. Moves and rides name it; `open` and `closed` stay the names
 * of the travel's two ends.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionStop
{
	GENERATED_BODY()

	/** The stop's name (`atrium`), never `open` or `closed`. */
	UPROPERTY(meta = (MuseeRequired))
	FName Stop;

	/** Where the stop is, as a share of the travel from 0 to 1. This decides. */
	UPROPERTY(meta = (MuseeRequired))
	double At = 0.0;

	/** The designer's note of the stop's level in the stage's frame, in metres. It is never read. */
	UPROPERTY()
	double Z = 0.0;
};
