#pragma once

#include "CoreMinimal.h"
#include "MatterPaletteFinish.h"
#include "MatterPaletteResearch.h"
#include "MuseeKey.h"

#include "MatterPaletteEntry.generated.h"

/**
 * One material of the museum's story (D-018): the specific real material a role is made of, why it belongs, and the
 * finishes the rooms take from it. Its key is what a finish schedule names (`stone.travertine-romano`).
 */
USTRUCT()
struct MUSEEMATTER_API FMatterPaletteEntry
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The D-014 family, or `working` for a palette entry that is a working (gilt). */
	UPROPERTY(meta = (MuseeRequired))
	FString Family;

	/** For an accent, the core family it ties back to. */
	UPROPERTY()
	FString AccentOf;

	/** The substance whose typed record (Data/Matter/Shading/Substances) shades it; none while it has no record. */
	UPROPERTY()
	FMuseeKey Substance;

	UPROPERTY(meta = (MuseeRequired))
	FString RealMaterial;

	UPROPERTY()
	TMap<FString, FString> WorkingsByEra;

	UPROPERTY()
	TArray<FString> Roles;

	UPROPERTY()
	FString Why;

	UPROPERTY()
	FString OldChoice;

	UPROPERTY()
	FString Verdict;

	UPROPERTY()
	FString VerdictReason;

	UPROPERTY()
	FMatterPaletteResearch Research;

	/** The finishes made from it; each is one material instance. */
	UPROPERTY()
	TArray<FMatterPaletteFinish> Finishes;
};
