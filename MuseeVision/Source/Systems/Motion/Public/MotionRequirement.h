#pragma once

#include "CoreMinimal.h"
#include "MotionChannelCondition.h"
#include "MuseeKey.h"

#include "MotionRequirement.generated.h"

/**
 * What must hold before a ride is offered, written as structure and never as prose: the ports it runs through must be
 * mated (a port is mated when the site plan mates it, so the car never runs into a well no stage fills), and its
 * channels must lie within their bounds. A condition that needs memory of the visit is not in the first pass.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionRequirement
{
	GENERATED_BODY()

	/** The ports that must be mated (`atrium.down`). */
	UPROPERTY()
	TArray<FMuseeKey> Mated;

	/** The channels that must lie within their bounds. */
	UPROPERTY()
	TArray<FMotionChannelCondition> Channels;
};
