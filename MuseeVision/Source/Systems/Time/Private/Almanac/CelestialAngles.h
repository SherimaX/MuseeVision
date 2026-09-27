#pragma once

#include "CoreMinimal.h"

/**
 * Angles as the almanac writes them. Meeus's Astronomical Algorithms and the NREL Solar Position Algorithm state
 * every formula in degrees, so the almanac keeps degrees throughout and converts only at the trigonometric call.
 */
namespace Musee::Time
{
	inline constexpr double DegreesPerTurn = 360.0;
	inline constexpr double HalfTurnDegrees = 180.0;
	inline constexpr double ArcsecondsPerDegree = 3600.0;
	/** The Earth turns 15 degrees of hour angle in an hour of mean solar time (360 degrees in 24 hours). */
	inline constexpr double DegreesPerHour = 15.0;

	inline double SinDegrees(double Degrees) { return FMath::Sin(FMath::DegreesToRadians(Degrees)); }
	inline double CosDegrees(double Degrees) { return FMath::Cos(FMath::DegreesToRadians(Degrees)); }
	inline double TanDegrees(double Degrees) { return FMath::Tan(FMath::DegreesToRadians(Degrees)); }
	inline double AsinDegrees(double Value) { return FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(Value, -1.0, 1.0))); }
	inline double AcosDegrees(double Value) { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Value, -1.0, 1.0))); }
	inline double AtanDegrees(double Value) { return FMath::RadiansToDegrees(FMath::Atan(Value)); }
	inline double Atan2Degrees(double Y, double X) { return FMath::RadiansToDegrees(FMath::Atan2(Y, X)); }

	/** The angle reduced to one turn, [0, 360). */
	inline double ReduceDegrees(double Degrees)
	{
		double Reduced = FMath::Fmod(Degrees, DegreesPerTurn);
		if (Reduced < 0.0) { Reduced += DegreesPerTurn; }
		return Reduced >= DegreesPerTurn ? Reduced - DegreesPerTurn : Reduced;
	}

	/** The angle reduced to (-180, 180]: the shorter way round. */
	inline double SignedDegrees(double Degrees)
	{
		const double Reduced = ReduceDegrees(Degrees);
		return Reduced > HalfTurnDegrees ? Reduced - DegreesPerTurn : Reduced;
	}
}
