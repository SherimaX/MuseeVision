#include "Drawing/RisingPath.h"

#include "Drawing/Draw.h"
#include "Drawing/DrawnProfile.h"
#include "Kernel/Profile.h"
#include "Kernel/Tolerances.h"

namespace Musee::Form::Drawing
{
	namespace
	{
		/** Two distances along the path closer than the weld tolerance are one. */
		constexpr double SameAlongRise = WeldToleranceMeters;

		/** The rise as its stations: distances along the plan path, heights, and whether it turns crisply at each. */
		struct FRiseKnots
		{
			TArray<double> U;
			TArray<double> Z;
			TArray<bool> Crisp;

			/** The knot standing at a distance, or INDEX_NONE. */
			int32 KnotAt(double At) const
			{
				for (int32 Index = 0; Index < U.Num(); ++Index)
				{
					if (FMath::Abs(U[Index] - At) <= SameAlongRise)
					{
						return Index;
					}
				}
				return INDEX_NONE;
			}

			/** The grade of the chord from knot K to the next: zero past the ends, where the path runs level. */
			double ChordGrade(int32 K) const
			{
				return K >= 0 && K + 1 < U.Num() ? (Z[K + 1] - Z[K]) / (U[K + 1] - U[K]) : 0.0;
			}

			/** The chord holding a distance (the one before the knot at its end), or -1 before the first knot. */
			int32 ChordAt(double At) const
			{
				int32 Chord = -1;
				while (Chord + 1 < U.Num() && U[Chord + 1] <= At)
				{
					++Chord;
				}
				return Chord;
			}

			double HeightAt(double At) const
			{
				const int32 Chord = ChordAt(At);
				if (Chord < 0)
				{
					return Z[0];
				}
				if (Chord + 1 >= U.Num())
				{
					return Z.Last();
				}
				return FMath::Lerp(Z[Chord], Z[Chord + 1], (At - U[Chord]) / (U[Chord + 1] - U[Chord]));
			}

			/**
			 * The grade the path travels at through a distance: the chord's, between knots; at a knot, the arriving chord's
			 * (bLeaving false) or the leaving one's, or where the rise runs smoothly through the knot the grade halving the
			 * two directions.
			 */
			double GradeAt(double At, bool bLeaving) const
			{
				const int32 Knot = KnotAt(At);
				if (Knot == INDEX_NONE)
				{
					return ChordGrade(ChordAt(At));
				}
				const double Before = ChordGrade(Knot - 1);
				const double After = ChordGrade(Knot);
				if (Crisp[Knot])
				{
					return bLeaving ? After : Before;
				}
				return FMath::Tan(0.5 * (FMath::Atan(Before) + FMath::Atan(After)));
			}
		};

		bool ReadRise(const FFormSweep& Shape, FRiseKnots& Out, FString& OutWhy)
		{
			if (!CheckProfile(Shape.Rise, TEXT("the rise"), false, OutWhy))
			{
				return false;
			}
			for (const FProfileVertex& Vertex : Resolve(Shape.Rise).Vertices)
			{
				if (!Out.U.IsEmpty() && Vertex.Point.X <= Out.U.Last() + SameAlongRise)
				{
					OutWhy = FString::Printf(TEXT("the rise runs back or stands vertical at %.4f m along the path: it gives one height at each distance"),
						Vertex.Point.X);
					return false;
				}
				Out.U.Add(Vertex.Point.X);
				Out.Z.Add(Vertex.Point.Y);
				Out.Crisp.Add(Vertex.bCrisp);
			}
			return Out.U.Num() >= 2;
		}

		/**
		 * The plan curvature at each station of a run, per metre, positive towards the plan normal (the run's AxisA): the
		 * turn at the station over the mean of its two chords. A run's ends take their neighbour's.
		 */
		TArray<double> PlanBends(TConstArrayView<FSweepFrame> Run)
		{
			TArray<double> Bends;
			Bends.Init(0.0, Run.Num());
			for (int32 Index = 1; Index + 1 < Run.Num(); ++Index)
			{
				const FVector2D P0(Run[Index - 1].Origin);
				const FVector2D P1(Run[Index].Origin);
				const FVector2D P2(Run[Index + 1].Origin);
				const FVector2D In = (P1 - P0).GetSafeNormal();
				const FVector2D Out = (P2 - P1).GetSafeNormal();
				const double Turn = FMath::Atan2(FVector2D::CrossProduct(In, Out), FVector2D::DotProduct(In, Out));
				const double Chord = 0.5 * (FVector2D::Distance(P0, P1) + FVector2D::Distance(P1, P2));
				Bends[Index] = Chord > 0.0 ? Turn / Chord : 0.0;
			}
			if (Run.Num() > 2)
			{
				Bends[0] = Bends[1];
				Bends.Last() = Bends[Bends.Num() - 2];
			}
			return Bends;
		}

		/** A plan frame raised to a height, travelling up a grade, its section plumb or square to the rake. */
		FSweepFrame RaisedFrame(const FSweepFrame& Plan, double Height, double Grade, double Bend, bool bPlumb)
		{
			FSweepFrame F = Plan;
			F.Origin.Z = Height;
			const FVector Level = Plan.Along.GetSafeNormal2D();
			const FVector Travel = (Level + FVector::UpVector * Grade).GetSafeNormal();
			F.Along = Travel;
			if (bPlumb)
			{
				F.AxisB = FVector::UpVector;
				F.NormalB = FVector::UpVector;
				F.Rake = -Grade * Level;
				F.Bend = Bend;
				return F;
			}
			const FVector Up = (FVector::UpVector - Travel * Travel.Z).GetSafeNormal();
			F.AxisB = Up;
			F.NormalB = Up;
			return F;
		}

		/**
		 * Joins two runs of square sections at the station they share: the arriving section is cut on the plane halving
		 * the turn, and the leaving one starts from that same cut, so the two meet point for point. Where the path turns
		 * in plan alone, or in rise alone, this is the true mitre.
		 */
		void MitreJoint(FSweepFrame& Arriving, FSweepFrame& Leaving)
		{
			const FVector Normal = (Arriving.Along + Leaving.Along).GetSafeNormal();
			const double Along = FVector::DotProduct(Arriving.Along, Normal);
			if (Normal.IsNearlyZero() || Along <= UE_DOUBLE_KINDA_SMALL_NUMBER)
			{
				return;
			}
			Arriving.AxisA -= Arriving.Along * (FVector::DotProduct(Arriving.AxisA, Normal) / Along);
			Arriving.AxisB -= Arriving.Along * (FVector::DotProduct(Arriving.AxisB, Normal) / Along);
			Leaving.AxisA = Arriving.AxisA;
			Leaving.AxisB = Arriving.AxisB;
		}

		/** Raises one plan run onto the rise, splitting it where the rise turns crisply. */
		void RaiseRun(TConstArrayView<FSweepFrame> Plan, const FRiseKnots& Rise, const FFormSweep& Shape, TArray<TArray<FSweepFrame>>& OutRuns)
		{
			const TArray<double> Bends = PlanBends(Plan);
			TArray<FSweepFrame> Run;
			for (int32 Index = 0; Index < Plan.Num(); ++Index)
			{
				const FSweepFrame& P = Plan[Index];
				const double Height = Shape.ElevationMeters + Rise.HeightAt(P.U);
				const int32 Knot = Rise.KnotAt(P.U);
				const bool bFirst = Index == 0;
				const bool bLast = Index + 1 == Plan.Num();
				auto At = [&](bool bLeaving) { return RaisedFrame(P, Height, Rise.GradeAt(P.U, bLeaving), Bends[Index], Shape.bPlumb); };
				if (Knot != INDEX_NONE && Rise.Crisp[Knot] && !bFirst && !bLast)
				{
					Run.Add(At(false));
					OutRuns.Add(MoveTemp(Run));
					Run.Reset();
					Run.Add(At(true));
					continue;
				}
				Run.Add(At(!bLast));
			}
			OutRuns.Add(MoveTemp(Run));
		}
	}

	bool RisingRuns(const FFormSweep& Shape, TArray<TArray<FSweepFrame>>& OutRuns, FString& OutWhy)
	{
		if (Shape.IsOnFace() || Shape.Path.Closed)
		{
			OutWhy = TEXT("a path drawn on a face, or round a closed loop, doesn't rise");
			return false;
		}
		FRiseKnots Rise;
		if (!ReadRise(Shape, Rise, OutWhy))
		{
			if (OutWhy.IsEmpty())
			{
				OutWhy = TEXT("the rise has fewer than two stations");
			}
			return false;
		}
		const TArray<TArray<FSweepFrame>> PlanRunsOf = PathRuns(Resolve(Shape.Path), 0.0, Rise.U);
		if (PlanRunsOf.IsEmpty())
		{
			OutWhy = TEXT("the path has fewer than two stations, or doubles back too sharply to mitre");
			return false;
		}
		OutRuns.Reset();
		for (const TArray<FSweepFrame>& Plan : PlanRunsOf)
		{
			RaiseRun(Plan, Rise, Shape, OutRuns);
		}
		// Square sections meet on the plane halving each crisp turn; plumb ones stand on the same plumb section either side.
		for (int32 Index = 1; !Shape.bPlumb && Index < OutRuns.Num(); ++Index)
		{
			MitreJoint(OutRuns[Index - 1].Last(), OutRuns[Index][0]);
		}
		// U is metres along the rising reference line; runs meeting at a station share its U.
		double Travelled = 0.0;
		const FSweepFrame* Previous = nullptr;
		for (TArray<FSweepFrame>& Run : OutRuns)
		{
			for (FSweepFrame& F : Run)
			{
				Travelled += Previous ? FVector::Distance(Previous->Origin, F.Origin) : 0.0;
				F.U = Travelled;
				Previous = &F;
			}
		}
		return true;
	}
}
