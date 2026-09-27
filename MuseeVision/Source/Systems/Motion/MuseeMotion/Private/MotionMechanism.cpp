#include "MotionMechanism.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MotionConditions.h"
#include "MotionGuard.h"
#include "MotionProfile.h"
#include "MuseeRegistry.h"
#include "MuseeSignals.h"
#include "MuseeUnits.h"

const FName UMotionMechanism::Closed(TEXT("closed"));
const FName UMotionMechanism::Open(TEXT("open"));

namespace
{
	const FName Revolute(TEXT("revolute"));
	const FName NoGuard(TEXT("none"));
	const FName ReversingGuard(TEXT("body-in-travel-reverse"));

	/**
	 * Two drive positions are the same state within this share of the travel. A move lands exactly on its state, so it
	 * only absorbs rounding: a billionth of the Élan car's 44 m is 44 nm.
	 */
	constexpr double SameShare = 1.0e-9;

	FMotionStep MoveStep(FName State)
	{
		FMotionStep Step;
		Step.Move = State;
		return Step;
	}
}

UMotionMechanism::UMotionMechanism()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMotionMechanism::Configure(const FMotionMechanismSpec& InSpec, USceneComponent* InBody)
{
	MechanismSpec = InSpec;
	BodyComponent = InBody;
	DrivePosition = MechanismSpec.DrawnPosition();
	if (!BodyComponent)
	{
		return;
	}
	Drawn = BodyComponent->GetRelativeTransform();
	const FVector StageAxis = MechanismSpec.Axis.Num() == 3 ? FVector(MechanismSpec.Axis[0], MechanismSpec.Axis[1], MechanismSpec.Axis[2]) : FVector::UpVector;
	const FTransform Stage = GetOwner() ? GetOwner()->GetActorTransform() : FTransform::Identity;
	const USceneComponent* Parent = BodyComponent->GetAttachParent();
	const FTransform ParentPose = Parent ? Parent->GetComponentTransform() : FTransform::Identity;
	AxisDirection = ParentPose.InverseTransformVectorNoScale(Stage.TransformVectorNoScale(StageAxis.GetSafeNormal()));

	// A moving body and everything it carries move: the pieces, the water, the plants and the controls on it.
	BodyComponent->SetMobility(EComponentMobility::Movable);
	TArray<USceneComponent*> Carried;
	BodyComponent->GetChildrenComponents(true, Carried);
	for (USceneComponent* Part : Carried)
	{
		Part->SetMobility(EComponentMobility::Movable);
	}
	DrivenRadiusMeters = MechanismSpec.Joint == Revolute
		? static_cast<float>(Musee::Motion::ReachFromAxis(*BodyComponent, BodyComponent->GetComponentLocation(), ParentPose.TransformVectorNoScale(AxisDirection)))
		: 0.0f;
}

void UMotionMechanism::SetMatedPorts(const TArray<FMuseeKey>& Ports)
{
	MatedPorts = Ports;
}

void UMotionMechanism::SetInterlocks(const TArray<FMotionInterlock>& InInterlocks)
{
	// Its own interlocks, and every channel's: any sequence of its may ramp any channel.
	Interlocks = InInterlocks.FilterByPredicate([this](const FMotionInterlock& Interlock)
	{
		return Interlock.Mechanism == MechanismSpec.Key || !Interlock.Channel.IsNone();
	});
}

FName UMotionMechanism::State() const
{
	for (const FMotionStop& Stop : MechanismSpec.Stops)
	{
		if (FMath::IsNearlyEqual(DrivePosition, Stop.At, SameShare))
		{
			return Stop.Stop;
		}
	}
	if (DrivePosition <= SameShare)
	{
		return Closed;
	}
	return DrivePosition >= 1.0 - SameShare ? Open : NAME_None;
}

bool UMotionMechanism::IsAt(FName InState) const
{
	const TOptional<double> At = MechanismSpec.PositionOf(InState);
	return At.IsSet() && FMath::IsNearlyEqual(DrivePosition, At.GetValue(), SameShare);
}

void UMotionMechanism::StandAtStart()
{
	Running.Reset();
	Waiting = EWait::None;
	HoldingInterlock = FMuseeKey();
	LeftSeconds = 0.0f;
	SetPosition(MechanismSpec.StartPosition());
}

void UMotionMechanism::MoveTo(FName InState)
{
	Start({ MoveStep(InState) }, {});
}

void UMotionMechanism::Start(const TArray<FMotionStep>& Steps, const TArray<FMotionStep>& Reverse)
{
	FRun Run;
	Run.Steps = Steps;
	Run.Reverse = MechanismSpec.Guard == ReversingGuard ? Reverse : TArray<FMotionStep>();
	Running = MoveTemp(Run);
	Waiting = EWait::None;
	HoldingInterlock = FMuseeKey();
	LeftSeconds = 0.0f;
}

void UMotionMechanism::Advance(float Seconds)
{
	if (MechanismSpec.Follows.IsSet())
	{
		Follow(Seconds);
		return;
	}
	if (!Running.IsSet())
	{
		WaitToReturn(Seconds);
		return;
	}
	if (Running->bAfterGroup)
	{
		if (!IsGroupClosed())
		{
			Waiting = EWait::Group;
			return;
		}
		Running->bAfterGroup = false;
		Waiting = EWait::None;
	}
	float Left = Seconds;
	// Each pass either finishes a step (and the next begins with the time left over) or ends the advance, so steps
	// that take no time (a channel set at once) run in the same advance as the step before them.
	while (Running.IsSet())
	{
		if (!Running->Steps.IsValidIndex(Running->Index))
		{
			Running.Reset();
			break;
		}
		const FMotionStep Step = Running->Steps[Running->Index];
		const TOptional<float> Leftover = Step.IsMove() ? RunMoveStep(Left) : RunChannelStep(Step, Left);
		if (!Leftover.IsSet() || !Running.IsSet())
		{
			break;
		}
		++Running->Index;
		Running->bStarted = false;
		Left = Leftover.GetValue();
	}
}

TOptional<float> UMotionMechanism::RunChannelStep(const FMotionStep& Step, float Seconds)
{
	if (!Running->bStarted)
	{
		Running->bStarted = true;
		Running->Elapsed = 0.0f;
		Running->FromValue = Musee::Motion::ChannelValue(GetWorld(), Step.Channel);
	}
	const float Elapsed = Running->Elapsed + Seconds;
	const float Share = Step.Time > 0.0f ? FMath::Min(1.0f, Elapsed / Step.Time) : 1.0f;
	// A ramp that ends lands on its value exactly, as a limit switch reads a door fully open.
	const float Value = Share < 1.0f ? FMath::Lerp(Running->FromValue, Step.To, Share) : Step.To;
	if (const FMotionInterlock* Forbids = Forbidding(FMuseeKey(), Step.Channel, Value))
	{
		Waiting = EWait::Interlock;
		HoldingInterlock = Forbids->Key;
		return {};
	}
	Waiting = EWait::None;
	Running->Elapsed = Elapsed;
	Musee::Motion::PublishChannel(GetWorld(), Step.Channel, Value);
	if (Share < 1.0f)
	{
		return {};
	}
	return FMath::Max(0.0f, Running->Elapsed - Step.Time);
}

TOptional<float> UMotionMechanism::RunMoveStep(float Seconds)
{
	const FMotionStep Step = Running->Steps[Running->Index];
	if (!Running->bStarted)
	{
		Running->bStarted = true;
		Running->Elapsed = 0.0f;
		Running->FromPosition = DrivePosition;
		Running->ToPosition = MechanismSpec.PositionOf(Step.Move).Get(DrivePosition);
		Running->Curve = MechanismSpec.Drive.IsSet()
			? MechanismSpec.Drive.Run(Running->FromPosition, Running->ToPosition, MechanismSpec.Travel)
			: Musee::Motion::FSCurve();
	}
	const double Duration = MoveSeconds();
	if (Duration <= 0.0)
	{
		SetPosition(Running->ToPosition);
		Moved.Broadcast(MechanismSpec.Key, Step.Move);
		return Seconds;
	}
	const double Before = Running->Elapsed;
	const double Elapsed = FMath::Min(Duration, Before + Seconds);
	const FMoveSample Next = SampleMove(Elapsed, Duration);
	const FMotionInterlock* Forbids = Forbidding(MechanismSpec.Key, FMuseeKey(), Next.Position);
	if (Forbids || IsGuarded(Next.Position))
	{
		if (Waiting == EWait::None)
		{
			PublishActuation(0.0);
		}
		Waiting = Forbids ? EWait::Interlock : EWait::Visitor;
		HoldingInterlock = Forbids ? Forbids->Key : FMuseeKey();
		if (!Forbids && Running->Reverse.Num() > 0)
		{
			// A door turns back to the state it left: its other sequence runs from here, and holds if it too is blocked.
			Start(Running->Reverse, {});
		}
		return {};
	}
	Waiting = EWait::None;
	Running->Elapsed = static_cast<float>(Elapsed);
	if (Elapsed < Duration)
	{
		SetPosition(Next.Position);
		PublishActuation(DrivenPointSpeed(Next.Rate));
		return {};
	}
	SetPosition(Running->ToPosition);
	PublishActuation(0.0);
	Moved.Broadcast(MechanismSpec.Key, Step.Move);
	return static_cast<float>(Before + Seconds - Duration);
}

double UMotionMechanism::MoveSeconds() const
{
	if (MechanismSpec.Drive.IsSet())
	{
		return Running->Curve.Duration();
	}
	return FullTravelSeconds() * FMath::Abs(Running->ToPosition - Running->FromPosition);
}

UMotionMechanism::FMoveSample UMotionMechanism::SampleMove(double Seconds, double Duration) const
{
	const double Span = Running->ToPosition - Running->FromPosition;
	FMoveSample Sample;
	if (MechanismSpec.Drive.IsSet())
	{
		const Musee::Motion::FSCurve::FSample Run = Running->Curve.At(Seconds);
		const double Direction = Span < 0.0 ? -1.0 : 1.0;
		Sample.Position = Running->FromPosition + Direction * Run.Covered / MechanismSpec.Travel;
		Sample.Rate = Direction * Run.Speed / MechanismSpec.Travel;
		return Sample;
	}
	const Musee::Motion::EProfile Profile = Musee::Motion::ParseProfile(MechanismSpec.Profile).Get(Musee::Motion::EProfile::Linear);
	const double Share = Seconds / Duration;
	Sample.Position = Running->FromPosition + Span * Musee::Motion::Covered(Profile, Share);
	Sample.Rate = Span / Duration * Musee::Motion::RelativeSpeed(Profile, Share);
	return Sample;
}

void UMotionMechanism::Follow(float Seconds)
{
	const UMuseeSignals* Signals = GetWorld() ? GetWorld()->GetSubsystem<UMuseeSignals>() : nullptr;
	const TOptional<float> Value = Signals ? Signals->Read(MechanismSpec.Follows.Channel) : TOptional<float>();
	if (!Value.IsSet() || MechanismSpec.Travel <= 0.0f)
	{
		return;
	}
	const double Target = FMath::Clamp(MechanismSpec.Follows.At(Value.GetValue()) / MechanismSpec.Travel, 0.0, 1.0);
	if (FMath::IsNearlyEqual(Target, DrivePosition))
	{
		return;
	}
	Waiting = IsGuarded(Target) ? EWait::Visitor : EWait::None;
	if (Waiting != EWait::None)
	{
		PublishActuation(0.0);
		return;
	}
	const double Rate = Seconds > 0.0f ? (Target - DrivePosition) / Seconds : 0.0;
	SetPosition(Target);
	PublishActuation(DrivenPointSpeed(Rate));
}

bool UMotionMechanism::IsGuarded(double NextPosition) const
{
	const UWorld* World = GetWorld();
	if (!World || !BodyComponent || MechanismSpec.Guard.IsNone() || MechanismSpec.Guard == NoGuard)
	{
		return false;
	}
	const USceneComponent* Parent = BodyComponent->GetAttachParent();
	const FTransform ParentPose = Parent ? Parent->GetComponentTransform() : FTransform::Identity;
	return Musee::Motion::IsTravelOccupied(*World, *BodyComponent, PoseAt(DrivePosition) * ParentPose, PoseAt(NextPosition) * ParentPose,
		MechanismSpec.Clearance);
}

const FMotionInterlock* UMotionMechanism::Forbidding(const FMuseeKey& Mechanism, const FMuseeKey& Channel, double NextValue) const
{
	for (const FMotionInterlock& Interlock : Interlocks)
	{
		const bool bGuards = Mechanism.IsNone() ? Interlock.Channel == Channel : Interlock.Mechanism == Mechanism;
		if (!bGuards || !Interlock.Guards(NextValue))
		{
			continue;
		}
		const bool bAllowed = Interlock.Allows.ContainsByPredicate([this](const FMotionRequirement& Allowed) { return Holds(Allowed); });
		if (!bAllowed)
		{
			return &Interlock;
		}
	}
	return nullptr;
}

FTransform UMotionMechanism::PoseAt(double InPosition) const
{
	const double FromDrawn = double(MechanismSpec.Travel) * (InPosition - MechanismSpec.DrawnPosition());
	FTransform Pose = Drawn;
	if (MechanismSpec.Joint == Revolute)
	{
		Pose.SetRotation(FQuat(AxisDirection, FMath::DegreesToRadians(FromDrawn)) * Drawn.GetRotation());
	}
	else
	{
		Pose.AddToTranslation(AxisDirection * Musee::ToEngine(Musee::Meters(FromDrawn)));
	}
	return Pose;
}

void UMotionMechanism::SetPosition(double InPosition)
{
	DrivePosition = FMath::Clamp(InPosition, 0.0, 1.0);
	if (BodyComponent)
	{
		BodyComponent->SetRelativeTransform(PoseAt(DrivePosition));
	}
	if (!MechanismSpec.Publishes.IsNone())
	{
		Musee::Motion::PublishChannel(GetWorld(), MechanismSpec.Publishes, static_cast<float>(DrivePosition));
	}
}

void UMotionMechanism::PublishActuation(double SpeedMetersPerSecond) const
{
	UMuseeSignals* Signals = GetWorld() ? GetWorld()->GetSubsystem<UMuseeSignals>() : nullptr;
	if (!Signals)
	{
		return;
	}
	FMuseeActuation Actuation;
	Actuation.Mechanism = MechanismSpec.Key;
	Actuation.SpeedMetersPerSecond = static_cast<float>(SpeedMetersPerSecond);
	Signals->Publish(Actuation);
}

double UMotionMechanism::FullTravelSeconds() const
{
	return FMath::Max(0.0, double(MechanismSpec.Time));
}

double UMotionMechanism::DrivenPointSpeed(double PositionPerSecond) const
{
	const double Rate = FMath::Abs(PositionPerSecond) * MechanismSpec.Travel;
	return MechanismSpec.Joint == Revolute ? FMath::DegreesToRadians(Rate) * DrivenRadiusMeters : Rate;
}

void UMotionMechanism::BeginPlay()
{
	Super::BeginPlay();
	if (UMuseeRegistry* Registry = GetWorld() ? GetWorld()->GetSubsystem<UMuseeRegistry>() : nullptr)
	{
		Registry->Register(MechanismSpec.Body, BodyComponent);
	}
	StandAtStart();
}

void UMotionMechanism::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UMuseeRegistry* Registry = GetWorld() ? GetWorld()->GetSubsystem<UMuseeRegistry>() : nullptr)
	{
		Registry->Unregister(MechanismSpec.Body, BodyComponent);
	}
	Super::EndPlay(Reason);
}

void UMotionMechanism::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}
