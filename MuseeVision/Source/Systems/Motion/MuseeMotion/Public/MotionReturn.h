#pragma once

#include "CoreMinimal.h"

#include "MotionReturn.generated.h"

/**
 * A machine that goes home by itself when it is left, as a lift's controller parks an idle car: the Atrium's car, left
 * open at the landing with nobody near it for 12 s, runs its close sequence (Mechanisms.json `returns`). It waits while
 * the body rests at From where its open sequence leaves it; the wait restarts whenever the visitor stands within Radius
 * of the joint's axis on the body's floor (from the body's origin up to a standing body's stature).
 */
USTRUCT()
struct MUSEEMOTION_API FMotionReturn
{
	GENERATED_BODY()

	/** The state it is left at (`atrium`). */
	UPROPERTY(meta = (MuseeRequired))
	FName From;

	/** How long it waits with nobody near before it runs its close sequence. */
	UPROPERTY(meta = (MuseeRequired, Units = "Seconds"))
	float After = 0.0f;

	/** How near the joint's axis a visitor keeps it waiting. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	float Radius = 0.0f;

	/** Whether the machine returns by itself. */
	[[nodiscard]] bool IsSet() const { return !From.IsNone(); }
};
