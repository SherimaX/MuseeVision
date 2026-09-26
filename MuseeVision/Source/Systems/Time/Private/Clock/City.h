#pragma once

#include "CoreMinimal.h"
#include "Almanac/Almanac.h"
#include "Clock/ZoneRule.h"

namespace Musee::Time
{
	/**
	 * A place Time knows by key: where it stands, for the sky, and how its clocks are set. The museum stands in one
	 * such city (Beijing by default, Decision D-002); the remote weather places are others (Decision D-005). The
	 * records, with their names and sources, are Data/Time/Cities.json.
	 */
	struct FCity
	{
		FName Key;
		FObserver Observer;
		FZoneRule Zone;
	};
}
