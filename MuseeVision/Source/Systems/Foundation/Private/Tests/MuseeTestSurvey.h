#pragma once

#include "CoreMinimal.h"
#include "MuseeText.h"
#include "MuseeKey.h"

#include "MuseeTestSurvey.generated.h"

/**
 * A record the reader's tests read vectors into: a surveyed point, a size, a plan outline and a path, as a stage's
 * schedules write them (`[x, y, z]`, `[x, y]`, `[[x, y], ...]`), and ranges with an open end and a key that names nothing,
 * as a lighting schedule and a planting plan write them (`[50, null]`, `""`).
 */
USTRUCT()
struct FMuseeTestSurvey
{
	GENERATED_BODY()

	UPROPERTY(meta = (Units = "Meters"))
	FVector PointMeters = FVector::ZeroVector;

	UPROPERTY(meta = (Units = "Meters"))
	FVector2D SizeMeters = FVector2D::ZeroVector;

	UPROPERTY()
	TArray<FVector2D> Outline;

	UPROPERTY()
	TArray<FVector> Path;

	UPROPERTY()
	TMap<FName, FVector2D> Marks;

	/** The survey's labels: texts written as a plain string and as a key with its source. */
	UPROPERTY()
	TArray<FMuseeText> Labels;

	/** A target's range, open above: at least its least. */
	UPROPERTY(meta = (MuseeRange))
	TArray<double> Lux;

	/** A ratio's range, open below: at most its most. */
	UPROPERTY(meta = (MuseeRange))
	TArray<float> Ratio;

	/** A body the survey rides, or none. */
	UPROPERTY()
	FMuseeKey Body;
};
