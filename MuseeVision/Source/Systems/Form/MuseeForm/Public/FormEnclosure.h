#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "FormEnclosure.generated.h"

/** What kind of space an enclosure bounds, and so how it must be sealed. */
UENUM()
enum class EFormEnclosureKind : uint8
{
	/** A closed shell: light and weather enter only through its declared openings. */
	Enclosed,
	/** A shell whose top is one declared Open opening: a courtyard. */
	Court,
	/** No shell: the grounds. */
	Outdoors,
};

/**
 * One space of a stage and the pieces that bound it: the oval's shell, the stair's shaft. Each bounding piece of an
 * Enclosed or Court space is a closed solid (Form.Sealed), so its skin has nothing for light or water to pass through.
 */
USTRUCT()
struct MUSEEFORM_API FFormEnclosure
{
	GENERATED_BODY()

	/** The space's key, under its stage: nympheas.oval. */
	UPROPERTY()
	FMuseeKey Key;

	UPROPERTY()
	EFormEnclosureKind Kind = EFormEnclosureKind::Enclosed;

	/** The names of the drawings whose pieces bound it (FFormDrawing::Name; a repeated drawing's every member). */
	UPROPERTY()
	TArray<FName> Pieces;
};
