#pragma once

#include "CoreMinimal.h"
#include "MuseeMeasurement.h"

#include "MuseeFinding.generated.h"

/**
 * What a check found wrong: the reading that broke a truth, the system that owns the truth, and the system (or stage)
 * that drew the piece at fault, so the fix goes to its cause. A check returns no findings when its truth holds.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeFinding
{
	GENERATED_BODY()

	/** The check that found it (`Form.Sealed`). Set by FMuseeCheck::Evaluate. */
	UPROPERTY()
	FName Check;

	/** The system that owns the broken truth: the check's first name. Set by FMuseeCheck::Evaluate. */
	UPROPERTY()
	FName Owner;

	/** The system or stage whose drawing made the piece at fault (`Rotunda`, `Site`), where the fix belongs. */
	UPROPERTY()
	FName Drawer;

	/** What is wrong, in one sentence. */
	UPROPERTY()
	FString Summary;

	/** The reading that broke the truth. */
	UPROPERTY()
	FMuseeMeasurement Measurement;

	/** The circumstance the check ran under, as the runner names it: a configuration, a moment or a capture. */
	UPROPERTY()
	FString Context;

	/** A finding a check reports: who drew the piece, what is wrong, and the reading. */
	[[nodiscard]] static FMuseeFinding Make(FName InDrawer, const FString& InSummary, const FMuseeMeasurement& InMeasurement);

	/** The finding in one line, for reports and logs. */
	[[nodiscard]] FString ToString() const;
};
