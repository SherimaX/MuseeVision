#pragma once

#include "CoreMinimal.h"

#include "MatterQuantity.generated.h"

/**
 * One measured or derived number about a material, with its unit and where it comes from: the quantity form of
 * Docs/Matter/Schema.md. A quantity is a value, or a range (min and max), or both; a number the record leaves out keeps
 * the unset mark (the largest double), so a builder can tell a missing number from a zero.
 *
 * The unit is written as data (`"1"` for a fraction or a ratio, `"mm"`, `"1/year"`), and each builder names the unit it
 * expects and refuses any other.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterQuantity
{
	GENERATED_BODY()

	UPROPERTY()
	double Value = TNumericLimits<double>::Max();

	UPROPERTY()
	double Min = TNumericLimits<double>::Max();

	UPROPERTY()
	double Max = TNumericLimits<double>::Max();

	UPROPERTY(meta = (MuseeRequired))
	FString Unit;

	/** How strong the evidence is: measured, manufacturer, guidance, secondary, historical, derived, design, record, or
	 *  unverified (a number no source measures, carried until a specimen settles it). */
	UPROPERTY(meta = (MuseeRequired))
	FString Grade;

	UPROPERTY(meta = (MuseeRequired))
	FString Source;

	/** The formula or procedure, when the grade is derived. */
	UPROPERTY()
	FString Derivation;

	UPROPERTY()
	FString Conditions;

	UPROPERTY()
	FString Note;

	/** Whether the record gives a value (not only a range). */
	bool HasValue() const { return Value != TNumericLimits<double>::Max(); }
};
