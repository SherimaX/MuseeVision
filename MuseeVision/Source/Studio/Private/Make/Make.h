#pragma once

#include "CoreMinimal.h"
#include "Make/MakePlan.h"
#include "Make/MakeReport.h"

namespace Musee::Studio
{
	/**
	 * Makes a plan in its fixed order: casts every stage's pieces, writes each stage's level in the stage's own frame,
	 * then writes the museum level that places every stage at its translation. A package is written only when its
	 * inputs changed; a stage with a failed piece keeps its last good level, and the museum level is written only when
	 * every stage succeeded. The plan's descriptions are consumed.
	 */
	FMakeReport Make(FMakePlan& Plan);
}
