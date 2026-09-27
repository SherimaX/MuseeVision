#pragma once

#include "CoreMinimal.h"

#include "TimeWeather.generated.h"

/**
 * The weather at one of Time's weather places (Decision D-005: `museum`, `giverny`, `london`) at the museum's instant,
 * as an observer's instruments would read it, with what the recent weather has left on the ground. Matter wets and
 * whitens surfaces from it, Nature makes rain and snow from it, Light dims the sky under its cloud.
 */
USTRUCT()
struct MUSEETIME_API FTimeWeather
{
	GENERATED_BODY()

	/** The place's key: `museum`, `giverny` or `london`. */
	UPROPERTY()
	FName Place;

	/** Whether the weather follows the live feed now; otherwise the place's seeded generator gives it. */
	UPROPERTY()
	bool bLive = false;

	/** Air temperature at screen height (2 m). */
	UPROPERTY(meta = (Units = "Celsius"))
	double AirTemperatureCelsius = 0.0;

	/** The fraction of the sky covered by cloud, 0 to 1. */
	UPROPERTY()
	double CloudCover = 0.0;

	/** Rain falling now, in millimetres an hour, as a rain gauge fills. */
	UPROPERTY()
	double RainMillimetersPerHour = 0.0;

	/** Snow falling now, as millimetres of water an hour. */
	UPROPERTY()
	double SnowfallMillimetersPerHour = 0.0;

	/** Mean wind at 10 m. */
	UPROPERTY(meta = (Units = "MetersPerSecond"))
	double WindSpeedMetersPerSecond = 0.0;

	/** The direction the wind blows from, clockwise from true north. */
	UPROPERTY(meta = (Units = "Degrees"))
	double WindFromDegrees = 0.0;

	/** How wet an exposed impervious surface is, 0 (dry) to 1 (its depression storage full). */
	UPROPERTY()
	double Wetness = 0.0;

	/** The fraction of open ground under lying snow, 0 to 1. */
	UPROPERTY()
	double SnowCover = 0.0;

	/** The depth of lying snow. */
	UPROPERTY(meta = (Units = "Meters"))
	double SnowDepthMeters = 0.0;
};
