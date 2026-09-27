#pragma once

#include "CoreMinimal.h"

/**
 * How a drive moves its body from one state to the next: the share of the move covered at each share of its time.
 * A real drive starts and stops without a jolt, so the museum's machines use the minimum-jerk profile, the smoothest
 * move between two rests (Flash and Hogan, "The coordination of arm movements", J. Neurosci. 5(7), 1985): position
 * 10t^3 - 15t^4 + 6t^5, whose speed and acceleration are both zero at each end. Its peak speed is 1.875 times the
 * average, reached halfway; its peak acceleration is 5.77 times travel over time squared, at t = 0.211.
 */
namespace Musee::Motion
{
	/** A move's profile, named in Mechanisms.json. */
	enum class EProfile : uint8
	{
		Linear,
		EaseInOut,
	};

	/** The profile a name spells (`linear`, `ease-in-out`), or nothing. */
	[[nodiscard]] MUSEEMOTION_API TOptional<EProfile> ParseProfile(FName Name);

	/** The share of the move covered at share T of its time (T clamped to 0..1). */
	[[nodiscard]] MUSEEMOTION_API double Covered(EProfile Profile, double T);

	/** The speed of the move at share T of its time, as a multiple of its average speed. */
	[[nodiscard]] MUSEEMOTION_API double RelativeSpeed(EProfile Profile, double T);
}
