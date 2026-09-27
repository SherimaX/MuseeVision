#include "NympheasCurves.h"

namespace Musee::Nympheas
{
	namespace
	{
		/**
		 * The steps the ellipse's arc length is tabulated at round one turn. The trapezium rule's error falls with the
		 * step's square: at 20,000 steps it is below a micrometre round the oval's 58.6 m.
		 */
		constexpr int32 ArcSteps = 20000;
		constexpr double Turn = 2.0 * UE_DOUBLE_PI;
		/** Bisection stops when the bracket is narrower than this, in radians (well under a micrometre on the oval). */
		constexpr double BisectionRadians = 1e-12;
	}

	FOvalCurve::FOvalCurve(double InA, double InB)
		: SemiA(InA), SemiB(InB)
	{
		auto Speed = [this](double T) { return FMath::Sqrt(FMath::Square(SemiA * FMath::Sin(T)) + FMath::Square(SemiB * FMath::Cos(T))); };
		ArcTable.SetNumZeroed(ArcSteps + 1);
		const double Step = Turn / ArcSteps;
		for (int32 Index = 1; Index <= ArcSteps; ++Index)
		{
			ArcTable[Index] = ArcTable[Index - 1] + 0.5 * (Speed(Step * (Index - 1)) + Speed(Step * Index)) * Step;
		}
	}

	FVector2D FOvalCurve::Point(double T) const
	{
		return FVector2D(SemiA * FMath::Cos(T), SemiB * FMath::Sin(T));
	}

	FVector2D FOvalCurve::Outward(double T) const
	{
		return FVector2D(SemiB * FMath::Cos(T), SemiA * FMath::Sin(T)).GetSafeNormal();
	}

	double FOvalCurve::Turning(double T) const
	{
		const double Angle = FMath::Atan2(SemiA * FMath::Sin(T), SemiB * FMath::Cos(T));
		return Angle + Turn * FMath::RoundToDouble((T - Angle) / Turn);
	}

	double FOvalCurve::Length(double D) const
	{
		return ArcTable.Last() + D * Turn;
	}

	double FOvalCurve::ArcAt(double T, double D) const
	{
		const double X = FMath::Clamp(T / Turn, 0.0, 1.0) * ArcSteps;
		const int32 Index = FMath::Clamp(FMath::FloorToInt32(X), 0, ArcSteps - 1);
		const double S = FMath::Lerp(ArcTable[Index], ArcTable[Index + 1], X - Index);
		return S + D * Turning(T);
	}

	double FOvalCurve::TAt(double U, double D) const
	{
		const double Whole = Length(D);
		const double Turns = FMath::FloorToDouble(U / Whole);
		const double Rest = U - Turns * Whole;
		double Low = 0.0;
		double High = Turn;
		while (High - Low > BisectionRadians)
		{
			const double Mid = 0.5 * (Low + High);
			(ArcAt(Mid, D) < Rest ? Low : High) = Mid;
		}
		return 0.5 * (Low + High) + Turns * Turn;
	}

	double FOvalCurve::TWhereY(double Y, double D, double Low, double High) const
	{
		const bool bRising = Parallel(High, D).Y > Parallel(Low, D).Y;
		while (High - Low > BisectionRadians)
		{
			const double Mid = 0.5 * (Low + High);
			((Parallel(Mid, D).Y < Y) == bRising ? Low : High) = Mid;
		}
		return 0.5 * (Low + High);
	}

	TArray<FVector2D> FOvalCurve::Stations(double D, double From, double To, int32 Count) const
	{
		TArray<FVector2D> Points;
		for (int32 Index = 0; Index <= Count; ++Index)
		{
			Points.Add(Parallel(FMath::Lerp(From, To, double(Index) / Count), D));
		}
		return Points;
	}

	FMuseeFrame FrameOfAxes(const FVector& OriginMeters, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(OriginMeters, FRotationMatrix::MakeFromXY(X, Y).ToQuat());
	}

	FFormProfile Polygon(TConstArrayView<FVector2D> Points)
	{
		return FFormProfile::Polyline(Points, true);
	}

	FFormProfile SmoothPath(TConstArrayView<FVector2D> Points)
	{
		FFormProfile Path = FFormProfile::From(Points[0]);
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			Path.LineTo(Points[Index], EFormJoint::Smooth);
		}
		return Path;
	}

	TArray<FVector2D> CatmullRom(TConstArrayView<FVector2D> Points, int32 Subdivisions)
	{
		const int32 Last = Points.Num() - 1;
		auto At = [&](int32 Index)
		{
			// Past either end the spline's control point is the end's reflection, so the end's tangent is its span's.
			return Index < 0 ? 2.0 * Points[0] - Points[1] : (Index > Last ? 2.0 * Points[Last] - Points[Last - 1] : Points[Index]);
		};
		TArray<FVector2D> Out = {Points[0]};
		for (int32 Span = 0; Span < Last; ++Span)
		{
			const FVector2D P0 = At(Span - 1);
			const FVector2D P1 = At(Span);
			const FVector2D P2 = At(Span + 1);
			const FVector2D P3 = At(Span + 2);
			for (int32 Step = 1; Step <= Subdivisions; ++Step)
			{
				const double T = double(Step) / Subdivisions;
				const double T2 = T * T;
				const double T3 = T2 * T;
				Out.Add(0.5 * (2.0 * P1 + (P2 - P0) * T + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * T2 + (3.0 * P1 - P0 - 3.0 * P2 + P3) * T3));
			}
		}
		return Out;
	}

	double PolylineLength(TConstArrayView<FVector2D> Points)
	{
		double Length = 0.0;
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			Length += FVector2D::Distance(Points[Index - 1], Points[Index]);
		}
		return Length;
	}

	void PolylineAt(TConstArrayView<FVector2D> Points, double Distance, FVector2D& OutPoint, FVector2D& OutDirection)
	{
		double Travelled = 0.0;
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			const double Span = FVector2D::Distance(Points[Index - 1], Points[Index]);
			if (Travelled + Span >= Distance || Index == Points.Num() - 1)
			{
				OutDirection = (Points[Index] - Points[Index - 1]).GetSafeNormal();
				OutPoint = Points[Index - 1] + OutDirection * FMath::Clamp(Distance - Travelled, 0.0, Span);
				return;
			}
			Travelled += Span;
		}
		OutPoint = Points.IsEmpty() ? FVector2D::ZeroVector : Points[0];
		OutDirection = FVector2D(1.0, 0.0);
	}

	TArray<FVector2D> PolylineBetween(TConstArrayView<FVector2D> Points, double From, double To)
	{
		FVector2D Start;
		FVector2D End;
		FVector2D Direction;
		PolylineAt(Points, From, Start, Direction);
		PolylineAt(Points, To, End, Direction);
		TArray<FVector2D> Out = {Start};
		double Travelled = 0.0;
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			Travelled += FVector2D::Distance(Points[Index - 1], Points[Index]);
			if (Travelled > From && Travelled < To && !Points[Index].Equals(Out.Last(), UE_KINDA_SMALL_NUMBER))
			{
				Out.Add(Points[Index]);
			}
		}
		if (!End.Equals(Out.Last(), UE_KINDA_SMALL_NUMBER))
		{
			Out.Add(End);
		}
		return Out;
	}
}
