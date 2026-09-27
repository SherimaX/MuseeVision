#include "Kernel/Lathe.h"

#include "Kernel/Profile.h"
#include "Kernel/Sweep.h"

namespace Musee::Form
{
	FVector FLatheAxis::Radial(double AngleRadians) const
	{
		const FVector Quarter = FVector::CrossProduct(Up, Zero);
		return Zero * FMath::Cos(AngleRadians) + Quarter * FMath::Sin(AngleRadians);
	}

	FVector FLatheAxis::Tangent(double AngleRadians) const
	{
		const FVector Quarter = FVector::CrossProduct(Up, Zero);
		return Quarter * FMath::Cos(AngleRadians) - Zero * FMath::Sin(AngleRadians);
	}

	double LargestRadius(const FProfileStations& Profile)
	{
		double Largest = 0.0;
		for (const FProfileVertex& Vertex : Profile.Vertices)
		{
			Largest = FMath::Max(Largest, Vertex.Point.X);
		}
		return Largest;
	}

	TArray<double> LatheStations(const FProfileStations& Profile, double FromRadians, double ToRadians,
		double ToleranceMeters, TConstArrayView<double> RequiredRadians)
	{
		return ArcStations(LargestRadius(Profile), FromRadians, ToRadians, ToleranceMeters, RequiredRadians);
	}

	TArray<FSweepFrame> LatheFrames(const FLatheAxis& Axis, TConstArrayView<double> AnglesRadians,
		double ReferenceRadiusMeters)
	{
		TArray<FSweepFrame> Frames;
		const bool bIncreasing = AnglesRadians.Num() < 2 || AnglesRadians.Last() > AnglesRadians[0];
		for (const double Angle : AnglesRadians)
		{
			FSweepFrame F;
			F.Origin = Axis.Origin;
			F.AxisA = Axis.Radial(Angle);
			F.AxisB = Axis.Up;
			F.NormalA = F.AxisA;
			F.NormalB = Axis.Up;
			F.Along = bIncreasing ? Axis.Tangent(Angle) : -Axis.Tangent(Angle);
			F.U = ReferenceRadiusMeters * Angle;
			Frames.Add(F);
		}
		return Frames;
	}

	void Lathe(FMeshBuilder& Mesh, const FProfileStations& Profile, const FLatheAxis& Axis,
		TConstArrayView<double> AnglesRadians)
	{
		for (const FProfileVertex& Vertex : Profile.Vertices)
		{
			if (!ensureMsgf(Vertex.Point.X >= 0.0, TEXT("A lathe's profile lies on one side of its axis (radius %f m)."),
				Vertex.Point.X))
			{
				return;
			}
		}
		Sweep(Mesh, LatheFrames(Axis, AnglesRadians, LargestRadius(Profile)), Profile);
	}
}
