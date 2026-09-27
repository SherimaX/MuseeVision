#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"
#include "Kernel/Profile.h"
#include "Kernel/SweepFrame.h"

/**
 * The public drawing's profiles, read into the kernel's. A profile is resolved at the one chord tolerance
 * (Stations.h), so every piece drawn from one profile shares its stations.
 */
namespace Musee::Form::Drawing
{
	/** The kernel's profile of a drawn one. The profile must be well formed (FFormProfile::IsWellFormed). */
	FProfile ToKernel(const FFormProfile& Profile);

	/** A drawn profile's stations at the museum's chord tolerance. */
	FProfileStations Resolve(const FFormProfile& Profile);

	/**
	 * A closed plan loop run so its right (the side PlanRuns lays a section's A along) faces out of the loop: the
	 * loop clockwise on the plan reversed, in (x east, y south) whose right is (-dy, dx).
	 */
	FProfileStations RunOutward(const FProfileStations& Loop);

	/**
	 * A plan path's frames at an elevation, with frames added wherever the path must have a station (the jambs of an
	 * opening, the stations of an arch): each run of PlanRuns, with every required distance along the path that falls
	 * inside a run added on its chord. The chords are within the tolerance of the curve, so points on them are too.
	 */
	TArray<TArray<FSweepFrame>> PathRuns(const FProfileStations& Path, double ElevationMeters, TConstArrayView<double> RequiredMeters);

	/** The frame at a distance along a run, on the chord between its stations. The run must cover the distance. */
	FSweepFrame FrameAt(TConstArrayView<FSweepFrame> Run, double UMeters);
}
