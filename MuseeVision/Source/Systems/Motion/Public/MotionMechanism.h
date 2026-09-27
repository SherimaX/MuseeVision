#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "MotionMechanismSpec.h"
#include "MuseeUse.h"

#include "MotionMechanism.generated.h"

class USceneComponent;

/**
 * A machine in the museum: a drive that moves one rigid body (a lily pond on its rams, a door on its hinges) along one
 * joint between two named states, `closed` (where the body was made) and `open` (a full travel away). A control's use
 * runs the open or the close sequence of its spec: channel ramps and moves, in order. While the body travels, its
 * actuation is published for Sound, and before each step of travel the guard sweeps the slice the body is about to
 * enter for the visitor's body: the drive holds (a lift) or turns back (a door) rather than move into anyone.
 *
 * Motion is kinematic and deterministic: the body's pose is a function of the drive's position, never of physics.
 * The dresser places one per mechanism on the stage actor at making time, with the body's made pose as its home.
 */
UCLASS(ClassGroup = Musee)
class MUSEEMOTION_API UMotionMechanism : public UActorComponent
{
	GENERATED_BODY()

public:
	/** The body's state where it was made. */
	static const FName Closed;
	/** The body a full travel from where it was made. */
	static const FName Open;

	/** Heard when the body reaches a named state. */
	using FMoved = TMulticastDelegate<void(const FMuseeKey& Mechanism, FName State)>;

	UMotionMechanism();

	/** Sets the mechanism up to drive Body from its present pose, which becomes the closed state. Making time only. */
	void Configure(const FMotionMechanismSpec& InSpec, USceneComponent* InBody);

	[[nodiscard]] const FMotionMechanismSpec& Spec() const { return MechanismSpec; }
	[[nodiscard]] USceneComponent* Body() const { return BodyComponent; }

	/** The named state the body rests in, or none while it is between states. */
	[[nodiscard]] FName State() const;

	/** Where the drive is, from 0 (closed) to 1 (open). */
	[[nodiscard]] double Position() const { return DrivePosition; }

	/** Whether a sequence is running. */
	[[nodiscard]] bool IsRunning() const { return Running.IsSet(); }

	/** Whether the guard is holding the drive because the visitor is in the body's way. */
	[[nodiscard]] bool IsHeld() const { return bHeld; }

	/** Drives the body to a named state at the drive's own pace, without the sequence's channel steps. */
	void MoveTo(FName State);

	/** How the mechanism's control can be used now: open when closed, close when open, nothing while it runs. */
	[[nodiscard]] FMuseeUse Usage() const;

	/** The control's use: runs the open sequence from closed, the close sequence from open. */
	void Use();

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
		/** The sequence a reversing guard runs instead when it turns the body back. */
		TArray<FMotionStep> Reverse;
	};

	void Start(const TArray<FMotionStep>& Steps, const TArray<FMotionStep>& Reverse);
	/** Runs the current step for up to Seconds: the seconds left over when it finishes, or nothing while it goes on. */
	TOptional<float> RunChannelStep(const FMotionStep& Step, float Seconds);
	TOptional<float> RunMoveStep(float Seconds);
	/** Whether the guard stops the body from moving on to a drive position. */
	[[nodiscard]] bool IsGuarded(double NextPosition) const;
	/** The body's pose relative to its parent at a drive position. */
	[[nodiscard]] FTransform PoseAt(double InPosition) const;
	void SetPosition(double InPosition);
	void PublishActuation(double SpeedMetersPerSecond) const;
	[[nodiscard]] double StatePosition(FName State) const;
	[[nodiscard]] double FullTravelSeconds() const;
	[[nodiscard]] double DrivenPointSpeed(double PositionPerSecond) const;
	[[nodiscard]] FText Verb(bool bOpening) const;

	UPROPERTY()
	FMotionMechanismSpec MechanismSpec;

	UPROPERTY()
	TObjectPtr<USceneComponent> BodyComponent;

	/** The body's pose relative to its parent in the closed state: the pose it was made in. */
	UPROPERTY()
	FTransform Home;

	/** The joint's axis in the body's parent's frame, normalised. */
	UPROPERTY()
	FVector AxisDirection = FVector::UpVector;

	/**
	 * The distance from a revolute joint's axis to the farthest point of the body (a door's leading edge), in metres:
	 * the point whose speed the actuation reports. Zero for a prismatic joint.
	 */
	UPROPERTY()
	float DrivenRadiusMeters = 0.0f;

	double DrivePosition = 0.0;
	TOptional<FRun> Running;
	bool bHeld = false;
	FMoved Moved;
};
