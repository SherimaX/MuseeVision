#pragma once

#include "Canvas/WallPath.h"
#include "CoreMinimal.h"
#include "MuseeFrame.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::ArtMaking
{
	/**
	 * A stretched canvas as it hangs: linen stretched over a wooden stretcher, its painted face standing the stretcher's
	 * depth proud of the wall, the linen turned round the stretcher's sides (the tacking edges, where the tacks hold it)
	 * and across its back. On a curved wall the stretcher is curved to the wall, as the Orangerie's are.
	 */
	struct FCanvasSpec
	{
		/** Where the work starts along the wall, and its width along it (arc length on a curved wall). */
		double StartMeters = 0.0;
		double WidthMeters = 0.0;
		/** Its lower edge above the floor, and its height. */
		double BottomMeters = 0.0;
		double HeightMeters = 0.0;
		/** Whether the image reads left to right as the path runs; otherwise its left edge is at the far end. */
		bool bReadsWithPath = true;
		/** Where the canvas's left edge falls in the work's image, from the image's left edge: the canvases before it. */
		double ImageOffsetMeters = 0.0;
	};

	/**
	 * The stretcher's depth: the canvas's face stands 40 mm off the wall (the museum's stretcher; museum-grade
	 * stretcher bars run 38 to 45 mm deep).
	 */
	inline constexpr double StretcherDepthMeters = 0.04;

	/** The stretcher's back stands 2 mm off the plaster on its hanging plates, as the as-built canvases did. */
	inline constexpr double HangingGapMeters = 0.002;

	/**
	 * Builds a canvas's two surfaces, each in a piece frame given in the stage's frame, to the Form kernel's mesh
	 * contract: the painted face, its UV0 in metres from the work's image's top left corner as seen from the room; and the
	 * linen of its tacking edges and back, its UV0 in metres along the linen.
	 */
	void BuildCanvas(const FWallPath& Wall, const FCanvasSpec& Canvas, const FMuseeFrame& PieceFrame, UE::Geometry::FDynamicMesh3& OutFace,
		UE::Geometry::FDynamicMesh3& OutLinen);
}
