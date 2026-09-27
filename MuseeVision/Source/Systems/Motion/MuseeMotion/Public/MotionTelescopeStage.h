#pragma once

#include "CoreMinimal.h"

#include "MotionTelescopeStage.generated.h"

/**
 * One moving stage of a telescopic cylinder, as a multi-stage hydraulic ram is built: a tube (or, last, the solid
 * plunger) sliding in the bore of the stage before it, with a stroke shorter than its own length so that part of it
 * always stays in its guide. The fixed barrel the first stage slides in is the stage's own, drawn by the room.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionTelescopeStage
{
	GENERATED_BODY()

	/** The stage's outer radius. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double Radius = 0.0;

	/** The radius of its bore, in which the next stage slides; 0 for the solid plunger. */
	UPROPERTY(meta = (Units = "Meters"))
	double Bore = 0.0;

	/** Its length. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double Length = 0.0;

	/** How far it slides out of the stage before it. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double Stroke = 0.0;
};
