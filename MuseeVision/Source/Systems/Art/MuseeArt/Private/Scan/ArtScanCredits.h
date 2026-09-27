#pragma once

#include "CoreMinimal.h"

#include "ArtScanCredits.generated.h"

/**
 * The credit a scan the museum shows is owed (Data/Art/Scans.json), in the words its source asks for, read from the
 * collection's own credits (assets/classical/CREDITS.md, assets/sculptures/CREDITS.md). A line names one mesh, or a
 * folder of meshes that share one credit when its name ends in `/`.
 */
USTRUCT()
struct FArtScanCredit
{
	GENERATED_BODY()

	/** The mesh, or the folder, from the repository's root, as a hanging plan's `scan.mesh` spells it. */
	UPROPERTY(meta = (MuseeRequired))
	FString Mesh;

	/** The words on the label. */
	UPROPERTY(meta = (MuseeRequired))
	FString Credit;

	/** Where the credit was read. */
	UPROPERTY(meta = (MuseeRequired))
	FString Source;
};

/** Every scan's credit. */
USTRUCT()
struct FArtScanCredits
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Scans.json");

	UPROPERTY(meta = (MuseeRequired))
	TArray<FArtScanCredit> Scans;
};
