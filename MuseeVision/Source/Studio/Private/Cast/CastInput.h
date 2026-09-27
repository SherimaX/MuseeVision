#pragma once

#include "CoreMinimal.h"
#include "MeshDescription.h"
#include "UObject/SoftObjectPath.h"

namespace Musee::Studio
{
	/** One material slot of a cast: the name the mesh gives the slot, and the material that fills it. */
	struct FCastSlot
	{
		FName Name;
		FSoftObjectPath Material;
	};

	/**
	 * One mesh to cast: a surface drawn by its owner, with polygon group i filled by Slots[i]. Positions are in engine
	 * units; every vertex instance carries the drawing's unit normal and unit tangent, and UV channel 0 in metres. The
	 * caster keeps all of it exactly as drawn.
	 */
	struct FCastInput
	{
		/** The long package name of the cast's mesh, e.g. /Game/Generated/Stages/Rotunda/SM_Rotunda_Drum. */
		FString PackageName;
		FMeshDescription Description;
		TArray<FCastSlot> Slots;
		/** Whether bodies and queries collide with it; its collision is then the mesh itself. */
		bool bCollides = true;
	};
}
