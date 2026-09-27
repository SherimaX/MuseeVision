#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

#include "MuseeDefect.generated.h"

/**
 * One defect in the Journal (`Journal/Defects.md`): something the museum got wrong, the check that catches it, the
 * system or stage whose code was at fault, the physical truth it broke and the fix made in its owner. A defect must
 * name an existing check, so it can't come back unnoticed.
 *
 * In the Journal each defect is a second-level heading (its number, a middle dot U+00B7 as in Decisions.md, and its
 * title) followed by its fields as list items:
 *
 *     ## D-0001 [middle dot] A light streak across the dome
 *     - **Symptom:** A bright line between two coffer rings at 15:00 in June.
 *     - **Check:** Form.Sealed
 *     - **Drawer:** Rotunda
 *     - **Cause:** The coffer ring's outer edge stopped 2 mm short of the drum, leaving the shell open.
 *     - **Fix:** The ring is drawn to the drum's inner face.
 *     - **Status:** closed
 *
 * Numbers run from D-0001. Each field is one line, and the fix may be left out while a defect is open. Text before the
 * first entry is the file's preface.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeDefect
{
	GENERATED_BODY()

	/** `D-` and four digits. */
	UPROPERTY()
	FString Number;

	/** The defect in a few words. */
	UPROPERTY()
	FString Title;

	/** What was seen, where and when. */
	UPROPERTY()
	FString Symptom;

	/** The check that catches it (`Form.Sealed`). */
	UPROPERTY()
	FName Check;

	/** The system or stage whose code was at fault, where the fix went. */
	UPROPERTY()
	FName Drawer;

	/** The physical truth the model broke. */
	UPROPERTY()
	FString Cause;

	/** What changed in the owner; empty while the defect is open. */
	UPROPERTY()
	FString Fix;

	/** Whether the fix is made and the check passes. */
	UPROPERTY()
	bool bClosed = false;

	/** Whether text is a defect number: `D-` and four digits. */
	[[nodiscard]] static bool IsValidNumber(FStringView Text);

	/** Whether the defect names a check that is registered. */
	[[nodiscard]] bool NamesExistingCheck() const;

	/** The defect as its Journal entry, in the form above. */
	[[nodiscard]] FString ToMarkdown() const;

	/**
	 * Every defect in a Journal file's text, in order. Fails, naming the entry and the field, on a heading that isn't a
	 * defect number, a missing or unknown field, a check name that breaks the check rule, or a closed defect without
	 * its fix.
	 */
	[[nodiscard]] static TValueOrError<TArray<FMuseeDefect>, FString> ParseJournal(FStringView Markdown);
};
