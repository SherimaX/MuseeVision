#include "Drawing/DrawnProfile.h"

#include "Kernel/PlanRuns.h"
#include "Kernel/Tolerances.h"
#include "Stations/Stations.h"

namespace Musee::Form::Drawing
{
	namespace
	{
		/**
		 * A required station within ten weld tolerances (10 micrometres) of an existing one is that station. The builder
		 * drops triangles with an edge under two weld tolerances, so a strip this narrow would open the shell; ten
		 * micrometres of U is a hundredth of the finest made detail (Tolerances.h).
		 */
		constexpr double StationMergeMeters = 10.0 * WeldToleranceMeters;

		EProfileJoint JointOf(EFormJoint Joint)
		{
			switch (Joint)
			{
			case EFormJoint::Crisp: return EProfileJoint::Crisp;
			case EFormJoint::Smooth: return EProfileJoint::Smooth;
			default: return EProfileJoint::Auto;
			}
		}

		FVector2D PointOf(const TArray<double>& Values)
		{
			return FVector2D(Values[0], Values[1]);
		}

		/**
		 * A frame part of the way from one station to the next, on the chord between them. It stands square to that chord:
		 * a corner's mitre stretches the section across the corner alone, so a station set between two corners of a straight
		 * run (a door's jamb in a room's wall) carries no share of their mitres, which would lean its section along the run
		 * and splay a jamb's reveal by the wall's thickness times its distance from the run's middle over half the run.
		 */
		FSweepFrame Between(const FSweepFrame& A, const FSweepFrame& B, double Alpha)
		{
			FSweepFrame F;
			F.Origin = FMath::Lerp(A.Origin, B.Origin, Alpha);
			F.AxisB = FMath::Lerp(A.AxisB, B.AxisB, Alpha);
			F.NormalA = FMath::Lerp(A.NormalA, B.NormalA, Alpha).GetSafeNormal();
			F.AxisA = F.NormalA;
			F.NormalB = FMath::Lerp(A.NormalB, B.NormalB, Alpha).GetSafeNormal();
			F.Rake = FMath::Lerp(A.Rake, B.Rake, Alpha);
			F.Bend = FMath::Lerp(A.Bend, B.Bend, Alpha);
			F.Along = FMath::Lerp(A.Along, B.Along, Alpha).GetSafeNormal();
			F.U = FMath::Lerp(A.U, B.U, Alpha);
			return F;
		}
	}

	FProfile ToKernel(const FFormProfile& Profile)
	{
		FProfile Kernel(Profile.StartPoint());
		for (const FFormProfileStep& Step : Profile.Steps)
		{
			if (!Step.IsArc())
			{
				Kernel.LineTo(PointOf(Step.Line), JointOf(Step.Joint));
				continue;
			}
			const double RadiusA = Step.Arc.RadiiMeters[0];
			const double RadiusB = Step.Arc.RadiiMeters.Last();
			Kernel.Arc(PointOf(Step.Arc.CenterMeters), RadiusA, RadiusB, FMath::DegreesToRadians(Step.Arc.FromDegrees),
				FMath::DegreesToRadians(Step.Arc.ToDegrees), JointOf(Step.Joint));
		}
		if (Profile.Closed)
		{
			Kernel.Close();
		}
		return Kernel;
	}

	FProfileStations Resolve(const FFormProfile& Profile)
	{
		return ToKernel(Profile).Resolve(DefaultChordToleranceMeters);
	}

	FProfileStations RunOutward(const FProfileStations& Loop)
	{
		// With y south, (-dy, dx) is the walker's right; for a loop of positive signed area in (x, y) it points inwards.
		return Loop.SignedAreaSquareMeters() > 0.0 ? Loop.Reversed() : Loop;
	}

	FSweepFrame FrameAt(TConstArrayView<FSweepFrame> Run, double UMeters)
	{
		for (int32 Index = 0; Index + 1 < Run.Num(); ++Index)
		{
			const FSweepFrame& A = Run[Index];
			const FSweepFrame& B = Run[Index + 1];
			if (UMeters <= B.U || Index + 2 == Run.Num())
			{
				const double Span = B.U - A.U;
				return Between(A, B, Span > 0.0 ? FMath::Clamp((UMeters - A.U) / Span, 0.0, 1.0) : 0.0);
			}
		}
		return Run.IsEmpty() ? FSweepFrame() : Run[0];
	}

	TArray<TArray<FSweepFrame>> PathRuns(const FProfileStations& Path, double ElevationMeters, TConstArrayView<double> RequiredMeters)
	{
		TArray<TArray<FSweepFrame>> Runs = PlanRuns(Path, ElevationMeters);
		for (TArray<FSweepFrame>& Run : Runs)
		{
			for (const double U : RequiredMeters)
			{
				if (Run.Num() < 2 || U <= Run[0].U + WeldToleranceMeters || U >= Run.Last().U - WeldToleranceMeters)
				{
					continue;
				}
				const int32 After = Run.IndexOfByPredicate([U](const FSweepFrame& F) { return F.U >= U; });
				// A station this close already stands there: it takes the required distance exactly, and no sliver of a
				// strip is left between the two.
				for (const int32 Near : {After - 1, After})
				{
					if (FMath::Abs(Run[Near].U - U) <= StationMergeMeters)
					{
						Run[Near].U = U;
					}
				}
				if (Run[After].U == U || Run[After - 1].U == U)
				{
					continue;
				}
				const FSweepFrame& A = Run[After - 1];
				const FSweepFrame& B = Run[After];
				Run.Insert(Between(A, B, (U - A.U) / (B.U - A.U)), After);
			}
		}
		return Runs;
	}
}
