#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MotionConditions.h"
#include "MotionGuard.h"
#include "MotionMechanism.h"
#include "MuseeHumanMeasure.h"

// The mechanism's uses: its own control's open and close, the rides a lift offers at its stops, its group, and its
// return home when it is left.

namespace
{
	/** A joint's axis within this angle of the vertical is a lift, whose verbs are Raise and Lower: 30 degrees, the
	 * steepest an inclined lift runs (EN 81-22 covers inclined lifts from 15 to 75 degrees from the horizontal). */
	constexpr double LiftAxisCosine = 0.866;

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
	switch (Waiting)
	{
	case EWait::Visitor: return NSLOCTEXT("Musee", "motion.held", "Something is in its way.");
	case EWait::Interlock: return NSLOCTEXT("Musee", "motion.interlocked", "It waits until it is safe to move.");
	case EWait::Group: return NSLOCTEXT("Musee", "motion.group", "It waits for the others to close.");
	default: return NSLOCTEXT("Musee", "motion.moving", "It is moving.");
	}
}

FMuseeUse UMotionMechanism::Usage() const
{
	const bool bOpening = IsOpening();
	if (!bOpening && MechanismSpec.Returns.IsSet() && !IsRunning())
	{
		// Left open, it goes home by itself: its control has nothing to offer.
		return FMuseeUse();
	}
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
	if (!Usage().bAvailable || !BodyComponent)
	{
		return;
	}
	if (!IsOpening())
	{
		Start(MechanismSpec.Close, MechanismSpec.Open);
		return;
	}
	const TArray<UMotionMechanism*> Peers = GroupPeers();
	for (UMotionMechanism* Peer : Peers)
	{
		Peer->CloseForGroup();
	}
	Start(MechanismSpec.Open, MechanismSpec.Close);
	Running->bAfterGroup = !Peers.IsEmpty();
}

void UMotionMechanism::CloseForGroup()
{
	if (MechanismSpec.Follows.IsSet() || MechanismSpec.Close.IsEmpty() || (IsAt(Closed) && !IsRunning()))
	{
		return;
	}
	Start(MechanismSpec.Close, MechanismSpec.Open);
}

TArray<UMotionMechanism*> UMotionMechanism::GroupPeers() const
{
	TArray<UMotionMechanism*> Peers;
	if (MechanismSpec.Group.IsNone() || !GetOwner())
	{
		return Peers;
	}
	GetOwner()->GetComponents(Peers);
	return Peers.FilterByPredicate([this](const UMotionMechanism* Peer) { return Peer != this && Peer->Spec().Group == MechanismSpec.Group; });
}

bool UMotionMechanism::IsGroupClosed() const
{
	for (const UMotionMechanism* Peer : GroupPeers())
	{
		if (Peer->IsRunning() || !Peer->IsAt(Closed))
		{
			return false;
		}
	}
	return true;
}

void UMotionMechanism::WaitToReturn(float Seconds)
{
	const FMotionReturn& Return = MechanismSpec.Returns;
	if (!Return.IsSet() || !BodyComponent || !IsAt(Return.From) || IsOpening())
	{
		LeftSeconds = 0.0f;
		return;
	}
	const USceneComponent* Parent = BodyComponent->GetAttachParent();
	const FVector Axis = Parent ? Parent->GetComponentTransform().TransformVectorNoScale(AxisDirection) : AxisDirection;
	const UWorld* World = GetWorld();
	if (World && Musee::Motion::IsVisitorWithin(*World, BodyComponent->GetComponentLocation(), Axis, Return.Radius, FMuseeHumanMeasure::Stature.Get()))
	{
		LeftSeconds = 0.0f;
		return;
	}
	LeftSeconds += Seconds;
	if (LeftSeconds >= Return.After)
	{
		Start(MechanismSpec.Close, {});
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
	const FMotionRide* WaitingRide = nullptr;
	bool bServes = false;
	for (const FMotionRide& Ride : MechanismSpec.Rides)
	{
		if (!(Ride.Use == UseAnchor) || !Musee::Motion::IsMated(MatedPorts, Ride.Requires))
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
		WaitingRide = WaitingRide ? WaitingRide : &Ride;
	}
	if (WaitingRide)
	{
		return FMuseeUse::Unavailable(WaitingRide->Verb.ToText(), CaptionOf(WaitingRide->Caption), NSLOCTEXT("Musee", "motion.not-ready", "It is not ready."));
	}
	if (bServes && IsRunning())
	{
		return FMuseeUse::Unavailable(FText::GetEmpty(), FText::GetEmpty(), WhyBusy());
	}
	return FMuseeUse();
}

bool UMotionMechanism::Holds(const FMotionRequirement& Requirement) const
{
	return Musee::Motion::Holds(GetWorld(), MatedPorts, Requirement);
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
