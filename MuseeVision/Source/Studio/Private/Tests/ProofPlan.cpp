#include "Tests/ProofPlan.h"

#include "Tests/ProofPieces.h"

namespace Musee::Studio
{
	namespace
	{
		const TCHAR* const ProofRoot = TEXT("/Game/Generated/Studio");
		const TCHAR* const ProofStage = TEXT("Proof");

		/** The engine's stock opaque surface and its stock translucent one. */
		const TCHAR* const OpaqueMaterial = TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial");
		const TCHAR* const TranslucentMaterial = TEXT("/Engine/EngineDebugMaterials/M_SimpleTranslucent.M_SimpleTranslucent");

		/** The columns stand 1.2 m south of the wall's centreline, 2.6 m either side of its middle. */
		constexpr double ColumnOffsetEastMeters = 2.6;
		constexpr double ColumnOffsetSouthMeters = 1.2;

		/** Where the proof's museum level places the stage's origin. */
		const FVector StageTranslationMeters(24.0, -16.0, 0.0);
	}

	FMakePaths ProofPaths()
	{
		return FMakePaths{ProofRoot};
	}

	FMakePlan ProofPlan(const FProofShape& Shape)
	{
		FMakeStage Stage;
		Stage.Name = ProofStage;
		Stage.TranslationMeters = StageTranslationMeters;

		FMakePiece& Column = Stage.Pieces.AddDefaulted_GetRef();
		Column.Name = TEXT("Column");
		Column.Description = TurnColumn(Shape.ColumnHeightMeters);
		Column.Slots = {{TEXT("Stone"), FSoftObjectPath(OpaqueMaterial)}};

		FMakePiece& Wall = Stage.Pieces.AddDefaulted_GetRef();
		Wall.Name = TEXT("Wall");
		Wall.Description = BuildGlazedWall();
		Wall.Slots = {{TEXT("Stone"), FSoftObjectPath(OpaqueMaterial)}, {TEXT("Glass"), FSoftObjectPath(TranslucentMaterial)}};

		Stage.Placements.Add({TEXT("ColumnWest"), TEXT("Column"), FVector(-ColumnOffsetEastMeters, ColumnOffsetSouthMeters, 0.0), FQuat::Identity});
		Stage.Placements.Add({TEXT("ColumnEast"), TEXT("Column"), FVector(ColumnOffsetEastMeters, ColumnOffsetSouthMeters, 0.0), FQuat::Identity});
		Stage.Placements.Add({TEXT("Wall"), TEXT("Wall"), FVector::ZeroVector, FQuat::Identity});

		FMakePlan Plan;
		Plan.Paths = ProofPaths();
		Plan.Stages.Add(MoveTemp(Stage));
		return Plan;
	}
}
