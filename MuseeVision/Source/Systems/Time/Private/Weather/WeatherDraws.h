#pragma once

#include "CoreMinimal.h"

namespace Musee::Time
{
	/** The separate purposes a day's draws serve, so that each is independent of the others. */
	enum class EWeatherStream : uint32
	{
		Occurrence,
		Temperature,
		WindSpeed,
		WindDirection,
		Amount,
		Cloud,
		RainTiming,
	};

	/**
	 * Reproducible random draws for one place, one day and one purpose. Each draw is the SplitMix64 mix (Steele, Lea
	 * and Flood, "Fast splittable pseudorandom number generators", OOPSLA 2014) of the place's seed, the day, the
	 * stream and a counter, so any day's weather can be drawn directly and always comes out the same.
	 */
	class FWeatherDraws
	{
	public:
		FWeatherDraws(uint64 Seed, int64 DayIndex, EWeatherStream Stream);

		/** Uniform in (0, 1): never exactly 0 or 1. */
		double Uniform();
		/** Standard normal (Box-Muller). */
		double Normal();
		/** Gamma with the given shape and unit scale (Marsaglia and Tsang, ACM TOMS 26, 2000; shapes below 1 by
		 *  Stuart's boost, Gamma(a) = Gamma(a + 1) U^(1/a)). */
		double Gamma(double Shape);
		/** Beta, as the ratio of two gammas. */
		double Beta(double A, double B);
		/** Exponential with unit mean. */
		double Exponential();

	private:
		uint64 Key = 0;
		uint64 Counter = 0;
	};
}
