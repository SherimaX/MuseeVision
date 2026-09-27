#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A single flat face with holes, seen from one side: a finish laid on a surface the piece doesn't own, a backing, a
 * leaf of cloth. It is drawn in the drawing frame's (x, y) plane and faces the frame's z axis. A face is never a
 * closed solid: a slab is an extrusion.
 */
struct FFormFace
{
	/** The outline, closed, in the frame's (x, y), metres. */
	FFormProfile Outline;

	/** Holes in the face, each closed and inside the outline. */
	TArray<FFormProfile> Holes;
};
