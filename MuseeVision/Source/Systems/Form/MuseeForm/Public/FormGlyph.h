#pragma once

#include "CoreMinimal.h"

#include "FormGlyph.generated.h"

/**
 * One letter of a typeface as a letter-cutter or a founder takes it from the drawing: its outline, flattened into
 * contours, and everything needed to cast or cut it true. Its units are cap heights: the baseline is y 0 and the cap line
 * y 1, x runs from the pen's position. Each contour is wound with the letter's solid on its left (outlines anticlockwise,
 * counters clockwise, y up). The outline is triangulated once, so a face over it and the same face inset for a bevel
 * share their triangles; each point carries the mitre into the solid that a bevel of one cap height would move it along,
 * narrowed where a stroke is too thin to take the typeface's deepest bevel.
 */
USTRUCT()
struct MUSEEFORM_API FFormGlyph
{
	GENERATED_BODY()

	/** The letter, as one character: A. */
	UPROPERTY(meta = (MuseeRequired))
	FString Letter;

	/** How far the pen moves on after it, in cap heights. */
	UPROPERTY(meta = (MuseeRequired))
	double Advance = 0.0;

	/** The outline's points, (x, y) in cap heights, contour after contour. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<FVector2D> Points;

	/** Where each contour starts in Points; the last runs to the end. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<int32> ContourStarts;

	/** Whether the outline runs smoothly through each point (a curve) or turns there (a corner), one per point. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<bool> Smooth;

	/** Each point's mitre into the solid for a bevel of one cap height, one per point. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<FVector2D> Mitres;

	/** The outline's triangles, three indices into Points each. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<int32> Triangles;

	/** Whether the arrays agree: a mitre and a smooth flag per point, whole triangles within the points, contours in order. */
	[[nodiscard]] bool IsWellFormed(FString* OutWhy = nullptr) const;
};
