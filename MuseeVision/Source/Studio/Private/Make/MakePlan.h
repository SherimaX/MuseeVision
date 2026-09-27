#pragma once

#include "Cast/CastInput.h"
#include "CoreMinimal.h"
#include "Make/MakePaths.h"
#include "MeshDescription.h"

namespace Musee::Studio
{
	/** A piece a stage casts: its surface, drawn in the piece's own frame, and the materials of its slots. */
	struct FMakePiece
	{
		/** The piece's name within its stage; its mesh is SM_<Stage>_<Name>. */
		FString Name;
		FMeshDescription Description;
		TArray<FCastSlot> Slots;
		bool bCollides = true;
	};

	/** One placement of a piece in its stage's frame, in metres. */
	struct FMakePlacement
	{
		/** The placement's name within its stage; it names the placed components. */
		FString Name;
		FString Piece;
		FVector LocationMeters = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
	};

	/** A stage to make: its pieces, where they stand in its frame, and where the museum places its origin. */
	struct FMakeStage
	{
		FString Name;
		TArray<FMakePiece> Pieces;
		TArray<FMakePlacement> Placements;
		FVector TranslationMeters = FVector::ZeroVector;
	};

	/** Everything a make makes, and where. */
	struct FMakePlan
	{
		FMakePaths Paths;
		TArray<FMakeStage> Stages;
	};
}
