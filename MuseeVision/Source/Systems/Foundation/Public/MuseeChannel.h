#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "MuseeChannel.generated.h"

/**
 * A control point, as in a building-automation system: a named point with one value. One system publishes a demand
 * on the point and whatever follows the point acts on it. Light publishes a blind's shade demand on
 * `<opening>.shade` and the Motion blind that follows the point moves to it; a joint publishes its light lock's state
 * on `<port>.lock` and Art's immersive base reads it. The stage that owns both ends declares the key, so no system
 * names another.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeChannel
{
	GENERATED_BODY()

	/** The point's key. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The point's value, in the meaning its publisher declares (a shade demand from 0 open to 1 closed, a lock state). */
	UPROPERTY(meta = (MuseeRequired))
	float Value = 0.0f;
};
