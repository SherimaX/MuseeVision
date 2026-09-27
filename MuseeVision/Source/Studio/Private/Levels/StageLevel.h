#pragma once

#include "CoreMinimal.h"
#include "Make/MadePackage.h"
#include "UObject/SoftObjectPath.h"

class AActor;

namespace Musee::Studio
{
	/**
	 * A moving body of a stage: a scene component tagged with the body's key, which Motion drives and pieces ride on. A
	 * body may ride another (the Atrium car's leaves ride the car), and then hangs from that body's component.
	 */
	struct FStageBody
	{
		/** The component's name, unique within the stage. */
		FString Name;
		/** The body's key, the component's tag: nympheas.basin. */
		FName Key;
		/** The body it rides on (another FStageBody's Name, listed before it), or empty for the stage's frame. */
		FString Parent;
		/** The body's frame in its parent's frame (the parent body's, or the stage's), metres. */
		FVector LocationMeters = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
	};

	/** One copy of an instanced mesh: where its origin sits in the placement's parent's frame, metres, and how it is turned. */
	struct FStageInstance
	{
		FVector LocationMeters = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
	};

	/** One mesh placed in a stage: a static mesh component of the stage's actor, on its body or on the stage's frame. */
	struct FStagePlacement
	{
		/** The component's name, unique within the stage. */
		FString Name;
		FSoftObjectPath Mesh;
		/** Where the mesh's origin sits in its parent's frame (x east, y south, z up, true north), in metres. */
		FVector LocationMeters = FVector::ZeroVector;
		/** How the mesh is turned in its parent's frame. */
		FQuat Rotation = FQuat::Identity;
		/** The body it rides on (FStageBody::Name), or empty for the stage's frame. */
		FString Body;
		/** The component's collision profile, or NAME_None to keep the engine's default. */
		FName Collision;
		/** The component's tags (musee.piece:<name>), one for each piece it places, in the order of its instances. */
		TArray<FName> Tags;
		/**
		 * The copies of one mesh that identical pieces share (the Salon's rosettes): the component is then an instanced
		 * static mesh component at the placement's frame, drawing the mesh once per instance. Empty: a single mesh.
		 */
		TArray<FStageInstance> Instances;
	};

	/**
	 * A stage's level: a level without World Partition holding one actor named after the stage. The actor's root is
	 * the stage's origin; its bodies hang from the root, and every placement is a static mesh component on its body or
	 * on the root. Once furnished, Dress (when set) lets the systems add their components before the level is saved.
	 */
	struct FStageLevel
	{
		FString StageName;
		/** The level's long package name, e.g. /Game/Generated/Stages/Rotunda/L_Rotunda. */
		FString PackageName;
		TArray<FStageBody> Bodies;
		TArray<FStagePlacement> Placements;
		/** The hash of everything else the level is made from: the records its dressing reads. Part of its input hash. */
		FString DependencyHash;
		/** Furnishes the stage actor further (DressStage); false, with the reason, when it can't. */
		TFunction<bool(AActor& Stage, FString& OutWhy)> Dress;
	};

	/** The revision of the stage-level writer, part of every stage level's input hash. */
	inline constexpr int64 StageLevelRevision = 3;

	/** A stage level's input hash: the writer's revision, the stage's name, its bodies, placements, instances and dependencies. */
	FString HashOf(const FStageLevel& Stage);

	/** Writes a stage's level, unless the level on disk already carries the same input hash. */
	FMadePackage WriteStageLevel(const FStageLevel& Stage);
}
