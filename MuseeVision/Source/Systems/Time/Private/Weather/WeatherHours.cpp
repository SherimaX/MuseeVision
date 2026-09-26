#include "Weather/WeatherGenerator.h"

#include "Almanac/CelestialAngles.h"

// The hours within a generated day: how its temperature, precipitation, cloud and wind unfold between the day's
// drawn values.
namespace Musee::Time::WeatherHours
{
	/** The Parton and Logan (1981) curve for air at 150 cm: the maximum lags solar noon by a = 1.86 h, the minimum
	 *  comes c = -0.17 h after sunrise, and the night cools with b = 2.20 ("A model for diurnal variation in soil
	 *  and air temperature", Agricultural Meteorology 23, 205-216, table 1). */
	constexpr double MaximumLagHours = 1.86;
	constexpr double NightCooling = 2.20;
	constexpr double MinimumLagHours = -0.17;
	/** Precipitation is equally likely to fall as rain or snow at an air temperature of 1.0 C, the mean threshold over
	 *  Northern Hemisphere land (Jennings, Winchell, Livneh and Molotch, Nature Communications 9, 1148, 2018). */
	constexpr double EvenRainSnowCelsius = 1.0;
	/** A day's values are its values at local noon; between noons they pass from one day to the next. */
	constexpr double NoonHour = 12.0;

	double DaytimeTemperature(const FGeneratedDay& Day, double Hour)
	{
		const double SinceMinimum = Hour - (Day.SunriseHour + MinimumLagHours);
		const double DayLength = Day.SunsetHour - Day.SunriseHour;
		return Day.MinimumCelsius + (Day.MaximumCelsius - Day.MinimumCelsius)
			* FMath::Sin(UE_DOUBLE_PI * SinceMinimum / (DayLength + 2.0 * MaximumLagHours));
	}

	/** Parton and Logan's exponential night, scaled so that it meets the next minimum exactly at the night's end. */
	double NightTemperature(double SunsetCelsius, double NextMinimumCelsius, double HoursSinceSunset, double NightHours)
	{
		const double End = FMath::Exp(-NightCooling);
		const double Fraction = (FMath::Exp(-NightCooling * HoursSinceSunset / NightHours) - End) / (1.0 - End);
		return NextMinimumCelsius + (SunsetCelsius - NextMinimumCelsius) * Fraction;
	}

	/** A day's spell of precipitation: a sin^2 swell holding the day's amount. */
	double SpellRate(const FGeneratedDay& Day, double HoursFromDayStart)
	{
		const double Into = HoursFromDayStart - Day.RainStartHour;
		if (!Day.bWet || Day.RainHours <= 0.0 || Into < 0.0 || Into > Day.RainHours) { return 0.0; }
		const double Peak = 2.0 * Day.PrecipitationMillimeters / Day.RainHours;
		return Peak * FMath::Square(FMath::Sin(UE_DOUBLE_PI * Into / Day.RainHours));
	}

	/** 1 while a day's spell falls, easing from 0 over the lead hours before and after it. */
	double SpellCloud(const FGeneratedDay& Day, double HoursFromDayStart, double LeadHours)
	{
		if (!Day.bWet || Day.RainHours <= 0.0) { return 0.0; }
		const double Start = Day.RainStartHour;
		const double End = Day.RainStartHour + Day.RainHours;
		if (HoursFromDayStart < Start) { return FMath::SmoothStep(Start - LeadHours, Start, HoursFromDayStart); }
		if (HoursFromDayStart > End) { return 1.0 - FMath::SmoothStep(End, End + LeadHours, HoursFromDayStart); }
		return 1.0;
	}

	/** Where an hour falls between two days' noons: the earlier day's offset from today (-1 or 0) and the blend. */
	void NoonBlend(double Hour, int64& OutEarlierOffset, double& OutBlend)
	{
		OutEarlierOffset = Hour < NoonHour ? -1 : 0;
		const double SinceEarlierNoon = Hour < NoonHour ? Hour + NoonHour : Hour - NoonHour;
		OutBlend = FMath::SmoothStep(0.0, HoursPerDay, SinceEarlierNoon);
	}
}

namespace Musee::Time
{
	FWeatherState FWeatherGenerator::StateAt(const FDateTime& Utc) const
	{
		using namespace WeatherHours;
		const int64 DayIndex = DayIndexAt(Utc);
		const double Hour = SolarHourAt(Utc);
		FWeatherState State;
		State.AirTemperatureCelsius = TemperatureAt(DayIndex, Hour);
		const double Precipitation = PrecipitationAt(DayIndex, Hour);
		const double SnowShare = FMath::Clamp(
			(EvenRainSnowCelsius - State.AirTemperatureCelsius) / Design.SnowToRainBandCelsius + 0.5, 0.0, 1.0);
		State.SnowfallMillimetersPerHour = Precipitation * SnowShare;
		State.RainMillimetersPerHour = Precipitation * (1.0 - SnowShare);
		State.CloudCover = CloudAt(DayIndex, Hour);
		WindAt(DayIndex, Hour, State.WindSpeedMetersPerSecond, State.WindFromDegrees);
		return State;
	}

	double FWeatherGenerator::TemperatureAt(int64 DayIndex, double Hour) const
	{
		using namespace WeatherHours;
		const FGeneratedDay Today = Day(DayIndex);
		const double MinimumHour = Today.SunriseHour + MinimumLagHours;
		if (Hour < MinimumHour)
		{
			const FGeneratedDay Yesterday = Day(DayIndex - 1);
			const double NightHours = HoursPerDay - Yesterday.SunsetHour + MinimumHour;
			return NightTemperature(DaytimeTemperature(Yesterday, Yesterday.SunsetHour), Today.MinimumCelsius,
				Hour + HoursPerDay - Yesterday.SunsetHour, NightHours);
		}
		if (Hour <= Today.SunsetHour)
		{
			return DaytimeTemperature(Today, Hour);
		}
		const FGeneratedDay Tomorrow = Day(DayIndex + 1);
		const double NightHours = HoursPerDay - Today.SunsetHour + Tomorrow.SunriseHour + MinimumLagHours;
		return NightTemperature(DaytimeTemperature(Today, Today.SunsetHour), Tomorrow.MinimumCelsius,
			Hour - Today.SunsetHour, NightHours);
	}

	double FWeatherGenerator::PrecipitationAt(int64 DayIndex, double Hour) const
	{
		using namespace WeatherHours;
		// A spell may run on past midnight into the next day.
		return SpellRate(Day(DayIndex), Hour) + SpellRate(Day(DayIndex - 1), Hour + HoursPerDay);
	}

	double FWeatherGenerator::CloudAt(int64 DayIndex, double Hour) const
	{
		using namespace WeatherHours;
		int64 EarlierOffset = 0;
		double Blend = 0.0;
		NoonBlend(Hour, EarlierOffset, Blend);
		const double Earlier = Day(DayIndex + EarlierOffset).CloudCover;
		const double Later = Day(DayIndex + EarlierOffset + 1).CloudCover;
		const double DayCloud = FMath::Lerp(Earlier, Later, Blend);

		// No precipitation falls from a clear sky: the cloud builds before each spell and clears after it.
		double Spell = 0.0;
		for (int64 Offset = -1; Offset <= 1; ++Offset)
		{
			Spell = FMath::Max(Spell, SpellCloud(Day(DayIndex + Offset), Hour - HoursPerDay * Offset, Design.RainCloudLeadHours));
		}
		return FMath::Max(DayCloud, Design.RainingCloudCover * Spell);
	}

	void FWeatherGenerator::WindAt(int64 DayIndex, double Hour, double& OutSpeed, double& OutFromDegrees) const
	{
		using namespace WeatherHours;
		int64 EarlierOffset = 0;
		double Blend = 0.0;
		NoonBlend(Hour, EarlierOffset, Blend);
		const FGeneratedDay Earlier = Day(DayIndex + EarlierOffset);
		const FGeneratedDay Later = Day(DayIndex + EarlierOffset + 1);
		OutSpeed = FMath::Lerp(Earlier.WindSpeedMetersPerSecond, Later.WindSpeedMetersPerSecond, Blend);
		// The wind veers or backs the shorter way round.
		OutFromDegrees = ReduceDegrees(Earlier.WindFromDegrees
			+ SignedDegrees(Later.WindFromDegrees - Earlier.WindFromDegrees) * Blend);
	}
}
