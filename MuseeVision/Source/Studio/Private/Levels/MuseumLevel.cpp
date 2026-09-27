#include "Levels/MuseumLevel.h"

#include "Engine/World.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "Levels/LevelDraft.h"
#include "Make/EngineUnits.h"
#include "Make/InputHash.h"
#include "Misc/PackageName.h"

namespace Musee::Studio
{
	namespace
	{
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
			return true;
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
