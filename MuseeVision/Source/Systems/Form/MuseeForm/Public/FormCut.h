#pragma once

#include "CoreMinimal.h"

/**
 * A plane a made solid is cut on, as a mason saws a block or a smith cuts a bar: what lies on the side the normal points
 * to is taken away, and the solid is closed again by the cut face. It trims a moulding whose drawn section sinks behind
 * the face it is fixed to (the as-built profiles' sink points) back to that face, cuts a slab's end on a mitre where it
 * meets its neighbour, and ends a tube, a turned ring or a rail on the slant of the plate or the plane it stops against.
 */
struct FFormCut
{
	/** A point of the plane, metres, in the shape's own frame (the drawing's frame). */
	FVector PointMeters = FVector::ZeroVector;

	/** The plane's normal, towards the side cut away. */
	FVector Normal = FVector::UpVector;
};
