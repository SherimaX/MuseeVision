#pragma once

#include "CoreMinimal.h"

#include "StoneRecord.generated.h"

/**
 * How a kind of scholar's rock is formed (`Data/Art/Stones.json`): a limestone that water has taken away, where
 * travertine is one it laid down. A Taihu rock is tall, waisted and pierced, the lake's water having dissolved its holes
 * right through it; a Lingbi rock is dense and compact, a small mountain grooved and pitted by the soil it lay in, with
 * few holes through. The kind gives the rock's proportions and its wear, as shares of its height; each rock's own height
 * is its plan's and its foot's radius the record's (the ground or stand it stands on).
 */
USTRUCT()
struct FArtStoneKind
{
	GENERATED_BODY()

	/** `taihu` or `lingbi`. */
	UPROPERTY(meta = (MuseeRequired))
	FName Kind;

	/** Its depth front to back as a share of its width. */
	UPROPERTY(meta = (MuseeRequired))
	double DepthPerWidth = 0.0;

	/**
	 * Its silhouette: [height share, half-width share] from its foot (0) to its top (1), the half-width a share of its
	 * widest; at least two points, their heights rising from 0 to 1.
	 */
	UPROPERTY(meta = (MuseeRequired))
	TArray<FVector2D> Profile;

	/** The peaks along its top, and the height share of the saddles between them. */
	UPROPERTY(meta = (MuseeRequired))
	int32 Peaks = 1;

	UPROPERTY(meta = (MuseeRequired))
	double Saddle = 1.0;

	/** The holes water wore right through it (透, 漏), and their radii [least, most] as shares of its height. */
	UPROPERTY()
	int32 Holes = 0;

	UPROPERTY()
	TArray<double> HoleRadius;

	/** The blind pits worn into its skin, and their radii [least, most] as shares of its height. */
	UPROPERTY()
	int32 Pits = 0;

	UPROPERTY()
	TArray<double> PitRadius;

	/** How deep its skin is wrinkled (皺), drawn out up its height, as a share of its height. */
	UPROPERTY(meta = (MuseeRequired))
	double Wrinkle = 0.0;

	/** How far its whole form is bent from straight, as a share of its height. */
	UPROPERTY(meta = (MuseeRequired))
	double Warp = 0.0;

	/** The cells up its height its surface is found on: the finest wear it shows is about one cell. */
	UPROPERTY(meta = (MuseeRequired))
	int32 Cells = 0;
};

/** One scholar's rock of a stage (a work its plan stands in the round): its kind, and the radius its foot must keep within. */
USTRUCT()
struct FArtStone
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FString Work;

	UPROPERTY(meta = (MuseeRequired))
	FName Kind;

	/** The radius its foot keeps within: the clearing it stands in, or the carved stand it is set on. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double FootRadiusMeters = 0.0;

	UPROPERTY()
	FString Source;
};

/** How the museum's scholar's rocks are formed, and each rock (`Data/Art/Stones.json`). */
USTRUCT()
struct FArtStones
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Stones.json");

	UPROPERTY(meta = (MuseeRequired))
	TArray<FArtStoneKind> Kinds;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FArtStone> Stones;

	[[nodiscard]] const FArtStone* Find(const FString& Work) const
	{
		return Stones.FindByPredicate([&Work](const FArtStone& Stone) { return Stone.Work == Work; });
	}

	[[nodiscard]] const FArtStoneKind* FindKind(FName Kind) const
	{
		return Kinds.FindByPredicate([Kind](const FArtStoneKind& Each) { return Each.Kind == Kind; });
	}
};
