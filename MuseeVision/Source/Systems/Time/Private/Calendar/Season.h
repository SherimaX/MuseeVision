#pragma once

#include "CoreMinimal.h"

/**
 * The season, by one of the three reckonings in common use. The reckoning the museum keeps is declared in
 * Data/Time/Museum.json. Each is stated for the northern hemisphere and turned half a year for an observer south of
 * the equator.
 */
namespace Musee::Time
{
	enum class ESeason : uint8
	{
		Spring,
		Summer,
		Autumn,
		Winter,
	};

	enum class ESeasonReckoning : uint8
	{
		/** Equinoxes and solstices: spring from the Sun at 0 degrees of apparent longitude, summer from 90, and so on. */
		Astronomical,
		/** The Chinese calendar's four "starts" (the four li terms): spring from Lichun at 315 degrees, summer from
		 *  Lixia at 45, autumn from Liqiu at 135, winter from Lidong at 225, midway between equinox and solstice. */
		SolarTerms,
		/** The climatologists' whole months: spring March to May, summer June to August, and so on (WMO usage). */
		Meteorological,
	};

	/** The season and how far through it, [0, 1). */
	struct FSeasonPlace
	{
		ESeason Season = ESeason::Spring;
		double Progress = 0.0;
	};

	/**
	 * The season at an instant for an observer: from the Sun's apparent longitude for the astronomical and solar-term
	 * reckonings, from the civil date for the meteorological one.
	 */
	FSeasonPlace SeasonAt(ESeasonReckoning Reckoning, double ApparentSolarLongitudeDegrees, const FDateTime& CivilDate,
		double LatitudeDegrees);
}
