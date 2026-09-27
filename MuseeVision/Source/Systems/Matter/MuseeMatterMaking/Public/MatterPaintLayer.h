#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceConstant;
class UMaterialInterface;
class UTexture;

/**
 * A hung work's own paint layer: the paint master's instance that carries the work's image at the work's true size
 * (D-015: Matter defines, Studio writes).
 *
 * The paint's physics (the white its image's white stands for, its varnish's index and gloss, and its history) come
 * from a palette finish's records, the museum's oil on linen, varnished and clean; the image and the size come from the
 * work. Art builds the canvas's painted face with its UV0 in metres from the image's top-left corner, so the image
 * spans the face whatever its shape. Studio makes the instance at Musee::Matter::InstancePackage(Work, Working, History)
 * (MatterNames.h), where Musee::Matter::Resolve finds it for the work's canvas piece.
 */
namespace Musee::Matter
{
	/** The palette finish a paint layer's physics come from. */
	namespace PaintFinish
	{
		inline constexpr const TCHAR* Palette = TEXT("accent.oil-on-linen");
		inline constexpr const TCHAR* Working = TEXT("varnished");
		inline constexpr const TCHAR* History = TEXT("clean");
	}

	/** One work's paint layer. */
	struct FMatterPaintLayer
	{
		/** The work's paint key, paint.<work id> (Musee::Art::PaintMaterial); the instance's palette key. */
		FName Work;

		/** The palette finish whose records give the paint's physics, and the working and history the instance is
		 *  named with. */
		FName Palette = PaintFinish::Palette;
		FName Working = PaintFinish::Working;
		FName History = PaintFinish::History;

		/** The work's image as a texture (Musee::ArtMaking::FillWorkImage), sRGB. */
		UTexture* Image = nullptr;

		/** The work's width and height, which its image spans (metres). */
		double WidthMeters = 0.0;
		double HeightMeters = 0.0;
	};

	/**
	 * Composes Instance as Layer's paint layer, parented to PaintMaster (M_Matter_Paint, MasterPackage("paint")): the
	 * palette finish's parameters, then the work's image and size. Its identifiers derive from all of them, so the same
	 * work and records compose the same instance. Returns the problems; an empty list is success. A work with no image,
	 * no size, or a palette finish the paint master does not shade is a problem, never a default.
	 */
	[[nodiscard]] MUSEEMATTERMAKING_API TArray<FString> ComposePaintLayer(const FMatterPaintLayer& Layer, UMaterialInterface& PaintMaster,
		UMaterialInstanceConstant& Instance);
}
