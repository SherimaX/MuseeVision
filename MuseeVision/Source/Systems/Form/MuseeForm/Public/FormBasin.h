#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A basin: a vessel with a bed and sides round a closed plan outline, a pond's lining, a fountain's bowl, a planter.
 * The outline is the line of the sides' inner face; the sides are WallMeters thick outside it, the bed FloorMeters
 * thick, and the rim stands DepthMeters above the bed. It is one closed solid of these parts:
 */
struct FFormBasin
{
	/** The inner face of the sides and the bed: what holds the water or the soil. */
	static constexpr const TCHAR* LiningPart = TEXT("lining");
	/** The top of the sides. */
	static constexpr const TCHAR* RimPart = TEXT("rim");
	/** The outer face of the sides and the underside. */
	static constexpr const TCHAR* OuterPart = TEXT("outer");

	/** The inner face's line in plan, closed, (x east, y south), metres; either way round. */
	FFormProfile Outline;

	/** The bed to the rim, metres. */
	double DepthMeters = 0.0;

	/** The thickness of the bed and of the sides, metres. */
	double FloorMeters = 0.0;
	double WallMeters = 0.0;
};
