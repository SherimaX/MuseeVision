#include "MotionSCurve.h"

namespace Musee::Motion
{
	namespace
	{
		/** The phases of a rest-to-rest run: jerk up, hold, jerk down, cruise, jerk down, hold, jerk up. */
		constexpr int32 PhaseCount = 7;

		/** The state after a time under constant jerk: the exact integrals of jerk to acceleration, speed and distance. */
		FSCurve::FSample Integrate(const FSCurve::FSample& From, double Jerk, double Seconds)
		{
			FSCurve::FSample To;
			To.Accel = From.Accel + Jerk * Seconds;
			To.Speed = From.Speed + From.Accel * Seconds + 0.5 * Jerk * Seconds * Seconds;
			To.Covered = From.Covered + From.Speed * Seconds + 0.5 * From.Accel * Seconds * Seconds + Jerk * Seconds * Seconds * Seconds / 6.0;
			return To;
		}

		/**
		 * The distance a run covers speeding up from rest to Speed. Above the knee speed a^2/j the acceleration reaches its
		 * limit and holds, and the distance is v (v/a + a/j) / 2; below it the acceleration peaks at sqrt(v j) and the
		 * distance is v sqrt(v/j).
		 */
		double SpeedingUpDistance(double Speed, double Accel, double Jerk)
		{
			return Speed >= Accel * Accel / Jerk
				? 0.5 * Speed * (Speed / Accel + Accel / Jerk)
				: Speed * FMath::Sqrt(Speed / Jerk);
		}

		/**
		 * The highest speed a run over Distance can reach and still stop: the speed limit when there is room to cruise,
		 * otherwise the speed at which speeding up and slowing down together cover the distance. With the acceleration
		 * held that is the root of v^2 + v a^2/j - a D = 0; without, of 2 v sqrt(v/j) = D.
		 */
		double PeakSpeedFor(double Distance, double Speed, double Accel, double Jerk)
		{
			if (2.0 * SpeedingUpDistance(Speed, Accel, Jerk) <= Distance)
			{
				return Speed;
			}
			const double Knee = Accel * Accel / Jerk;
			const double Held = 0.5 * (-Knee + FMath::Sqrt(Knee * Knee + 4.0 * Accel * Distance));
			if (Held >= Knee)
			{
				return Held;
			}
			return FMath::Pow(0.5 * Distance * FMath::Sqrt(Jerk), 2.0 / 3.0);
		}
	}

	FSCurve FSCurve::RestToRest(double Distance, double SpeedLimit, double AccelLimit, double JerkLimit)
	{
		FSCurve Curve;
		if (Distance <= 0.0 || SpeedLimit <= 0.0 || AccelLimit <= 0.0 || JerkLimit <= 0.0)
		{
			return Curve;
		}
		Curve.Length = Distance;
		const double Peak = PeakSpeedFor(Distance, SpeedLimit, AccelLimit, JerkLimit);
		const bool bHolds = Peak >= AccelLimit * AccelLimit / JerkLimit;
		const double JerkTime = bHolds ? AccelLimit / JerkLimit : FMath::Sqrt(Peak / JerkLimit);
		const double HoldTime = bHolds ? Peak / AccelLimit - AccelLimit / JerkLimit : 0.0;
		const double CruiseTime = FMath::Max(0.0, (Distance - 2.0 * SpeedingUpDistance(Peak, AccelLimit, JerkLimit)) / Peak);
		const double Durations[PhaseCount] = { JerkTime, HoldTime, JerkTime, CruiseTime, JerkTime, HoldTime, JerkTime };
		const double Jerks[PhaseCount] = { JerkLimit, 0.0, -JerkLimit, 0.0, -JerkLimit, 0.0, JerkLimit };
		FSample State;
		for (int32 Phase = 0; Phase < PhaseCount; ++Phase)
		{
			FPhase& Added = Curve.Phases.AddDefaulted_GetRef();
			Added.Seconds = Durations[Phase];
			Added.Jerk = Jerks[Phase];
			Added.Start = State;
			State = Integrate(State, Jerks[Phase], Durations[Phase]);
			Curve.Total += Durations[Phase];
		}
		return Curve;
	}

	double FSCurve::PeakSpeed() const
	{
		return Phases.Num() == PhaseCount ? Phases[3].Start.Speed : 0.0;
	}

	FSCurve::FSample FSCurve::At(double Seconds) const
	{
		FSample Rest;
		if (Seconds >= Total)
		{
			Rest.Covered = Length;
			return Rest;
		}
		double Left = FMath::Max(0.0, Seconds);
		for (const FPhase& Phase : Phases)
		{
			if (Left <= Phase.Seconds)
			{
				return Integrate(Phase.Start, Phase.Jerk, Left);
			}
			Left -= Phase.Seconds;
		}
		Rest.Covered = Length;
		return Rest;
	}
}
