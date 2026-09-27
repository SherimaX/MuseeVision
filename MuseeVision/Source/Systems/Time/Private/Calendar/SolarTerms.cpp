#include "Calendar/SolarTerms.h"

#include "Almanac/CelestialAngles.h"
#include "Almanac/SolarCoordinates.h"

namespace Musee::Time::SolarTermSearch
{
	/** UTC of a Julian Ephemeris Day, taking Delta T at the instant itself. */
	FDateTime UtcOfEphemerisDay(double JulianEphemerisDay, double DeltaTGuessSeconds)
	{
		const FDateTime Estimate = UtcFromJulianDay(JulianEphemerisDay - DeltaTGuessSeconds / SecondsPerDay);
		return UtcFromJulianDay(JulianEphemerisDay - DeltaTSeconds(Estimate) / SecondsPerDay);
	}
}

namespace Musee::Time
{
	FSolarTermPlace SolarTermAt(double ApparentSolarLongitudeDegrees)
	{
		const double FromFirst = ReduceDegrees(ApparentSolarLongitudeDegrees - FirstSolarTermLongitudeDegrees);
		const double Terms = FromFirst / SolarTermSpanDegrees;
		FSolarTermPlace Place;
		Place.Index = FMath::Clamp(FMath::FloorToInt32(Terms), 0, SolarTermCount - 1);
		Place.Progress = FMath::Clamp(Terms - Place.Index, 0.0, 1.0);
		return Place;
	}

	double SolarTermStartLongitudeDegrees(int32 Index)
	{
		return ReduceDegrees(FirstSolarTermLongitudeDegrees + SolarTermSpanDegrees * Index);
	}

	FSolarTermSpan SolarTermSpan(const FInstant& Instant)
	{
		using namespace SolarTermSearch;
		const double JulianEphemerisDay = Instant.JulianEphemerisDay();
		FSolarTermSpan Span;
		Span.Place = SolarTermAt(SolarCoordinates(JulianEphemerisDay).ApparentLongitudeDegrees);

		// Each boundary is sought from its mean-motion estimate, well inside the half-year the search reaches.
		const double DaysIn = Span.Place.Progress * SolarTermSpanDegrees / SolarMeanMotionDegreesPerDay;
		const double DaysLeft = (1.0 - Span.Place.Progress) * SolarTermSpanDegrees / SolarMeanMotionDegreesPerDay;
		const double Start = SolarLongitudeInstant(SolarTermStartLongitudeDegrees(Span.Place.Index), JulianEphemerisDay - DaysIn);
		const double End = SolarLongitudeInstant(
			SolarTermStartLongitudeDegrees((Span.Place.Index + 1) % SolarTermCount), JulianEphemerisDay + DaysLeft);
		Span.StartUtc = UtcOfEphemerisDay(Start, Instant.DeltaTSeconds);
		Span.EndUtc = UtcOfEphemerisDay(End, Instant.DeltaTSeconds);
		return Span;
	}
}
