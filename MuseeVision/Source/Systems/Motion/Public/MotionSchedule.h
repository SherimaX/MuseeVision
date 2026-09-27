#pragma once

#include "CoreMinimal.h"
#include "MotionMechanismSpec.h"
#include "MuseeKey.h"

#include "MotionSchedule.generated.h"

/** A stage's mechanism schedule, `Data/Stages/<stage>/Mechanisms.json`: every machine the stage carries. */
USTRUCT()
struct MUSEEMOTION_API FMotionSchedule
{
	GENERATED_BODY()

	/** The file every stage keeps its mechanisms in, inside its schedule folder. */
	static constexpr const TCHAR* FileName = TEXT("Mechanisms.json");

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Stage;

	UPROPERTY()
	TArray<FMotionMechanismSpec> Mechanisms;
};
