#include "MotionMechanism.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MotionGuard.h"
#include "MotionProfile.h"
#include "MuseeRegistry.h"
#include "MuseeSignals.h"
#include "MuseeUnits.h"

const FName UMotionMechanism::Closed(TEXT("closed"));
const FName UMotionMechanism::Open(TEXT("open"));

namespace
{
	/** A joint's axis within this angle of the vertical is a lift, whose verbs are Raise and Lower: 30 degrees, the
	 * steepest an inclined lift runs (EN 81-22 covers inclined lifts from 15 to 75 degrees from the horizontal). */
	constexpr double LiftAxisCosine = 0.866;

	const FName Prismatic(TEXT("prismatic"));
	const FName Revolute(TEXT("revolute"));
	const FName NoGuard(TEXT("none"));
	const FName ReversingGuard(TEXT("body-in-travel-reverse"));

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
	DrivePosition = 0.0;
	if (!BodyComponent)
	{
		return;
	}
	Home = BodyComponent->GetRelativeTransform();
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

FName UMotionMechanism::State() const
{
	if (DrivePosition <= 0.0)
	{
		return Closed;
	}
	return DrivePosition >= 1.0 ? Open : NAME_None;
}

void UMotionMechanism::MoveTo(FName InState)
{
	Start({ MoveStep(InState) }, {});
}

FMuseeUse UMotionMechanism::Usage() const
{
	const bool bOpening = DrivePosition < 0.5;
	const FMotionUseText& Text = bOpening ? MechanismSpec.OpenUse : MechanismSpec.CloseUse;
	const FText Caption = Text.Caption.Key.IsNone() ? FText::GetEmpty() : Text.Caption.ToText();
	if (IsRunning())
	{
		const FText WhyNot = bHeld
			? NSLOCTEXT("Musee", "motion.held", "Something is in its way.")
			: NSLOCTEXT("Musee", "motion.moving", "It is moving.");
		return FMuseeUse::Unavailable(Verb(bOpening), Caption, WhyNot);
	}
	return FMuseeUse::Available(Verb(bOpening), Caption);
}

void UMotionMechanism::Use()
{
	if (IsRunning() || !BodyComponent)
	{
		return;
	}
	if (DrivePosition < 0.5)
	{
		Start(MechanismSpec.Open, MechanismSpec.Close);
	}
	else
	{
		Start(MechanismSpec.Close, MechanismSpec.Open);
	}
}

void UMotionMechanism::Start(const TArray<FMotionStep>& Steps, const TArray<FMotionStep>& Reverse)
{
	FRun Run;
	Run.Steps = Steps;
	Run.Reverse = MechanismSpec.Guard == ReversingGuard ? Reverse : TArray<FMotionStep>();
	Running = MoveTemp(Run);
	bHeld = false;
}

void UMotionMechanism::Advance(float Seconds)
{
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
		if (!Leftover.IsSet())
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
	UMuseeSignals* Signals = GetWorld() ? GetWorld()->GetSubsystem<UMuseeSignals>() : nullptr;
	if (!Running->bStarted)
	{
		Running->bStarted = true;
		Running->Elapsed = 0.0f;
		Running->FromValue = Signals ? Signals->Read(Step.Channel).Get(0.0f) : 0.0f;
	}
	Running->Elapsed += Seconds;
	const float Share = Step.Time > 0.0f ? FMath::Min(1.0f, Running->Elapsed / Step.Time) : 1.0f;
	if (Signals)
	{
		FMuseeChannel Point;
		Point.Key = Step.Channel;
		Point.Value = FMath::Lerp(Running->FromValue, Step.To, Share);
		Signals->Publish(Point);
	}
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
		Running->ToPosition = StatePosition(Step.Move);
	}
	const double Span = Running->ToPosition - Running->FromPosition;
	const double Duration = FullTravelSeconds() * FMath::Abs(Span);
	if (Duration <= 0.0)
	{
		SetPosition(Running->ToPosition);
		Moved.Broadcast(MechanismSpec.Key, Step.Move);
		return Seconds;
	}
	const Musee::Motion::EProfile Profile = Musee::Motion::ParseProfile(MechanismSpec.Profile).Get(Musee::Motion::EProfile::Linear);
	const double Before = Running->Elapsed;
	const double Elapsed = FMath::Min(Duration, Before + Seconds);
	const double Share = Elapsed / Duration;
	const double Next = Running->FromPosition + Span * Musee::Motion::Covered(Profile, Share);
	if (IsGuarded(Next))
	{
		if (!bHeld)
		{
			PublishActuation(0.0);
		}
		bHeld = true;
		if (Running->Reverse.Num() > 0)
		{
			// A door turns back to the state it left: its other sequence runs from here, and holds if it too is blocked.
			Start(Running->Reverse, {});
		}
		return {};
	}
	bHeld = false;
	Running->Elapsed = static_cast<float>(Elapsed);
	SetPosition(Next);
	const double PositionPerSecond = Span / Duration * Musee::Motion::RelativeSpeed(Profile, Share);
	if (Share < 1.0)
	{
		PublishActuation(DrivenPointSpeed(PositionPerSecond));
		return {};
	}
	PublishActuation(0.0);
	Moved.Broadcast(MechanismSpec.Key, Step.Move);
	return static_cast<float>(Before + Seconds - Duration);
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

FTransform UMotionMechanism::PoseAt(double InPosition) const
{
	FTransform Pose = Home;
	if (MechanismSpec.Joint == Revolute)
	{
		const double Angle = FMath::DegreesToRadians(double(MechanismSpec.Travel) * InPosition);
		Pose.SetRotation(FQuat(AxisDirection, Angle) * Home.GetRotation());
	}
	else
	{
		Pose.AddToTranslation(AxisDirection * Musee::ToEngine(Musee::Meters(double(MechanismSpec.Travel) * InPosition)));
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

double UMotionMechanism::StatePosition(FName InState) const
{
	return InState == Open ? 1.0 : 0.0;
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
		return (Rise > 0.0) == bOpening ? NSLOCTEXT("Musee", "motion.raise", "Raise") : NSLOCTEXT("Musee", "motion.lower", "Lower");
	}
	return bOpening ? NSLOCTEXT("Musee", "motion.open", "Open") : NSLOCTEXT("Musee", "motion.close", "Close");
}

void UMotionMechanism::BeginPlay()
{
	Super::BeginPlay();
	if (UMuseeRegistry* Registry = GetWorld() ? GetWorld()->GetSubsystem<UMuseeRegistry>() : nullptr)
	{
		Registry->Register(MechanismSpec.Body, BodyComponent);
	}
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
