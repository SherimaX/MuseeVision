#include "Engine/World.h"
#include "MotionMechanism.h"
#include "MuseeSignals.h"

// The mechanism's uses: its own control's open and close, and the rides a lift offers at its stops.

namespace
{
	/** A joint's axis within this angle of the vertical is a lift, whose verbs are Raise and Lower: 30 degrees, the
	 * steepest an inclined lift runs (EN 81-22 covers inclined lifts from 15 to 75 degrees from the horizontal). */
	constexpr double LiftAxisCosine = 0.866;

	/** A channel point nobody has published on stands at its rest, 0 (a door shut, an iris closed, no work performing). */
	constexpr float UnpublishedValue = 0.0f;

	const FName Prismatic(TEXT("prismatic"));

	FText CaptionOf(const FMuseeText& Caption)
	{
		return Caption.Key.IsNone() ? FText::GetEmpty() : Caption.ToText();
	}
}

FName UMotionMechanism::SequenceEnd(const TArray<FMotionStep>& Steps, FName Otherwise) const
{
	for (int32 Index = Steps.Num() - 1; Index >= 0; --Index)
	{
		if (Steps[Index].IsMove())
		{
			return Steps[Index].Move;
		}
	}
	return Otherwise;
}

bool UMotionMechanism::IsOpening() const
{
	return !IsAt(SequenceEnd(MechanismSpec.Open, Open));
}

FText UMotionMechanism::WhyBusy() const
{
	return bHeld
		? NSLOCTEXT("Musee", "motion.held", "Something is in its way.")
		: NSLOCTEXT("Musee", "motion.moving", "It is moving.");
}

FMuseeUse UMotionMechanism::Usage() const
{
	const bool bOpening = IsOpening();
	const FMotionUseText& Text = bOpening ? MechanismSpec.OpenUse : MechanismSpec.CloseUse;
	const FText Caption = CaptionOf(Text.Caption);
	if (IsRunning() || MechanismSpec.Follows.IsSet())
	{
		return FMuseeUse::Unavailable(Verb(bOpening), Caption, WhyBusy());
	}
	return FMuseeUse::Available(Verb(bOpening), Caption);
}

void UMotionMechanism::Use()
{
	if (IsRunning() || !BodyComponent || MechanismSpec.Follows.IsSet())
	{
		return;
	}
	if (IsOpening())
	{
		Start(MechanismSpec.Open, MechanismSpec.Close);
	}
	else
	{
		Start(MechanismSpec.Close, MechanismSpec.Open);
	}
}

FMuseeUse UMotionMechanism::UsageAt(const FMuseeKey& UseAnchor) const
{
	if (UseAnchor == MechanismSpec.Control)
	{
		return Usage();
	}
	const FMotionRide* Ride = nullptr;
	return RideUsage(UseAnchor, Ride);
}

void UMotionMechanism::UseAt(const FMuseeKey& UseAnchor)
{
	if (UseAnchor == MechanismSpec.Control)
	{
		Use();
		return;
	}
	const FMotionRide* Ride = nullptr;
	if (BodyComponent && RideUsage(UseAnchor, Ride).bAvailable && Ride)
	{
		Start(Ride->Sequence, {});
	}
}

FMuseeUse UMotionMechanism::RideUsage(const FMuseeKey& UseAnchor, const FMotionRide*& OutRide) const
{
	OutRide = nullptr;
	// A panel's buttons: the rides at the anchor whose ports are mated. At rest, it offers the first at the body's stop
	// whose channels hold, or names the first there and why it waits; while the body runs it says so.
	const FMotionRide* Waiting = nullptr;
	bool bServes = false;
	for (const FMotionRide& Ride : MechanismSpec.Rides)
	{
		if (!(Ride.Use == UseAnchor) || !IsMated(Ride.Requires))
		{
			continue;
		}
		bServes = true;
		if (IsRunning() || !IsAt(Ride.At))
		{
			continue;
		}
		if (Holds(Ride.Requires))
		{
			OutRide = &Ride;
			return FMuseeUse::Available(Ride.Verb.ToText(), CaptionOf(Ride.Caption));
		}
		Waiting = Waiting ? Waiting : &Ride;
	}
	if (Waiting)
	{
		return FMuseeUse::Unavailable(Waiting->Verb.ToText(), CaptionOf(Waiting->Caption), NSLOCTEXT("Musee", "motion.not-ready", "It is not ready."));
	}
	if (bServes && IsRunning())
	{
		return FMuseeUse::Unavailable(FText::GetEmpty(), FText::GetEmpty(), WhyBusy());
	}
	return FMuseeUse();
}

bool UMotionMechanism::IsMated(const FMotionRequirement& Requirement) const
{
	for (const FMuseeKey& Port : Requirement.Mated)
	{
		if (!MatedPorts.Contains(Port))
		{
			return false;
		}
	}
	return true;
}

bool UMotionMechanism::Holds(const FMotionRequirement& Requirement) const
{
	if (!IsMated(Requirement))
	{
		return false;
	}
	const UMuseeSignals* Signals = GetWorld() ? GetWorld()->GetSubsystem<UMuseeSignals>() : nullptr;
	for (const FMotionChannelCondition& Condition : Requirement.Channels)
	{
		const float Value = Signals ? Signals->Read(Condition.Channel).Get(UnpublishedValue) : UnpublishedValue;
		if (!Condition.Holds(Value))
		{
			return false;
		}
	}
	return true;
}

FText UMotionMechanism::Verb(bool bOpening) const
{
	const FMotionUseText& Text = bOpening ? MechanismSpec.OpenUse : MechanismSpec.CloseUse;
	if (!Text.Verb.Key.IsNone())
	{
		return Text.Verb.ToText();
	}
	const double Rise = FVector::DotProduct(AxisDirection, FVector::UpVector);
	if (MechanismSpec.Joint == Prismatic && FMath::Abs(Rise) >= LiftAxisCosine)
	{
		// The sequence's end above or below where the body rests says whether it raises or lowers.
		const FName End = bOpening ? SequenceEnd(MechanismSpec.Open, Open) : SequenceEnd(MechanismSpec.Close, Closed);
		const double Towards = MechanismSpec.PositionOf(End).Get(bOpening ? 1.0 : 0.0) - DrivePosition;
		const bool bUpward = Towards == 0.0 ? (Rise > 0.0) == bOpening : (Rise > 0.0) == (Towards > 0.0);
		return bUpward ? NSLOCTEXT("Musee", "motion.raise", "Raise") : NSLOCTEXT("Musee", "motion.lower", "Lower");
	}
	return bOpening ? NSLOCTEXT("Musee", "motion.open", "Open") : NSLOCTEXT("Musee", "motion.close", "Close");
}
