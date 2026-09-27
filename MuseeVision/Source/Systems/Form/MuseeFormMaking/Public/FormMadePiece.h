#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "MuseeFrame.h"
#include "MuseeKey.h"

/**
 * One made piece of a stage, ready for Studio to cast: a solid of one surface (a stretch of coping, a canvas, the water
 * of a pond, a clump of irises), with the name its finish schedule gives it, the moving body it rides on (if any), how
 * it collides, and whether it renders as Nanite. The mesh follows the kernel's contract (Private/Kernel/DynamicMeshOut.h):
 * welded vertices, normals and UV-derived tangents in normal layers 0-2, UV layer 0 in metres, layer 1 Matter's piece
 * layout, MaterialID the slot.
 */
struct FFormMadePiece
{
	/** The piece's name within its stage, unique: coping.north, canvas.monet-willows-two-willows, pond.water. */
	FName Name;

	/** The surface the finish schedule names (Data/Stages/<stage>/Finishes.json): coping, lip, water, canvas.<work>. */
	FName Surface;

	/**
	 * A palette key that overrides the finish schedule for this piece, when the piece brings its own material by
	 * nature (a painting's own paint layer, a species' own leaf). Empty: the finish schedule decides.
	 */
	FName Material;

	/** The moving body the piece rides on (nympheas.basin), or empty when the piece stands still. */
	FMuseeKey Body;

	/** The collision profile (Musee::Collision: Solid, Walkable), or NAME_None for a piece nothing collides with. */
	FName Collision;

	/** False for translucent pieces and water, which Nanite can't render. */
	bool bNanite = true;

	/** Where the piece sits, in its stage's frame (its body's frame when Body is set). */
	FMuseeFrame Frame;

	/** The geometry, in engine units (Musee::ToEngine), in the piece's own frame. */
	UE::Geometry::FDynamicMesh3 Mesh;
};
