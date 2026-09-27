#pragma once

#include "CoreMinimal.h"
#include "MotionFollowing.h"
#include "MuseeKey.h"

#include "MotionChannelSpec.generated.h"

/**
 * A control point a stage keeps (Mechanisms.json `channels`), as a building's controller keeps its points. Either it is
 * derived from another, as a relay derives one circuit from another's state (the Cube's mast pinspots are on while no
 * work performs, so `cube.mast.light` follows `cube.performance.active` from 1 down to 0, by the same clamped linear
 * map a following mechanism uses); or it is a point the stage's sequences ramp, standing at its rest when the museum
 * opens (the Élan car's light is lit, 1, until a ride dims it). A point the stage names nowhere rests at 0.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionChannelSpec
{
	GENERATED_BODY()

	/** The channel (`cube.mast.light`). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The channel it follows and the map, whose Out is this channel's value, 0 to 1; unset for a point sequences ramp. */
	UPROPERTY()
	FMotionFollowing Follows;

	/** Where a point the sequences ramp stands when the museum opens, 0 to 1. */
	UPROPERTY()
	float Rest = 0.0f;

	/** The designer's note. */
	UPROPERTY()
	FString Note;
};
