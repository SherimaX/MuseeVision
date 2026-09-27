#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "MotionMechanismSpec.h"
#include "MotionSCurve.h"
#include "MuseeUse.h"

#include "MotionMechanism.generated.h"

class USceneComponent;

/**
 * A machine in the museum: a drive that moves one rigid body (a lily pond on its rams, a door on its hinges, a lift's
 * car in its well) along one joint between named states: `closed` and `open` at the travel's ends, and the stops a lift
 * names between. A control's use runs the open or the close sequence of its spec, and a ride offered at a stop runs its
 * own: channel ramps and moves, in order. While the body travels, its actuation is published for Sound, and before each
 * step of travel the guard sweeps the slice the body is about to enter for the visitor's body: the drive holds (a lift)
 * or turns back (a door) rather than move into anyone.
 *
 * Motion is kinematic and deterministic: the body's pose is a function of the drive's position, never of physics.
 * The dresser places one per mechanism on the stage actor at making time, with the body's drawn pose as its reference.
 */
UCLASS(ClassGroup = Musee)
class MUSEEMOTION_API UMotionMechanism : public UActorComponent
{
	GENERATED_BODY()

public:
	/** The travel's start. */
	static const FName Closed;
	/** The travel's end, a full travel from its start. */
	static const FName Open;

	/** Heard when the body reaches a named state. */
	using FMoved = TMulticastDelegate<void(const FMuseeKey& Mechanism, FName State)>;

	UMotionMechanism();

	/** Sets the mechanism up to drive Body from its present pose, the pose its spec says it is drawn in. Making time only. */
	void Configure(const FMotionMechanismSpec& InSpec, USceneComponent* InBody);

	/** The ports the site plan mates, which rides require. None are mated until they are given. Making time only. */
	void SetMatedPorts(const TArray<FMuseeKey>& Ports);

	[[nodiscard]] const FMotionMechanismSpec& Spec() const { return MechanismSpec; }
	[[nodiscard]] USceneComponent* Body() const { return BodyComponent; }

	/** The named state the body rests in (a stop first, then closed or open), or none while it is between states. */
	[[nodiscard]] FName State() const;

	/** Whether the body rests at a named state. */
	[[nodiscard]] bool IsAt(FName State) const;

	/** Where the drive is, from 0 (closed) to 1 (open). */
	[[nodiscard]] double Position() const { return DrivePosition; }

	/** Whether a sequence is running. */
	[[nodiscard]] bool IsRunning() const { return Running.IsSet(); }

	/** Whether the guard is holding the drive because the visitor is in the body's way. */
	[[nodiscard]] bool IsHeld() const { return bHeld; }

	/** Sets the body at its home at once, as the machine stands when the museum opens. BeginPlay does this. */
	void StandAtHome();

	/** Drives the body to a named state at the drive's own pace, without the sequence's channel steps. */
	void MoveTo(FName State);

	/**
	 * How the mechanism's own control can be used now: its open sequence from anywhere but the state that sequence ends
	 * in, its close sequence from there; nothing while it runs or follows.
	 */
	[[nodiscard]] FMuseeUse Usage() const;

	/** The own control's use: runs the sequence its usage offers. */
	void Use();

	/**
	 * How the control at a Use anchor can be used now: the mechanism's own control, or else the first of its rides at
	 * that anchor whose ports are mated, offered while the body rests at the ride's stop and its channels hold. With the
	 * body at another stop the anchor offers nothing; while it runs, it says so.
	 */
	[[nodiscard]] FMuseeUse UsageAt(const FMuseeKey& UseAnchor) const;

	/** The use of the control at a Use anchor: runs what its usage offers, if it is available. */
	void UseAt(const FMuseeKey& UseAnchor);

	/** Whether a requirement holds now: every port it names is mated and every channel lies within its bounds. */
	[[nodiscard]] bool Holds(const FMotionRequirement& Requirement) const;

	/** Runs the mechanism for a span of time: its sequence, its drive, its guard and its signals. */
	void Advance(float Seconds);

	/** Heard when the body reaches a named state. */
	FMoved& OnMoved() { return Moved; }

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** A running sequence: its steps, the one under way, and how that step began. */
	struct FRun
	{
		TArray<FMotionStep> Steps;
		int32 Index = 0;
		float Elapsed = 0.0f;
		bool bStarted = false;
		float FromValue = 0.0f;
		double FromPosition = 0.0;
		double ToPosition = 0.0;
		/** The drive's run for a move under a drive. */
		Musee::Motion::FSCurve Curve;
		/** The sequence a reversing guard runs instead when it turns the body back. */
		TArray<FMotionStep> Reverse;
	};

	/** Where a move is a time into it: the drive's position and its rate (shares of the travel a second). */
	struct FMoveSample
	{
		double Position = 0.0;
		double Rate = 0.0;
	};

	void Start(const TArray<FMotionStep>& Steps, const TArray<FMotionStep>& Reverse);
	/** Follows the channel the spec names: moves straight to the travel it asks for, unless the guard forbids it. */
	void Follow(float Seconds);
	/** Runs the current step for up to Seconds: the seconds left over when it finishes, or nothing while it goes on. */
	TOptional<float> RunChannelStep(const FMotionStep& Step, float Seconds);
	TOptional<float> RunMoveStep(float Seconds);
	/** How long the current move takes, and where it is a time into it. */
	[[nodiscard]] double MoveSeconds() const;
	[[nodiscard]] FMoveSample SampleMove(double Seconds, double Duration) const;
	/** Whether the guard stops the body from moving on to a drive position. */
	[[nodiscard]] bool IsGuarded(double NextPosition) const;
	/** The body's pose relative to its parent at a drive position. */
	[[nodiscard]] FTransform PoseAt(double InPosition) const;
	void SetPosition(double InPosition);
	void PublishActuation(double SpeedMetersPerSecond) const;
	[[nodiscard]] double FullTravelSeconds() const;
	[[nodiscard]] double DrivenPointSpeed(double PositionPerSecond) const;
	/** The state a sequence leaves the body in: its last move's, or Otherwise when it has none. */
	[[nodiscard]] FName SequenceEnd(const TArray<FMotionStep>& Steps, FName Otherwise) const;
	/** Whether the own control's use now runs the open sequence. */
	[[nodiscard]] bool IsOpening() const;
	[[nodiscard]] FText Verb(bool bOpening) const;
	/** Why nothing can be used while the mechanism runs: it is moving, or held. */
	[[nodiscard]] FText WhyBusy() const;
	/** The usage of the rides at a Use anchor, and the ride it would run (null when none is available). */
	[[nodiscard]] FMuseeUse RideUsage(const FMuseeKey& UseAnchor, const FMotionRide*& OutRide) const;
	[[nodiscard]] bool IsMated(const FMotionRequirement& Requirement) const;

	UPROPERTY()
	FMotionMechanismSpec MechanismSpec;

	UPROPERTY()
	TObjectPtr<USceneComponent> BodyComponent;

	/** The body's pose relative to its parent where the stage draws it. */
	UPROPERTY()
	FTransform Drawn;

	/** The joint's axis in the body's parent's frame, normalised. */
	UPROPERTY()
	FVector AxisDirection = FVector::UpVector;

	/**
	 * The distance from a revolute joint's axis to the farthest point of the body (a door's leading edge), in metres:
	 * the point whose speed the actuation reports. Zero for a prismatic joint.
	 */
	UPROPERTY()
	float DrivenRadiusMeters = 0.0f;

	/** The ports the site plan mates. */
	UPROPERTY()
	TArray<FMuseeKey> MatedPorts;

	/** Where the drive is; saved with the level, so the body and its drive agree where it is drawn. */
	UPROPERTY()
	double DrivePosition = 0.0;

	TOptional<FRun> Running;
	bool bHeld = false;
	FMoved Moved;
};
