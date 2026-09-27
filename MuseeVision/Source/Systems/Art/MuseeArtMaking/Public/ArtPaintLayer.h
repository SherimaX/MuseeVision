#pragma once

#include "CoreMinimal.h"
#include "MuseeStageDresser.h"

/**
 * A hung work's paint layer, as Studio makes it (D-015): the work's image, which Studio imports as a texture, and the
 * material instance for the work's own layer, `paint.<id>`, derived from the palette finish it is made in (the oil
 * paint's for a canvas, the glass's for stained glass) and carrying the image and the image's size in metres. A work on
 * several canvases that share its one image has one layer: each canvas's face shows its part of it, its UV0 in metres
 * from the image's top-left corner. A work whose objects each show their own image (the Rouen canvases, the Tristram
 * panels) has one layer per object, `paint.<id>.<index>`.
 */
struct FArtPaintLayer
{
	/** The work's id, or `<id>.<index>` for one object's own image: what Studio names the layer's texture for. */
	FString Work;

	/** Its image, a full path (Musee::Art::FindImage); empty for a painted vessel, whose image is composed. */
	FString ImagePath;

	/**
	 * For a painted vessel, the size [width, height] it is turned to (metres), round which its decoration is composed
	 * from the views its record places (Data/Art/Vessels.json); zero for a work shown as its photograph.
	 */
	FVector2D VesselMeters = FVector2D::ZeroVector;

	/** The image's size in metres: the canvases side by side, their widths summed. */
	FVector2D SizeMeters = FVector2D::ZeroVector;

	/** The layer's own palette key, `paint.<id>` or `paint.<id>.<index>` (Musee::Art::PaintMaterial). */
	FName Palette;

	/**
	 * The palette entry it derives from, with the working and history it is named with: its support's finish
	 * (Data/Art/Supports.json: the oil paint on a canvas, a print's paper, a daguerreotype's silver, ware's lustre), or
	 * the finish of the glass a stained-glass panel is fired in or a transparency's plate is made of.
	 */
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
