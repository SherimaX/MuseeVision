#pragma once

#include "CoreMinimal.h"
#include "Make/MadePackage.h"

namespace Musee::Studio
{
	/** A stage as the museum places it: its level, moved by a translation. The site plan never rotates a stage. */
	struct FMuseumStage
	{
		FString StageName;
		/** The stage level's long package name, e.g. /Game/Generated/Stages/Rotunda/L_Rotunda. */
		FString LevelPackageName;
		/** Where the stage's origin sits in the museum's frame, in metres. */
		FVector TranslationMeters = FVector::ZeroVector;
	};

	/**
	 * The museum's level: a level without World Partition holding one Level Instance per stage, each streaming its
	 * stage level at the stage's translation.
	 */
	struct FMuseumLevel
	{
		/** The level's long package name, e.g. /Game/Generated/Site/L_Museum. */
		FString PackageName;
		TArray<FMuseumStage> Stages;

		/** Where the visit begins, in the museum's frame: a player start with this tag, at this point, facing this way. */
		FName ArrivalTag;
		FVector ArrivalMeters = FVector::ZeroVector;
		double ArrivalFacingDegrees = 0.0;

		/** The game mode the level plays under, or null for the project's default. */
		UClass* GameMode = nullptr;
	};

	/** The revision of the museum-level writer, part of the museum level's input hash. */
	inline constexpr int64 MuseumLevelRevision = 2;

	/** The museum level's input hash: the writer's revision, every stage's level and translation, the arrival and the game mode. */
	FString HashOf(const FMuseumLevel& Museum);

	/** Writes the museum level, unless the level on disk already carries the same input hash. Stage levels must exist. */
	FMadePackage WriteMuseumLevel(const FMuseumLevel& Museum);
}
