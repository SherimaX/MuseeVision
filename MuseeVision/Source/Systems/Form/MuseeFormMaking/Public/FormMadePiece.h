#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "MuseeFrame.h"
#include "MuseeKey.h"

/**
 * One made piece of a stage, ready for Studio to cast: a solid of one surface or a few (a stretch of coping, a canvas,
 * the water of a pond, a clump of irises, a wall with its reveals), with the names its finish schedule gives its
 * surfaces, the moving body it rides on (if any), how it collides, and whether it renders as Nanite. The mesh follows
 * the kernel's contract (Private/Kernel/DynamicMeshOut.h): welded vertices, normals and UV-derived tangents in normal
 * layers 0-2, UV layer 0 in metres, layer 1 Matter's piece layout, and MaterialID the surface: 0 for Surface, k for
 * SlotSurfaces[k - 1].
 */
struct FFormMadePiece
{
	/** The piece's name within its stage, unique: coping.north, canvas.monet-willows-two-willows, pond.water. */
	FName Name;

	/** The surface the finish schedule names (Data/Stages/<stage>/Finishes.json): coping, lip, water, canvas.<work>. */
	FName Surface;

	/** The surfaces of the mesh's further material slots, in MaterialID order from 1: a wall's back and reveals. */
	TArray<FName> SlotSurfaces;

	/**
	 * A palette key that overrides the finish schedule for this piece, when the piece brings its own material by
	 * nature (a painting's own paint layer, a species' own leaf, the bronze of a blind's doors), with the working and
	 * history it is resolved with. Empty: the finish schedule decides.
	 */
	FName Material;
	FName Working;
	FName History;

	/** The moving body the piece rides on (nympheas.basin), or empty when the piece stands still. */
	FMuseeKey Body;

	/** The collision profile (Musee::Collision: Solid, Walkable), or NAME_None for a piece nothing collides with. */
	FName Collision;

	/** False for translucent pieces and water, which Nanite can't render. */
	bool bNanite = true;

	/** Where the piece sits, in its stage's frame (its body's frame when Body is set). */
	FMuseeFrame Frame;

	/** Who drew it: the stage, or the system that added it (Site, Nature, Art), so a finding goes to its cause. */
	FName Drawer;

	/** The geometry, in engine units (Musee::ToEngine), in the piece's own frame. */
	UE::Geometry::FDynamicMesh3 Mesh;

	/** The surface of a material slot (a triangle's MaterialID), or NAME_None past the last. */
	[[nodiscard]] FName SurfaceOfSlot(int32 Slot) const
	{
		return Slot == 0 ? Surface : (SlotSurfaces.IsValidIndex(Slot - 1) ? SlotSurfaces[Slot - 1] : NAME_None);
	}

	/** How many material slots the piece has. */
	[[nodiscard]] int32 NumSlots() const { return 1 + SlotSurfaces.Num(); }
};
