#pragma once

#include "CoreMinimal.h"
#include "Weather/WeatherPlaces.h"

#if WITH_DEV_AUTOMATION_TESTS

// Test fixtures for the weather: two contrasting climates and the design the museum declares. The climates are
// shaped like the museum's places (a monsoon continental city and a maritime one) but are fixtures, not their records:
// the generator's fidelity to whatever normals it is given is what the tests measure.
namespace Musee::Time::Tests
{
	/** Fills a month of normals. */
	inline FClimateMonth ClimateMonth(double Maximum, double Minimum, double Precipitation, double WetDays, double Sunshine,
		double Wind, double WindFrom)
	{
		FClimateMonth Month;
		Month.MeanMaximumCelsius = Maximum;
		Month.MeanMinimumCelsius = Minimum;
		Month.PrecipitationMillimeters = Precipitation;
		Month.WetDays = WetDays;
		Month.SunshineHours = Sunshine;
		Month.WindSpeedMetersPerSecond = Wind;
		Month.WindFromDegrees = WindFrom;
		return Month;
	}

	/** A continental climate with a summer monsoon: cold dry winters, hot wet summers (wet days counted from 0.1 mm). */
	inline FWeatherPlace ContinentalPlace(uint64 Seed)
	{
		FWeatherPlace Place;
		Place.Observer = FObserver::At(40.0, 116.0, 50.0);
		Place.Seed = Seed;
		Place.Normals.WetDayThresholdMillimeters = 0.1;
		const FClimateMonth Months[NormalsMonthCount] = {
			ClimateMonth(2, -7, 2, 1.6, 190, 3.3, 0), ClimateMonth(6, -4, 6, 2.3, 190, 3.3, 0), ClimateMonth(13, 2, 9, 3.0, 230, 3.6, 0),
			ClimateMonth(21, 9, 22, 4.7, 240, 3.9, 0), ClimateMonth(27, 15, 36, 6.0, 265, 3.8, 180), ClimateMonth(31, 20, 72, 10.0, 220, 3.0, 180),
			ClimateMonth(32, 23, 170, 11.9, 190, 2.6, 180), ClimateMonth(31, 22, 113, 10.5, 205, 2.4, 180), ClimateMonth(27, 16, 54, 7.1, 205, 2.7, 180),
			ClimateMonth(19, 9, 29, 5.2, 200, 3.0, 0), ClimateMonth(10, 1, 14, 2.9, 175, 3.1, 0), ClimateMonth(4, -5, 2, 1.6, 177, 3.2, 0),
		};
		for (int32 Month = 0; Month < NormalsMonthCount; ++Month) { Place.Normals.Months[Month] = Months[Month]; }
		return Place;
	}

	/** A maritime climate: mild, cloudy and wet all year (wet days counted from 1 mm). */
	inline FWeatherPlace MaritimePlace(uint64 Seed)
	{
		FWeatherPlace Place;
		Place.Observer = FObserver::At(51.5, -0.1, 10.0);
		Place.Seed = Seed;
		Place.Normals.WetDayThresholdMillimeters = 1.0;
		const FClimateMonth Months[NormalsMonthCount] = {
			ClimateMonth(8.4, 2.7, 59, 11.5, 61, 4.2, 270), ClimateMonth(9.0, 2.7, 45, 9.5, 79, 4.0, 270), ClimateMonth(11.7, 4.1, 39, 8.5, 124, 4.0, 270),
			ClimateMonth(15.0, 6.0, 42, 8.8, 177, 3.6, 270), ClimateMonth(18.4, 9.1, 46, 8.0, 207, 3.7, 270), ClimateMonth(21.6, 12.0, 47, 8.3, 208, 3.6, 270),
			ClimateMonth(23.9, 14.2, 46, 7.9, 218, 3.7, 270), ClimateMonth(23.4, 14.1, 53, 8.4, 202, 3.6, 270), ClimateMonth(20.2, 11.6, 50, 7.9, 157, 3.5, 270),
			ClimateMonth(15.8, 8.8, 65, 10.8, 115, 3.7, 270), ClimateMonth(11.5, 5.3, 67, 11.2, 71, 3.6, 270), ClimateMonth(8.8, 3.1, 57, 10.8, 55, 4.0, 270),
		};
		for (int32 Month = 0; Month < NormalsMonthCount; ++Month) { Place.Normals.Months[Month] = Months[Month]; }
		return Place;
	}

	/** The design Data/Time/Weather.json declares. */
	inline FWeatherDesign MuseumWeatherDesign()
	{
		FWeatherDesign Design;
		Design.Smoothing.TemperatureSeconds = 300.0;
		Design.Smoothing.CloudCoverSeconds = 300.0;
		Design.Smoothing.PrecipitationSeconds = 120.0;
		Design.Smoothing.WindSeconds = 180.0;
		Design.History.RainMemoryHours = {1.0, 6.0, 48.0};
		Design.History.SnowMeltMillimetersPerDayCelsius = 3.0;
		Design.LiveValidMinutes = 90.0;
		Design.LongestStepSeconds = 60.0;
		Design.RespinAfterMinutes = 60.0;
		Design.SpinUpDays = 60.0;
		Design.SpinUpStepMinutes = 60.0;
		const double Maximum[NormalsMonthCount] = {5.72, 5.44, 5.50, 5.44, 4.83, 3.94, 3.44, 3.22, 3.61, 4.11, 4.72, 5.17};
		const double Minimum[NormalsMonthCount] = {5.50, 4.94, 4.50, 3.67, 3.22, 2.94, 2.56, 2.50, 3.28, 3.72, 4.22, 4.61};
		FWeatherGeneratorDesign& Generator = Design.Generator;
		for (int32 Month = 0; Month < NormalsMonthCount; ++Month)
		{
			Generator.MaximumSpreadCelsius[Month] = Maximum[Month];
			Generator.MinimumSpreadCelsius[Month] = Minimum[Month];
		}
		Generator.AnomalyMemoryDays = 2.5;
		Generator.WetDayCloudCover = 0.85;
		Generator.CloudCoverPrecision = 2.0;
		Generator.RainingCloudCover = 0.95;
		Generator.RainCloudLeadHours = 2.0;
		Generator.RainMeanHours = 4.0;
		Generator.RainShortestHours = 1.0;
		Generator.RainLongestHours = 20.0;
		Generator.SnowToRainBandCelsius = 2.0;
		Generator.WindDirectionSpreadDegrees = 45.0;
		return Design;
	}
}

#endif
