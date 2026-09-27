#include "MotionDrive.h"

Musee::Motion::FSCurve FMotionDrive::Run(double FromShare, double ToShare, double Travel) const
{
	const double Low = FMath::Min(FromShare, ToShare);
	const double High = FMath::Max(FromShare, ToShare);
	// The move's stretches between the zones' edges: each lies wholly inside the zones that hold its middle, or
	// outside them all.
	TArray<double> Cuts = { Low, High };
	for (const FMotionDriveZone& Zone : Zones)
	{
		for (const double Edge : { Zone.From, Zone.To })
		{
			if (Edge > Low && Edge < High)
			{
				Cuts.Add(Edge);
			}
		}
	}
	Cuts.Sort();
	double SpeedLimit = TNumericLimits<double>::Max();
	double AccelLimit = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index + 1 < Cuts.Num(); ++Index)
	{
		if (Cuts[Index + 1] <= Cuts[Index])
		{
			continue;
		}
		const double Middle = 0.5 * (Cuts[Index] + Cuts[Index + 1]);
		bool bZoned = false;
		for (const FMotionDriveZone& Zone : Zones)
		{
			if (Middle > Zone.From && Middle < Zone.To)
			{
				SpeedLimit = FMath::Min(SpeedLimit, Zone.Speed);
				AccelLimit = FMath::Min(AccelLimit, Zone.Accel);
				bZoned = true;
			}
		}
		if (!bZoned)
		{
			SpeedLimit = FMath::Min(SpeedLimit, Speed);
			AccelLimit = FMath::Min(AccelLimit, Accel);
		}
	}
	return Musee::Motion::FSCurve::RestToRest((High - Low) * Travel, SpeedLimit, AccelLimit, Jerk);
}
