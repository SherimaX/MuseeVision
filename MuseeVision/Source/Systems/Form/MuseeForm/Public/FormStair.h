#pragma once

#include "CoreMinimal.h"

/** A stair's steps as set out: how many risers, how tall, how deep the goings, and the pace they make. */
struct MUSEEFORM_API FFormStairSteps
{
	int32 Risers = 0;
	double RiserMeters = 0.0;
	double GoingMeters = 0.0;

	/** Two risers and one going: the step of the body climbing it (Blondel's rule). */
	[[nodiscard]] double PaceMeters() const { return 2.0 * RiserMeters + GoingMeters; }
};

/**
 * A straight flight of solid steps with a sloping soffit, set out by Blondel's rule: two risers and one going make one
 * pace of the body (FMuseeHumanMeasure::Pace, 0.63 m; F. Blondel, Cours d'architecture, 1675, and DIN 18065:2020,
 * which admits 0.59 to 0.65 m). It climbs along the drawing frame's x axis from its first riser at x = 0 on the lower
 * floor (z = 0), centred across y, with the soffit WaistMeters under the line of the nosings.
 *
 * Give the risers, the run, or both. With only the risers, each going is the pace less two risers. With only the run
 * (first nosing to last), the number of risers is the one whose steps come closest to the pace without a riser taller
 * than the body steps. A stair with neither can't be set out.
 */
struct MUSEEFORM_API FFormStair
{
	/** Floor to floor, metres. */
	double RiseMeters = 0.0;

	/** The number of risers; zero to choose it by Blondel's rule from the run. */
	int32 Risers = 0;

	/** From the first nosing to the last, metres; zero to take each going from Blondel's rule. */
	double RunMeters = 0.0;

	double WidthMeters = 0.0;

	/** The thickness of the flight, square to its pitch, under the nosings, metres. */
	double WaistMeters = 0.0;

	/** The steps this stair is set out with; zero risers when it can't be (see above). */
	[[nodiscard]] FFormStairSteps SetOut() const;
};
