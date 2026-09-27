#pragma once

#include "CoreMinimal.h"
#include "MotionRequirement.h"
#include "MuseeKey.h"

#include "MotionInterlock.generated.h"

/**
 * A safety interlock, as a lift's safety chain holds one: a band of a mechanism's travel, or of a channel's value, that
 * may be entered or crossed only while a condition holds (Mechanisms.json `interlocks`). The Atrium's car passes the
 * collar iris only with the iris open (A-4); the collar iris opens only with the Sphere's neck iris shut (A-5, the
 * airlock). A move or a ramp whose next step would take the guarded thing strictly between Above and Below waits
 * until one of Allows holds; it never runs through. The sequences are written so that no interlock ever holds them:
 * the interlock is the check that they are.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionInterlock
{
	GENERATED_BODY()

	/** The interlock's key (`atrium.a-4`), as the sheet numbers it. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The mechanism whose position (a share of its travel, 0 to 1) is guarded; none when a channel is. */
	UPROPERTY()
	FMuseeKey Mechanism;

	/** The channel whose value (0 to 1) is guarded; none when a mechanism is. */
	UPROPERTY()
	FMuseeKey Channel;

	/** The band's floor, exclusive: 0 guards everything above rest. */
	UPROPERTY()
	double Above = 0.0;

	/** The band's ceiling, exclusive: 1 guards everything below the end. */
	UPROPERTY()
	double Below = 1.0;

	/** The conditions any one of which lets the guarded thing into the band. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<FMotionRequirement> Allows;

	/** The designer's note: the rule in words, and its source. */
	UPROPERTY()
	FString Note;

	/** Whether a value lies in the guarded band. */
	[[nodiscard]] bool Guards(double Value) const { return Value > Above && Value < Below; }
};
