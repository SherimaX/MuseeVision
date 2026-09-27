#pragma once

#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"

struct FMotionMechanismSpec;

namespace Musee::Motion
{
	/** Motion dresses a stage first (order 10): the bodies it moves are what the others attach to. */
	inline constexpr int32 DresserOrder = 10;

	/**
	 * Motion's step in furnishing a stage: reads its Mechanisms.json (a stage without one has no machines) and places,
	 * for each mechanism, its drive on the stage actor, driving the scene component tagged with its body's key, and
	 * its control at its Use anchor. Anything the schedule gets wrong is a finding, and that mechanism isn't placed.
	 */
	void DressMechanisms(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings);

	/** What is wrong with a mechanism's spec on its own, one sentence each; empty when it is sound. */
	[[nodiscard]] TArray<FString> Faults(const FMotionMechanismSpec& Spec);
}
