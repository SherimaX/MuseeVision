#pragma once

#include "CoreMinimal.h"
#include "MatterQuantity.h"
#include "MatterTriple.h"

#include "MatterFilm.generated.h"

/**
 * A mineral film over a metal: a patina a working lays on it, or the tarnish its history grows. A film is a dielectric
 * crust with its own diffuse colour and index (Metal.ush, MatterFilmed).
 */
USTRUCT()
struct MUSEEMATTER_API FMatterFilm
{
	GENERATED_BODY()

	/** The film's diffuse albedo (CIELAB or linear Rec.709). */
	UPROPERTY()
	FMatterTriple DiffuseAlbedo;

	/** The film mineral's refractive index in the visible (unit 1). */
	UPROPERTY()
	FMatterQuantity RefractiveIndex;

	/** The Slab roughness of the film's surface (unit 1). */
	UPROPERTY()
	FMatterQuantity Roughness;

	/** The fraction of the metal the film covers (unit 1). */
	UPROPERTY()
	FMatterQuantity Cover;
};
