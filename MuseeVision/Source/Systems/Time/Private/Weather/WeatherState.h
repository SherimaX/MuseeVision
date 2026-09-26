#pragma once

#include "CoreMinimal.h"

/**
 * The weather at one place and instant, in physical units: what an observer's instruments would read. Time keeps the
 * weather; Matter wets and whitens surfaces from it, Nature makes the rain and snow in the air, Sound lets it be
 * heard (Decision D-005 lets a room's glazing take another place's weather).
 */
namespace Musee::Time
{
	struct FWeatherState
	{
		/** Air temperature at screen height (2 m). */
		double AirTemperatureCelsius = 0.0;
		/** The fraction of the sky covered by cloud, 0 to 1. */
		double CloudCover = 0.0;
		/** Liquid precipitation: the rate a rain gauge would fill. */
		double RainMillimetersPerHour = 0.0;
		/** Snowfall, as its liquid-water equivalent. */
		double SnowfallMillimetersPerHour = 0.0;
		/** Mean wind at 10 m, and the direction it blows from, clockwise from true north. */
		double WindSpeedMetersPerSecond = 0.0;
		double WindFromDegrees = 0.0;

		double PrecipitationMillimetersPerHour() const { return RainMillimetersPerHour + SnowfallMillimetersPerHour; }
	};

	/** A reading of the live feed: the weather at the UTC instant it describes, and the snow lying, if reported. */
	struct FWeatherObservation
	{
		FDateTime Utc;
		FWeatherState State;
		TOptional<double> SnowDepthMeters;
	};
}
