#include "Almanac/LunarPhase.h"

#include "Almanac/CelestialAngles.h"
#include "Almanac/LunarCoordinates.h"
#include "Almanac/SolarCoordinates.h"

namespace Musee::Time::LunarPhaseConstants
{
	/** The astronomical unit, 149597870.7 km (IAU 2012 Resolution B2). */
	constexpr double KilometersPerAu = 149597870.7;
	/** The mean synodic month, 29.530588861 days (Meeus eq. 49.1), and the Moon's mean gain on the Sun per day. */
	constexpr double SynodicMonthDays = 29.530588861;
	constexpr double MeanElongationDegreesPerDay = DegreesPerTurn / SynodicMonthDays;
	/** A phase is found when the longitudes are within 1e-7 degree, about 0.7 ms of time. */
	constexpr double PhaseToleranceDegrees = 1e-7;
	constexpr int32 PhaseIterations = 16;

	double LongitudeDifferenceAt(double JulianEphemerisDay)
	{
		const double MoonLongitude = LunarCoordinates(JulianEphemerisDay).ApparentLongitudeDegrees;
		const double SunLongitude = SolarCoordinates(JulianEphemerisDay).ApparentLongitudeDegrees;
		return ReduceDegrees(MoonLongitude - SunLongitude);
	}
}

namespace Musee::Time
{
	FLunarPhase LunarPhase(const FLunarCoordinates& Lunar, const FSolarCoordinates& Solar)
	{
		using namespace LunarPhaseConstants;
		FLunarPhase Phase;
		const double RightAscensionGap = Solar.RightAscensionDegrees - Lunar.RightAscensionDegrees;
		Phase.ElongationDegrees = AcosDegrees(SinDegrees(Solar.DeclinationDegrees) * SinDegrees(Lunar.DeclinationDegrees)
			+ CosDegrees(Solar.DeclinationDegrees) * CosDegrees(Lunar.DeclinationDegrees) * CosDegrees(RightAscensionGap));

		const double SunKilometers = Solar.DistanceAu() * KilometersPerAu;
		Phase.PhaseAngleDegrees = Atan2Degrees(SunKilometers * SinDegrees(Phase.ElongationDegrees),
			Lunar.DistanceKilometers - SunKilometers * CosDegrees(Phase.ElongationDegrees));
		Phase.IlluminatedFraction = (1.0 + CosDegrees(Phase.PhaseAngleDegrees)) / 2.0;

		Phase.BrightLimbAngleDegrees = ReduceDegrees(Atan2Degrees(
			CosDegrees(Solar.DeclinationDegrees) * SinDegrees(RightAscensionGap),
			SinDegrees(Solar.DeclinationDegrees) * CosDegrees(Lunar.DeclinationDegrees)
				- CosDegrees(Solar.DeclinationDegrees) * SinDegrees(Lunar.DeclinationDegrees) * CosDegrees(RightAscensionGap)));
		Phase.LongitudeDifferenceDegrees = ReduceDegrees(Lunar.ApparentLongitudeDegrees - Solar.ApparentLongitudeDegrees);
		return Phase;
	}

	double LunarPhaseInstant(double TargetDegrees, double JulianEphemerisDayGuess)
	{
		using namespace LunarPhaseConstants;
		double JulianEphemerisDay = JulianEphemerisDayGuess;
		for (int32 Iteration = 0; Iteration < PhaseIterations; ++Iteration)
		{
			const double Remaining = SignedDegrees(TargetDegrees - LongitudeDifferenceAt(JulianEphemerisDay));
			JulianEphemerisDay += Remaining / MeanElongationDegreesPerDay;
			if (FMath::Abs(Remaining) < PhaseToleranceDegrees) { break; }
		}
		return JulianEphemerisDay;
	}

	double PreviousNewMoon(double JulianEphemerisDay)
	{
		using namespace LunarPhaseConstants;
		const double SinceNew = LongitudeDifferenceAt(JulianEphemerisDay) / MeanElongationDegreesPerDay;
		double NewMoon = LunarPhaseInstant(0.0, JulianEphemerisDay - SinceNew);
		if (NewMoon > JulianEphemerisDay)
		{
			// The Moon's true motion can put the nearest new Moon just after the instant; the one before is a month back.
			NewMoon = LunarPhaseInstant(0.0, NewMoon - SynodicMonthDays);
		}
		return NewMoon;
	}
}
