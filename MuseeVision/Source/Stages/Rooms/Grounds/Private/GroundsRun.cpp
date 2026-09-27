#include "GroundsRun.h"

namespace Musee::Grounds
{
	namespace
	{
		/** Two directions closer than this (a thousandth of a radian) meet tangent, and their joint is square. */
		constexpr double TangentTolerance = 1e-3;

		FVector2D OnArc(const FVector2D& Centre, double Radius, double Degrees)
		{
			const double Radians = FMath::DegreesToRadians(Degrees);
			return Centre + Radius * FVector2D(FMath::Cos(Radians), FMath::Sin(Radians));
		}
	}

	double FRunLeg::Length() const
	{
		return bArc ? Radius * FMath::DegreesToRadians(FMath::Abs(ToDegrees - FromDegrees)) : FVector2D::Distance(Start, End);
	}

	FVector2D FRunLeg::PointAt(double Distance) const
	{
		const double Fraction = Length() > 0.0 ? Distance / Length() : 0.0;
		return bArc ? OnArc(Centre, Radius, FMath::Lerp(FromDegrees, ToDegrees, Fraction)) : FMath::Lerp(Start, End, Fraction);
	}

	FVector2D FRunLeg::DirectionAt(double Distance) const
	{
		if (!bArc)
		{
			return (End - Start).GetSafeNormal();
		}
		const double Radians = FMath::DegreesToRadians(FMath::Lerp(FromDegrees, ToDegrees, Distance / Length()));
		const double Sense = ToDegrees >= FromDegrees ? 1.0 : -1.0;
		return Sense * FVector2D(-FMath::Sin(Radians), FMath::Cos(Radians));
	}

	void FRunLeg::Extend(FFormProfile& Path, double Distance0, double Distance1) const
	{
		if (!bArc)
		{
			Path.LineTo(PointAt(Distance1), EFormJoint::Crisp);
			return;
		}
		const double Total = Length();
		const double Near = FMath::Lerp(FromDegrees, ToDegrees, Distance0 / Total);
		const double Far = FMath::Lerp(FromDegrees, ToDegrees, Distance1 / Total);
		Path.ArcAbout(Centre, Radius, Radius, Near, Far, EFormJoint::Crisp);
	}

	FRun FRun::From(const FVector2D& Point)
	{
		FRun Run;
		Run.RunStart = Point;
		return Run;
	}

	FRun FRun::Through(TConstArrayView<FVector2D> Points, bool bLoop)
	{
		FRun Run = From(Points[0]);
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			Run.LineTo(Points[Index]);
		}
		if (bLoop)
		{
			Run.Close();
		}
		return Run;
	}

	FRun& FRun::LineTo(const FVector2D& Point)
	{
		// The leg starts where the run ends before it is added: once added, it is the run's end.
		FRunLeg Leg;
		Leg.Start = End();
		Leg.End = Point;
		RunLegs.Add(Leg);
		return *this;
	}

	FRun& FRun::ArcTo(const FVector2D& Centre, double ToDegrees, bool bIncreasing)
	{
		FRunLeg Leg;
		Leg.bArc = true;
		Leg.Start = End();
		Leg.Centre = Centre;
		Leg.Radius = FVector2D::Distance(Centre, Leg.Start);
		// The start's angle is taken on the turn that makes the arc run the way asked, and less than a whole turn.
		const double StartDegrees = FMath::RadiansToDegrees(FMath::Atan2(Leg.Start.Y - Centre.Y, Leg.Start.X - Centre.X));
		const double Sweep = FMath::Fmod(ToDegrees - StartDegrees, 360.0);
		const double Turned = Sweep < 0.0 ? Sweep + 360.0 : Sweep;
		Leg.ToDegrees = ToDegrees;
		Leg.FromDegrees = bIncreasing ? ToDegrees - Turned : ToDegrees + (Turned > 0.0 ? 360.0 - Turned : 0.0);
		Leg.End = OnArc(Centre, Leg.Radius, ToDegrees);
		RunLegs.Add(Leg);
		return *this;
	}

	FRun& FRun::Close()
	{
		if (!End().Equals(RunStart, UE_KINDA_SMALL_NUMBER))
		{
			LineTo(RunStart);
		}
		bClosed = true;
		return *this;
	}

	double FRun::Length() const
	{
		double Total = 0.0;
		for (const FRunLeg& Leg : RunLegs)
		{
			Total += Leg.Length();
		}
		return Total;
	}

	FVector2D FRun::End() const
	{
		return RunLegs.IsEmpty() ? RunStart : RunLegs.Last().End;
	}

	FFormProfile FRun::Path() const
	{
		FFormProfile Profile = FFormProfile::From(RunStart);
		for (const FRunLeg& Leg : RunLegs)
		{
			Leg.Extend(Profile, 0.0, Leg.Length());
		}
		if (bClosed)
		{
			Profile.Close();
		}
		return Profile;
	}

	FVector FRun::MitreBefore(int32 Index) const
	{
		if (Index == 0 && !bClosed)
		{
			return FVector::ZeroVector;
		}
		const FRunLeg& Before = RunLegs[(Index + RunLegs.Num() - 1) % RunLegs.Num()];
		const FVector2D In = Before.DirectionAt(Before.Length());
		const FVector2D Out = RunLegs[Index].DirectionAt(0.0);
		if (FVector2D::DotProduct(In, Out) > 1.0 - TangentTolerance)
		{
			return FVector::ZeroVector;
		}
		// The bisecting plane of the corner holds the vertical and the difference of the two directions.
		const FVector2D Normal = (In + Out).GetSafeNormal();
		return FVector(Normal.X, Normal.Y, 0.0);
	}

	TArray<FRunStone> FRun::Stones(double Nominal) const
	{
		TArray<FRunStone> Out;
		for (int32 Index = 0; Index < RunLegs.Num(); ++Index)
		{
			const FRunLeg& Leg = RunLegs[Index];
			const double LegLength = Leg.Length();
			const int32 Count = FMath::Max(1, FMath::RoundToInt32(LegLength / Nominal));
			for (int32 Stone = 0; Stone < Count; ++Stone)
			{
				const double Near = LegLength * Stone / Count;
				const double Far = LegLength * (Stone + 1) / Count;
				FRunStone& Made = Out.AddDefaulted_GetRef();
				Made.Path = FFormProfile::From(Leg.PointAt(Near));
				Leg.Extend(Made.Path, Near, Far);
				if (Stone == 0)
				{
					Made.StartCut = MitreBefore(Index);
				}
				if (Stone == Count - 1 && (bClosed || Index + 1 < RunLegs.Num()))
				{
					Made.EndCut = MitreBefore((Index + 1) % RunLegs.Num());
				}
			}
		}
		return Out;
	}
}
