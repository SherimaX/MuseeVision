#pragma once

#include "CoreMinimal.h"

#include "MatterTriple.generated.h"

/**
 * A quantity of three components: a colour or a per-primary spectrum sample, with its unit, its space and its source
 * (the colour-vector form of Docs/Matter/Schema.md).
 *
 * The space says what the three numbers are. Two spaces are read:
 * - `linear Rec.709 (sRGB primaries), D65 white`: linear channels, red, green, blue;
 * - `CIELAB (D65, 2 deg)`: a measured L*, a*, b*, which Musee::Matter::LinearRec709FromCieLab converts.
 * A per-primary sample of a spectrum (an absorption coefficient, a reflectance) is written in the first space, its
 * components taken at the wavelengths `conditions` names.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterTriple
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Value;

	UPROPERTY(meta = (MuseeRequired))
	FString Space;

	UPROPERTY(meta = (MuseeRequired))
	FString Unit;

	UPROPERTY(meta = (MuseeRequired))
	FString Grade;

	UPROPERTY(meta = (MuseeRequired))
	FString Source;

	UPROPERTY()
	FString Derivation;

	UPROPERTY()
	FString Conditions;

	UPROPERTY()
	FString Note;

	/** Whether the record gives three numbers. */
	bool HasValue() const { return Value.Num() == 3; }
};
