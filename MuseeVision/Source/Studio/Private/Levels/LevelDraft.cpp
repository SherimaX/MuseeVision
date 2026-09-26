#include "Levels/LevelDraft.h"

#include "Engine/Engine.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Make/InputHash.h"
#include "Make/PackageSave.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace Musee::Studio
{
	namespace
	{
		/** Why a level already in memory cannot be replaced (someone is using its world); empty when it can. */
		FString WhyInUse(UPackage& Existing)
		{
			UWorld* World = UWorld::FindWorldInPackage(&Existing);
			if (!World)
			{
				return FString();
			}
			bool bInUse = World->bIsWorldInitialized || (World->PersistentLevel && World->PersistentLevel->OwningWorld != World);
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				bInUse |= Context.World() == World;
			}
			return bInUse ? FString::Printf(TEXT("%s is open in the editor or in play; open another level and make again"), *Existing.GetName())
						  : FString();
		}

		/** A world with nothing running in it: no scene, physics, navigation, audio or World Partition. */
		UWorld* CreateDraftWorld(UPackage& Package)
		{
			const FWorldInitializationValues Values = FWorldInitializationValues()
				.InitializeScenes(false)
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.EnableTraceCollision(false)
				.SetTransactional(false)
				.CreateFXSystem(false)
				.CreateWorldPartition(false)
				.EnableWorldPartitionStreaming(false);
			const FName Name(*FPackageName::GetShortName(&Package));
			UWorld* World = UWorld::CreateWorld(EWorldType::Inactive, /*bInformEngineOfWorld*/ false, Name, &Package,
				/*bAddToRoot*/ false, ERHIFeatureLevel::Num, &Values);
			World->SetFlags(RF_Public | RF_Standalone);
			// A new level's build-data ID is random (ULevel::PostInitProperties). It names baked lighting, which the museum
			// never has (Lumen lights it), so it is derived from the level's name and every write of the level agrees.
			World->PersistentLevel->LevelBuildDataId = FGuid::NewDeterministicGuid(Package.GetName());
			return World;
		}
	}

	FLevelDraft::FLevelDraft(UPackage* InPackage, UWorld* InWorld)
		: Package(InPackage)
		, DraftWorld(InWorld)
	{
	}

	TUniquePtr<FLevelDraft> FLevelDraft::Start(const FString& PackageName, FString& OutWhy)
	{
		if (UPackage* Existing = FindPackage(nullptr, *PackageName))
		{
			OutWhy = WhyInUse(*Existing);
			if (!OutWhy.IsEmpty())
			{
				return nullptr;
			}
			ReleasePackages({Existing});
			if (UPackage* Held = FindPackage(nullptr, *PackageName); Held && UWorld::FindWorldInPackage(Held))
			{
				OutWhy = FString::Printf(TEXT("%s could not be unloaded: something still holds its world"), *PackageName);
				return nullptr;
			}
		}
		UPackage* Package = CreateMadePackage(PackageName);
		return TUniquePtr<FLevelDraft>(new FLevelDraft(Package, CreateDraftWorld(*Package)));
	}

	FLevelDraft::~FLevelDraft()
	{
		DraftWorld->DestroyWorld(/*bInformEngineOfWorld*/ false);
		ReleasePackages({Package});
	}

	AActor* FLevelDraft::Spawn(UClass* Class, const FString& Name, const FTransform& Transform)
	{
		if (StaticFindObjectFast(nullptr, DraftWorld->PersistentLevel, FName(*Name)))
		{
			return nullptr;
		}
		FActorSpawnParameters Parameters;
		Parameters.Name = FName(*Name);
		Parameters.OverrideLevel = DraftWorld->PersistentLevel;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Parameters.bNoFail = true;
		Parameters.OverrideActorGuid = FGuid::NewDeterministicGuid(Package->GetName() + TEXT(".") + Name);
		AActor* Actor = DraftWorld->SpawnActor(Class, &Transform, Parameters);
		Actor->SetActorLabel(Name);
		return Actor;
	}

	bool FLevelDraft::Save(const FString& InputHash, FString& OutWhy)
	{
		StoreInputHash(*DraftWorld, InputHash);
		return SaveMadePackage(*DraftWorld, OutWhy);
	}
}
