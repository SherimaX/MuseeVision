#pragma once

#include "CoreMinimal.h"

#include "FeedRecords.generated.h"

// The live feed's answer, as Open-Meteo's forecast API returns current conditions for several places at once
// (https://open-meteo.com/en/docs): one object per place, in the order asked. The field names are the feed's own; the
// record reader matches them without regard to case.

USTRUCT()
struct FTimeFeedUnits
{
	GENERATED_BODY()

	UPROPERTY() FString Time;
	UPROPERTY() FString Interval;
	UPROPERTY() FString Temperature_2m;
	UPROPERTY() FString Cloud_Cover;
	UPROPERTY() FString Rain;
	UPROPERTY() FString Showers;
	UPROPERTY() FString Snowfall;
	UPROPERTY() FString Wind_Speed_10m;
	UPROPERTY() FString Wind_Direction_10m;
	UPROPERTY() FString Snow_Depth;
};

USTRUCT()
struct FTimeFeedCurrent
{
	GENERATED_BODY()

	/** The instant the reading describes, in seconds of the Unix epoch. */
	UPROPERTY(meta = (MuseeRequired)) int64 Time = 0;
	/** The span the precipitation sums cover, in seconds. */
	UPROPERTY(meta = (MuseeRequired)) int64 Interval = 0;
	UPROPERTY(meta = (MuseeRequired)) double Temperature_2m = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double Cloud_Cover = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double Rain = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double Showers = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double Snowfall = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double Wind_Speed_10m = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double Wind_Direction_10m = 0.0;
	/** Absent where the model has no snow depth; negative marks it absent. */
	UPROPERTY() double Snow_Depth = -1.0;
};

USTRUCT()
struct FTimeFeedPlace
{
	GENERATED_BODY()

	UPROPERTY() double Latitude = 0.0;
	UPROPERTY() double Longitude = 0.0;
	UPROPERTY() double GenerationTime_Ms = 0.0;
	UPROPERTY() int32 Utc_Offset_Seconds = 0;
	UPROPERTY() FString Timezone;
	UPROPERTY() FString Timezone_Abbreviation;
	UPROPERTY() double Elevation = 0.0;
	UPROPERTY() int32 Location_Id = 0;
	UPROPERTY(meta = (MuseeRequired)) FTimeFeedUnits Current_Units;
	UPROPERTY(meta = (MuseeRequired)) FTimeFeedCurrent Current;
};
