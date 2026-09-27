#include "Stations/Stations.h"

#include "Algo/Reverse.h"
#include "Kernel/Tolerances.h"

namespace Musee::Form
{
	namespace
	{
		/** Parameters that agree to within this fraction of their span are one station: the difference is rounding. */
		constexpr double RoundingFraction = 1.0e-12;

		/**
		 * The largest step whose chord on a circle of this radius stays within the tolerance. The sagitta of a step
		 * is s = 2 r sin^2(step / 4), written this way so it stays exact for tolerances far below the radius.
		 */
		double StepWithinTolerance(double RadiusMeters, double ToleranceMeters)
		{
			if (RadiusMeters <= 0.0 || ToleranceMeters >= 2.0 * RadiusMeters)
			{
				return MaxStationStepRadians;
			}
			const double Step = 4.0 * FMath::Asin(FMath::Sqrt(ToleranceMeters / (2.0 * RadiusMeters)));
			return FMath::Min(MaxStationStepRadians, Step);
		}

		/**
		 * The breaks of a span: its two ends and every required station strictly between them, ascending, with
		 * stations closer than Coincident taken as one.
		 */
		TArray<double> Breaks(double Low, double High, TConstArrayView<double> Required, double Coincident)
		{
			TArray<double> Inside;
			for (const double Station : Required)
			{
				if (Station > Low + Coincident && Station < High - Coincident)
				{
					Inside.Add(Station);
				}
			}
			Inside.Sort();
			TArray<double> Result;
			Result.Add(Low);
			for (const double Station : Inside)
			{
				if (Station > Result.Last() + Coincident)
				{
					Result.Add(Station);
				}
			}
			Result.Add(High);
			return Result;
		}

		/**
		 * Stations from From to To: the breaks of the span, each gap divided into StepsFor(gap) equal steps. Breaks
		 * are kept exactly, so a required station is bit-for-bit the value the caller gave.
		 */
		TArray<double> Divide(double From, double To, TConstArrayView<double> Required, double Coincident,
			TFunctionRef<int32(double)> StepsFor)
		{
			if (From == To)
			{
				return {From};
			}
			const double Low = FMath::Min(From, To);
			const double High = FMath::Max(From, To);
			const double Merge = FMath::Max(Coincident, RoundingFraction * (High - Low));
			const TArray<double> Gaps = Breaks(Low, High, Required, Merge);
			TArray<double> Result;
			Result.Add(Gaps[0]);
			for (int32 Gap = 0; Gap + 1 < Gaps.Num(); ++Gap)
			{
				const double A = Gaps[Gap];
				const double B = Gaps[Gap + 1];
				const int32 Steps = FMath::Max(1, StepsFor(B - A));
				for (int32 Step = 1; Step < Steps; ++Step)
				{
					Result.Add(A + (B - A) * Step / Steps);
				}
				Result.Add(B);
			}
			if (To < From)
			{
				Algo::Reverse(Result);
			}
			return Result;
		}

		/**
		 * The fewest equal steps over a parameter gap that keep the interpolation bound h^2 * bound / 8 within the
		 * tolerance.
		 */
		int32 CurveSteps(double Gap, double SecondDerivativeBound, double ToleranceMeters)
		{
			if (SecondDerivativeBound <= 0.0)
			{
				return 1;
			}
			const double MaxStep = FMath::Sqrt(8.0 * ToleranceMeters / SecondDerivativeBound);
			return FMath::Max(1, FMath::CeilToInt32(Gap / MaxStep));
		}
	}

	double ArcChordError(double RadiusMeters, double StepRadians)
	{
		const double Quarter = FMath::Sin(0.25 * StepRadians);
		return 2.0 * RadiusMeters * Quarter * Quarter;
	}

	int32 ArcSegmentCount(double RadiusMeters, double SweepRadians, double ToleranceMeters)
	{
		const double Sweep = FMath::Abs(SweepRadians);
		if (Sweep <= 0.0)
		{
			return 0;
		}
		return FMath::Max(1, FMath::CeilToInt32(Sweep / StepWithinTolerance(RadiusMeters, ToleranceMeters)));
	}

	TArray<double> ArcStations(double RadiusMeters, double FromRadians, double ToRadians, double ToleranceMeters,
		TConstArrayView<double> RequiredRadians)
	{
		// Angles closer than a micrometre of arc are one station.
		const double Coincident = WeldToleranceMeters / FMath::Max(RadiusMeters, WeldToleranceMeters);
		return Divide(FromRadians, ToRadians, RequiredRadians, Coincident,
			[RadiusMeters, ToleranceMeters](double Gap) { return ArcSegmentCount(RadiusMeters, Gap, ToleranceMeters); });
	}

	TArray<double> EllipseStations(double RadiusAMeters, double RadiusBMeters, double FromRadians, double ToRadians,
		double ToleranceMeters)
	{
		const double Largest = FMath::Max(RadiusAMeters, RadiusBMeters);
		if (FMath::IsNearlyEqual(RadiusAMeters, RadiusBMeters, RoundingFraction * Largest))
		{
			return ArcStations(Largest, FromRadians, ToRadians, ToleranceMeters);
		}
		// |P''(t)| = |(RadiusA cos t, RadiusB sin t)| never exceeds the larger radius.
		return Divide(FromRadians, ToRadians, {}, 0.0, [Largest, ToleranceMeters](double Gap)
		{
			const int32 Turns = FMath::CeilToInt32(Gap / MaxStationStepRadians);
			return FMath::Max(Turns, CurveSteps(Gap, Largest, ToleranceMeters));
		});
	}

	TArray<double> CurveStations(double From, double To, double SecondDerivativeBound, double ToleranceMeters,
		TConstArrayView<double> Required)
	{
		return Divide(From, To, Required, 0.0, [SecondDerivativeBound, ToleranceMeters](double Gap)
		{
			return CurveSteps(Gap, SecondDerivativeBound, ToleranceMeters);
		});
	}

	double MeasureChordError(TFunctionRef<FVector(double)> Curve, TConstArrayView<double> Stations, int32 SamplesPerSpan)
	{
		double Largest = 0.0;
		for (int32 Span = 0; Span + 1 < Stations.Num(); ++Span)
		{
			const double T0 = Stations[Span];
			const double T1 = Stations[Span + 1];
			const FVector A = Curve(T0);
			const FVector B = Curve(T1);
			for (int32 Sample = 1; Sample < SamplesPerSpan; ++Sample)
			{
				const FVector P = Curve(FMath::Lerp(T0, T1, double(Sample) / SamplesPerSpan));
				Largest = FMath::Max(Largest, FMath::PointDistToSegment(P, A, B));
			}
		}
		return Largest;
	}
}
