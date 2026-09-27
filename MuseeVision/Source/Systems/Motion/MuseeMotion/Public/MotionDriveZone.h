#pragma once

#include "CoreMinimal.h"

#include "MotionDriveZone.generated.h"

/**
 * A stretch of a drive's travel where the machine runs under other limits than its default: the Élan car hangs on the
 * Cube's mast below the clamp level and runs slower there than on its rails above it. The jerk limit stays the drive's.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionDriveZone
{
	GENERATED_BODY()

	/** Where the zone begins, as a share of the travel from 0 to 1. */
	UPROPERTY(meta = (MuseeRequired))
	double From = 0.0;

	/** Where it ends, as a share of the travel, beyond From. */
	UPROPERTY(meta = (MuseeRequired))
	double To = 0.0;

	/** The speed limit in the zone: metres a second, or degrees a second for a revolute joint. */
	UPROPERTY(meta = (MuseeRequired))
	double Speed = 0.0;

	/** The acceleration limit in the zone: metres (or degrees) a second squared. */
	UPROPERTY(meta = (MuseeRequired))
	double Accel = 0.0;
};
