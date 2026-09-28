#pragma once

#include "CoreMinimal.h"

class UTexture2D;
struct FArtPaintLayer;

/**
 * The image of a work's paint layer as a texture: the photograph of the work (found by Musee::Art::FindImage), carried
 * into the museum at its full resolution and in its own colour space, so the paint layer shows the work's colours as
 * its photographer measured them.
 */
namespace Musee::ArtMaking
{
	/**
	 * The largest texture a work's image is shown at: 8192 px, as the as-built willows were (about 0.6 px a millimetre
	 * across a 12.75 m canvas: finer than the eye resolves from the room, and light on video memory).
	 */
	inline constexpr int32 WorkImageMaxPixels = 8192;

	/**
	 * Composes a texture Studio has made from a work's image: its source pixels (whose hash is the source's id, so the
	 * same image makes the same texture), sRGB when the image is, the world's texture group, the size above and its edges clamped (a
	 * photograph of a bounded work, never a tile). Studio
	 * names, builds and saves the texture (D-015). Fails, saying why, when the file can't be read as an image.
	 */
	[[nodiscard]] MUSEEARTMAKING_API bool ComposeWorkImage(const FString& ImagePath, UTexture2D& Texture, FString& OutWhy);

	/**
	 * Composes a texture Studio has made for a paint layer: its work's photograph (as ComposeWorkImage), or for a painted
	 * vessel its decoration, composed from its views round its turned form in the layout its mesh's UV0 shares
	 * (Vessel/VesselDecoration.h). Fails, saying why, when a photograph can't be read or the vessel's record can't.
	 */
	[[nodiscard]] MUSEEARTMAKING_API bool ComposeLayerImage(const FArtPaintLayer& Layer, UTexture2D& Texture, FString& OutWhy);

	/**
	 * What a layer's image is made from, as one hash: its photograph's bytes; or a painted vessel's views' bytes, its
	 * record's numbers, its size and the composition's revision. The same inputs give the same identity, so Studio
	 * remakes the texture only when one changes. Empty when a file can't be read.
	 */
	[[nodiscard]] MUSEEARTMAKING_API FString LayerImageIdentity(const FArtPaintLayer& Layer);
}
