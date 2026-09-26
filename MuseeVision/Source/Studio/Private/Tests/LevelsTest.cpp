#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "Make/EngineUnits.h"
#include "Make/Make.h"
#include "Make/PackageSave.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Tests/ProofPlan.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Studio
{
	namespace
	{
		/** A made level loaded from disk, as the editor would open it, and let go of afterwards. */
		class FLoadedLevel
		{
		public:
			explicit FLoadedLevel(const FString& PackageName)
				: Package(LoadPackage(nullptr, *PackageName, LOAD_None))
				, World(Package ? UWorld::FindWorldInPackage(Package) : nullptr)
			{
			}
			~FLoadedLevel() { ReleasePackages({Package}); }

			AActor* FindActor(const FString& Name) const
			{
				for (AActor* Actor : World->PersistentLevel->Actors)
				{
					if (Actor && Actor->GetName() == Name) { return Actor; }
				}
				return nullptr;
			}

			UPackage* Package;
			UWorld* World;
		};

		UStaticMeshComponent* FindComponent(AActor& Actor, const TCHAR* Name)
		{
			for (UActorComponent* Component : Actor.GetInstanceComponents())
			{
				if (Component && Component->GetName() == Name) { return Cast<UStaticMeshComponent>(Component); }
			}
			return nullptr;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioLevelsTest, "Musee.Studio.Levels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStudioLevelsTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;

	FMakePlan Plan = ProofPlan();
	const FMakeStage& Stage = Plan.Stages[0];
	const FMakePlacement ColumnWest = Stage.Placements[0];
	const FVector Translation = Stage.TranslationMeters;
	if (!TestTrue(TEXT("the proof is made"), Make(Plan).Succeeded()))
	{
		return false;
	}
	{
		FLoadedLevel Level(ProofPaths().StageLevel(TEXT("Proof")));
		if (!TestNotNull(TEXT("the stage level loads"), Level.World))
		{
			return false;
		}
		TestFalse(TEXT("the stage level has no World Partition"), Level.World->IsPartitionedWorld());
		AActor* Actor = Level.FindActor(TEXT("Proof"));
		if (!TestNotNull(TEXT("the stage has one actor named for it"), Actor))
		{
			return false;
		}
		TestEqual(TEXT("its root is the stage's frame, at the origin"), Actor->GetRootComponent()->GetRelativeLocation(), FVector::ZeroVector);
		UStaticMeshComponent* West = FindComponent(*Actor, *ColumnWest.Name);
		UStaticMeshComponent* Pane = FindComponent(*Actor, TEXT("Wall_Translucent"));
		if (!TestTrue(TEXT("each placed part is a component"), West && Pane && FindComponent(*Actor, TEXT("Wall")) && FindComponent(*Actor, TEXT("ColumnEast"))))
		{
			return false;
		}
		TestEqual(TEXT("a placement stands where the stage's frame says, in engine units"),
			West->GetRelativeLocation(), EngineFromMeters(ColumnWest.LocationMeters));
		TestEqual(TEXT("it places its piece's mesh"), FSoftObjectPath(West->GetStaticMesh()).GetLongPackageName(),
			ProofPaths().StageMesh(TEXT("Proof"), TEXT("Column")));
		TestTrue(TEXT("placements are static"), West->Mobility == EComponentMobility::Static);
	}
	{
		FLoadedLevel Museum(ProofPaths().MuseumLevel());
		if (!TestNotNull(TEXT("the museum level loads"), Museum.World))
		{
			return false;
		}
		TestFalse(TEXT("the museum level has no World Partition"), Museum.World->IsPartitionedWorld());
		ALevelInstance* Instance = Cast<ALevelInstance>(Museum.FindActor(TEXT("Proof")));
		if (!TestNotNull(TEXT("the stage is a Level Instance"), Instance))
		{
			return false;
		}
		const FString StageLevel = ProofPaths().StageLevel(TEXT("Proof"));
		TestEqual(TEXT("it instances the stage level"), Instance->GetWorldAsset().ToSoftObjectPath().GetLongPackageName(), StageLevel);
		TestTrue(TEXT("it streams its level"), Instance->GetDesiredRuntimeBehavior() == ELevelInstanceRuntimeBehavior::LevelStreaming);
		TestEqual(TEXT("it stands at the stage's translation"), Instance->GetRootComponent()->GetRelativeLocation(), EngineFromMeters(Translation));
		TestTrue(TEXT("it is not rotated"), Instance->GetRootComponent()->GetRelativeRotation().IsZero());
	}
	return true;
}

#endif
