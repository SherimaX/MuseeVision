#pragma once

#include "CoreMinimal.h"

class AActor;

/**
 * A live bake (-Live, editor binaries only): an actor's procedural components turned into transient static meshes with
 * distance fields and Lumen surface cards, so a wing rebuilt live (UMuseeLiveGeometry) is lit as its cooked bake is.
 * Procedural meshes give Lumen no cards: after a live rebuild the bounce light under the roofs and indoors was gone (a
 * hall's interior at half its brightness, the rockery's hollows black). Nothing is saved; no Nanite (the shapes and
 * the light, not the cost, are what a round checks). The procedural components stay for collision, hidden.
 */
namespace MuseeLiveBake
{
	/** Bakes the actor's visible procedural components; returns how many meshes were made (0 in a non-editor build). */
	int32 Bake(AActor* Actor);

	/** Drops a live bake: its static mesh components go, the procedural ones are shown again. */
	void Unbake(AActor* Actor);
}
