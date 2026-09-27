#include "MotionMechanismSpec.h"

#include "MotionMechanism.h"
#include "MotionTexts.h"

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

double FMotionMechanismSpec::StartPosition() const
{
	return Start.IsNone() ? HomePosition() : PositionOf(Start).Get(HomePosition());
}

void FMotionMechanismSpec::KeyTexts()
{
	using Musee::Motion::KeyText;
	KeyText(OpenUse.Verb, Key, TEXTVIEW("open-use.verb"));
	KeyText(OpenUse.Caption, Key, TEXTVIEW("open-use.caption"));
	KeyText(CloseUse.Verb, Key, TEXTVIEW("close-use.verb"));
	KeyText(CloseUse.Caption, Key, TEXTVIEW("close-use.caption"));
	for (int32 Index = 0; Index < Rides.Num(); ++Index)
	{
		const FString Ride = FString::Printf(TEXT("ride-%d"), Index + 1);
		KeyText(Rides[Index].Verb, Key, Ride + TEXT(".verb"));
		KeyText(Rides[Index].Caption, Key, Ride + TEXT(".caption"));
	}
}
