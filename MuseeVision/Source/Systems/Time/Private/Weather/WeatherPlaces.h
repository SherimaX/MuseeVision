#pragma once

#include "CoreMinimal.h"
#include "Weather/PrecipitationHistory.h"
#include "Weather/WeatherGenerator.h"
#include "Weather/WeatherSmoother.h"

namespace Musee::Time
{
	/** How the museum keeps its weather, from Data/Time/Weather.json. */
	struct FWeatherDesign
	{
		FWeatherSmoothing Smoothing;
		FPrecipitationHistoryDesign History;
		FWeatherGeneratorDesign Generator;
		/** A live reading stands for the weather this long either side of the instant it describes. */
		double LiveValidMinutes = 0.0;
		/** The longest step the easing and the history take; a longer advance is cut into such steps. */
		double LongestStepSeconds = 0.0;
		/** A jump of the museum's clock further than this, or backwards, rebuilds the history instead of stepping. */
		double RespinAfterMinutes = 0.0;
		/** How far back the history is rebuilt from the generator after a jump, and the step it is rebuilt in. */
		double SpinUpDays = 0.0;
		double SpinUpStepMinutes = 0.0;
	};

	/**
	 * The weather Time keeps: one place per key (Decision D-005: museum, giverny and london). Each place follows its
	 * live reading while the reading is current for the museum's instant, and its seeded generator otherwise, so a
	 * set hour far from the real one, or a lost feed, still has plausible weather of its own. What the museum shows
	 * is eased towards that target, and the ground's history integrates what it shows.
	 */
	class FWeatherPlaces
	{
	public:
		explicit FWeatherPlaces(const FWeatherDesign& InDesign);

		/** Keeps (or replaces, starting afresh) a place under a key. */
		void SetPlace(FName Key, const FWeatherPlace& Place);
		void RemovePlace(FName Key);
		/** A live reading for a place. A reported snow depth is taken at once when the reading is current. */
		void Observe(FName Key, const FWeatherObservation& Observation);
		/** Brings every place to the museum's instant. */
		void Advance(const FDateTime& Utc);

		const FWeatherState* Weather(FName Key) const;
		const FPrecipitationHistory* History(FName Key) const;
		/** Whether the place's target at the last advance was its live reading. */
		bool IsLive(FName Key) const;
		TArray<FName> Keys() const;

	private:
		struct FKeptPlace
		{
			TUniquePtr<FWeatherGenerator> Generator;
			TUniquePtr<FPrecipitationHistory> History;
			FWeatherSmoother Smoother;
			TOptional<FWeatherObservation> Live;
			TOptional<FDateTime> LastUtc;
			bool bLive = false;
		};

		bool IsCurrent(const FWeatherObservation& Observation, const FDateTime& Utc) const;
		FWeatherState TargetAt(const FKeptPlace& Place, const FDateTime& Utc, bool& bOutLive) const;
		void Respin(FKeptPlace& Place, const FDateTime& Utc) const;
		void Step(FKeptPlace& Place, const FDateTime& Utc) const;

		FWeatherDesign Design;
		TMap<FName, FKeptPlace> Places;
	};
}
