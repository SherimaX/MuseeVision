#pragma once

#include "CoreMinimal.h"

namespace Musee::Motion
{
	/**
	 * One run of a drive from rest to rest, as a lift's controller commands it: the jerk-limited S-curve. The jerk ramps
	 * the acceleration up and down, so the rider never feels a step in it; the acceleration is held at its limit; the
	 * speed cruises at its limit. Seven phases: jerk up, hold, jerk down, cruise, and their mirror. A run too short to
	 * reach the speed limit peaks lower and does not cruise; one too short to reach the acceleration limit has no hold.
	 * Ride quality is judged by exactly these three quantities (ISO 18738-1, the measurement of lift ride quality);
	 * the limits come from the stage's Mechanisms.json.
	 *
	 * Units are the travel's: metres (or degrees for a revolute joint) and seconds. The run is time-optimal under its
	 * limits and symmetric, and it is computed in closed form, so it is the same on every machine.
	 */
	class MUSEEMOTION_API FSCurve
	{
	public:
		/** The run's state at an instant: the distance covered, the speed and the acceleration. */
		struct FSample
		{
			double Covered = 0.0;
			double Speed = 0.0;
			double Accel = 0.0;
		};

		/** A run of no distance. */
		FSCurve() = default;

		/** The run over Distance (zero or more) under a speed, an acceleration and a jerk limit (each more than zero). */
		[[nodiscard]] static FSCurve RestToRest(double Distance, double SpeedLimit, double AccelLimit, double JerkLimit);

		/** The distance the run covers. */
		[[nodiscard]] double Distance() const { return Length; }

		/** How long the run takes. */
		[[nodiscard]] double Duration() const { return Total; }

		/** The highest speed the run reaches: its cruise, or its peak when it is too short to cruise. */
		[[nodiscard]] double PeakSpeed() const;

		/** The run's state a time after it starts: at rest where it began before 0, at rest where it ends after Duration. */
		[[nodiscard]] FSample At(double Seconds) const;

	private:
		/** One phase: how long it lasts, its constant jerk, and the state it starts from. */
		struct FPhase
		{
			double Seconds = 0.0;
			double Jerk = 0.0;
			FSample Start;
		};

		TArray<FPhase> Phases;
		double Length = 0.0;
		double Total = 0.0;
	};
}
