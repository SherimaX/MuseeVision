#pragma once

#include "ArtWallPath.h"
#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "ArtWall.generated.h"

/** A curved hanging wall: the Wall anchor where it starts, and the path its face follows. */
USTRUCT()
struct MUSEEART_API FArtWall
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Anchor;

	UPROPERTY(meta = (MuseeRequired))
	FArtWallPath Path;

	UPROPERTY()
	FString Note;
};
