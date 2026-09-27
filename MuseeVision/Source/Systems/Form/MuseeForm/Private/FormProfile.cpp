#include "FormProfile.h"

namespace
{
	TArray<double> PointOf(const FVector2D& Point)
	{
		return {Point.X, Point.Y};
	}

	bool IsPoint(const TArray<double>& Values)
	{
		return Values.Num() == 2;
	}
}

FFormProfile FFormProfile::From(const FVector2D& StartMeters)
{
	FFormProfile Profile;
	Profile.Start = PointOf(StartMeters);
	return Profile;
}

FFormProfile& FFormProfile::LineTo(const FVector2D& EndMeters, EFormJoint Joint)
{
	FFormProfileStep& Step = Steps.AddDefaulted_GetRef();
	Step.Line = PointOf(EndMeters);
	Step.Joint = Joint;
	return *this;
}

FFormProfile& FFormProfile::ArcAbout(const FVector2D& CenterMeters, double RadiusAMeters, double RadiusBMeters, double FromDegrees,
	double ToDegrees, EFormJoint Joint)
{
	FFormProfileStep& Step = Steps.AddDefaulted_GetRef();
	Step.Arc.CenterMeters = PointOf(CenterMeters);
	Step.Arc.RadiiMeters = {RadiusAMeters, RadiusBMeters};
	Step.Arc.FromDegrees = FromDegrees;
	Step.Arc.ToDegrees = ToDegrees;
	Step.Joint = Joint;
	return *this;
}

FFormProfile& FFormProfile::Close()
{
	Closed = true;
	return *this;
}

FFormProfile FFormProfile::Polyline(TConstArrayView<FVector2D> PointsMeters, bool bClosed)
{
	FFormProfile Profile = From(PointsMeters.IsEmpty() ? FVector2D::ZeroVector : PointsMeters[0]);
	for (int32 Index = 1; Index < PointsMeters.Num(); ++Index)
	{
		Profile.LineTo(PointsMeters[Index], EFormJoint::Crisp);
	}
	Profile.Closed = bClosed;
	return Profile;
}

FFormProfile FFormProfile::Polygon(TConstArrayView<FVector2D> PointsMeters)
{
	// A point closer to its neighbour than a micrometre (the kernel's weld tolerance) repeats it; one whose two edges
	// turn by less than a microradian runs straight on or doubles back.
	constexpr double SamePointMeters = 1.0e-6;
	constexpr double StraightRadians = 1.0e-6;
	TArray<FVector2D> Corners(PointsMeters.GetData(), PointsMeters.Num());
	bool bDropped = true;
	while (bDropped && Corners.Num() >= 3)
	{
		bDropped = false;
		for (int32 Index = 0; Index < Corners.Num() && Corners.Num() >= 3; ++Index)
		{
			const FVector2D In = Corners[Index] - Corners[(Index + Corners.Num() - 1) % Corners.Num()];
			const FVector2D Out = Corners[(Index + 1) % Corners.Num()] - Corners[Index];
			const double Sine = FVector2D::CrossProduct(In, Out);
			if (In.Size() <= SamePointMeters || Out.Size() <= SamePointMeters || FMath::Abs(Sine) <= StraightRadians * In.Size() * Out.Size())
			{
				Corners.RemoveAt(Index);
				bDropped = true;
				--Index;
			}
		}
	}
	return Polyline(Corners, true);
}

FFormProfile FFormProfile::Rectangle(const FVector2D& MinMeters, const FVector2D& MaxMeters)
{
	const FVector2D Corners[4] = {MinMeters, FVector2D(MaxMeters.X, MinMeters.Y), MaxMeters, FVector2D(MinMeters.X, MaxMeters.Y)};
	return Polyline(Corners, true);
}

FFormProfile FFormProfile::Ellipse(const FVector2D& CenterMeters, double RadiusAMeters, double RadiusBMeters, double StartDegrees,
	bool bClockwise)
{
	const double T = FMath::DegreesToRadians(StartDegrees);
	FFormProfile Profile = From(CenterMeters + FVector2D(RadiusAMeters * FMath::Cos(T), RadiusBMeters * FMath::Sin(T)));
	Profile.ArcAbout(CenterMeters, RadiusAMeters, RadiusBMeters, StartDegrees, StartDegrees + (bClockwise ? 360.0 : -360.0),
		EFormJoint::Smooth);
	return Profile.Close();
}

FVector2D FFormProfile::StartPoint() const
{
	return IsPoint(Start) ? FVector2D(Start[0], Start[1]) : FVector2D::ZeroVector;
}

bool FFormProfile::IsWellFormed(FString* OutWhy) const
{
	auto Fail = [OutWhy](const FString& Why)
	{
		if (OutWhy)
		{
			*OutWhy = Why;
		}
		return false;
	};
	if (!IsPoint(Start))
	{
		return Fail(TEXT("its start is not a point (two numbers, A and B)"));
	}
	if (Steps.IsEmpty())
	{
		return Fail(TEXT("it draws nothing"));
	}
	for (int32 Index = 0; Index < Steps.Num(); ++Index)
	{
		const FFormProfileStep& Step = Steps[Index];
		if (!Step.IsArc() && !IsPoint(Step.Line))
		{
			return Fail(FString::Printf(TEXT("step %d: a line's end is two numbers, A and B"), Index));
		}
		if (Step.IsArc() && (!IsPoint(Step.Arc.CenterMeters) || Step.Arc.RadiiMeters.IsEmpty() || Step.Arc.RadiiMeters.Num() > 2))
		{
			return Fail(FString::Printf(TEXT("step %d: an arc has a centre (A, B) and one or two radii"), Index));
		}
		if (Step.IsArc() && (Step.Arc.RadiiMeters[0] <= 0.0 || Step.Arc.RadiiMeters.Last() <= 0.0))
		{
			return Fail(FString::Printf(TEXT("step %d: an arc's radii are positive"), Index));
		}
	}
	return true;
}
