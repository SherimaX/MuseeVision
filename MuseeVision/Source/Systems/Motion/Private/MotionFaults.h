#pragma once

#include "CoreMinimal.h"

struct FMotionMechanismSpec;

namespace Musee::Motion
{
	/**
	 * What is wrong with a mechanism's spec on its own, one sentence each; empty when it is sound. A sound spec names
	 * its joint, axis and travel; times its moves by a time and a profile or by a drive, never both; names each stop
	 * once, within the travel; draws and homes the body at states it has; and moves only to states it has, in its
	 * sequences and its rides alike.
	 */
	[[nodiscard]] TArray<FString> Faults(const FMotionMechanismSpec& Spec);
}
