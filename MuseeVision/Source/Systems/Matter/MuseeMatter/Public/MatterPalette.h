#pragma once

#include "CoreMinimal.h"
#include "MatterPaletteEntry.h"

#include "MatterPalette.generated.h"

/**
 * The museum's material story as data (D-018; Docs/Matter/Story.md tells it): every role's real material, and the
 * finishes the rooms take from them. Read from `Data/Matter/Palette.json`.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterPalette
{
	GENERATED_BODY()

	/** `musee.matter.palette/1`. */
	UPROPERTY(meta = (MuseeRequired))
	FString Schema;

	/** The story document, relative to the project. */
	UPROPERTY()
	FString Story;

	/** The decision the palette carries out. */
	UPROPERTY()
	FString Decision;

	/** What each era does to the materials, in plain words. */
	UPROPERTY()
	TMap<FString, FString> Eras;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FMatterPaletteEntry> Entries;

	/** The entry with this key, or nullptr. */
	const FMatterPaletteEntry* Find(const FMuseeKey& Key) const
	{
		return Entries.FindByPredicate([&Key](const FMatterPaletteEntry& Entry) { return Entry.Key == Key; });
	}
};
