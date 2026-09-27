#pragma once

#include "CoreMinimal.h"
#include "MotionChannelSpec.h"
#include "MotionExtentSpec.h"
#include "MotionInterlock.h"
#include "MotionMechanismSpec.h"
#include "MotionPerformanceSpec.h"
#include "MuseeKey.h"
#include "Templates/ValueOrError.h"

#include "MotionSchedule.generated.h"

/**
 * A stage's mechanism schedule, `Data/Stages/<stage>/Mechanisms.json`: every machine the stage carries, the channels it
 * derives from theirs, the timed pieces its visitors start, the interlocks that keep its machines safe together, and
 * the pieces whose length its machines change.
 */
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

	UPROPERTY()
	TArray<FMotionChannelSpec> Channels;

	UPROPERTY()
	TArray<FMotionPerformanceSpec> Performances;

	UPROPERTY()
	TArray<FMotionInterlock> Interlocks;

	UPROPERTY()
	TArray<FMotionExtentSpec> Extents;

	/**
	 * Reads a schedule (a path as the record reader takes it) and gives every text written as a plain string its key:
	 * a mechanism's under the mechanism's key, a performance's under the performance's.
	 */
	[[nodiscard]] static TValueOrError<FMotionSchedule, FString> Read(const FString& Path);

	/** The mechanism with a key, or null. */
	[[nodiscard]] const FMotionMechanismSpec* FindMechanism(const FMuseeKey& Key) const;
};
