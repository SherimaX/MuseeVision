#pragma once

#include "CoreMinimal.h"

#include "MuseeCaption.generated.h"

/**
 * What the museum says about a thing the visitor looks at: a title, a body of text and a credit, all display text.
 * A painting's wall label, a fitting's plate, a mechanism's description. The overlay shows it; its owner writes it.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeCaption
{
	GENERATED_BODY()

	/** The title (`Impression, Sunrise`). */
	UPROPERTY()
	FText Title;

	/** The text under it: artist, date, medium, a few sentences. */
	UPROPERTY()
	FText Body;

	/** Where the thing comes from and on whose licence (`The National Gallery, London`). */
	UPROPERTY()
	FText Credit;
};
