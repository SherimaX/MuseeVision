#pragma once

#include "CoreMinimal.h"
#include "MotionRequirement.h"
#include "MuseeKey.h"
#include "MuseeText.h"

#include "MotionPerformanceSpec.generated.h"

/**
 * A timed piece a visitor starts, as a projectionist starts a reel: the Cube's Sea Light, eight minutes long, offered
 * at the Cube's performance control only when the room is sealed (Mechanisms.json `performances`). While it runs it
 * publishes its channel at 1, and otherwise 0 (the stage contract). Its fades are its image's: the fade-in opens its
 * seconds and the fade-out closes them, and the same control ends it early, after its fade-out plays out.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionPerformanceSpec
{
	GENERATED_BODY()

	/** The piece's key (`cube.sea-light`); its texts are keyed under it. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The Use anchor of the control that starts and ends it. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Use;

	/** How long it runs from start to end, fades included. */
	UPROPERTY(meta = (MuseeRequired, Units = "Seconds"))
	float Seconds = 0.0f;

	/** The channel it publishes: 1 while it runs, 0 otherwise. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Publishes;

	/** What must hold before it is offered: the same shape as a ride's. */
	UPROPERTY()
	FMotionRequirement Requires;

	/** The verb the control offers (`II · Sea Light, 8 min`). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeText Verb;

	/** What the piece is, in a sentence or two; none when the verb says it all. */
	UPROPERTY()
	FMuseeText Caption;

	/** How long its image takes to come up at the start. */
	UPROPERTY(meta = (Units = "Seconds"))
	float FadeIn = 0.0f;

	/** How long its image takes to go down at the end, which an early end plays out too. */
	UPROPERTY(meta = (Units = "Seconds"))
	float FadeOut = 0.0f;

	/** The designer's note. */
	UPROPERTY()
	FString Note;
};
