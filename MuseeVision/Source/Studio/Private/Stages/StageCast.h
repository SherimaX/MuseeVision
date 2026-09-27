#pragma once

#include "CoreMinimal.h"
#include "Levels/StageLevel.h"
#include "Make/MakeReport.h"

namespace Musee::Studio
{
	struct FStageFabric;
	struct FStageMakeOptions;

	/** What a stage's cast gives its level: the moving bodies, and a placement for each mesh a piece became. */
	struct FStageCast
	{
		TArray<FStageBody> Bodies;
		TArray<FStagePlacement> Placements;
	};

	/**
	 * The stage contract's fourth step. Each piece's slots take their materials (the piece's own palette key, or its
	 * surface's line in the finish schedule, resolved to Matter's material); a piece with any slot unresolved is a
	 * finding and is left uncast. The rest are cast, only what changed rebuilt, and placed on their bodies, each tagged
	 * musee.piece:<name>. The finish schedule is read within the fabric's dependency set.
	 */
	FStageCast CastStage(FStageFabric& Fabric, const FStageMakeOptions& Options, const FString& ScheduleDir, FMakeReport& Report);
}
