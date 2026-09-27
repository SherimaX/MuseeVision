#include "Levels/MuseumLevel.h"

#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "Levels/LevelDraft.h"
#include "Make/EngineUnits.h"
#include "Make/InputHash.h"
#include "Misc/PackageName.h"

namespace Musee::Studio
{
	namespace
	{
		/** The name of the arrival's player start in the museum level. */
		const TCHAR* const ArrivalName = TEXT("Arrival");

		/** Marks where the visit begins, and the game mode the museum plays under. */
		bool Arrive(FLevelDraft& Draft, const FMuseumLevel& Museum, FString& OutWhy)
		{
			if (Museum.GameMode)
			{
				Draft.World().GetWorldSettings()->DefaultGameMode = Museum.GameMode;
			}
			if (Museum.ArrivalTag.IsNone())
			{
				return true;
			}
			const FRotator Facing(0.0, Museum.ArrivalFacingDegrees, 0.0);
			const FTransform Placement(Facing, EngineFromMeters(Museum.ArrivalMeters));
			APlayerStart* Start = Cast<APlayerStart>(Draft.Spawn(APlayerStart::StaticClass(), ArrivalName, Placement));
			if (!Start)
			{
				OutWhy = TEXT("a stage is named Arrival, the name of the museum's arrival");
				return false;
			}
			Start->PlayerStartTag = Museum.ArrivalTag;
			return true;
		}

		/** Puts one Level Instance per stage into the draft; false with the reason when a stage level cannot be used. */
		bool Furnish(FLevelDraft& Draft, const FMuseumLevel& Museum, FString& OutWhy)
		{
			for (const FMuseumStage& Stage : Museum.Stages)
			{
				const FTransform Placement(EngineFromMeters(Stage.TranslationMeters));
				ALevelInstance* Instance = Cast<ALevelInstance>(Draft.Spawn(ALevelInstance::StaticClass(), Stage.StageName, Placement));
				if (!Instance)
				{
					OutWhy = FString::Printf(TEXT("stage %s is placed twice"), *Stage.StageName);
					return false;
				}
				// In a level without World Partition, a Level Instance can only stream its level as a level of its own.
				Instance->SetDesiredRuntimeBehavior(ELevelInstanceRuntimeBehavior::LevelStreaming);
				const FSoftObjectPath Level(Stage.LevelPackageName + TEXT(".") + FPackageName::GetShortName(Stage.LevelPackageName));
				if (!Instance->SetWorldAsset(TSoftObjectPtr<UWorld>(Level)))
				{
					OutWhy = FString::Printf(TEXT("stage %s: its level %s cannot be instanced (see LogLevelInstance)"),
						*Stage.StageName, *Level.ToString());
					return false;
				}
			}
			return Arrive(Draft, Museum, OutWhy);
		}
	}

	FString HashOf(const FMuseumLevel& Museum)
	{
		FInputHasher Hasher;
		Hasher.Add(TEXT("museum-level")).Add(MuseumLevelRevision);
		for (const FMuseumStage& Stage : Museum.Stages)
		{
			Hasher.Add(Stage.StageName).Add(Stage.LevelPackageName).Add(Stage.TranslationMeters);
		}
		Hasher.Add(Museum.ArrivalTag.ToString()).Add(Museum.ArrivalMeters).Add(Museum.ArrivalFacingDegrees);
		Hasher.Add(Museum.GameMode ? Museum.GameMode->GetPathName() : FString());
		return Hasher.Finish();
	}

	FMadePackage WriteMuseumLevel(const FMuseumLevel& Museum)
	{
		FMadePackage Made{Museum.PackageName, EMadeOutcome::Failed, FString()};
		const FString Hash = HashOf(Museum);
		if (StoredInputHash(Museum.PackageName, FPackageName::GetShortName(Museum.PackageName)) == Hash)
		{
			Made.Outcome = EMadeOutcome::Unchanged;
			return Made;
		}
		TUniquePtr<FLevelDraft> Draft = FLevelDraft::Start(Museum.PackageName, Made.Why);
		if (Draft && Furnish(*Draft, Museum, Made.Why) && Draft->Save(Hash, Made.Why))
		{
			Made.Outcome = EMadeOutcome::Written;
		}
		return Made;
	}
}
