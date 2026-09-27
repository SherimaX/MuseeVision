#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::ArtMaking
{
	/**
	 * A flat sheet a work is made on or seen through: a print or a stereograph's card, a daguerreotype's plate, a pane of
	 * glass. It is drawn in its own frame, the frame of the wall, reader or light it stands at moved to its lower-left
	 * corner: x to the viewer's right, y out of its face towards the viewer, z up the sheet. Its front face lies at
	 * y = FrontMeters, its back the thickness behind it.
	 */
	struct FSheetSpec
	{
		/** Its width and height (metres). */
		FVector2D SizeMeters = FVector2D::ZeroVector;
		double ThicknessMeters = 0.0;
		double FrontMeters = 0.0;
		/** Where its left edge falls in the image laid over it, from the image's left edge (the recto of an opening shown whole). */
		double ImageLeftMeters = 0.0;
		/** Where its top edge falls in the image, below the image's top edge (an image laid to cover a sheet and cut at its head). */
		double ImageTopMeters = 0.0;
	};

	/**
	 * Builds the sheet as one closed solid to the Form kernel's mesh contract: its two faces carry UV0 in metres from the
	 * top-left corner of the image laid over it as seen from the front (so the image spans it at its size, its left edge
	 * ImageLeftMeters and its top edge ImageTopMeters into the image; the back sees the same point of it, mirrored, as through glass), and its four edges
	 * UV0 in metres along them.
	 */
	void BuildSheet(const FSheetSpec& Sheet, UE::Geometry::FDynamicMesh3& Out);
}
