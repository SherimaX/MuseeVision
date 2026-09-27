#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A slab or block cut from a closed outline: a floor round its opening, a plinth, a door leaf, a stair's side. The
 * outline and its holes are drawn in the drawing frame's (x, y) plane and pushed DepthMeters along its z axis, giving a
 * closed solid with both faces and every side.
 */
struct FFormExtrusion
{
	/** The outline, closed, in the frame's (x, y), metres. */
	FFormProfile Outline;

	/** Holes through the slab, each closed and inside the outline. */
	TArray<FFormProfile> Holes;

	/** How far the outline is pushed along the frame's z axis, metres. */
	double DepthMeters = 0.0;
};
