#pragma once

#include "CoreMinimal.h"

#include "LightingRecords.generated.h"

// A stage's lighting schedule, Data/Stages/<stage>/Lighting.json, as the stage contract writes it (metres, in the
// stage's frame).

USTRUCT()
struct FLightScheduleDaylight
{
	GENERATED_BODY()

	/** The Glazed or Open opening that admits daylight. */
	UPROPERTY(meta = (MuseeRequired)) FString Opening;
	/** Its glazing, a palette key. */
	UPROPERTY() FString Glazing;
	/** Whose weather falls on it: `museum`, `giverny` or `london` (Decision D-005). */
	UPROPERTY(meta = (MuseeRequired)) FString Weather;
	/** Its daylight control from the catalogue, or `none`. */
	UPROPERTY() FString Control;
	UPROPERTY() FString Note;
};

USTRUCT()
struct FLightScheduleLuminaire
{
	GENERATED_BODY()

	/** The luminaire's name within the stage (`rim`). */
	UPROPERTY(meta = (MuseeRequired)) FString Id;
	/** Its catalogue key. */
	UPROPERTY(meta = (MuseeRequired)) FString Type;
	/** The key of the Mount anchor it sits on. */
	UPROPERTY(meta = (MuseeRequired)) FString Mount;
	/** The direction its light leaves along, in the stage's frame; the mount's x axis when absent. */
	UPROPERTY() TArray<double> Aim;
	/** `always`, `dusk` or `presence`. */
	UPROPERTY(meta = (MuseeRequired)) FString Operation;
	/** The channel (0 to 1) that dims it. */
	UPROPERTY() FString Follows;
	/** The moving body it rides on. */
	UPROPERTY() FString Body;
	/** The length laid, for a linear type (a strip, a cove line, a slot). */
	UPROPERTY(meta = (Units = "Meters")) double Length = 0.0;
	UPROPERTY() FString Note;
};

USTRUCT()
struct FLightScheduleTarget
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Surface;
	UPROPERTY() FString Class;
	/** The target range, in lux. */
	UPROPERTY() TArray<double> Lux;
	UPROPERTY() FString Source;
	UPROPERTY() FString Note;
};

USTRUCT()
struct FLightScheduleDesign
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Brighter;
	UPROPERTY(meta = (MuseeRequired)) FString Darker;
	UPROPERTY() TArray<double> Ratio;
	UPROPERTY() FString Note;
};

/** A stage's lighting schedule. */
USTRUCT()
struct FLightSchedule
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Stage;
	UPROPERTY() FString Notes;
	UPROPERTY() TArray<FLightScheduleDaylight> Daylight;
	UPROPERTY() TArray<FLightScheduleLuminaire> Luminaires;
	UPROPERTY() TArray<FLightScheduleTarget> Targets;
	UPROPERTY() TArray<FLightScheduleDesign> Design;
};
