#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

class UMaterialInstanceConstant;
class UMaterialInterface;
class UTexture;

/**
 * A hung work's own layer: the instance that carries the work's image at the work's true size (D-015: Matter defines,
 * Studio writes), over the paint master for a canvas and over the stained-glass master for a stained-glass panel.
 *
 * The layer's physics come from a palette finish's records: for a canvas the museum's oil on linen, varnished and clean
 * (the white its image's white stands for, its varnish's index and gloss, and its history); for a panel the glass it is
 * fired in (the clear glass's index, absorption and thickness, which its image's white stands for). The image and the
 * size come from the work. Art builds the painted face, or the panel's, with its UV0 in metres from the image's top-left
 * corner, so the image spans the face whatever its shape. Studio makes the instance at
 * Musee::Matter::InstancePackage(Work, Working, History) (MatterNames.h), where Musee::Matter::Resolve finds it.
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

	/** The long package name of the paint master a canvas's layer is parented to, as Matter's make step makes it. */
	[[nodiscard]] MUSEEMATTERMAKING_API FString PaintMasterPackage();

	/**
	 * The long package name of the master a layer is composed over, from the palette finish it derives from: the paint
	 * master for a paint (a canvas, a print on paper, a lustre-painted glaze), the plate master (M_Matter_Plate) for a
	 * daguerreotype's silvered plate, the stained-glass master (M_Matter_Stained) for a glass (a stained-glass panel,
	 * whose image is the colour of its glass). A finish that is none of these carries no image, and that is the error.
	 */
	[[nodiscard]] MUSEEMATTERMAKING_API TValueOrError<FString, FString> LayerMasterPackage(const FMatterPaintLayer& Layer);

	/**
	 * Composes Instance as Layer's own layer, parented to LayerMaster, the master LayerMasterPackage names for it: the
	 * palette finish's parameters, then the work's image and size. Its identifiers derive from all of them, so the same
	 * work and records compose the same instance. Returns the problems; an empty list is success. A work with no image,
	 * no size, or a palette finish that carries no image is a problem, never a default.
	 */
	[[nodiscard]] MUSEEMATTERMAKING_API TArray<FString> ComposePaintLayer(const FMatterPaintLayer& Layer, UMaterialInterface& LayerMaster,
		UMaterialInstanceConstant& Instance);
}
