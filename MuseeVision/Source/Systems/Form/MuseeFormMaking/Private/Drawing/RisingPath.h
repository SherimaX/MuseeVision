#pragma once

#include "CoreMinimal.h"
#include "FormSweep.h"
#include "Kernel/SweepFrame.h"

/**
 * The frames of a sweep whose path rises as it runs (FFormSweep::Rise): a stair's string and a handrail wound up a helix,
 * a balustrade's bars over flights and landings.
 *
 * The plan path is set out as any plan sweep's is, with a station added wherever the rise has one, and each station is
 * raised to the rise's height there. The rise is followed on its chords, as every drawn curve is. The path splits into
 * runs wherever the plan or the rise turns crisply. A plumb section keeps its A level and its B straight up, and its
 * normals carry the rake (FSweepFrame's Rake and Bend); a square section stands square to the path's rake at each
 * station, and where a run turns crisply its section is cut on the plane that halves the turn, the run beyond starting
 * from the same cut section, so the two meet point for point. U is metres along the rising reference line itself.
 */
namespace Musee::Form::Drawing
{
	/** The runs of a rising sweep's frames, or false with the reason. */
	bool RisingRuns(const FFormSweep& Shape, TArray<TArray<FSweepFrame>>& OutRuns, FString& OutWhy);
}
