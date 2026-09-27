#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "MotionChannelCondition.generated.h"

/**
 * A channel a ride waits on, and the bounds its value must lie in, as a lift's controller reads a door's or an iris's
 * limit switch before it runs: `atrium.car.doors` at least 1 (the leaves are open), `cube.performance.active` at most 0
 * (no work performs). A point nobody has published on stands at 0, its rest.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionChannelCondition
{
	GENERATED_BODY()

	/** The channel read. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Channel;

	/** The least value that holds; 0 (the channel's floor) when absent. */
	UPROPERTY()
	float AtLeast = 0.0f;

	/** The greatest value that holds; 1 (the channel's ceiling) when absent. */
	UPROPERTY()
	float AtMost = 1.0f;

	/** Whether a value lies within the bounds. */
	[[nodiscard]] bool Holds(float Value) const { return Value >= AtLeast && Value <= AtMost; }
};
