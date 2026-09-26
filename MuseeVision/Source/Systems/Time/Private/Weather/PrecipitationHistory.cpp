#include "Weather/PrecipitationHistory.h"

#include "Almanac/AstronomicalTime.h"

namespace Musee::Time::GroundWater
{
	/** Water reaches the ground measurably at 0.1 mm in an hour: the smallest amount a rain gauge reports; less is a
	 *  trace (WMO Guide to Instruments and Methods of Observation, WMO-No. 8, ch. 6). */
	constexpr double MeasurableMillimetersPerHour = 0.1;
	/** Fresh snow's density (kg m^-3) against air temperature (C): 67.92 + 51.25 exp(T / 2.59) (Hedstrom and Pomeroy,
	 *  Hydrological Processes 12, 1998, 1611-1625). */
	constexpr double FreshSnowBaseDensity = 67.92;
	constexpr double FreshSnowWarmDensity = 51.25;
	constexpr double FreshSnowTemperatureScaleCelsius = 2.59;
	/** Lying snow settles towards 300 kg m^-3 with an e-folding time of 100 hours (Verseghy, CLASS, 1991). */
	constexpr double SettledSnowDensity = 300.0;
	constexpr double SettlingHours = 100.0;
	/** Snow water below a thousandth of a millimetre is gone. */
	constexpr double VanishingSnowWaterMillimeters = 1e-3;

	double FreshSnowDensity(double AirTemperatureCelsius)
	{
		return FreshSnowBaseDensity + FreshSnowWarmDensity * FMath::Exp(AirTemperatureCelsius / FreshSnowTemperatureScaleCelsius);
	}
}

namespace Musee::Time
{
	FPrecipitationHistory::FPrecipitationHistory(const FPrecipitationHistoryDesign& InDesign)
		: Design(InDesign)
	{
		Reset();
	}

	void FPrecipitationHistory::Reset()
	{
		RainRecords.Init(0.0, Design.RainMemoryHours.Num());
		SecondsSinceWet.Reset();
		SnowWater = 0.0;
		SnowDepth = 0.0;
	}

	double FPrecipitationHistory::SnowDensityKilogramsPerCubicMeter() const
	{
		return SnowDepth > 0.0 ? SnowWater / SnowDepth : 0.0;
	}

	TOptional<double> FPrecipitationHistory::HoursSinceWet() const
	{
		return SecondsSinceWet.IsSet() ? TOptional<double>(SecondsSinceWet.GetValue() / SecondsPerHour) : TOptional<double>();
	}

	void FPrecipitationHistory::ObserveSnowDepth(double DepthMeters)
	{
		using namespace GroundWater;
		// A report gives the depth but not the snow's age: an unmodelled cover is taken as settled snow.
		const double Density = SnowDepth > 0.0 ? SnowDensityKilogramsPerCubicMeter() : SettledSnowDensity;
		SnowDepth = FMath::Max(DepthMeters, 0.0);
		SnowWater = SnowDepth * Density;
	}

	void FPrecipitationHistory::AdvanceSnow(const FWeatherState& Weather, double Seconds, double& OutMeltMillimetersPerHour)
	{
		using namespace GroundWater;
		const double Hours = Seconds / SecondsPerHour;
		OutMeltMillimetersPerHour = 0.0;
		if (SnowWater > 0.0 && SnowDepth > 0.0)
		{
			// Settling, then melting at the density reached. One millimetre of water is one kilogram per square metre.
			const double Density = SnowDensityKilogramsPerCubicMeter();
			const double Settled = Density < SettledSnowDensity
				? SettledSnowDensity + (Density - SettledSnowDensity) * FMath::Exp(-Hours / SettlingHours) : Density;
			const double Melt = Weather.AirTemperatureCelsius > 0.0
				? FMath::Min(SnowWater, Design.SnowMeltMillimetersPerDayCelsius * Weather.AirTemperatureCelsius * Hours / HoursPerDay)
				: 0.0;
			SnowWater -= Melt;
			SnowDepth = SnowWater / Settled;
			OutMeltMillimetersPerHour = Hours > 0.0 ? Melt / Hours : 0.0;
			if (SnowWater < VanishingSnowWaterMillimeters)
			{
				SnowWater = 0.0;
				SnowDepth = 0.0;
			}
		}
		const double Fresh = Weather.SnowfallMillimetersPerHour * Hours;
		if (Fresh > 0.0)
		{
			SnowWater += Fresh;
			SnowDepth += Fresh / FreshSnowDensity(Weather.AirTemperatureCelsius);
		}
	}

	void FPrecipitationHistory::Advance(const FWeatherState& Weather, double Seconds)
	{
		using namespace GroundWater;
		double MeltMillimetersPerHour = 0.0;
		AdvanceSnow(Weather, Seconds, MeltMillimetersPerHour);
		const double LiquidMillimetersPerHour = Weather.RainMillimetersPerHour + MeltMillimetersPerHour;

		for (int32 Record = 0; Record < RainRecords.Num(); ++Record)
		{
			const double MemoryHours = Design.RainMemoryHours[Record];
			const double Fading = FMath::Exp(-Seconds / (MemoryHours * SecondsPerHour));
			RainRecords[Record] = RainRecords[Record] * Fading + LiquidMillimetersPerHour * MemoryHours * (1.0 - Fading);
		}

		if (LiquidMillimetersPerHour >= MeasurableMillimetersPerHour)
		{
			SecondsSinceWet = 0.0;
		}
		else if (SecondsSinceWet.IsSet())
		{
			SecondsSinceWet = SecondsSinceWet.GetValue() + Seconds;
		}
	}
}
