#pragma once

#include "CoreMinimal.h"

#include "ArtScan.generated.h"

/** A catalogued work's digitised scan (a sculpture's), as the sculptures catalogue records it. */
USTRUCT()
struct MUSEEART_API FArtScan
{
	GENERATED_BODY()

	UPROPERTY()
	FString Url;

	UPROPERTY()
	FString License;

	UPROPERTY()
	bool Permissive = false;

	UPROPERTY()
	FString Source;

	UPROPERTY()
	int64 Triangles = 0;
};
