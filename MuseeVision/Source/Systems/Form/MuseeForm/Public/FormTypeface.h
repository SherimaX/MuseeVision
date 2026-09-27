#pragma once

#include "CoreMinimal.h"
#include "FormGlyph.h"
#include "MuseeKey.h"

#include "FormTypeface.generated.h"

/**
 * A typeface the museum letters in: the capitals of an inscription or a dial, baked from a face's outlines into
 * Data/Form/Typefaces.json (Form's catalogue), each glyph in cap heights. It holds the letters its works use.
 */
USTRUCT()
struct MUSEEFORM_API FFormTypeface
{
	GENERATED_BODY()

	/** The catalogue key: centaur.capitals. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** Where the outlines come from: the face, its designer, and how they were baked. */
	UPROPERTY(meta = (MuseeRequired))
	FString Source;

	/** The deepest bevel its mitres were checked for, in cap heights: a deeper bevel may turn a serif's tip over. */
	UPROPERTY(meta = (MuseeRequired))
	double MaxBevel = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FFormGlyph> Glyphs;

	/** The glyph of a letter, or null when the face lacks it. */
	[[nodiscard]] const FFormGlyph* Find(TCHAR Letter) const
	{
		return Glyphs.FindByPredicate([Letter](const FFormGlyph& Glyph) { return Glyph.Letter.Len() == 1 && Glyph.Letter[0] == Letter; });
	}
};
