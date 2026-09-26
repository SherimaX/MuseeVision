#pragma once

#include "Cast/CastInput.h"
#include "Cast/CastRules.h"
#include "CoreMinimal.h"
#include "MeshDescription.h"

namespace Musee::Studio
{
	/** One part of a cast: the polygon groups of one kind, as a mesh of their own with their slots in order. */
	struct FCastPart
	{
		ECastKind Kind = ECastKind::Opaque;
		FMeshDescription Description;
		TArray<FCastSlot> Slots;
	};

	/**
	 * Splits a cast by kind: one part for each kind its slots use, opaque first. Each part keeps its groups' surfaces
	 * and every attribute untouched, with the groups in their original order and named after their slots.
	 */
	TArray<FCastPart> SplitByKind(const FMeshDescription& Description, TConstArrayView<FCastSlot> Slots, TConstArrayView<ECastKind> SlotKinds);

	/** The polygon groups Keep of Description, as a compact mesh of their own. */
	FMeshDescription ExtractGroups(const FMeshDescription& Description, TConstArrayView<FPolygonGroupID> Keep);

	/** The largest side of a mesh description's bounding box, in metres. */
	double LargestExtentMeters(const FMeshDescription& Description);
}
