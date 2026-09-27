#include "MotionMechanismSpec.h"

#include "MotionMechanism.h"

TOptional<double> FMotionMechanismSpec::PositionOf(FName State) const
{
	if (State == UMotionMechanism::Closed)
	{
		return 0.0;
	}
	if (State == UMotionMechanism::Open)
	{
		return 1.0;
	}
	const FMotionStop* Stop = Stops.FindByPredicate([State](const FMotionStop& S) { return S.Stop == State; });
	return Stop ? TOptional<double>(Stop->At) : TOptional<double>();
}

double FMotionMechanismSpec::DrawnPosition() const
{
	return Drawn.IsNone() ? 0.0 : PositionOf(Drawn).Get(0.0);
}

double FMotionMechanismSpec::HomePosition() const
{
	return Home.IsNone() ? DrawnPosition() : PositionOf(Home).Get(DrawnPosition());
}
