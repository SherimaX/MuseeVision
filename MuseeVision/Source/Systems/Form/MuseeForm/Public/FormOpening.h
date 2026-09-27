#pragma once

#include "CoreMinimal.h"
#include "MuseeFrame.h"
#include "MuseeKey.h"

#include "FormOpening.generated.h"

/** What passes through an opening in a shell. */
UENUM()
enum class EFormOpeningKind : uint8
{
	/** Daylight, through glazing or any transmissive infill (glass, stained glass, silk). */
	Glazed,
	/** Everything: unglazed to the sky, a courtyard's top, an open oculus. */
	Open,
	/** The rough opening at a port, which a joint or a blind fills. */
	Port,
	/** A pocket that houses a mechanism. */
	Housing,
};

/**
 * An opening a stage declares in one of its shells. Light admits daylight only through Glazed and Open openings, and
 * the leak probe allows light only there; Site fills Port openings. The opening lies in the plane of its frame: its
 * (x, y) span the opening, centred on the origin, and its z faces out of the enclosure.
 */
USTRUCT()
struct MUSEEFORM_API FFormOpening
{
	GENERATED_BODY()

	/** The opening's key, under its stage: nympheas.roof. */
	UPROPERTY()
	FMuseeKey Key;

	UPROPERTY()
	EFormOpeningKind Kind = EFormOpeningKind::Glazed;

	/** The enclosure it opens. */
	UPROPERTY()
	FMuseeKey Enclosure;

	/** Where it is, in the stage's frame. */
	UPROPERTY()
	FMuseeFrame Frame;

	/** Its extent along the frame's x and y, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	FVector2D SizeMeters = FVector2D::ZeroVector;

	/** The finish schedule's surface of its glazing, for a Glazed opening. */
	UPROPERTY()
	FName Glazing;
};
