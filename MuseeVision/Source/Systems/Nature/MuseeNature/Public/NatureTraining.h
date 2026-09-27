#pragma once

#include "CoreMinimal.h"

#include "NatureTraining.generated.h"

/**
 * One path a climber is trained along, as a gardener ties it in: points [x, y, z] in the stage's frame, in metres. A
 * vine's first path is its stem, from the soil up its support; each further path is a cane tied along a bar or wire,
 * from where it leaves the stem's cordon.
 */
USTRUCT()
struct MUSEENATURE_API FNatureTraining
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FVector> Path;
};
