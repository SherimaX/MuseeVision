#pragma once

#include "CoreMinimal.h"

/**
 * How a city sets its clocks: a standard offset from UTC and, where the law says so, daylight saving. Civil time is
 * what the museum's visitors read on a watch; the museum's clock shows the civil time of its city.
 */
namespace Musee::Time
{
	/** The daylight-saving laws Time knows. */
	enum class EDaylightSaving : uint8
	{
		/** No daylight saving: China, which has kept UTC+8 all year since 1992. */
		None,
		/**
		 * The United States (15 U.S.C. 260a): from 2007 (Energy Policy Act of 2005), 02:00 local standard time on the
		 * second Sunday of March to 02:00 local daylight time on the first Sunday of November; 1987-2006, the first
		 * Sunday of April to the last Sunday of October; 1967-1986, the last Sunday of April to the last Sunday of
		 * October. Earlier years, and the 1974-75 emergency year-round rule, are outside the museum's time.
		 */
		UnitedStates,
		/**
		 * European summer time (EU Directive 2000/84/EC, the rule in force since 1996, which the United Kingdom's
		 * Summer Time Order 2002 follows): 01:00 UTC on the last Sunday of March to 01:00 UTC on the last Sunday of
		 * October. Earlier national rules are outside the museum's time.
		 */
		European,
	};

	struct FZoneRule
	{
		double StandardOffsetHours = 0.0;
		EDaylightSaving DaylightSaving = EDaylightSaving::None;

		/** Whether daylight saving is in force at a UTC instant. */
		bool IsDaylightSaving(const FDateTime& Utc) const;
		/** Civil time minus UTC at a UTC instant. */
		FTimespan UtcOffset(const FDateTime& Utc) const;
		/** The civil (wall-clock) time at a UTC instant. */
		FDateTime CivilFromUtc(const FDateTime& Utc) const;
		/**
		 * The UTC instant of a civil time. In the spring gap, where the civil time never occurs, it is read on standard
		 * time (so 02:30 on the US spring-forward day is 03:30 daylight time). In the autumn hour that occurs twice,
		 * the first occurrence (daylight time) is taken.
		 */
		FDateTime UtcFromCivil(const FDateTime& Civil) const;
		/** The UTC instant of local midnight at the start of a civil date (its time of day is ignored). */
		FDateTime CivilMidnightUtc(const FDateTime& CivilDate) const;
	};
}
