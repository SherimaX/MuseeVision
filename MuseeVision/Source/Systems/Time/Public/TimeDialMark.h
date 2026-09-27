#pragma once

#include "CoreMinimal.h"

namespace Musee::Time
{
	/**
	 * One mark the museum's sun clock carries: a minute of the civil day at which the node's point of shadow falls on the
	 * hour ring, and where on the ring it falls. The stage lays its inlay at these marks (an hour line, a minute tick, a
	 * numeral) and never types a place by hand.
	 */
	struct FDialMark
	{
		/** The minute of the civil day, from midnight (360 is 06:00). */
		int32 Minute = 0;

		/** Where the shadow falls, in degrees about the dial's centre in its stage's frame, from +x (east) towards +y (south). */
		double AngleDegrees = 0.0;

		/** The same place as a unit direction from the centre, in the plane of the dial. */
		FVector2D Direction = FVector2D::UnitX();

		/** The civil hour, with its minutes as a fraction. */
		[[nodiscard]] double Hour() const { return Minute / 60.0; }

		/** The minute within its hour, 0 on the hour. */
		[[nodiscard]] int32 MinuteOfHour() const { return Minute % 60; }

		/** Whether it marks a whole hour. */
		[[nodiscard]] bool IsHour() const { return MinuteOfHour() == 0; }
	};
}
