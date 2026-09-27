#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A moulding run along a plan path: a skirting round a room, a cove under a ceiling, a coping round a pond. The
 * section's A lies to the right of the path in plan and its B is up; the section is drawn with its solid on its left.
 * The path turns crisply where it is drawn crisp and smoothly along its arcs, and every turn is mitred, so the section
 * keeps its shape round the room. A closed section on an open path is capped at both ends; a closed section on a
 * closed path is a ring and needs no caps.
 */
struct FFormSweep
{
	/** The section, (A to the path's right, B up), metres. */
	FFormProfile Section;

	/** The path in plan, (x east, y south), metres. */
	FFormProfile Path;

	/** The height of the section's B = 0 above the drawing frame's origin, metres. */
	double ElevationMeters = 0.0;
};
