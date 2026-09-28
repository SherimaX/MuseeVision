#pragma once

#include "CoreMinimal.h"
#include "Make/MakeReport.h"

class UMaterial;
class UTexture2D;

namespace Musee::Studio
{
	struct FStageMakeOptions;

	/**
	 * Fills a texture with Light's star field drawn from the catalogue's bytes (Musee::Light::Image::DrawStarField): its
	 * texels half-float luminances in candelas per square metre, HDR, unmipped, wrapping round the sky in right ascension
	 * and clamped at the poles, filtered bilinearly, as Light's image material samples it. False, with Light's reason, for
	 * a catalogue it can't read.
	 */
	bool FillStarField(const TArray<uint8>& CatalogueBytes, UTexture2D& Texture, FString& OutWhy);

	/**
	 * Builds Light's image material on the star field: unlit (Substrate's unlit BSDF), two-sided, its emission Light's
	 * (Musee::Light::Image::EmissionHlsl) read along the camera vector, with Light's parameters at rest (the sky
	 * unturned, no Moon, no peak white). Anything the material held before is replaced.
	 */
	void BuildImageMaterial(UMaterial& Material, UTexture2D& StarField);

	/**
	 * Makes Light's image for its enclosing image surfaces (the Sphere's sky): the star field from the collection's star
	 * catalogue and the image material over it, at <Root>/Light/<name>, which for the museum's root is where Light finds
	 * them (Musee::Light::Image::Folder). The material's shaders are compiled for the museum's platform, so a fault in its
	 * emission is a finding of the make, not a grey sky. Either one whose inputs are unchanged is left as it is, so a
	 * second make writes nothing. Returns the fingerprint of what was made, for the levels that place the image.
	 */
	FString MakeLightImage(const FStageMakeOptions& Options, FMakeReport& Report);
}
