#pragma once

#include "CoreMinimal.h"
#include "Weather/WeatherState.h"

namespace Musee::Time
{
	/** How quickly each quantity follows its target: first-order time constants, from Data/Time/Weather.json. */
	struct FWeatherSmoothing
	{
		double TemperatureSeconds = 0.0;
		double CloudCoverSeconds = 0.0;
		double PrecipitationSeconds = 0.0;
		double WindSeconds = 0.0;
	};

	/**
	 * The weather the museum shows, eased towards its target. Readings of the live feed come in steps every quarter
	 * hour and the feed can switch to the generator; the sky must not jump. Each quantity relaxes exponentially
	 * towards its target, exactly for a target held over the step. The wind relaxes as a vector, so it veers the
	 * short way and a calm has no direction to swing through.
	 */
	class FWeatherSmoother
	{
	public:
		/** Starts at a state, with nothing to ease. */
		void Reset(const FWeatherState& State);
		/** Eases for a number of seconds towards a target held over them. The first call after construction starts at
		 *  the target. */
		const FWeatherState& Advance(const FWeatherState& Target, double Seconds, const FWeatherSmoothing& Smoothing);

		const FWeatherState& State() const { return Current; }
		bool IsStarted() const { return bStarted; }

	private:
		FWeatherState Current;
		/** The wind as a vector (towards east, towards north, m/s) blowing from the current direction. */
		FVector2D WindFrom = FVector2D::ZeroVector;
		bool bStarted = false;
	};
}
