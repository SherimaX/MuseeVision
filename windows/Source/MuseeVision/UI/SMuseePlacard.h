#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;
struct FMuseeArtwork;
struct FSlateFontInfo;

/**
 * The wall label, in the museum's order (the Met's, the Orsay's):
 *
 *   ─── (gilt rule)
 *   CLAUDE MONET
 *   Impression, Sunrise
 *   Impression, soleil levant
 *   1872
 *   Oil on canvas
 *   Musée Marmottan Monet, Paris
 *   ───────────────────────────
 *   Gave the movement its name at the 1874 exhibition.
 *
 * Printed on a travertine card with a soft shadow; every line wraps to the label's measure, and
 * lines the catalogue leaves empty are left out.
 */
class SMuseePlacard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMuseePlacard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Set the label for a work. */
	void SetWork(const FMuseeArtwork& Work);

private:
	static TSharedRef<STextBlock> MakeLine(const FSlateFontInfo& InFont, const FLinearColor& InColour, float LineHeight = 1.f);
	/** Show a line with this text, or leave it out if the text is empty. */
	static void ShowLine(STextBlock& Line, const FString& Text);

	TSharedPtr<STextBlock> ArtistLine;
	TSharedPtr<STextBlock> TitleLine;
	TSharedPtr<STextBlock> OriginalLine;
	TSharedPtr<STextBlock> YearLine;
	TSharedPtr<STextBlock> MediumLine;
	TSharedPtr<STextBlock> CollectionLine;
	TSharedPtr<SWidget> NotesRule;
	TSharedPtr<STextBlock> NotesLine;
};
