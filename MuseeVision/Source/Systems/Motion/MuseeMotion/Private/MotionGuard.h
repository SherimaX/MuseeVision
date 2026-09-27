#pragma once

#include "CoreMinimal.h"

class USceneComponent;
class UWorld;

namespace Musee::Motion
{
	/**
	 * The occupancy guard's one question (D-014): would the body, moving from one world pose to the next, enter the
	 * visitor's body? Each solid part of the body (every primitive under it that collides, except the controls) is
	 * bounded by its own box. For a slide, the guard sweeps the slice each box is about to enter: its leading face
	 * pushed forward by the step and the clearance, and widened across the travel by the clearance. For a turn, it
	 * tests each box in its next pose, grown by the clearance. Only the visitor's Body channel is asked.
	 */
	[[nodiscard]] bool IsTravelOccupied(const UWorld& World, const USceneComponent& Body, const FTransform& From, const FTransform& To,
		double ClearanceMeters);

	/** The farthest any solid part of the body reaches from a line through Origin along Axis (world units), in metres. */
	[[nodiscard]] double ReachFromAxis(const USceneComponent& Body, const FVector& Origin, const FVector& Axis);

	/**
	 * Whether the visitor's body stands within a cylinder: RadiusMeters about a line through Base (world) along Axis, from
	 * Base up to HeightMeters along it. A body counts by its centre, as a lift's presence sensor sees a person on the
	 * floor it watches; one off that floor (above or below the cylinder) is not there.
	 */
	[[nodiscard]] bool IsVisitorWithin(const UWorld& World, const FVector& Base, const FVector& Axis, double RadiusMeters, double HeightMeters);
}
