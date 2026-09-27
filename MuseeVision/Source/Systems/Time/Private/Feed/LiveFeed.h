#pragma once

#include "CoreMinimal.h"
#include "Almanac/Almanac.h"
#include "Weather/WeatherState.h"

/**
 * The live weather feed: Open-Meteo's current conditions for every one of Time's places, asked in one request. The
 * answer is kept on disk as it came (wrapped as a record file), read back through Foundation's record reader, and
 * turned into one observation per place in physical units. Time is the only system that uses HTTP.
 */
namespace Musee::Time::LiveFeed
{
	/** The request for current conditions at the places, in their order, with the units the museum reads. */
	FString RequestUrl(const FString& Endpoint, TConstArrayView<FObserver> Places);

	/** Where the last answer for a museum city is kept: Saved/Musee/Time/LiveWeather.<city>.json. */
	FString CachePath(FName City);

	/** The feed's answer (a JSON array, one object per place) as a record file holding those objects as its records. */
	FString AsRecordFile(const FString& Answer);

	/**
	 * Reads a kept answer into one observation per place, in the order asked. False, with the reason, when the file
	 * can't be read, holds another number of places, or reports a quantity in a unit other than the one asked for.
	 */
	bool Read(const FString& Path, int32 PlaceCount, TArray<FWeatherObservation>& OutObservations, FString& OutError);
}
