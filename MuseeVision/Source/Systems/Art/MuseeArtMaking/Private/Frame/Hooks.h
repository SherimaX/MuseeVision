#pragma once

#include "ArtHangEntry.h"
#include "ArtMesh.h"
#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "Frame/Moulding.h"
#include "MuseeFrame.h"
#include "Objects/HangingRecords.h"

namespace Musee::ArtMaking
{
	/**
	 * Where a framed work's pair of S-hooks hang on its mesh, in its Wall anchor's frame (x along, y out of the plane,
	 * z up): each eye's place along, halfway between two uprights on the back of a side of the frame; the eyes' height, a
	 * share of the frame down from its top; and the level wire the hooks hang over, the lowest that leaves each hook its
	 * least shank.
	 */
	struct FHookPlan
	{
		double Along[2] = { 0.0, 0.0 };
		double EyeHeight = 0.0;
		double WireHeight = 0.0;
	};

	/** The mesh a Wall anchor's works hang on, or null when they hang on a wall. */
	[[nodiscard]] const FArtMesh* MeshOf(TConstArrayView<FArtMesh> Meshes, const FMuseeKey& Anchor);

	/** What is wrong with a mesh as the plan gives it, one clause each: runs of wires and a wire of some size, behind the plane. */
	[[nodiscard]] TArray<FString> MeshFaults(const FArtMesh& Mesh);

	/**
	 * Where a framed canvas's hooks hang on its mesh, its frame in this moulding, its back on the hanging gap; or why they
	 * can't: the mesh too close behind the frame for the hooks' bends, no level wire high enough over the eyes, or no gap
	 * between uprights on a side of the frame's back.
	 */
	[[nodiscard]] TValueOrError<FHookPlan, FString> PlanHooks(const FArtHooks& Hooks, const FArtMesh& Mesh, const FArtMoulding& Moulding,
		const FArtHangEntry& Entry);

	/**
	 * The hooks' drawings, in the mesh's `hooks` surface, on the Wall anchor (in the stage): the two S-hooks, named
	 * HookName.1 and .2, each a round wire from its end behind the level wire, over the wire, down its shank, round the
	 * eye's ring and up inside it; and the two screw eyes, named EyeName.1 and .2, their rings lying level against the
	 * frame's back.
	 */
	[[nodiscard]] TArray<FFormDrawing> HookDrawings(const FHookPlan& Plan, const FArtHooks& Hooks, const FArtMesh& Mesh, const FMuseeFrame& Anchor,
		FName HookName, FName EyeName);
}
