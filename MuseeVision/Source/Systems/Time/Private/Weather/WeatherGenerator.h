#pragma once

#include "CoreMinimal.h"
#include "Almanac/Almanac.h"
#include "Weather/ClimateNormals.h"
#include "Weather/WeatherState.h"

namespace Musee::Time
{
	/** A place whose weather Time keeps: where it is, its climate, and its seed (from its key, by Foundation). */
	struct FWeatherPlace
	{
		FObserver Observer;
		FClimateNormals Normals;
		uint64 Seed = 0;
	};

	/**
	 * The numbers the generator needs that the normals do not give. Every field is read from Data/Time/Weather.json,
	 * where each is declared with its reasoning and source.
	 */
	struct FWeatherGeneratorDesign
	{
		/** Standard deviations of the daily maximum and minimum about their normals, by month. */
		TStaticArray<double, NormalsMonthCount> MaximumSpreadCelsius{InPlace, 0.0};
		TStaticArray<double, NormalsMonthCount> MinimumSpreadCelsius{InPlace, 0.0};
		/** The e-folding time of the weather's day-to-day anomalies: lag-one correlation exp(-1 / days). */
		double AnomalyMemoryDays = 0.0;
		/** The mean cloud cover of a wet day. */
		double WetDayCloudCover = 0.0;
		/** The precision (a + b) of the Beta distribution of a day's cloud cover about its mean. */
		double CloudCoverPrecision = 0.0;
		/** Cloud cover while rain falls, and the hours the cloud takes to build before it and clear after it. */
		double RainingCloudCover = 0.0;
		double RainCloudLeadHours = 0.0;
		/** A wet day's precipitation falls in one spell: its mean, shortest and longest duration in hours. */
		double RainMeanHours = 0.0;
		double RainShortestHours = 0.0;
		double RainLongestHours = 0.0;
		/** The width of the air-temperature band over which precipitation turns from snow to rain. */
		double SnowToRainBandCelsius = 0.0;
		/** The standard deviation of the day's wind direction about the month's prevailing direction. */
		double WindDirectionSpreadDegrees = 0.0;
	};

	/** One generated day at a place, on its local mean solar day. Hours are local mean solar time from midnight. */
	struct FGeneratedDay
	{
		int64 DayIndex = 0;
		bool bWet = false;
		/** The day's precipitation, as liquid water. */
		double PrecipitationMillimeters = 0.0;
		double MaximumCelsius = 0.0;
		double MinimumCelsius = 0.0;
		/** The day's mean cloud cover. */
		double CloudCover = 0.0;
		double WindSpeedMetersPerSecond = 0.0;
		double WindFromDegrees = 0.0;
		/** When the day's precipitation begins and how long it lasts. */
		double RainStartHour = 0.0;
		double RainHours = 0.0;
		/** The sun's rising and setting that shape the day's temperature. */
		double SunriseHour = 0.0;
		double SunsetHour = 0.0;
	};

	/** What the generator draws each calendar month from, derived from the normals. */
	struct FWeatherMonthStatistics
	{
		/** Wet-day transition probabilities of the first-order Markov chain. */
		double WetAfterDry = 0.0;
		double WetAfterWet = 0.0;
		/** A wet day's amount above the wet-day threshold: gamma with this shape and scale; a shape of 0 means every
		 *  wet day has the mean. */
		double AmountShape = 0.0;
		double AmountScaleMillimeters = 0.0;
		double MeanWetDayMillimeters = 0.0;
		/** The month's mean cloud cover (from its sunshine), and the means of its wet and dry days. */
		double MeanCloudCover = 0.0;
		double WetDayCloudCover = 0.0;
		double DryDayCloudCover = 0.0;
		/** The Weibull scale of the daily mean wind speed. */
		double WindScaleMetersPerSecond = 0.0;
	};

	/**
	 * A seeded weather generator conditioned on a place's climate normals, so the weather stays plausible and
	 * deterministic when the live feed cannot be reached (Decision D-014). Each day is drawn by the classical daily
	 * scheme (Richardson 1981; Wilks and Wilby 1999): a first-order Markov chain for wet days with transition
	 * probabilities from the monthly wet-day counts (Geng, Penning de Vries and Supit 1986), gamma amounts, daily
	 * maximum and minimum temperatures about Epstein-interpolated normals with a persistent anomaly, cloud from the
	 * monthly sunshine, and wind from the monthly means. Within the day, temperature follows the Parton and Logan
	 * (1981) curve between the sun's rising and setting, the day's precipitation falls in one spell, and cloud and
	 * wind pass smoothly from day to day.
	 *
	 * Any day is drawn from its own draws and those of the days just before it (the chains are run over a spin-up of
	 * several of their memory times), so every instant has one answer however it is reached.
	 */
	class FWeatherGenerator
	{
	public:
		FWeatherGenerator(const FWeatherPlace& PlaceToKeep, const FWeatherGeneratorDesign& InDesign);

		/** The weather at a UTC instant. */
		FWeatherState StateAt(const FDateTime& Utc) const;

		/** One day, by the index of its local mean solar day (the Julian Day Number of that day's date). */
		FGeneratedDay Day(int64 DayIndex) const;

		/** The local mean solar day an instant falls in, and the hour within it. */
		int64 DayIndexAt(const FDateTime& Utc) const;
		double SolarHourAt(const FDateTime& Utc) const;

		const FWeatherMonthStatistics& MonthStatistics(int32 MonthIndex) const { return Statistics[MonthIndex]; }
		const FWeatherPlace& Place() const { return WeatherPlace; }

	private:
		struct FChains
		{
			bool bWet = false;
			double TemperatureAnomaly = 0.0;
			double WindAnomaly = 0.0;
			double DirectionAnomaly = 0.0;
		};

		void DeriveStatistics();
		static int32 MonthIndexOf(int64 DayIndex);
		FChains RunChains(int64 DayIndex) const;
		void StepChains(FChains& Chains, int64 DayIndex) const;
		FGeneratedDay DrawDay(int64 DayIndex) const;
		double DrawCloud(double MeanCloudCover, int64 DayIndex) const;
		void PlaceSun(FGeneratedDay& Result) const;

		double TemperatureAt(int64 DayIndex, double Hour) const;
		double PrecipitationAt(int64 DayIndex, double Hour) const;
		double CloudAt(int64 DayIndex, double Hour) const;
		void WindAt(int64 DayIndex, double Hour, double& OutSpeed, double& OutFromDegrees) const;

		FWeatherPlace WeatherPlace;
		FWeatherGeneratorDesign Design;
		TStaticArray<FWeatherMonthStatistics, NormalsMonthCount> Statistics;
		TUniquePtr<FMonthlyCurve> MaximumCurve;
		TUniquePtr<FMonthlyCurve> MinimumCurve;
		/** Lag-one correlation of the anomalies, and the days the chains run before the day drawn. */
		double Persistence = 0.0;
		int32 SpinUpDays = 0;
		/** Days already drawn. The generator is used from one thread (the game thread, or a check). */
		mutable TMap<int64, FGeneratedDay> Days;
	};
}
