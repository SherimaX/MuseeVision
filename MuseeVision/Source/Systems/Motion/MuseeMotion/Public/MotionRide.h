#pragma once

#include "CoreMinimal.h"
#include "MotionRequirement.h"
#include "MotionStep.h"
#include "MuseeKey.h"
#include "MuseeText.h"

#include "MotionRide.generated.h"

/**
 * A further use of a moving body, offered at one of its stops: a button on a lift car's panel ("Down to the Cube"), or
 * a call on a landing the car serves. While the body rests at the stop and the requirement holds, the control at the
 * Use anchor offers the ride's verb; using it runs the ride's sequence of channel steps and moves.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionRide
{
	GENERATED_BODY()

	/** The Use anchor of the control that offers the ride (`atrium.car.panel`). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Use;

	/** The stop the body must rest at for the ride to be offered. */
	UPROPERTY(meta = (MuseeRequired))
	FName At;

	/** The verb the control offers (`Down to the Cube`). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeText Verb;

	/** What the ride will do, in a sentence; none when the verb says it all. */
	UPROPERTY()
	FMuseeText Caption;

	/** What must hold before the ride is offered. */
	UPROPERTY()
	FMotionRequirement Requires;

	/** The ride's steps, in order: channel ramps and moves to stops. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<FMotionStep> Sequence;
};
