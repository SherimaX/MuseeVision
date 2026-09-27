#pragma once

#include "CoreMinimal.h"
#include "FormArris.h"
#include "Kernel/Regions.h"

namespace Musee::Form
{
	class FMeshBuilder;
	struct FProfileStations;

	/** The builder slots a worked prism is drawn in: its near face (z = 0), far face, sides and pockets. */
	struct FPrismParts
	{
		FName Near;
		FName Far;
		FName Side;
		FName Pocket;
	};

	/** A pocket sunk square into one face of a prism, with a flat floor. */
	struct FPrismPocket
	{
		/** What it takes from the face: the part of it outside the prism takes nothing. */
		FRegion Region;
		double DepthMeters = 0.0;
		/** Sunk into the near face (z = 0) rather than the far one (z = depth). */
		bool bNearFace = false;
		/** How far under the face it lies: zero opens it at the face; more opens it only at the sides it runs over. */
		double CoverMeters = 0.0;

		/** The heights it spans in the prism, bottom and top. */
		double Low(double PrismDepth) const { return bNearFace ? CoverMeters : PrismDepth - CoverMeters - DepthMeters; }
		double High(double PrismDepth) const { return bNearFace ? CoverMeters + DepthMeters : PrismDepth - CoverMeters; }
	};

	/**
	 * A prism of a region pushed DepthMeters up z (x, y and z metres, the region in (x, y)), with pockets sunk into its
	 * faces: blind pockets, rebates open to the sides (a pocket that runs over the region's edge), pockets stepped one in
	 * another, housings buried under a cover and open at a side. It is built in layers between the levels of the faces and
	 * the pockets' floors, each layer the region less the pockets that reach it; each level is closed by the faces where
	 * the layers change, and every loop is set out on the points of every other, so the solid is closed vertex for vertex.
	 * The sides of the region take Parts.Side and the pockets' walls and floors Parts.Pocket. Returns false, with the
	 * reason, for a depth that isn't positive, a pocket as deep as the prism, or a face that can't be triangulated.
	 */
	bool PocketedPrism(FMeshBuilder& Mesh, const FRegion& Base, double DepthMeters, TConstArrayView<FPrismPocket> Pockets,
		const FPrismParts& Parts, FString& OutWhy);

	/**
	 * A prism pushed DepthMeters up z from a closed outline and its holes (Loops: the outline first; any winding), with
	 * the arrises round its near and far faces eased (FFormArris): its sides are the loops' eased section run round each
	 * loop, mitred at its corners, and its faces are closed where the eases end. Blind pockets may be sunk into either
	 * face (with no cover), clear of the eased arrises and of each other. Returns false, with the reason, when the eases
	 * meet across the depth, a pocket isn't clear, or a face can't be triangulated.
	 */
	bool ArrisedPrism(FMeshBuilder& Mesh, TConstArrayView<FProfileStations> Loops, double DepthMeters, const FFormArris& Near,
		const FFormArris& Far, TConstArrayView<FPrismPocket> Pockets, const FPrismParts& Parts, FString& OutWhy);
}
