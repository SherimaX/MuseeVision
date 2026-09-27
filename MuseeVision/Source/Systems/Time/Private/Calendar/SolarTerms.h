#pragma once

#include "CoreMinimal.h"
#include "Almanac/AstronomicalTime.h"

/**
 * The 24 solar terms (jieqi) of the Chinese calendar: the year divided by the Sun's apparent longitude into 24
 * spans of 15 degrees. The museum counts them as the old museum showed them, from Lichun (the start of spring, the
 * Sun at 315 degrees) to Dahan (major cold). Their keys and names, in the same order, are Data/Time/SolarTerms.json.
 * The terms always follow the real Sun (Decision D-003).
 */
namespace Musee::Time
{
	inline constexpr int32 SolarTermCount = 24;
	/** Lichun begins when the Sun's apparent longitude reaches 315 degrees; each term spans 15 degrees. */
	inline constexpr double FirstSolarTermLongitudeDegrees = 315.0;
	inline constexpr double SolarTermSpanDegrees = 15.0;

	/** Where the Sun is in the terms: the term's index (0 Lichun ... 23 Dahan) and how far through it, [0, 1). */
	struct FSolarTermPlace
	{
		int32 Index = 0;
		double Progress = 0.0;
	};

	/** The term in force when the Sun's apparent longitude has the given value. */
	FSolarTermPlace SolarTermAt(double ApparentSolarLongitudeDegrees);

	/** The apparent solar longitude at which a term begins. */
	double SolarTermStartLongitudeDegrees(int32 Index);

	/** The term in force at an instant, with the UTC instants it began and ends. */
	struct FSolarTermSpan
	{
		FSolarTermPlace Place;
		FDateTime StartUtc;
		FDateTime EndUtc;
	};

	FSolarTermSpan SolarTermSpan(const FInstant& Instant);
}
