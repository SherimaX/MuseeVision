#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A moulding run along a path: a skirting round a room, a cove under a ceiling, a coping round a pond; or, drawn on a
 * face, an architrave up a door's jambs and over its arch, a rib following a velarium's rise, a glazing bar under a dome's
 * glass. The section is drawn with its solid on its left, its A to the path's right and its B up (in plan) or out of the
 * face (on a face). The path turns crisply where it is drawn crisp and smoothly along its arcs, and every turn is mitred,
 * so the section keeps its shape. A closed section on an open path is capped at both ends, square to the path unless an
 * end is chamfered; a closed section on a closed path is a ring and needs no caps.
 */
struct FFormSweep
{
	/** The section, (A to the path's right, B up in plan or out of the face), metres. */
	FFormProfile Section;

	/** The path: in plan, (x east, y south); or on the face, (U along it, Z up); metres. */
	FFormProfile Path;

	/** The height of the path's zero above the drawing frame's origin, metres. */
	double ElevationMeters = 0.0;

	/**
	 * The face the path is drawn on, when it isn't drawn in plan: a vertical face standing on this line in plan (x east,
	 * y south) and looking to the line's left, as a wall's face does (FFormWall), drawn in its development: U is metres
	 * along the line from its start, Z is up. A curved face is followed exactly, so a moulding drawn on an oval's wall lies
	 * on it and stands out of it along its normal. The face is one straight or smooth stretch. Undrawn (no start): the
	 * path is in plan.
	 */
	FFormProfile Face;

	/**
	 * Chamfered ends: an open sweep's first and last sections cut on a plane through the path's end, whose normal (in the
	 * drawing's frame) is given, instead of square to the path: a glazing bar cut to stand against its curb, a rib's end
	 * against its ring. Zero: square.
	 */
	FVector StartCut = FVector::ZeroVector;
	FVector EndCut = FVector::ZeroVector;

	/** The furthest a chamfered end may lean from square to the path, degrees: beyond it the end runs out to a feather. */
	static constexpr double MaxCutDegrees = 75.0;

	/**
	 * Trims the section's sinks, where the as-built profiles' sink points draw a closed section into what the moulding is
	 * fixed to: its part below zero on A (behind the wall a skirting runs along), on B (under the floor it stands on, or
	 * into the face a frame is fixed to), or both, is cut away, so the moulding meets that surface face to face, on a
	 * straight wall or a curved one.
	 */
	bool bTrimSinkA = false;
	bool bTrimSinkB = false;

	/** Whether the path is drawn on a face. */
	[[nodiscard]] bool IsOnFace() const { return !Face.Start.IsEmpty(); }
};
