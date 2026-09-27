#include "FormStair.h"

#include "MuseeHumanMeasure.h"

namespace
{
	/**
	 * The most risers a flight is searched over when its count is chosen from its run: a 1 m rise at the shallowest
	 * step anyone climbs (a 100 mm riser) has ten, and the museum's deepest stair (5.8 m) at that riser has 58.
	 */
	constexpr int32 MostRisersSearched = 200;
}

FFormStairSteps FFormStair::SetOut() const
{
	FFormStairSteps Steps;
	const double Pace = FMuseeHumanMeasure::Pace.Get();
	if (RiseMeters <= 0.0)
	{
		return Steps;
	}
	if (Risers > 0)
	{
		Steps.Risers = Risers;
		Steps.RiserMeters = RiseMeters / Risers;
		Steps.GoingMeters = (RunMeters > 0.0 && Risers > 1) ? RunMeters / (Risers - 1) : Pace - 2.0 * Steps.RiserMeters;
		return Steps;
	}
	if (RunMeters <= 0.0)
	{
		return Steps;
	}
	// The fewest risers that no body has to climb, then every count up to the search's end: the closest to the pace wins.
	const int32 Fewest = FMath::Max(2, FMath::CeilToInt32(RiseMeters / FMuseeHumanMeasure::MaxStepHeight.Get()));
	double BestError = TNumericLimits<double>::Max();
	for (int32 Count = Fewest; Count <= MostRisersSearched; ++Count)
	{
		const double Riser = RiseMeters / Count;
		const double Going = RunMeters / (Count - 1);
		const double Error = FMath::Abs(2.0 * Riser + Going - Pace);
		if (Error < BestError)
		{
			BestError = Error;
			Steps = FFormStairSteps{Count, Riser, Going};
		}
	}
	return Steps;
}
