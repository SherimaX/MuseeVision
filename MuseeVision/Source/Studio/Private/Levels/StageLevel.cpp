#include "Levels/StageLevel.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Levels/LevelDraft.h"
#include "Make/EngineUnits.h"
#include "Make/InputHash.h"
#include "Misc/PackageName.h"

namespace Musee::Studio
{
	namespace
	{
		/** The name of the component at a stage actor's root: the stage's frame. */
		const TCHAR* const FrameName = TEXT("Frame");

		/** Adds a static component the actor saves with itself (an instance component, not one its class makes). */
		template <typename TComponent>
		TComponent* AddComponent(AActor& Actor, const FString& Name)
		{
			TComponent* Component = NewObject<TComponent>(&Actor, FName(*Name));
			Component->CreationMethod = EComponentCreationMethod::Instance;
			Component->SetMobility(EComponentMobility::Static);
			Actor.AddInstanceComponent(Component);
			return Component;
		}

		/** Puts the stage's actor and every placement into the draft; false with the reason when one cannot be made. */
		bool Furnish(FLevelDraft& Draft, const FStageLevel& Stage, FString& OutWhy)
		{
			AActor* Actor = Draft.Spawn(AActor::StaticClass(), Stage.StageName, FTransform::Identity);
			if (!Actor)
			{
				OutWhy = FString::Printf(TEXT("the stage's name %s is taken by an actor every level has"), *Stage.StageName);
				return false;
			}
			USceneComponent* Frame = AddComponent<USceneComponent>(*Actor, FrameName);
			Actor->SetRootComponent(Frame);
			for (const FStagePlacement& Placement : Stage.Placements)
			{
				if (StaticFindObjectFast(nullptr, Actor, FName(*Placement.Name)))
				{
					OutWhy = FString::Printf(TEXT("placement %s: the stage already has a component of that name"), *Placement.Name);
					return false;
				}
				UStaticMesh* Mesh = Cast<UStaticMesh>(Placement.Mesh.TryLoad());
				if (!Mesh)
				{
					OutWhy = FString::Printf(TEXT("placement %s: no mesh at %s"), *Placement.Name, *Placement.Mesh.ToString());
					return false;
				}
				UStaticMeshComponent* Component = AddComponent<UStaticMeshComponent>(*Actor, Placement.Name);
				Component->SetStaticMesh(Mesh);
				Component->SetupAttachment(Frame);
				Component->SetRelativeTransform(FTransform(Placement.Rotation, EngineFromMeters(Placement.LocationMeters)));
			}
			return true;
		}
	}

	FString HashOf(const FStageLevel& Stage)
	{
		FInputHasher Hasher;
		Hasher.Add(TEXT("stage-level")).Add(StageLevelRevision).Add(Stage.StageName);
		for (const FStagePlacement& Placement : Stage.Placements)
		{
			Hasher.Add(Placement.Name).Add(Placement.Mesh.ToString()).Add(Placement.LocationMeters).Add(Placement.Rotation);
		}
		return Hasher.Finish();
	}

	FMadePackage WriteStageLevel(const FStageLevel& Stage)
	{
		FMadePackage Made{Stage.PackageName, EMadeOutcome::Failed, FString()};
		const FString Hash = HashOf(Stage);
		if (StoredInputHash(Stage.PackageName, FPackageName::GetShortName(Stage.PackageName)) == Hash)
		{
			Made.Outcome = EMadeOutcome::Unchanged;
			return Made;
		}
		TUniquePtr<FLevelDraft> Draft = FLevelDraft::Start(Stage.PackageName, Made.Why);
		if (Draft && Furnish(*Draft, Stage, Made.Why) && Draft->Save(Hash, Made.Why))
		{
			Made.Outcome = EMadeOutcome::Written;
		}
		return Made;
	}
}
