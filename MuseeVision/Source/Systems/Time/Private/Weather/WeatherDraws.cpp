#include "Weather/WeatherDraws.h"

namespace Musee::Time::SplitMix
{
	/** SplitMix64's increment (the golden ratio in 64 bits) and finalising multipliers (Steele, Lea and Flood 2014). */
	constexpr uint64 GoldenGamma = 0x9E3779B97F4A7C15ull;
	constexpr uint64 MixA = 0xBF58476D1CE4E5B9ull;
	constexpr uint64 MixB = 0x94D049BB133111EBull;
	/** A double has 53 bits of mantissa: the top 53 bits of a mix give a uniform on a grid of 2^-53. */
	constexpr int32 MantissaBits = 53;
	constexpr int32 DiscardedBits = 64 - MantissaBits;

	uint64 Mix(uint64 Value)
	{
		Value += GoldenGamma;
		Value = (Value ^ (Value >> 30)) * MixA;
		Value = (Value ^ (Value >> 27)) * MixB;
		return Value ^ (Value >> 31);
	}
}

namespace Musee::Time
{
	FWeatherDraws::FWeatherDraws(uint64 Seed, int64 DayIndex, EWeatherStream Stream)
	{
		using namespace SplitMix;
		Key = Mix(Mix(Seed) ^ static_cast<uint64>(DayIndex)) ^ Mix(static_cast<uint64>(Stream) + GoldenGamma);
	}

	double FWeatherDraws::Uniform()
	{
		using namespace SplitMix;
		const uint64 Bits = Mix(Key + GoldenGamma * ++Counter) >> DiscardedBits;
		// Centred on its grid cell, so the value is never 0 or 1.
		return (static_cast<double>(Bits) + 0.5) / static_cast<double>(1ull << MantissaBits);
	}

	double FWeatherDraws::Normal()
	{
		const double Radius = FMath::Sqrt(-2.0 * FMath::Loge(Uniform()));
		return Radius * FMath::Cos(2.0 * UE_DOUBLE_PI * Uniform());
	}

	double FWeatherDraws::Exponential()
	{
		return -FMath::Loge(Uniform());
	}

	double FWeatherDraws::Gamma(double Shape)
	{
		if (Shape < 1.0)
		{
			return Gamma(Shape + 1.0) * FMath::Pow(Uniform(), 1.0 / Shape);
		}
		const double D = Shape - 1.0 / 3.0;
		const double C = 1.0 / FMath::Sqrt(9.0 * D);
		for (;;)
		{
			const double X = Normal();
			const double V = FMath::Cube(1.0 + C * X);
			if (V <= 0.0) { continue; }
			const double U = Uniform();
			if (FMath::Loge(U) < 0.5 * X * X + D - D * V + D * FMath::Loge(V)) { return D * V; }
		}
	}

	double FWeatherDraws::Beta(double A, double B)
	{
		const double X = Gamma(A);
		const double Y = Gamma(B);
		const double Sum = X + Y;
		// Both draws can underflow to zero only for tiny shapes; the distribution's mean stands in for that case.
		return Sum > 0.0 ? X / Sum : A / (A + B);
	}
}
