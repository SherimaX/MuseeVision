#pragma once

#include "CoreMinimal.h"

#include "ArtGallery.generated.h"

/** A gallery a catalogue groups its works into (`data/artworks.json`). */
USTRUCT()
struct MUSEEART_API FArtGallery
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Period;
};
