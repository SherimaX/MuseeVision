#pragma once

#include "CoreMinimal.h"
#include "MotionDriveZone.h"
#include "MotionSCurve.h"

#include "MotionDrive.generated.h"

/**
 * A long drive's limits, as a lift's controller holds them (Mechanisms.json `drive`, in place of `time` and `profile`):
 * a speed, an acceleration and a jerk limit over the whole travel, and zones where the speed and acceleration are
 * lower (or higher). Every move is a jerk-limited S-curve from rest to rest (FSCurve).
 *
 * A move runs under the strictest limits of every stretch it passes through, as a controller that commands one run
 * per trip: a car that must crawl on the mast for part of its way crawls the whole of that trip. Every leg of the
 * museum's lifts stops at the zone's edge (the Élan car stops at the clamp), so no leg loses time by it.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionDrive
{
	GENERATED_BODY()

	/** The speed limit: metres a second, or degrees a second for a revolute joint. */
	UPROPERTY(meta = (MuseeRequired))
	double Speed = 0.0;

	/** The acceleration limit: metres (or degrees) a second squared. */
	UPROPERTY(meta = (MuseeRequired))
	double Accel = 0.0;

	/** The jerk limit: metres (or degrees) a second cubed, everywhere on the travel. */
	UPROPERTY(meta = (MuseeRequired))
	double Jerk = 0.0;

	/** The stretches with limits of their own. */
	UPROPERTY()
	TArray<FMotionDriveZone> Zones;

	/** Whether the mechanism gives a drive (otherwise its move is timed by `time` and `profile`). */
	[[nodiscard]] bool IsSet() const { return Speed > 0.0 || Accel > 0.0 || Jerk > 0.0 || !Zones.IsEmpty(); }

	/**
	 * The move from one share of the travel to another, over a travel of Travel metres (or degrees): one S-curve under
	 * the strictest speed and acceleration of the zones it passes through, and the drive's own where it passes outside
	 * them. A move that only touches a zone's edge doesn't pass through it.
	 */
	[[nodiscard]] Musee::Motion::FSCurve Run(double FromShare, double ToShare, double Travel) const;
};
