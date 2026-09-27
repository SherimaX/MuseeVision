#pragma once

#include "CoreMinimal.h"
#include "Weather/WeatherState.h"

namespace Musee::Time
{
	/** The history's design, from Data/Time/Weather.json. */
	struct FPrecipitationHistoryDesign
	{
		/** The memory times of the rain records, in hours. */
		TArray<double> RainMemoryHours;
		/** Snowmelt per degree above freezing, in millimetres of water per day per degree Celsius. */
		double SnowMeltMillimetersPerDayCelsius = 0.0;
	};

	/**
	 * What the recent weather has left: water that reached the ground, and snow lying on it. Matter's surface law reads
	 * these to wet and dry, whiten and thaw each surface by its own porosity and exposure; it cannot integrate the past
	 * per pixel, so Time integrates it once, as few numbers.
	 *
	 * Water reaching the ground (rain and snowmelt) is kept as exponentially fading records, one per memory time tau:
	 * R(t) = integral of r(t') exp(-(t - t') / tau) dt', in millimetres. Held rain of r mm/h fills a record towards
	 * r tau; after the rain it fades with e-folding time tau. A surface that dries over some hours is modelled by
	 * weighting the records.
	 *
	 * Snow on the ground is its water equivalent and its depth. Fresh snow falls at the density Hedstrom and Pomeroy
	 * (1998) measured for the air temperature; lying snow settles towards 300 kg m^-3 with an e-folding time of 100 hours
	 * (Verseghy, CLASS, International Journal of Climatology 11, 1991); it melts by the degree-day law (Hock, "Temperature
	 * index melt modelling in mountain areas", Journal of Hydrology 282, 2003). A reported depth replaces the modelled one.
	 */
	class FPrecipitationHistory
	{
	public:
		explicit FPrecipitationHistory(const FPrecipitationHistoryDesign& InDesign);

		/** Advances by some seconds of weather held over them. */
		void Advance(const FWeatherState& Weather, double Seconds);
		/** A reported snow depth (the live feed's), which replaces the modelled one. */
		void ObserveSnowDepth(double DepthMeters);
		/** Forgets everything: a dry, bare ground. */
		void Reset();

		int32 RainRecordCount() const { return RainRecords.Num(); }
		double RainMemoryHours(int32 Record) const { return Design.RainMemoryHours[Record]; }
		/** The faded total of water that reached the ground, for one memory time. */
		double RainRecordMillimeters(int32 Record) const { return RainRecords[Record]; }
		/** Hours since water last reached the ground at a measurable rate; unset if it has not in this history. */
		TOptional<double> HoursSinceWet() const;
		double SnowWaterMillimeters() const { return SnowWater; }
		double SnowDepthMeters() const { return SnowDepth; }
		double SnowDensityKilogramsPerCubicMeter() const;

	private:
		void AdvanceSnow(const FWeatherState& Weather, double Seconds, double& OutMeltMillimetersPerHour);

		FPrecipitationHistoryDesign Design;
		TArray<double> RainRecords;
		TOptional<double> SecondsSinceWet;
		double SnowWater = 0.0;
		double SnowDepth = 0.0;
	};
}
