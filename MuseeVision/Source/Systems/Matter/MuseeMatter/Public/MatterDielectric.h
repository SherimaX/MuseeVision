#pragma once

#include "CoreMinimal.h"
#include "MatterQuantity.h"
#include "MatterTriple.h"

#include "MatterDielectric.generated.h"

/**
 * A dielectric body's optics and pores, as a substance's typed record gives them: its diffuse colour, the refractive
 * index its reflection follows from, and the pores that take up water (History.ush's response).
 */
USTRUCT()
struct MUSEEMATTER_API FMatterDielectric
{
	GENERATED_BODY()

	/** The body's diffuse albedo, as a measured CIELAB colour or as linear Rec.709. */
	UPROPERTY()
	FMatterTriple DiffuseAlbedo;

	/** The refractive index of the face's interface in the visible (unit 1). */
	UPROPERTY()
	FMatterQuantity RefractiveIndex;

	/** Open porosity as a volume fraction (unit 1). */
	UPROPERTY()
	FMatterQuantity Porosity;

	/** The depth of the body whose pores govern its appearance, its optical sampling depth (unit mm). */
	UPROPERTY()
	FMatterQuantity WettingDepth;
};
