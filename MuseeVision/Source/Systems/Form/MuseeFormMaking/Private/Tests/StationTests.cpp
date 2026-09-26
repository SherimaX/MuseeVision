#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Stations/Stations.h"

namespace Musee::Form::Tests
{
	namespace
	{
		constexpr double Tolerance = 0.001;

		/** Chord errors this far over the tolerance are rounding in the measurement, not a broken bound. */
		constexpr double Rounding = 1.0e-9;

		FVector OnCircle(double Radius, double Angle)
		{
			return FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.0);
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormChordErrorBoundTest, "Musee.Form.ChordErrorBound",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormChordErrorBoundTest::RunTest(const FString& Parameters)
	{
		// From a 2 mm bead to a 200 m site arc, over a twelfth, a quarter and a whole turn.
		const double Radii[] = {0.002, 0.02, 0.2, 2.0, 20.0, 200.0};
		const double Sweeps[] = {UE_DOUBLE_PI / 6.0, UE_DOUBLE_HALF_PI, UE_DOUBLE_TWO_PI};
		for (const double Radius : Radii)
		{
			for (const double Sweep : Sweeps)
			{
				const TArray<double> Stations = ArcStations(Radius, 0.3, 0.3 + Sweep, Tolerance);
				const int32 Segments = Stations.Num() - 1;
				const double Measured = MeasureChordError([Radius](double T) { return OnCircle(Radius, T); }, Stations, 64);
				const FString Case = FString::Printf(TEXT("r %.3f m, sweep %.3f rad"), Radius, Sweep);
				TestTrue(FString::Printf(TEXT("%s: chord error %.6f m within the tolerance"), *Case, Measured),
					Measured <= Tolerance * (1.0 + Rounding));
				TestEqual(*FString::Printf(TEXT("%s: stations count the segments"), *Case), Segments,
					ArcSegmentCount(Radius, Sweep, Tolerance));
				TestEqual(*FString::Printf(TEXT("%s: the arc ends on its last station"), *Case), Stations.Last(),
					0.3 + Sweep);
				// The fewest: one segment fewer breaks the bound, unless the quarter-turn rule sets the count.
				const int32 Turns = FMath::CeilToInt32(Sweep / MaxStationStepRadians);
				if (Segments > Turns)
				{
					TestTrue(*FString::Printf(TEXT("%s: one chord fewer would exceed the tolerance"), *Case),
						ArcChordError(Radius, Sweep / (Segments - 1)) > Tolerance * (1.0 - Rounding));
				}
			}
		}

		// Required stations are kept bit for bit, and a descending arc is the ascending one reversed.
		const TArray<double> Required = {1.0, 1.0 + 1.0e-3, 4.0};
		const TArray<double> Shared = ArcStations(5.0, 0.0, UE_DOUBLE_TWO_PI, Tolerance, Required);
		for (const double Angle : Required)
		{
			TestTrue(*FString::Printf(TEXT("Required angle %.4f is a station"), Angle), Shared.Contains(Angle));
		}
		TestTrue(TEXT("Required stations keep the bound"),
			MeasureChordError([](double T) { return OnCircle(5.0, T); }, Shared, 64) <= Tolerance * (1.0 + Rounding));
		TArray<double> Descending = ArcStations(5.0, UE_DOUBLE_PI, 0.0, Tolerance);
		Algo::Reverse(Descending);
		TestTrue(TEXT("A descending arc takes the same stations"),
			Descending == ArcStations(5.0, 0.0, UE_DOUBLE_PI, Tolerance));

		// An ellipse (a 3 m by 1 m flattened arch) and a general curve keep the bound through their derivative bounds.
		const TArray<double> Ellipse = EllipseStations(3.0, 1.0, 0.0, UE_DOUBLE_PI, Tolerance);
		const double EllipseError = MeasureChordError(
			[](double T) { return FVector(3.0 * FMath::Cos(T), FMath::Sin(T), 0.0); }, Ellipse, 64);
		TestTrue(*FString::Printf(TEXT("Ellipse chord error %.6f m within the tolerance"), EllipseError),
			EllipseError <= Tolerance * (1.0 + Rounding));
		// (t, 0.5 sin 2t): |P''| = 2 |sin 2t| <= 2.
		const TArray<double> Wave = CurveStations(0.0, 6.0, 2.0, Tolerance);
		const double WaveError = MeasureChordError(
			[](double T) { return FVector(T, 0.5 * FMath::Sin(2.0 * T), 0.0); }, Wave, 64);
		TestTrue(*FString::Printf(TEXT("Curve chord error %.6f m within the tolerance"), WaveError),
			WaveError <= Tolerance * (1.0 + Rounding));
		TestEqual(TEXT("A straight line takes one step"), CurveStations(0.0, 10.0, 0.0, Tolerance).Num(), 2);
		return true;
	}
}

#endif
