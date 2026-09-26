#pragma once

#include "CoreMinimal.h"

#include "MuseeCheckScope.generated.h"

/**
 * Where a check's truth must hold, which tells Studio's runner how often to run it and what to hand it.
 */
UENUM()
enum class EMuseeCheckScope : uint8
{
	/** Once per stage, alone on the stand, over its own made pieces. */
	Stage,
	/** Once per stage in every mechanism configuration; the check takes the FMuseeConfiguration it runs under. */
	Configuration,
	/** Once over the assembled museum: every stage at its mated place, with the joints between them. */
	Museum,
	/** Once for each of Time's reference moments; the check takes the moment. */
	Moment,
	/** Once for each capture made on the stand: specimen sheets, grey cards, path-traced references. */
	Capture,
};
