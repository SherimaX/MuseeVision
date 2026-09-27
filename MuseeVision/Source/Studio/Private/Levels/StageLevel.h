#pragma once

#include "CoreMinimal.h"
#include "Make/MadePackage.h"
#include "UObject/SoftObjectPath.h"

namespace Musee::Studio
{
	/** One mesh placed in a stage: a static mesh component of the stage's actor, in the stage's own frame. */
	struct FStagePlacement
	{
		/** The component's name, unique within the stage. */
		FString Name;
		FSoftObjectPath Mesh;
		/** Where the mesh's origin sits in the stage's frame (x east, y south, z up, true north), in metres. */
		FVector LocationMeters = FVector::ZeroVector;
		/** How the mesh is turned in the stage's frame. */
		FQuat Rotation = FQuat::Identity;
	};

	/**
	 * A stage's level: a level without World Partition holding one actor named after the stage. The actor's root is
	 * the stage's origin, and every placement is a static mesh component under it.
	 */
	struct FStageLevel
	{
		FString StageName;
		/** The level's long package name, e.g. /Game/Generated/Stages/Rotunda/L_Rotunda. */
		FString PackageName;
		TArray<FStagePlacement> Placements;
	};

	/** The revision of the stage-level writer, part of every stage level's input hash. */
	inline constexpr int64 StageLevelRevision = 1;

	/** A stage level's input hash: the writer's revision, the stage's name and every placement. */
	FString HashOf(const FStageLevel& Stage);

	/** Writes a stage's level, unless the level on disk already carries the same input hash. */
	FMadePackage WriteStageLevel(const FStageLevel& Stage);
}
