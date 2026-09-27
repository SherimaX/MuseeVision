#pragma once

#include "CoreMinimal.h"

#include "ArtScrollMount.generated.h"

/**
 * The silk mount of a Chinese scroll, as the hanging plan proportions it (Chenghuai): a hanging scroll's heaven above
 * the painting and earth below it, the bands and borders framing it, and its roller; a handscroll's lead (the silk
 * before the painting), wrapper and the radius of its rolled end. All in metres; its silk is the finish of the
 * entry's frame surface.
 */
USTRUCT()
struct MUSEEART_API FArtScrollMount
{
	GENERATED_BODY()

	/** A hanging scroll's silk above the painting (tiandi's heaven). */
	UPROPERTY(meta = (Units = "Meters"))
	float Heaven = 0.0f;

	/** A hanging scroll's silk below the painting (its earth). */
	UPROPERTY(meta = (Units = "Meters"))
	float Earth = 0.0f;

	/** The bands (geshui) across the mount. */
	UPROPERTY(meta = (Units = "Meters"))
	float Band = 0.0f;

	/** The border (bian) round the painting. */
	UPROPERTY(meta = (Units = "Meters"))
	float Border = 0.0f;

	/** Whether a hanging scroll has its roller (zhou) at the foot. */
	UPROPERTY()
	bool Roller = false;

	/** A handscroll's lead: the silk unrolled before the painting's start. */
	UPROPERTY(meta = (Units = "Meters"))
	float Lead = 0.0f;

	/** A handscroll's wrapper at its outer end. */
	UPROPERTY(meta = (Units = "Meters"))
	float Wrapper = 0.0f;

	/** The radius of a handscroll's rolled remainder. Spelt as the plan's key, `roll_radius`. */
	UPROPERTY(meta = (Units = "Meters"))
	float Roll_Radius = 0.0f;
};
