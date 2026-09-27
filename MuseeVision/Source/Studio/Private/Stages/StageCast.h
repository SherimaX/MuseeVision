#pragma once

#include "CoreMinimal.h"
#include "Levels/StageLevel.h"
#include "Make/MakeReport.h"

namespace Musee::Studio
{
	struct FStageFabric;
	struct FStageMakeOptions;

	/** What a stage's cast gives its level: the moving bodies, and a placement for each mesh a piece or a set of identical pieces became. */
	struct FStageCast
	{
		TArray<FStageBody> Bodies;
		TArray<FStagePlacement> Placements;
		/** How many pieces are placed as instances of a mesh they share with identical pieces. */
		int32 Instanced = 0;
	};

	/**
	 * The stage contract's fourth step. Each piece's slots take their materials (the piece's own palette key, or its
	 * surface's line in the finish schedule, resolved to Matter's material); a piece with any slot unresolved is a
	 * finding and is left uncast. The rest are cast, only what changed rebuilt, and placed on their bodies, each tagged
	 * musee.piece:<name>. Pieces identical in drawing, materials and collision (the Salon's 750 rosettes) are cast once,
	 * as the first of them, and placed as instances of that mesh, one instanced component per body and collision they
	 * share. A work's pieces stay components of their own, for Art hangs each from its work's label: a stage's built work
	 * (work.<id>...) is cast alone, and Art's pieces share a cast but are placed one by one. Each body hangs from the body
	 * it rides, when the stage says it rides one. The finish schedule is read within the fabric's dependency set.
	 */
	FStageCast CastStage(FStageFabric& Fabric, const FStageMakeOptions& Options, const FString& ScheduleDir, FMakeReport& Report);

	/** The same cast with each mesh's package named by MeshPackage from its (first) piece's name: the site's joints' meshes. */
	FStageCast CastPieces(FStageFabric& Fabric, const FStageMakeOptions& Options, const FString& ScheduleDir, TFunctionRef<FString(FName)> MeshPackage,
		FMakeReport& Report);
}
