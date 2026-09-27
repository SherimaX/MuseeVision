#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "MotionStep.generated.h"

/**
 * One step of a mechanism's open or close sequence, as the stage's Mechanisms.json writes it. A step does one of two
 * things, as a lift's controller does in turn: it ramps a control point (a channel, such as the line of light round a
 * pond's lip) to a value over a time, or it drives the mechanism's body to one of its named states (a stop, or
 * "open" or "closed", the travel's ends) at the drive's own pace.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionStep
{
	GENERATED_BODY()

	/** The channel this step ramps (`nympheas.pond.rim`), or the none key for a move. */
	UPROPERTY()
	FMuseeKey Channel;

	/** The value the channel reaches, from 0 to 1 in the meaning its publisher declares. */
	UPROPERTY()
	float To = 0.0f;

	/** How long the ramp takes; zero sets the value at once. */
	UPROPERTY(meta = (Units = "Seconds"))
	float Time = 0.0f;

	/** The state a move drives to (a stop, "open" or "closed"), or none for a channel step. */
	UPROPERTY()
	FName Move;

	/** Whether the step drives the body (otherwise it ramps a channel). */
	[[nodiscard]] bool IsMove() const { return !Move.IsNone(); }
};
