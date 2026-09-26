#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"

/**
 * A place's climate normals: the 30-year monthly means its meteorological service publishes. The records, each value
 * with its source, are Data/Time/ClimateNormals/<place>.json. The weather generator draws plausible weather from
 * them when the live feed cannot be reached (Decision D-014), so the museum's weather never falls back to a default.
 */
namespace Musee::Time
{
	inline constexpr int32 NormalsMonthCount = 12;

	/** One calendar month of normals. */
	struct FClimateMonth
	{
		double MeanMaximumCelsius = 0.0;
		double MeanMinimumCelsius = 0.0;
		/** The month's mean total precipitation, as liquid water. */
		double PrecipitationMillimeters = 0.0;
		/** Mean days with at least the source's wet-day threshold of precipitation. */
		double WetDays = 0.0;
		/** Mean hours of bright sunshine (WMO: direct irradiance above 120 W m^-2). */
		double SunshineHours = 0.0;
		/** Mean wind speed at 10 m, and the prevailing direction it blows from. */
		double WindSpeedMetersPerSecond = 0.0;
		double WindFromDegrees = 0.0;
		/** Mean days with snowfall, and with air frost (a minimum below 0 C), where the source publishes them. They
		 *  check the generator's weather; they never drive it. */
		TOptional<double> SnowDays;
		TOptional<double> FrostDays;
	};

	struct FClimateNormals
	{
		TStaticArray<FClimateMonth, NormalsMonthCount> Months;
		/** The least precipitation that makes a day wet in the source's counts (0.1 mm for the CMA, 0.25 mm for NOAA,
		 *  1 mm for the Met Office and Meteo-France). */
		double WetDayThresholdMillimeters = 0.0;
	};

	/**
	 * A smooth yearly curve through twelve monthly means that keeps every month's mean: the means' discrete Fourier
	 * harmonics, each divided by the damping that averaging over a month gives it (Epstein, "On obtaining daily
	 * climatological values from monthly means", Journal of Climate 4, 1991, 365-368).
	 */
	class FMonthlyCurve
	{
	public:
		explicit FMonthlyCurve(const TStaticArray<double, NormalsMonthCount>& MonthlyMeans);

		/** The value at a point of the year in months: 0 at the start of January, 12 at the end of December. */
		double At(double YearMonths) const;

	private:
		static constexpr int32 HarmonicCount = NormalsMonthCount / 2;
		double Mean = 0.0;
		double Cosine[HarmonicCount] = {};
		double Sine[HarmonicCount] = {};
	};

	/** The point of the year, in months, of a date and time: January 1 at 00:00 is 0, each month one unit long. */
	double YearMonths(const FDateTime& Date);
}
