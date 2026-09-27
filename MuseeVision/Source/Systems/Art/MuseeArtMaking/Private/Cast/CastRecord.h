#pragma once

#include "CoreMinimal.h"

#include "CastRecord.generated.h"

/**
 * What a work in the round is cast in, by its medium as its catalogue writes it (`Cast after the marble`): the palette
 * finish of the real object that stands in the museum (the Royal Cast Collection's casts are plaster of Paris, sized,
 * their seams kept).
 */
USTRUCT()
struct FArtCastMedium
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

/**
 * A part of one cast work in another material than its body (the cotton tulle skirt of Degas's dancer): the band of
 * heights above its foot, [from, to] in metres at its true size, whose triangles are that part, and its palette finish.
 */
USTRUCT()
struct FArtCastPart
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FString Work;

	/** The part's name, a key segment: its piece is `cast.<id>.<part>`. */
	UPROPERTY(meta = (MuseeRequired))
	FName Part;

	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Band;

	UPROPERTY(meta = (MuseeRequired))
	FName Palette;

	UPROPERTY(meta = (MuseeRequired))
	FName Working;

	UPROPERTY(meta = (MuseeRequired))
	FName History;

	UPROPERTY()
	FString Note;
};

/**
 * How Art casts works in the round from their scans (`Data/Art/Casts.json`): the height of the base slice a work stands
 * on, how far a scan's width may stray from the width the hanging plan measured before it is a finding, each medium's
 * material, and the parts of a work cast in another material.
 */
USTRUCT()
struct FArtCasts
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Casts.json");

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double BaseSliceMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double WidthToleranceMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FArtCastMedium> Media;

	UPROPERTY()
	TArray<FArtCastPart> Parts;

	[[nodiscard]] const FArtCastMedium* FindMedium(const FString& Medium) const
	{
		return Media.FindByPredicate([&Medium](const FArtCastMedium& M) { return M.Medium == Medium; });
	}
};
