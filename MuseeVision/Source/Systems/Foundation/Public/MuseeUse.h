#pragma once

#include "CoreMinimal.h"
#include "MuseeFrame.h"

#include "MuseeUse.generated.h"

/**
 * How a usable thing can be used right now: the verb the visitor's hands offer (`Raise`), the caption that says what
 * will happen (`Raise the sun clock`), whether it can be used now and, if not, why not (`The cylinder is moving`).
 * A seat also gives the frame the visitor sits in.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeUse
{
	GENERATED_BODY()

	/** The verb offered to the visitor. */
	UPROPERTY()
	FText Verb;

	/** What using the thing will do, in a short sentence. */
	UPROPERTY()
	FText Caption;

	/** Whether the thing can be used now. */
	UPROPERTY()
	bool bAvailable = false;

	/** Why the thing can't be used now; empty while it can. */
	UPROPERTY()
	FText WhyNot;

	/** Whether using the thing seats the visitor in SeatFrame. */
	UPROPERTY()
	bool bSeats = false;

	/**
	 * The seat, in the museum's frame: the origin at the middle of the front edge of the seat's surface, x facing the
	 * way the sitter faces and z up. Visitor sets the body down on it by its own formula.
	 */
	UPROPERTY()
	FMuseeFrame SeatFrame;

	/** A use that is available now. */
	[[nodiscard]] static FMuseeUse Available(const FText& InVerb, const FText& InCaption);

	/** A use that isn't available now, and why. */
	[[nodiscard]] static FMuseeUse Unavailable(const FText& InVerb, const FText& InCaption, const FText& InWhyNot);

	/** The same use, seating the visitor in a frame. */
	[[nodiscard]] FMuseeUse Seating(const FMuseeFrame& InSeatFrame) const;
};
