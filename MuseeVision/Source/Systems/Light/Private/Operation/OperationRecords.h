#pragma once

#include "CoreMinimal.h"

#include "OperationRecords.generated.h"

// Data/Light/Operation.json as the record reader reads it.

USTRUCT()
struct FLightOperationSourceRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Values;
	UPROPERTY() FString Source;
	UPROPERTY() FString Url;
	UPROPERTY() FString Accessed;
};

USTRUCT()
struct FLightPresenceRecord
{
	GENERATED_BODY()

	/** Half the side of the detector's square field, centred under it. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters")) double DetectionHalfWidthMeters = 0.0;
	/** How long the light stays on after the detector last saw anyone. */
	UPROPERTY(meta = (MuseeRequired, Units = "Minutes")) double HoldMinutes = 0.0;
	UPROPERTY() FString Notes;
	UPROPERTY() TArray<FLightOperationSourceRecord> Sources;
};

/** How the museum's luminaires are operated. */
USTRUCT()
struct FLightOperationRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Notes;
	UPROPERTY(meta = (MuseeRequired)) FLightPresenceRecord Presence;
};

namespace Musee::Light
{
	/** Data/Light/Operation.json, read once; null (with the reason logged) if it can't be read. */
	const FLightOperationRecord* OperationDesign();
}
