#include "Make/Make.h"

#include "Cast/Caster.h"
#include "Levels/MuseumLevel.h"
#include "Levels/StageLevel.h"
#include "Make/MakeMeasures.h"

namespace Musee::Studio
{
	namespace
	{
		/** Each stage's casts by piece name. */
		using FStageCasts = TMap<FString, FCastResult>;

		TArray<FStageCasts> CastStep(FMakePlan& Plan, FMakeReport& Report)
		{
			FMakeStepTimer Timer(Report, TEXT("cast"));
			const FDerivedDataTally Before = MeasureDerivedData();
			TArray<FCastInput> Inputs;
			TArray<TPair<int32, FString>> Owners;
			for (int32 StageIndex = 0; StageIndex < Plan.Stages.Num(); ++StageIndex)
			{
				FMakeStage& Stage = Plan.Stages[StageIndex];
				for (FMakePiece& Piece : Stage.Pieces)
				{
					FCastInput& Input = Inputs.AddDefaulted_GetRef();
					Input.PackageName = Plan.Paths.StageMesh(Stage.Name, Piece.Name);
					Input.Description = MoveTemp(Piece.Description);
					Input.Slots = Piece.Slots;
					Input.bCollides = Piece.bCollides;
					Owners.Emplace(StageIndex, Piece.Name);
				}
			}
			TArray<FCastResult> Results = CastMeshes(Inputs, Report.Packages, Report.Cast);
			TArray<FStageCasts> ByStage;
			ByStage.SetNum(Plan.Stages.Num());
			for (int32 Index = 0; Index < Results.Num(); ++Index)
			{
				ByStage[Owners[Index].Key].Add(Owners[Index].Value, MoveTemp(Results[Index]));
			}
			Report.DerivedData = MeasureDerivedData() - Before;
			return ByStage;
		}

		/** A stage's level from its placements, one component per placed part; false with the reason if a piece failed. */
		bool DescribeStageLevel(const FMakePlan& Plan, const FMakeStage& Stage, const FStageCasts& Casts, const FMakeReport& Report,
			FStageLevel& OutLevel, FString& OutWhy)
		{
			OutLevel = FStageLevel{Stage.Name, Plan.Paths.StageLevel(Stage.Name), {}};
			for (const FMakePlacement& Placement : Stage.Placements)
			{
				const FCastResult* Result = Casts.Find(Placement.Piece);
				if (!Result || Result->Meshes.IsEmpty())
				{
					OutWhy = FString::Printf(TEXT("placement %s: piece %s was not cast"), *Placement.Name, *Placement.Piece);
					return false;
				}
				for (const FCastMesh& Mesh : Result->Meshes)
				{
					if (Report.HasFailed(Mesh.Path))
					{
						OutWhy = FString::Printf(TEXT("placement %s: mesh %s failed"), *Placement.Name, *Mesh.Path.ToString());
						return false;
					}
					OutLevel.Placements.Add({Placement.Name + PartSuffix(Mesh.Kind), Mesh.Path, Placement.LocationMeters, Placement.Rotation});
				}
			}
			return true;
		}

		void StageLevelsStep(const FMakePlan& Plan, const TArray<FStageCasts>& Casts, FMakeReport& Report)
		{
			FMakeStepTimer Timer(Report, TEXT("stage levels"));
			for (int32 StageIndex = 0; StageIndex < Plan.Stages.Num(); ++StageIndex)
			{
				FStageLevel Level;
				FString Why;
				if (DescribeStageLevel(Plan, Plan.Stages[StageIndex], Casts[StageIndex], Report, Level, Why))
				{
					Report.Packages.Add(WriteStageLevel(Level));
				}
				else
				{
					Report.Packages.Add({Level.PackageName, EMadeOutcome::Failed, Why});
				}
			}
		}

		void MuseumStep(const FMakePlan& Plan, FMakeReport& Report)
		{
			FMakeStepTimer Timer(Report, TEXT("museum level"));
			FMuseumLevel Museum{Plan.Paths.MuseumLevel(), {}};
			for (const FMakeStage& Stage : Plan.Stages)
			{
				Museum.Stages.Add({Stage.Name, Plan.Paths.StageLevel(Stage.Name), Stage.TranslationMeters});
			}
			if (Report.Succeeded())
			{
				Report.Packages.Add(WriteMuseumLevel(Museum));
			}
			else
			{
				Report.Packages.Add({Museum.PackageName, EMadeOutcome::Failed, TEXT("a stage failed, so the museum keeps its last good level")});
			}
		}
	}

	FMakeReport Make(FMakePlan& Plan)
	{
		FMakeReport Report;
		const TArray<FStageCasts> Casts = CastStep(Plan, Report);
		StageLevelsStep(Plan, Casts, Report);
		MuseumStep(Plan, Report);
		Report.PeakPhysMemoryMB = MeasurePeakPhysMemoryMB();
		return Report;
	}
}
