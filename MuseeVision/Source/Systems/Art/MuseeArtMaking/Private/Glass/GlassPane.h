#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::ArtMaking
{
	/**
	 * A stained-glass panel's glass as Art sets it in a stage's Glazing light (D-014: Art owns the image, Form the
	 * opening, Matter the glass): the image fired into mouth-blown glass, a sheet of the glass's thickness standing on
	 * the light's glass line. It is drawn in the panel's own frame, a Glazing anchor's axes moved to the image's
	 * lower-left corner: x along the light to the viewer's right, y out of the glass towards the viewer, z up; the
	 * sheet is centred on the glass line (y = 0).
	 */
	struct FGlassPaneSpec
	{
		/** The image's width and height, which the pane spans (metres). */
		FVector2D ImageMeters = FVector2D::ZeroVector;
	};

	/**
	 * Lamberts' genuine antique glass, the mouth-blown glass the band is leaded in, is "approx. 3 mm" thick
	 * (Data/Matter/Substances/antique-glass.json, stock.antiqueSheet.thickness; Glashuette Lamberts, "Genuine antique
	 * glass", lamberts.de). Its natural thickness runs 2 to 5 mm, which Matter's record carries in its optics.
	 */
	inline constexpr double PaneThicknessMeters = 0.003;

	/**
	 * Builds the pane as one closed solid to the Form kernel's mesh contract: its two faces carry UV0 in metres from the
	 * image's top-left corner as seen from inside (the same point of glass shows the same point of the image from either
	 * side, mirrored from outside as real glass is), and its four edges UV0 in metres along them.
	 */
	void BuildGlassPane(const FGlassPaneSpec& Pane, UE::Geometry::FDynamicMesh3& Out);
}
