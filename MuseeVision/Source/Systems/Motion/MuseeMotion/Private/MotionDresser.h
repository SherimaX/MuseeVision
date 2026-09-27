#pragma once

#include "CoreMinimal.h"
#include "MotionFaults.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"

namespace Musee::Motion
{
	/** Motion dresses a stage first (order 10): the bodies it moves are what the others attach to. */
	inline constexpr int32 DresserOrder = 10;

	/**
	 * Motion's step in furnishing a stage: reads its Mechanisms.json (a stage without one has no machines) and places,
	 * for each mechanism, its drive on the stage actor, driving the scene component tagged with its body's key, and a
	 * control at each Use anchor it is worked from (its own control's and its rides'). Anything the schedule gets wrong
	 * is a finding, and that mechanism isn't placed. The stage's context names no mated ports, so every mechanism is
	 * placed with none mated, as on the stand.
	 */
	void DressMechanisms(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings);
}
