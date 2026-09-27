#include "Weather/WeatherSmoother.h"

#include "Almanac/CelestialAngles.h"

namespace Musee::Time::WeatherEasing
{
	/** How much of the gap to a held target remains after a time: exp(-t / tau). A zero time constant follows at once. */
	double Remaining(double Seconds, double TimeConstantSeconds)
	{
		return TimeConstantSeconds > 0.0 ? FMath::Exp(-Seconds / TimeConstantSeconds) : 0.0;
	}

	double Ease(double Value, double Target, double Remainder)
	{
		return Target + (Value - Target) * Remainder;
	}

	/** The wind as the vector of the direction it blows from (x towards east, y towards north), scaled by its speed. */
	FVector2D WindVector(double SpeedMetersPerSecond, double FromDegrees)
	{
		return FVector2D(SinDegrees(FromDegrees), CosDegrees(FromDegrees)) * SpeedMetersPerSecond;
	}
}

namespace Musee::Time
{
	void FWeatherSmoother::Reset(const FWeatherState& State)
	{
		Current = State;
		WindFrom = WeatherEasing::WindVector(State.WindSpeedMetersPerSecond, State.WindFromDegrees);
		bStarted = true;
	}

	const FWeatherState& FWeatherSmoother::Advance(const FWeatherState& Target, double Seconds, const FWeatherSmoothing& Smoothing)
	{
		using namespace WeatherEasing;
		if (!bStarted)
		{
			Reset(Target);
			return Current;
		}
		const double Temperature = Remaining(Seconds, Smoothing.TemperatureSeconds);
		const double Cloud = Remaining(Seconds, Smoothing.CloudCoverSeconds);
		const double Precipitation = Remaining(Seconds, Smoothing.PrecipitationSeconds);
		const double Wind = Remaining(Seconds, Smoothing.WindSeconds);

		Current.AirTemperatureCelsius = Ease(Current.AirTemperatureCelsius, Target.AirTemperatureCelsius, Temperature);
		Current.CloudCover = Ease(Current.CloudCover, Target.CloudCover, Cloud);
		Current.RainMillimetersPerHour = Ease(Current.RainMillimetersPerHour, Target.RainMillimetersPerHour, Precipitation);
		Current.SnowfallMillimetersPerHour = Ease(Current.SnowfallMillimetersPerHour, Target.SnowfallMillimetersPerHour, Precipitation);

		const FVector2D TargetWind = WindVector(Target.WindSpeedMetersPerSecond, Target.WindFromDegrees);
		WindFrom = TargetWind + (WindFrom - TargetWind) * Wind;
		Current.WindSpeedMetersPerSecond = WindFrom.Size();
		// In a calm the direction is kept from the target, so a reading of it is never undefined.
		Current.WindFromDegrees = WindFrom.IsNearlyZero()
			? Target.WindFromDegrees : ReduceDegrees(Atan2Degrees(WindFrom.X, WindFrom.Y));
		return Current;
	}
}
