#pragma once

#include "CoreMinimal.h"
#include "GroundsRun.h"

/**
 * The Grounds' setting-out lines that more than one trade works to (§3.1, §3.5): where the walk east and the path north
 * meet the rond-point's circles and the terrace's, and the kerbs' centre lines as runs. The gravel stops on the kerbs'
 * faces, the kerbs stand on these lines, and the sheet's derived numbers (x 47.292, y 19.297 and 35.292, the gaps of
 * +/-16.60 and +/-5.92 degrees) fall out of them.
 */
namespace Musee::Grounds
{
	/** The point on the circle of a radius about the rond-point, west of its centre, at a y. */
	[[nodiscard]] FVector2D RingWestAt(double Y, double Radius);

	/** The point on the circle of a radius about the rond-point, north of its centre, at an x. */
	[[nodiscard]] FVector2D RingNorthAt(double X, double Radius);

	/** The point on the circle of a radius about the Élan's axis, south of it, at an x. */
	[[nodiscard]] FVector2D TerraceSouthAt(double X, double Radius);

	/**
	 * The kerbs' centre lines as runs, each named for its first kerb in the sheet's table, a run continuing round every
	 * corner where the sheet's kerbs meet end to end: cour.west turns into avenue.west; walk-east.outer runs from the
	 * avenue's east kerb through cour.east-south and the walk's outer kerb, the ring's long arc, path-north.east and the
	 * terrace's outer arc to its north end kerb; walk-east.inner from cour.east-north through the walk's inner kerb, the
	 * ring's short arc, path-north.west and the terrace's outer arc to its south end kerb.
	 */
	[[nodiscard]] TArray<TPair<FString, FRun>> KerbRuns();
}
