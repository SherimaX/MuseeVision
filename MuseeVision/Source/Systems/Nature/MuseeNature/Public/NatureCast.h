#pragma once

#include "CoreMinimal.h"

#include "NatureCast.generated.h"

/**
 * The material a cast specimen is made in: a plant cast from life (the golden iris, cast in gilt bronze from Iris
 * laevigata), as a palette key with its working and history. Every part of the grown form takes it in place of the
 * living tissues.
 */
USTRUCT()
struct MUSEENATURE_API FNatureCast
{
	GENERATED_BODY()

	/** The palette key (Data/Matter/Palette.json): `bronze.cusn10`. */
	UPROPERTY()
	FName Material;

	/** The working and history it is resolved with (Data/Matter/Shading): `gilt`, `handled`. */
	UPROPERTY()
	FName Working;

	UPROPERTY()
	FName History;

	/** Whether the bed is a cast specimen at all: it names a material. */
	[[nodiscard]] bool IsSet() const { return !Material.IsNone(); }
};
