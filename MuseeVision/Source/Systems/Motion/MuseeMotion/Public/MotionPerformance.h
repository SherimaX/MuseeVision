#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "MotionPerformanceSpec.h"
#include "MuseeUse.h"

#include "MotionPerformance.generated.h"

/**
 * A timed piece on a stage, run as a show controller runs a cue: the visitor starts it at its control when its
 * requirement holds; it publishes its channel at 1 for as long as it runs; and it ends by itself after its seconds, or
 * earlier when the visitor ends it at the same control, after playing out its fade-out. What the piece shows, and how
 * its image fades, is its work's; Motion keeps its time and its channel, from which the rest of the stage (the lift's
 * rides, the mast's lights) learns that a work performs.
 */
UCLASS(ClassGroup = Musee)
class MUSEEMOTION_API UMotionPerformance : public UActorComponent
{
	GENERATED_BODY()

public:
	UMotionPerformance();

	/** Sets the piece up from its spec. Making time only. */
	void Configure(const FMotionPerformanceSpec& InSpec);

	/** The ports the site plan mates, which its requirement may name. Making time only. */
	void SetMatedPorts(const TArray<FMuseeKey>& Ports);

	[[nodiscard]] const FMotionPerformanceSpec& Spec() const { return PerformanceSpec; }

	/** Whether it is running. */
	[[nodiscard]] bool IsRunning() const { return Clock.IsSet(); }

	/** How long it has run, or 0 when it isn't running. */
	[[nodiscard]] double Seconds() const { return Clock.Get(0.0); }

	/** What its control offers: the piece while its requirement holds, or ending it while it runs. */
	[[nodiscard]] FMuseeUse Usage() const;

	/** Its control's use: starts the piece, or ends it early. */
	void Use();

	/** Sets the piece at rest, as the museum opens: nothing performs. BeginPlay does this. */
	void StandAtStart();

	/** Runs the piece's clock for a span of time and publishes its channel. */
	void Advance(float DeltaSeconds);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** When it ends, on its clock. */
	[[nodiscard]] double EndsAt() const;
	void Publish() const;

	UPROPERTY()
	FMotionPerformanceSpec PerformanceSpec;

	UPROPERTY()
	TArray<FMuseeKey> MatedPorts;

	/** How long it has run; unset at rest. */
	TOptional<double> Clock;
	/** When the visitor ended it early, on its clock; unset unless they did. */
	TOptional<double> EndedAt;
};
