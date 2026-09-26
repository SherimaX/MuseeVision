#pragma once

#include "CoreMinimal.h"
#include "MuseeFrame.h"
#include "MuseeKey.h"

#include "MuseePose.generated.h"

/**
 * Where one moving body stands in one state of its mechanism: the body's key and its frame in the stage's frame. The
 * sun clock's cylinder raised, the Salon's pond lifted, a door open: each is a pose.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseePose
{
	GENERATED_BODY()

	/** The moving body this pose places. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The body's frame in its stage's frame. */
	UPROPERTY()
	FMuseeFrame Frame;
};
