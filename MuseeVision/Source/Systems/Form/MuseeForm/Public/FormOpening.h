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
	/**
	 * A doorway between two enclosures of one stage (the Rotunda's hall and its stair, through the raised sun clock's
	 * door): people, sound and air pass, and daylight only as the light of the enclosure beyond, never as sky.
	 */
	Passage,
};

/**
 * An opening a stage declares in one of its shells. Light admits daylight only through Glazed and Open openings, and
 * the leak probe allows light only there; Site fills Port openings. The opening spans its frame's (x, y), centred on the
 * origin, and its z faces out of the enclosure: in the frame's plane, or bent round a cylinder when it opens a curved
 * shell (a drum's glazing, a cove round a drum's foot).
 *
 * An opening between two of the stage's enclosures (a passage, a porch's door, a doorway from a court into a hall) is
 * declared once, from the enclosure it faces out of, and names the enclosure beyond.
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

	/** The enclosure it opens, which its z faces out of. */
	UPROPERTY()
	FMuseeKey Enclosure;

	/**
	 * The stage's enclosure on its other side, when it opens one of the stage's enclosures into another; the none key when
	 * it opens to the outside, the sky or another stage. A Passage always names one.
	 */
	UPROPERTY()
	FMuseeKey Beyond;

	/** Where it is, in the stage's frame. */
	UPROPERTY()
	FMuseeFrame Frame;

	/** Its extent along the frame's x (along the arc, when it is bent) and y, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	FVector2D SizeMeters = FVector2D::ZeroVector;

	/**
	 * Zero for an opening in its frame's plane. Otherwise it lies on a cylinder of this radius whose axis runs along the
	 * frame's y, through the point this far behind the origin along -z: the opening bows out of its enclosure, as a drum's
	 * glazing does seen from the room within. Negative, the axis lies in front, along +z, and the opening is hollow
	 * towards the outside. Metres.
	 */
	UPROPERTY(meta = (Units = "Meters"))
	double RadiusMeters = 0.0;

	/** The finish schedule's surface of its glazing, for a Glazed opening. */
	UPROPERTY()
	FName Glazing;

	/** Whether it joins two of the stage's enclosures. */
	[[nodiscard]] bool IsBetweenEnclosures() const { return !Beyond.IsNone(); }

	/** Whether it is bent round a cylinder rather than lying in a plane. */
	[[nodiscard]] bool IsCurved() const { return RadiusMeters != 0.0; }
};
