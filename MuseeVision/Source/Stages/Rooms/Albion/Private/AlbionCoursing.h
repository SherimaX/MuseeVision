#pragma once

#include "AlbionParts.h"
#include "CoreMinimal.h"

/**
 * The court's banded masonry (§3.1, BandCuts): buff Bath stone to 0.90, then a course of red Mansfield stone 0.30 high
 * every 1.20. Every wall is laid as two leaves meeting on the glass line, the court's leaf inside and the weathered leaf
 * outside, and each leaf is cut where its colour changes, each band one piece of its stone, pierced or notched by the
 * openings that cross it and following their outlines exactly.
 */
namespace Musee::Albion
{
	/** A rectangle of a leaf, in the face's (u, z), and which side of the building its face looks to. */
	struct FLeafBlock
	{
		double U0 = 0.0;
		double U1 = 0.0;
		double Z0 = 0.0;
		double Z1 = 0.0;
		/** True for the court's and the porch's faces (albion.wall.*), false for the weathered outside (albion.ext.*). */
		bool bInside = true;
	};

	/** One leaf of a wall: its face's frame (u and z in the frame's x and y, its depth pushed along the frame's z). */
	struct FLeaf
	{
		/** The pieces' names start with it: wall.west.inner. */
		FString Name;
		FMuseeFrame Frame;
		double DepthMeters = 0.0;
		TArray<FLeafBlock> Blocks;
		/** The openings through the leaf, each within one block. */
		TArray<FArch> Openings;
	};

	/** Whether a height lies in a red course. */
	bool IsRed(double Z);

	/** The leaf's pieces, band by band and block by block, into the drawings. */
	void LayLeaf(const FLeaf& Leaf, TArray<FFormDrawing>& Out);
}
