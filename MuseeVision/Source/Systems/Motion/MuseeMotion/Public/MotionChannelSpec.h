#pragma once

#include "CoreMinimal.h"
#include "MotionFollowing.h"
#include "MuseeKey.h"

#include "MotionChannelSpec.generated.h"

/**
 * A control point derived from another, as a relay derives one circuit from another's state: the Cube's mast pinspots
 * are on while no work performs, so `cube.mast.light` follows `cube.performance.active` from 1 down to 0. Its value is
 * the same clamped linear map a following mechanism uses (Mechanisms.json `channels`).
 */
USTRUCT()
struct MUSEEMOTION_API FMotionChannelSpec
{
	GENERATED_BODY()

	/** The derived channel (`cube.mast.light`). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The channel it follows and the map; Out is the derived channel's value, 0 to 1. */
	UPROPERTY(meta = (MuseeRequired))
	FMotionFollowing Follows;

	/** The designer's note. */
	UPROPERTY()
	FString Note;
};
