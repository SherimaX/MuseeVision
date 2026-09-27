#pragma once

#include "CoreMinimal.h"

#include "ArtMedia.generated.h"

/**
 * What a work in the round is made of, by its medium as its catalogue writes it (`Cast after the marble`, `Stoneware
 * with celadon glaze, ...`): the palette finish of the real object that stands in the museum (the Royal Cast
 * Collection's casts are plaster of Paris, sized, their seams kept; a Longquan vase is stoneware under its celadon).
 */
USTRUCT()
struct FArtMedium
{
	GENERATED_BODY()

	/** The catalogue's `medium`, exactly. */
	UPROPERTY(meta = (MuseeRequired))
	FString Medium;

	UPROPERTY(meta = (MuseeRequired))
	FName Palette;

	UPROPERTY(meta = (MuseeRequired))
	FName Working;

	UPROPERTY(meta = (MuseeRequired))
	FName History;

	/** Why this is the object's material. */
	UPROPERTY()
	FString Note;
};

/** Every medium a work in the round is made in, by its catalogue's words (`Data/Art/Media.json`). */
USTRUCT()
struct FArtMedia
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Media.json");

	UPROPERTY(meta = (MuseeRequired))
	TArray<FArtMedium> Media;

	[[nodiscard]] const FArtMedium* Find(const FString& Medium) const
	{
		return Media.FindByPredicate([&Medium](const FArtMedium& M) { return M.Medium == Medium; });
	}
};
