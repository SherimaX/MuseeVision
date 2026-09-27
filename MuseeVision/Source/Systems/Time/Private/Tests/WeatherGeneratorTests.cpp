#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Almanac/CelestialAngles.h"
#include "Tests/WeatherFixtures.h"
#include "Weather/ClimateNormals.h"
#include "Weather/WeatherDraws.h"
#include "Weather/WeatherGenerator.h"

// The weather generator: its curves, its draws, and whether three centuries of its weather keep the normals.
namespace Musee::Time::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeMonthlyCurveTest, "Musee.Time.MonthlyCurve", Flags)
	bool FTimeMonthlyCurveTest::RunTest(const FString& Parameters)
	{
		// Epstein's curve keeps every month's mean, even through a monsoon's spike, and closes on itself at the year's end.
		const double Series[][NormalsMonthCount] = {
			{-3, 0, 7, 15, 21, 25, 27, 26, 21, 14, 5, -1},
			{2, 6, 9, 22, 36, 72, 170, 113, 54, 29, 14, 2},
		};
		for (const auto& Values : Series)
		{
			TStaticArray<double, NormalsMonthCount> Means{InPlace, 0.0};
			for (int32 Month = 0; Month < NormalsMonthCount; ++Month) { Means[Month] = Values[Month]; }
			const FMonthlyCurve Curve(Means);
			double LargestError = 0.0;
			for (int32 Month = 0; Month < NormalsMonthCount; ++Month)
			{
				constexpr int32 Samples = 20000;   // the midpoint rule is then good to about 2e-8
				double Sum = 0.0;
				for (int32 Sample = 0; Sample < Samples; ++Sample) { Sum += Curve.At(Month + (Sample + 0.5) / Samples); }
				LargestError = FMath::Max(LargestError, FMath::Abs(Sum / Samples - Means[Month]));
			}
			Near(*this, TEXT("largest error of a month's mean"), LargestError, 0.0, 1e-6);
			Near(*this, TEXT("the curve closes at the year's end"), Curve.At(12.0) - Curve.At(0.0), 0.0, 1e-9);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeWeatherDrawsTest, "Musee.Time.WeatherDraws", Flags)
	bool FTimeWeatherDrawsTest::RunTest(const FString& Parameters)
	{
		FWeatherDraws First(42, 2461000, EWeatherStream::Amount);
		FWeatherDraws Again(42, 2461000, EWeatherStream::Amount);
		FWeatherDraws OtherStream(42, 2461000, EWeatherStream::Cloud);
		FWeatherDraws OtherDay(42, 2461001, EWeatherStream::Amount);
		bool bSame = true;
		bool bStreamsDiffer = false;
		bool bDaysDiffer = false;
		for (int32 Draw = 0; Draw < 16; ++Draw)
		{
			const double Value = First.Uniform();
			bSame &= Value == Again.Uniform();
			bStreamsDiffer |= Value != OtherStream.Uniform();
			bDaysDiffer |= Value != OtherDay.Uniform();
		}
		TestTrue(TEXT("the same place, day and stream draw the same"), bSame);
		TestTrue(TEXT("streams are independent"), bStreamsDiffer);
		TestTrue(TEXT("days are independent"), bDaysDiffer);

		// The samplers' moments over 200,000 draws (five standard errors allowed).
		constexpr int32 Count = 200000;
		FWeatherDraws Draws(7, 0, EWeatherStream::Temperature);
		double UniformSum = 0.0;
		double NormalSum = 0.0;
		double NormalSquares = 0.0;
		double SmallGamma = 0.0;
		double SmallGammaSquares = 0.0;
		double LargeGamma = 0.0;
		double BetaSum = 0.0;
		bool bInside = true;
		for (int32 Draw = 0; Draw < Count; ++Draw)
		{
			const double U = Draws.Uniform();
			bInside &= U > 0.0 && U < 1.0;
			UniformSum += U;
			const double N = Draws.Normal();
			NormalSum += N;
			NormalSquares += N * N;
			const double G = Draws.Gamma(0.7);
			SmallGamma += G;
			SmallGammaSquares += G * G;
			LargeGamma += Draws.Gamma(3.2);
			BetaSum += Draws.Beta(0.6, 1.4);
		}
		TestTrue(TEXT("uniforms lie strictly inside (0, 1)"), bInside);
		Near(*this, TEXT("uniform mean"), UniformSum / Count, 0.5, 0.0035);
		Near(*this, TEXT("normal mean"), NormalSum / Count, 0.0, 0.012);
		Near(*this, TEXT("normal variance"), NormalSquares / Count - FMath::Square(NormalSum / Count), 1.0, 0.016);
		Near(*this, TEXT("gamma(0.7) mean"), SmallGamma / Count, 0.7, 0.01);
		Near(*this, TEXT("gamma(0.7) variance"), SmallGammaSquares / Count - FMath::Square(SmallGamma / Count), 0.7, 0.03);
		Near(*this, TEXT("gamma(3.2) mean"), LargeGamma / Count, 3.2, 0.02);
		Near(*this, TEXT("beta(0.6, 1.4) mean"), BetaSum / Count, 0.3, 0.003);
		return true;
	}

	/** Three centuries of generated days, tallied by calendar month. */
	struct FMonthTally
	{
		int32 Days = 0;
		int32 WetDays = 0;
		double MaximumSum = 0.0;
		double MinimumSum = 0.0;
		double PrecipitationSum = 0.0;
		double CloudSum = 0.0;
		double WindSum = 0.0;
		double SmallestWetAmount = TNumericLimits<double>::Max();
	};

	void CheckClimate(FAutomationTestBase& Test, const FString& Name, const FWeatherPlace& Place, const FWeatherGeneratorDesign& Design)
	{
		const FWeatherGenerator Generator(Place, Design);
		TStaticArray<FMonthTally, NormalsMonthCount> Tallies;
		constexpr int32 Years = 300;
		const int64 FirstDay = static_cast<int64>(JulianDayFromUtc(FDateTime(1901, 1, 1, 12, 0, 0)));
		const int64 LastDay = static_cast<int64>(JulianDayFromUtc(FDateTime(1901 + Years, 1, 1, 12, 0, 0)));
		for (int64 DayIndex = FirstDay; DayIndex < LastDay; ++DayIndex)
		{
			const FGeneratedDay Day = Generator.Day(DayIndex);
			FMonthTally& Tally = Tallies[UtcFromJulianDay(static_cast<double>(DayIndex)).GetMonth() - 1];
			++Tally.Days;
			Tally.MaximumSum += Day.MaximumCelsius;
			Tally.MinimumSum += Day.MinimumCelsius;
			Tally.PrecipitationSum += Day.PrecipitationMillimeters;
			Tally.CloudSum += Day.CloudCover;
			Tally.WindSum += Day.WindSpeedMetersPerSecond;
			if (Day.bWet)
			{
				++Tally.WetDays;
				Tally.SmallestWetAmount = FMath::Min(Tally.SmallestWetAmount, Day.PrecipitationMillimeters);
			}
		}
		for (int32 Month = 0; Month < NormalsMonthCount; ++Month)
		{
			const FClimateMonth& Normal = Place.Normals.Months[Month];
			const FMonthTally& Tally = Tallies[Month];
			const double MonthsCounted = static_cast<double>(Years);
			const FString At = FString::Printf(TEXT("%s, month %d: "), *Name, Month + 1);
			Near(Test, At + TEXT("mean maximum (C)"), Tally.MaximumSum / Tally.Days, Normal.MeanMaximumCelsius, 0.6);
			Near(Test, At + TEXT("mean minimum (C)"), Tally.MinimumSum / Tally.Days, Normal.MeanMinimumCelsius, 0.6);
			Near(Test, At + TEXT("wet days per month"), Tally.WetDays / MonthsCounted, Normal.WetDays, 0.08 * Normal.WetDays + 0.1);
			Near(Test, At + TEXT("precipitation (mm)"), Tally.PrecipitationSum / MonthsCounted, Normal.PrecipitationMillimeters,
				0.12 * Normal.PrecipitationMillimeters + 0.5);
			Near(Test, At + TEXT("mean cloud cover"), Tally.CloudSum / Tally.Days, Generator.MonthStatistics(Month).MeanCloudCover, 0.02);
			Near(Test, At + TEXT("mean wind (m/s)"), Tally.WindSum / Tally.Days, Normal.WindSpeedMetersPerSecond,
				0.05 * Normal.WindSpeedMetersPerSecond);
			Test.TestTrue(At + TEXT("every wet day is wet by the source's count"),
				Tally.WetDays == 0 || Tally.SmallestWetAmount >= Place.Normals.WetDayThresholdMillimeters);
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeWeatherKeepsNormalsTest, "Musee.Time.WeatherKeepsNormals", Flags)
	bool FTimeWeatherKeepsNormalsTest::RunTest(const FString& Parameters)
	{
		const FWeatherGeneratorDesign Design = MuseumWeatherDesign().Generator;
		CheckClimate(*this, TEXT("continental"), ContinentalPlace(20260926), Design);
		CheckClimate(*this, TEXT("maritime"), MaritimePlace(51), Design);

		// The cloud the sunshine implies: Beijing-like winters are clear, maritime ones grey.
		const FWeatherGenerator Continental(ContinentalPlace(1), Design);
		const FWeatherGenerator Maritime(MaritimePlace(1), Design);
		TestTrue(TEXT("a continental January is clearer than a maritime one"),
			Continental.MonthStatistics(0).MeanCloudCover + 0.3 < Maritime.MonthStatistics(0).MeanCloudCover);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeWeatherDeterminismTest, "Musee.Time.WeatherDeterminism", Flags)
	bool FTimeWeatherDeterminismTest::RunTest(const FString& Parameters)
	{
		// Every instant has one answer, however it is reached: forwards, backwards, or alone.
		const FWeatherGeneratorDesign Design = MuseumWeatherDesign().Generator;
		const FWeatherGenerator Forwards(ContinentalPlace(99), Design);
		const FWeatherGenerator Backwards(ContinentalPlace(99), Design);
		const FWeatherGenerator OtherSeed(ContinentalPlace(100), Design);
		const FDateTime Start(2026, 7, 1);
		constexpr int32 Samples = 200;
		TArray<FWeatherState> Seen;
		for (int32 Sample = 0; Sample < Samples; ++Sample) { Seen.Add(Forwards.StateAt(Start + FTimespan::FromHours(7.3 * Sample))); }
		bool bSame = true;
		bool bOtherSeedDiffers = false;
		for (int32 Sample = Samples - 1; Sample >= 0; --Sample)
		{
			const FDateTime At = Start + FTimespan::FromHours(7.3 * Sample);
			const FWeatherState State = Backwards.StateAt(At);
			bSame &= State.AirTemperatureCelsius == Seen[Sample].AirTemperatureCelsius && State.CloudCover == Seen[Sample].CloudCover
				&& State.RainMillimetersPerHour == Seen[Sample].RainMillimetersPerHour && State.WindFromDegrees == Seen[Sample].WindFromDegrees;
			bOtherSeedDiffers |= OtherSeed.StateAt(At).AirTemperatureCelsius != State.AirTemperatureCelsius;
		}
		TestTrue(TEXT("the same seed gives the same weather in any order"), bSame);
		TestTrue(TEXT("another seed gives other weather"), bOtherSeedDiffers);

		// The weather flows: over two months in steps of ten minutes nothing jumps.
		double LargestTemperatureStep = 0.0;
		double LargestCloudStep = 0.0;
		double LargestWindTurn = 0.0;
		FWeatherState Previous = Forwards.StateAt(Start);
		for (int32 Step = 1; Step <= 60 * 144; ++Step)
		{
			const FWeatherState State = Forwards.StateAt(Start + FTimespan::FromMinutes(10.0 * Step));
			LargestTemperatureStep = FMath::Max(LargestTemperatureStep, FMath::Abs(State.AirTemperatureCelsius - Previous.AirTemperatureCelsius));
			LargestCloudStep = FMath::Max(LargestCloudStep, FMath::Abs(State.CloudCover - Previous.CloudCover));
			LargestWindTurn = FMath::Max(LargestWindTurn, FMath::Abs(SignedDegrees(State.WindFromDegrees - Previous.WindFromDegrees)));
			Previous = State;
		}
		Near(*this, TEXT("largest temperature change in ten minutes (C)"), LargestTemperatureStep, 0.0, 1.0);
		Near(*this, TEXT("largest cloud change in ten minutes"), LargestCloudStep, 0.0, 0.2);
		Near(*this, TEXT("largest wind turn in ten minutes (degrees)"), LargestWindTurn, 0.0, 15.0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeWeatherSnowTest, "Musee.Time.WeatherSnow", Flags)
	bool FTimeWeatherSnowTest::RunTest(const FString& Parameters)
	{
		// Precipitation falls as snow below the band about 1 C, as rain above it, and never from a clear sky.
		const FWeatherGeneratorDesign Design = MuseumWeatherDesign().Generator;
		const FWeatherGenerator Generator(ContinentalPlace(7), Design);
		bool bPhaseByTemperature = true;
		bool bCloudWhenFalling = true;
		int32 Falling = 0;
		for (int32 Hour = 0; Hour < 24 * 365 * 3; ++Hour)
		{
			const FWeatherState State = Generator.StateAt(FDateTime(2024, 1, 1) + FTimespan::FromHours(Hour + 0.5));
			if (State.PrecipitationMillimetersPerHour() <= 0.0) { continue; }
			++Falling;
			const double Temperature = State.AirTemperatureCelsius;
			bPhaseByTemperature &= (Temperature <= 0.0 && State.RainMillimetersPerHour == 0.0)
				|| (Temperature >= 2.0 && State.SnowfallMillimetersPerHour == 0.0) || (Temperature > 0.0 && Temperature < 2.0);
			bCloudWhenFalling &= State.CloudCover >= Design.RainingCloudCover - 1e-9;
		}
		TestTrue(TEXT("precipitation falls in three years of hours"), Falling > 0);
		TestTrue(TEXT("snow below 0 C, rain above 2 C"), bPhaseByTemperature);
		TestTrue(TEXT("the sky is overcast while precipitation falls"), bCloudWhenFalling);
		return true;
	}
}

#endif
