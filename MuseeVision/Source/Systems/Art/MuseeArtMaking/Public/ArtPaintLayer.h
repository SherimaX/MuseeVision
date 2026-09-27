#pragma once

#include "CoreMinimal.h"
#include "MuseeStageDresser.h"

/**
 * A hung work's paint layer, as Studio makes it (D-015): the work's image, which Studio imports as a texture, and the
 * material instance for the work's own paint, `paint.<id>` with the working and history of the paint's palette entry,
 * derived from that entry's instance and carrying the image and the image's size in metres. A work on several canvases
 * has one paint layer: each canvas's face shows its part of the one image, its UV0 in metres from the image's top-left
 * corner.
 */
struct FArtPaintLayer
{
	/** The work's id. */
	FString Work;

	/** Its image, a full path (Musee::Art::FindImage). */
	FString ImagePath;

	/** The image's size in metres: the canvases side by side, their widths summed. */
	FVector2D SizeMeters = FVector2D::ZeroVector;

	/** The work's own palette key, `paint.<id>` (Musee::Art::PaintMaterial). */
	FName Palette;

	/** The palette entry it derives from (PaintPalette), whose one finish gives its working and history. */
	FName Parent;
	FName Working;
	FName History;
};

namespace Musee::ArtMaking
{
	/** The palette entry every work's paint derives from: oil paint on a primed linen canvas (Data/Matter/Palette.json). */
	inline constexpr const TCHAR* PaintPalette = TEXT("accent.oil-on-linen");

	/**
	 * The paint layers of a stage's hung works, one per work, in the hanging plan's order. A work Art's piece source
	 * can't make is left out; the piece source reports why.
	 */
	[[nodiscard]] MUSEEARTMAKING_API TArray<FArtPaintLayer> PaintLayers(const FMuseeStageContext& Stage);
}
