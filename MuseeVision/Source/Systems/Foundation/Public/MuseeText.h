#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "MuseeText.generated.h"

/**
 * A string the visitor reads, as a record holds it: a key and the source string in the museum's source language
 * (English). A wall label, a caption, a mechanism's prompt. Studio gathers every FMuseeText from the records into the
 * museum's localisation, under the namespace `Musee` and the text's key, so a translation finds it by the same key.
 *
 * A record may write a text as a plain string, its source alone (MuseeRecords.h): its key is then none until the
 * record's owner gives it the key its place in the record implies.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeText
{
	GENERATED_BODY()

	/** The localisation namespace every museum text is gathered under. */
	static constexpr const TCHAR* Namespace = TEXT("Musee");

	/** The text's key, scoped by its owner (`art.monet-impression-sunrise.title`). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The text as written in the source language. */
	UPROPERTY(meta = (MuseeRequired))
	FString Source;

	/** The text to display: localisable under its namespace and key, in the current culture where translated. */
	[[nodiscard]] FText ToText() const;
};
