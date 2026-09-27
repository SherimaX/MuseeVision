#pragma once

#include "CoreMinimal.h"

#include "ArtAfter.generated.h"

/**
 * The original a hung work is made after, when the museum shows it at another size than the original's: Albion's
 * Tristram band, glazed from the 1862 Harden Grange panels at about 2.3 to 2.4 times their size, as its approved
 * design states. The hanging plan's `size` is the work as hung; the original's size is written here, with where it
 * comes from, so a label and Art's true-size rule both know the work is not at the original's scale.
 */
USTRUCT()
struct MUSEEART_API FArtAfter
{
	GENERATED_BODY()

	/** The original's width and height [w, h], in metres. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Size;

	/** Where the original's size comes from. */
	UPROPERTY()
	FString Source;

	UPROPERTY()
	FString Note;

	/** Whether a size of the original is written. */
	[[nodiscard]] bool IsSet() const { return Size.Num() == 2; }

	/** The original's size, metres; zero when none is written. */
	[[nodiscard]] FVector2D OriginalMeters() const { return IsSet() ? FVector2D(Size[0], Size[1]) : FVector2D::ZeroVector; }
};
