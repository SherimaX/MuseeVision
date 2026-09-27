#pragma once

#include "CoreMinimal.h"

class UTexture2D;

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
	 * Fills a texture Studio has made with a work's image: its source pixels, sRGB when the image is, the world's
	 * texture group and the size above. Studio names, builds and saves the texture (D-015). Fails, saying why, when
	 * the file can't be read as an image.
	 */
	[[nodiscard]] MUSEEARTMAKING_API bool FillWorkImage(const FString& ImagePath, UTexture2D& Texture, FString& OutWhy);
}
