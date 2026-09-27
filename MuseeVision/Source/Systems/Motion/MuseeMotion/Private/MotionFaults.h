#pragma once

#include "CoreMinimal.h"

struct FMotionMechanismSpec;
struct FMotionRequirement;
struct FMotionSchedule;

namespace Musee::Motion
{
	/**
	 * What is wrong with a mechanism's spec on its own, one sentence each; empty when it is sound. A sound spec names
	 * its joint, axis and travel; times its moves by a time and a profile or by a drive, never both; names each stop
	 * once, within the travel; draws, homes and starts the body at states it has; moves only to states it has, in its
	 * sequences and its rides alike; and returns by itself only from a state it has, by a close sequence.
	 */
	[[nodiscard]] TArray<FString> Faults(const FMotionMechanismSpec& Spec);

	/** What is wrong with a requirement's channel bounds (each from 0 to 1, the least first), each sentence led by Where. */
	[[nodiscard]] TArray<FString> RequirementFaults(const FMotionRequirement& Requirement, const FString& Where);

	/**
	 * What is wrong with the schedule as a whole, beyond each mechanism: its derived channels' maps; its performances'
	 * times; its interlocks, each guarding one mechanism of the stage or one channel, over a band that exists, with
	 * something that allows it; its extents' lengths, telescopes and turns; and its groups, at most one of whose
	 * mechanisms starts away from closed, each worked by a control.
	 */
	[[nodiscard]] TArray<FString> ScheduleFaults(const FMotionSchedule& Schedule);
}
