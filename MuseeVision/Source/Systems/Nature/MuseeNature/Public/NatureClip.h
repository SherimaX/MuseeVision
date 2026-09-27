#pragma once

#include "CoreMinimal.h"

#include "NatureClip.generated.h"

/**
 * How a hedge is clipped: the section the shears keep along its line, in metres. The clipped surface is the hedge's
 * width at its foot and its height, narrowing towards the top by its batter on each side, its top corners rounded to the
 * shoulder's radius; inside it, a dense core of bare twigs lies the core inset in from the clipped surface. Its ends are
 * `open` (clipped square across) or `closed` (running into something). A single plant clipped to a ball (a box ball)
 * gives the ball's diameter and how far it is sunk into the soil in place of a section. Where the plan gives it, the
 * skin holds so many leaves to the square metre of clipped surface, as the shears keep it; else its species' own.
 */
USTRUCT()
struct MUSEENATURE_API FNatureClip
{
	GENERATED_BODY()

	UPROPERTY(meta = (Units = "Meters"))
	float Width = 0.0f;

	UPROPERTY(meta = (Units = "Meters"))
	float Height = 0.0f;

	/** How much narrower the top is than the foot, on each side. */
	UPROPERTY(meta = (Units = "Meters"))
	float Batter = 0.0f;

	/** The radius the shears round the top's corners to. */
	UPROPERTY(meta = (Units = "Meters"))
	float Shoulder = 0.0f;

	/** How far inside the clipped surface the core of bare twigs lies. */
	UPROPERTY(meta = (Units = "Meters"))
	float CoreInset = 0.0f;

	/** `open` or `closed`. */
	UPROPERTY()
	FString Ends;

	/** A ball's diameter: a single plant clipped to a sphere. */
	UPROPERTY(meta = (Units = "Meters"))
	float Ball = 0.0f;

	/** How far a ball is sunk into the soil. */
	UPROPERTY(meta = (Units = "Meters"))
	float Sink = 0.0f;

	/** Leaves to the square metre of clipped surface, as the shears keep it; zero for the species' own. */
	UPROPERTY()
	float LeavesPerM2 = 0.0f;

	/** Whether the bed is a clipped hedge at all: it gives a section. */
	[[nodiscard]] bool IsSet() const { return Width > 0.0f || Height > 0.0f; }

	/** Whether it is a single plant clipped to a ball: it gives a diameter, sunk less than its radius. */
	[[nodiscard]] bool IsBall() const { return Ball > 0.0f && Sink >= 0.0f && Sink < 0.5f * Ball; }
};
