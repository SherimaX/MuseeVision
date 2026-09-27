#pragma once

#include "CoreMinimal.h"
#include "MotionRequirement.h"
#include "MuseeKey.h"

class UWorld;

/**
 * The conditions Motion's machines wait on, read as a controller reads its limit switches: the ports the site plan
 * mates, and the channels' present values. A channel nobody has published on stands at its rest, 0 (a door shut, an
 * iris closed, no work performing).
 */
namespace Musee::Motion
{
	/** A channel's value now, or its rest where nobody has published on it. */
	[[nodiscard]] float ChannelValue(const UWorld* World, const FMuseeKey& Channel);

	/** Sets a channel's value. */
	void PublishChannel(UWorld* World, const FMuseeKey& Channel, float Value);

	/** Whether every port a requirement names is mated. */
	[[nodiscard]] bool IsMated(const TArray<FMuseeKey>& MatedPorts, const FMotionRequirement& Requirement);

	/** Whether a requirement holds now: its ports mated and its channels within their bounds. */
	[[nodiscard]] bool Holds(const UWorld* World, const TArray<FMuseeKey>& MatedPorts, const FMotionRequirement& Requirement);
}
