#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

/**
 * Time's settings and commands: `time.hour` (live, or a civil hour the clock is set to), `time.city` (the city the
 * museum stands in), and the hour and city keys, `musee.time.advance-hour` and `musee.time.advance-city`.
 */
namespace Musee::Time::Settings
{
	/** The hour setting's value while the clock is live: no hour of the day is negative. */
	inline constexpr double LiveHour = -1.0;
	/** The last hour the clock may be set to: one second before midnight. */
	inline constexpr double LastHour = 24.0 - 1.0 / 3600.0;

	FMuseeKey HourKey();
	FMuseeKey CityKey();

	/** The hour setting as the clock takes it: unset while live. */
	TOptional<double> ChosenHour();
	/** The city setting, or none while it isn't registered. */
	FName ChosenCity();

	/** Registers the settings and commands (the module's startup); false if Time's records can't be read. */
	bool Register();
	/** Unregisters them (the module's shutdown). */
	void Unregister();
}
