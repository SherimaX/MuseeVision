#include "Weather/WeatherGenerator.h"

#include "Almanac/CelestialAngles.h"
#include "Almanac/SolarCoordinates.h"
#include "Almanac/SolarDay.h"
#include "Weather/WeatherDraws.h"

#include <cmath>

namespace Musee::Time::WeatherModel
{
	/** Wet-day transitions from a month's wet fraction f: P(wet | dry) = 0.75 f and P(wet | wet) = 0.25 + 0.75 f
	 *  (Geng, Penning de Vries and Supit, "A simple method for generating daily rainfall data", Agricultural and
	 *  Forest Meteorology 36, 1986, 363-376). The chain's long-run wet fraction is f. */
	constexpr double WetAfterDryPerWetFraction = 0.75;
	constexpr double WetPersistence = 0.25;
	/** A wet day's amount above the source's wet-day threshold is gamma with scale -2.16 + 1.83 mu millimetres and
	 *  shape mu / scale, mu the month's mean amount above the threshold (Geng et al. 1986, applied above the threshold
	 *  so that every wet day drawn is wet by the source's own count and the month's total is kept); where the scale is
	 *  not positive every wet day takes the mean. */
	constexpr double AmountScaleInterceptMillimeters = -2.16;
	constexpr double AmountScaleSlope = 1.83;
	/** A month's mean daily wind is Weibull with shape 2 (Rayleigh), the value typical of surface winds (Justus,
	 *  Hargraves, Mikhail and Graber, Journal of Applied Meteorology 17, 1978, 350-353). */
	constexpr double WindWeibullShape = 2.0;
	/** Mean month lengths over a normals period, February with its leap days. */
	constexpr double MeanMonthDays[NormalsMonthCount] = {31, 28.25, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	/** A common (non-leap) year over which a month's possible sunshine is summed. */
	constexpr int32 ReferenceYear = 2001;
	/** The chains run until their starting state keeps less than a millionth of its weight. */
	constexpr double ChainForgottenFraction = 1e-6;
	constexpr double NoonHour = 12.0;

	/** The astronomical day length at a latitude for a solar declination: the Sun's upper limb above the horizon. */
	double DayLengthHours(double LatitudeDegrees, double DeclinationDegrees)
	{
		return 2.0 * CrossingHourAngleDegrees(LatitudeDegrees, DeclinationDegrees, SolarDayAltitudes::SunriseDegrees)
			/ DegreesPerHour;
	}

	/** The hours the Sun is up in each month at a latitude: the possible sunshine the measured sunshine is a share of. */
	TStaticArray<double, NormalsMonthCount> PossibleSunshineHours(double LatitudeDegrees)
	{
		TStaticArray<double, NormalsMonthCount> Hours{InPlace, 0.0};
		TStaticArray<int32, NormalsMonthCount> Days{InPlace, 0};
		for (FDateTime Noon(ReferenceYear, 1, 1, 12); Noon.GetYear() == ReferenceYear; Noon += FTimespan::FromDays(1))
		{
			const double Declination = SolarCoordinates(FInstant::FromUtc(Noon).JulianEphemerisDay()).DeclinationDegrees;
			Hours[Noon.GetMonth() - 1] += DayLengthHours(LatitudeDegrees, Declination);
			++Days[Noon.GetMonth() - 1];
		}
		for (int32 Month = 0; Month < NormalsMonthCount; ++Month) { Hours[Month] *= MeanMonthDays[Month] / Days[Month]; }
		return Hours;
	}
}

namespace Musee::Time
{
	FWeatherGenerator::FWeatherGenerator(const FWeatherPlace& PlaceToKeep, const FWeatherGeneratorDesign& InDesign)
		: WeatherPlace(PlaceToKeep)
		, Design(InDesign)
	{
		using namespace WeatherModel;
		TStaticArray<double, NormalsMonthCount> Maxima{InPlace, 0.0};
		TStaticArray<double, NormalsMonthCount> Minima{InPlace, 0.0};
		for (int32 Month = 0; Month < NormalsMonthCount; ++Month)
		{
			Maxima[Month] = WeatherPlace.Normals.Months[Month].MeanMaximumCelsius;
			Minima[Month] = WeatherPlace.Normals.Months[Month].MeanMinimumCelsius;
		}
		MaximumCurve = MakeUnique<FMonthlyCurve>(Maxima);
		MinimumCurve = MakeUnique<FMonthlyCurve>(Minima);

		Persistence = Design.AnomalyMemoryDays > 0.0 ? FMath::Exp(-1.0 / Design.AnomalyMemoryDays) : 0.0;
		const double SlowestForgetting = FMath::Max(Persistence, WetPersistence);
		SpinUpDays = FMath::CeilToInt32(FMath::Loge(ChainForgottenFraction) / FMath::Loge(SlowestForgetting));
		DeriveStatistics();
	}

	void FWeatherGenerator::DeriveStatistics()
	{
		using namespace WeatherModel;
		const TStaticArray<double, NormalsMonthCount> Possible = PossibleSunshineHours(WeatherPlace.Observer.LatitudeDegrees);
		const double WeibullMeanFactor = std::tgamma(1.0 + 1.0 / WindWeibullShape);
		for (int32 Month = 0; Month < NormalsMonthCount; ++Month)
		{
			const FClimateMonth& Normal = WeatherPlace.Normals.Months[Month];
			FWeatherMonthStatistics& Stats = Statistics[Month];
			const double WetFraction = FMath::Clamp(Normal.WetDays / MeanMonthDays[Month], 0.0, 1.0);
			Stats.WetAfterDry = WetAfterDryPerWetFraction * WetFraction;
			Stats.WetAfterWet = WetPersistence + WetAfterDryPerWetFraction * WetFraction;

			Stats.MeanWetDayMillimeters = Normal.WetDays > 0.0 ? Normal.PrecipitationMillimeters / Normal.WetDays : 0.0;
			const double MeanExcess = FMath::Max(Stats.MeanWetDayMillimeters - WeatherPlace.Normals.WetDayThresholdMillimeters, 0.0);
			const double Scale = AmountScaleInterceptMillimeters + AmountScaleSlope * MeanExcess;
			Stats.AmountScaleMillimeters = FMath::Max(Scale, 0.0);
			Stats.AmountShape = Scale > 0.0 ? MeanExcess / Scale : 0.0;

			// Bright sunshine is recorded while the Sun's disc is clear of cloud, so the share of possible sunshine
			// missed stands for the mean cloud cover (the chance of a clear line of sight through a random cloud layer).
			Stats.MeanCloudCover = FMath::Clamp(1.0 - Normal.SunshineHours / Possible[Month], 0.0, 1.0);
			Stats.WetDayCloudCover = WetFraction > 0.0
				? FMath::Min(Design.WetDayCloudCover, Stats.MeanCloudCover / WetFraction) : Design.WetDayCloudCover;
			Stats.DryDayCloudCover = WetFraction < 1.0
				? FMath::Clamp((Stats.MeanCloudCover - WetFraction * Stats.WetDayCloudCover) / (1.0 - WetFraction), 0.0, 1.0)
				: Stats.WetDayCloudCover;
			Stats.WindScaleMetersPerSecond = Normal.WindSpeedMetersPerSecond / WeibullMeanFactor;
		}
	}

	int32 FWeatherGenerator::MonthIndexOf(int64 DayIndex)
	{
		return UtcFromJulianDay(static_cast<double>(DayIndex)).GetMonth() - 1;
	}

	int64 FWeatherGenerator::DayIndexAt(const FDateTime& Utc) const
	{
		constexpr double HalfDay = 0.5;
		return FMath::FloorToInt64(JulianDayFromUtc(Utc) + HalfDay + WeatherPlace.Observer.LongitudeDegrees / DegreesPerTurn);
	}

	double FWeatherGenerator::SolarHourAt(const FDateTime& Utc) const
	{
		constexpr double HalfDay = 0.5;
		const double LocalDays = JulianDayFromUtc(Utc) + HalfDay + WeatherPlace.Observer.LongitudeDegrees / DegreesPerTurn;
		return (LocalDays - FMath::FloorToDouble(LocalDays)) * HoursPerDay;
	}

	FGeneratedDay FWeatherGenerator::Day(int64 DayIndex) const
	{
		if (const FGeneratedDay* Found = Days.Find(DayIndex)) { return *Found; }
		const FGeneratedDay Drawn = DrawDay(DayIndex);
		Days.Add(DayIndex, Drawn);
		return Drawn;
	}

	FWeatherGenerator::FChains FWeatherGenerator::RunChains(int64 DayIndex) const
	{
		const int64 FirstDay = DayIndex - SpinUpDays;
		const FWeatherMonthStatistics& First = Statistics[MonthIndexOf(FirstDay)];
		const double StartWetFraction = First.WetAfterDry / (1.0 - First.WetAfterWet + First.WetAfterDry);
		FChains Chains;
		Chains.bWet = FWeatherDraws(WeatherPlace.Seed, FirstDay, EWeatherStream::Occurrence).Uniform() < StartWetFraction;
		for (int64 Step = FirstDay + 1; Step <= DayIndex; ++Step) { StepChains(Chains, Step); }
		return Chains;
	}

	void FWeatherGenerator::StepChains(FChains& Chains, int64 DayIndex) const
	{
		const FWeatherMonthStatistics& Stats = Statistics[MonthIndexOf(DayIndex)];
		const uint64 Seed = WeatherPlace.Seed;
		const double Innovation = FMath::Sqrt(1.0 - Persistence * Persistence);
		const double WetChance = Chains.bWet ? Stats.WetAfterWet : Stats.WetAfterDry;
		Chains.bWet = FWeatherDraws(Seed, DayIndex, EWeatherStream::Occurrence).Uniform() < WetChance;
		Chains.TemperatureAnomaly = Persistence * Chains.TemperatureAnomaly
			+ Innovation * FWeatherDraws(Seed, DayIndex, EWeatherStream::Temperature).Normal();
		Chains.WindAnomaly = Persistence * Chains.WindAnomaly
			+ Innovation * FWeatherDraws(Seed, DayIndex, EWeatherStream::WindSpeed).Normal();
		Chains.DirectionAnomaly = Persistence * Chains.DirectionAnomaly
			+ Innovation * FWeatherDraws(Seed, DayIndex, EWeatherStream::WindDirection).Normal();
	}

	double FWeatherGenerator::DrawCloud(double MeanCloudCover, int64 DayIndex) const
	{
		if (MeanCloudCover <= 0.0 || MeanCloudCover >= 1.0) { return FMath::Clamp(MeanCloudCover, 0.0, 1.0); }
		FWeatherDraws Draws(WeatherPlace.Seed, DayIndex, EWeatherStream::Cloud);
		return Draws.Beta(MeanCloudCover * Design.CloudCoverPrecision, (1.0 - MeanCloudCover) * Design.CloudCoverPrecision);
	}

	FGeneratedDay FWeatherGenerator::DrawDay(int64 DayIndex) const
	{
		using namespace WeatherModel;
		const FChains Chains = RunChains(DayIndex);
		const int32 Month = MonthIndexOf(DayIndex);
		const FWeatherMonthStatistics& Stats = Statistics[Month];
		const double PointOfYear = YearMonths(UtcFromJulianDay(static_cast<double>(DayIndex)));

		FGeneratedDay Result;
		Result.DayIndex = DayIndex;
		Result.bWet = Chains.bWet;
		Result.MaximumCelsius = MaximumCurve->At(PointOfYear) + Design.MaximumSpreadCelsius[Month] * Chains.TemperatureAnomaly;
		Result.MinimumCelsius = FMath::Min(
			MinimumCurve->At(PointOfYear) + Design.MinimumSpreadCelsius[Month] * Chains.TemperatureAnomaly, Result.MaximumCelsius);
		if (Result.bWet)
		{
			FWeatherDraws Amount(WeatherPlace.Seed, DayIndex, EWeatherStream::Amount);
			const double Threshold = WeatherPlace.Normals.WetDayThresholdMillimeters;
			Result.PrecipitationMillimeters = Stats.AmountShape > 0.0
				? Threshold + Stats.AmountScaleMillimeters * Amount.Gamma(Stats.AmountShape)
				: FMath::Max(Stats.MeanWetDayMillimeters, Threshold);
			FWeatherDraws Timing(WeatherPlace.Seed, DayIndex, EWeatherStream::RainTiming);
			Result.RainHours = FMath::Clamp(Design.RainMeanHours * Timing.Exponential(), Design.RainShortestHours, Design.RainLongestHours);
			Result.RainStartHour = HoursPerDay * Timing.Uniform();
		}
		Result.CloudCover = DrawCloud(Result.bWet ? Stats.WetDayCloudCover : Stats.DryDayCloudCover, DayIndex);

		// The wind's anomaly is a standard normal; through its cumulative probability it picks the Weibull quantile.
		const double Probability = 0.5 * std::erfc(-Chains.WindAnomaly / UE_DOUBLE_SQRT_2);
		Result.WindSpeedMetersPerSecond = Stats.WindScaleMetersPerSecond
			* FMath::Pow(-FMath::Loge(1.0 - Probability), 1.0 / WindWeibullShape);
		Result.WindFromDegrees = ReduceDegrees(WeatherPlace.Normals.Months[Month].WindFromDegrees
			+ Design.WindDirectionSpreadDegrees * Chains.DirectionAnomaly);
		PlaceSun(Result);
		return Result;
	}

	void FWeatherGenerator::PlaceSun(FGeneratedDay& Result) const
	{
		using namespace WeatherModel;
		// The day's index is the Julian Day Number of its date, whose integer value falls at 12h UT; local mean noon
		// comes the east longitude's worth of hours earlier.
		const double LocalNoon = static_cast<double>(Result.DayIndex) - WeatherPlace.Observer.LongitudeDegrees / DegreesPerTurn;
		const FSolarCoordinates Solar = SolarCoordinates(FInstant::FromUtc(UtcFromJulianDay(LocalNoon)).JulianEphemerisDay());
		const double HalfDayLength = DayLengthHours(WeatherPlace.Observer.LatitudeDegrees, Solar.DeclinationDegrees) / 2.0;
		const double ApparentNoon = NoonHour - Solar.EquationOfTimeMinutes / MinutesPerHour;
		Result.SunriseHour = ApparentNoon - HalfDayLength;
		Result.SunsetHour = ApparentNoon + HalfDayLength;
	}
}
