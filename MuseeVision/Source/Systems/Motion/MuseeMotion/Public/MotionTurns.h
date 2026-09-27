#pragma once

#include "CoreMinimal.h"

#include "MotionTurns.generated.h"

/**
 * A column built turn by turn as it rises, as a spiral-band lift column is: steel band unwound from a housing and locked
 * into a rigid tube one turn at a time, so the column has no length at rest and grows from its housing by whole turns
 * of one pitch. Each turn is the same solid, so the column is those turns stacked back from its free end, the last one
 * still forming in the housing.
 */
USTRUCT()
struct MUSEEMOTION_API FMotionTurns
{
	GENERATED_BODY()

	/** The column's outer radius. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double Radius = 0.0;

	/** The radius of its bore. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double Bore = 0.0;

	/** The height of one turn along the column. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double Pitch = 0.0;

	/** Whether the column is built of turns. */
	[[nodiscard]] bool IsSet() const { return Pitch > 0.0; }
};
