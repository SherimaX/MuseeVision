#pragma once

#include "CoreMinimal.h"

/**
 * An arris as the mason or the joiner leaves it: sharp, eased to a small round, or cut by a small chamfer at 45 degrees
 * (the V of a V-joint between two such edges). A sharp arris chips at once in use, so worked stone and timber have their
 * edges eased: a bench seat's arrises rounded 10 mm, a bronze foot's top edges 6 mm, stone slabs' V-joints.
 */
struct FFormArris
{
	/** The size of the easing, metres: the round's radius, or the chamfer's leg on each face. Zero: sharp. */
	double Meters = 0.0;

	/** A chamfer at 45 degrees rather than a round. */
	bool bChamfer = false;

	/** An eased arris of this radius. */
	[[nodiscard]] static FFormArris Round(double InMeters) { return {InMeters, false}; }

	/** A chamfered arris of this leg. */
	[[nodiscard]] static FFormArris Chamfer(double InMeters) { return {InMeters, true}; }

	[[nodiscard]] bool IsSharp() const { return Meters <= 0.0; }
};
