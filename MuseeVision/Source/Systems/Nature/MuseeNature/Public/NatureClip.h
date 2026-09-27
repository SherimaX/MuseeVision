#pragma once

#include "CoreMinimal.h"

#include "NatureClip.generated.h"

/**
 * How a hedge is clipped: the section the shears keep along its line, in metres. The clipped surface is the hedge's
 * width at its foot and its height, narrowing towards the top by its batter on each side, its top corners rounded to the
 * shoulder's radius; inside it, a dense core of bare twigs lies the core inset in from the clipped surface. Its ends are
 * `open` (clipped square across) or `closed` (running into something).
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

	/** Whether the bed is a clipped hedge at all: it gives a section. */
	[[nodiscard]] bool IsSet() const { return Width > 0.0f || Height > 0.0f; }
};
