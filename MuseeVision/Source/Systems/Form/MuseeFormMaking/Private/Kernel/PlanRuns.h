#pragma once

#include "CoreMinimal.h"
#include "Kernel/SweepFrame.h"
#include "Kernel/Tolerances.h"

namespace Musee::Form
{
	struct FProfileStations;

	/**
	 * Frames along a plan path, for a moulding run round a room: a skirting, a dado rail, a cornice.
	 *
	 * The path is points in plan (x, y) metres at one elevation. The profile's A lies along the path's plan normal
	 * (-dy, dx) for a step (dx, dy) (with x east and y south, the right of someone walking the path) and its B is up.
	 * Every station is mitred, so the section keeps its shape through each turn. The path splits into runs at its
	 * crisp stations; within a run the moulding is smooth, its normal at each station the bisector of the two
	 * segments'. U is the length along the path itself, shared by the whole section (see FSweepFrame).
	 *
	 * Returns no runs for a path of fewer than two points, or one that doubles back too sharply to mitre.
	 */
	TArray<TArray<FSweepFrame>> PlanRuns(TConstArrayView<FVector2D> Path, TConstArrayView<bool> CrispAt, bool bClosed,
		double ElevationMeters);

	/**
	 * Plan runs for a path known only by its points: the crease rule makes a station crisp where it turns more than
	 * CreaseRadians.
	 */
	TArray<TArray<FSweepFrame>> PlanRuns(TConstArrayView<FVector2D> Path, bool bClosed, double ElevationMeters,
		double CreaseRadians = CreaseAngleRadians);

	/** Plan runs for a drawn path (a profile resolved in plan): crisp where the drawing is crisp. */
	TArray<TArray<FSweepFrame>> PlanRuns(const FProfileStations& Path, double ElevationMeters);
}
