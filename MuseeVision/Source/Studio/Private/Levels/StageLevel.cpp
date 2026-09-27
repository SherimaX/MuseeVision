#include "Levels/StageLevel.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Levels/LevelDraft.h"
#include "Make/EngineUnits.h"
#include "Make/InputHash.h"
#include "Misc/PackageName.h"
#include "MuseeCollision.h"

namespace Musee::Studio
{
	namespace
	{
		/** The name of the component at a stage actor's root: the stage's frame. */
		const TCHAR* const FrameName = TEXT("Frame");

		/** Adds a component the actor saves with itself (an instance component, not one its class makes). */
		template <typename TComponent>
		TComponent* AddComponent(AActor& Actor, const FString& Name, EComponentMobility::Type Mobility)
		{
			TComponent* Component = NewObject<TComponent>(&Actor, FName(*Name));
			Component->CreationMethod = EComponentCreationMethod::Instance;
			Component->SetMobility(Mobility);
			Actor.AddInstanceComponent(Component);
			return Component;
		}

		bool NameTaken(AActor& Actor, const FString& Name, FString& OutWhy)
		{
			if (StaticFindObjectFast(nullptr, &Actor, FName(*Name)))
			{
				OutWhy = FString::Printf(TEXT("%s: the stage already has a component of that name"), *Name);
				return true;
			}
			return false;
		}

		/**
		 * Hangs every body from the body it rides or from the stage's frame; a body moves, so it and all it carries are
		 * movable.
		 */
		bool AddBodies(AActor& Actor, USceneComponent& Frame, const FStageLevel& Stage, TMap<FString, USceneComponent*>& OutBodies,
			FString& OutWhy)
		{
			for (const FStageBody& Body : Stage.Bodies)
			{
				if (NameTaken(Actor, Body.Name, OutWhy))
				{
					return false;
				}
				USceneComponent* const* Parent = Body.Parent.IsEmpty() ? nullptr : OutBodies.Find(Body.Parent);
				if (!Body.Parent.IsEmpty() && !Parent)
				{
					OutWhy = FString::Printf(TEXT("body %s rides on %s, which isn't listed before it"), *Body.Name, *Body.Parent);
					return false;
				}
				USceneComponent* Component = AddComponent<USceneComponent>(Actor, Body.Name, EComponentMobility::Movable);
				Component->ComponentTags.Add(Body.Key);
				Component->SetupAttachment(Parent ? *Parent : &Frame);
				Component->SetRelativeTransform(FTransform(Body.Rotation, EngineFromMeters(Body.LocationMeters)));
				OutBodies.Add(Body.Name, Component);
			}
			return true;
		}

		bool AddPlacement(AActor& Actor, USceneComponent& Frame, const TMap<FString, USceneComponent*>& Bodies,
			const FStagePlacement& Placement, FString& OutWhy)
		{
			if (NameTaken(Actor, Placement.Name, OutWhy))
			{
				return false;
			}
			USceneComponent* const* Body = Placement.Body.IsEmpty() ? nullptr : Bodies.Find(Placement.Body);
			if (!Placement.Body.IsEmpty() && !Body)
			{
				OutWhy = FString::Printf(TEXT("placement %s rides on %s, which the stage doesn't have"), *Placement.Name, *Placement.Body);
				return false;
			}
			UStaticMesh* Mesh = Cast<UStaticMesh>(Placement.Mesh.TryLoad());
			if (!Mesh)
			{
				OutWhy = FString::Printf(TEXT("placement %s: no mesh at %s"), *Placement.Name, *Placement.Mesh.ToString());
				return false;
			}
			const EComponentMobility::Type Mobility = Body ? EComponentMobility::Movable : EComponentMobility::Static;
			UStaticMeshComponent* Component = Placement.Instances.IsEmpty()
				? AddComponent<UStaticMeshComponent>(Actor, Placement.Name, Mobility)
				: AddComponent<UInstancedStaticMeshComponent>(Actor, Placement.Name, Mobility);
			Component->SetStaticMesh(Mesh);
			Component->SetupAttachment(Body ? *Body : &Frame);
			Component->SetRelativeTransform(FTransform(Placement.Rotation, EngineFromMeters(Placement.LocationMeters)));
			if (UInstancedStaticMeshComponent* Instanced = Cast<UInstancedStaticMeshComponent>(Component))
			{
				TArray<FTransform> Copies;
				for (const FStageInstance& Instance : Placement.Instances)
				{
					Copies.Emplace(Instance.Rotation, EngineFromMeters(Instance.LocationMeters));
				}
				Instanced->AddInstances(Copies, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ false);
			}
			Component->ComponentTags.Append(Placement.Tags);
			if (!Placement.Collision.IsNone())
			{
				Component->SetCollisionProfileName(Placement.Collision);
			}
			return true;
		}

		/** A barrier's box: unseen, stopping only the body. */
		bool AddBarrier(AActor& Actor, USceneComponent& Frame, const FStageBarrier& Barrier, FString& OutWhy)
		{
			if (NameTaken(Actor, Barrier.Name, OutWhy))
			{
				return false;
			}
			UBoxComponent* Box = AddComponent<UBoxComponent>(Actor, Barrier.Name, EComponentMobility::Static);
			Box->SetupAttachment(&Frame);
			Box->SetRelativeTransform(FTransform(Barrier.Rotation, EngineFromMeters(Barrier.LocationMeters)));
			Box->SetBoxExtent(EngineFromMeters(Barrier.HalfSizeMeters), /*bUpdateOverlaps*/ false);
			Box->SetCollisionProfileName(Musee::Collision::Barrier.Profile());
			Box->SetHiddenInGame(true);
			Box->ComponentTags.Add(Barrier.Key);
			return true;
		}

		/**
		 * Hangs every component on its parent and makes its place in the world current, as registering it would. The
		 * draft's components are not registered (nothing runs in its world), so until now a parent doesn't list what hangs
		 * from it; a dresser that asks what a body carries and where (Motion: whether a control lies on a moving body's
		 * parts) must find the body's parts on it, where the level will hold them.
		 */
		void Place(AActor& Actor)
		{
			TInlineComponentArray<USceneComponent*> Components(&Actor);
			for (USceneComponent* Component : Components)
			{
				USceneComponent* Parent = Component->GetAttachParent();
				if (Parent && !Parent->GetAttachChildren().Contains(Component))
				{
					Component->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform, Component->GetAttachSocketName());
				}
			}
			for (USceneComponent* Component : Components)
			{
				Component->UpdateComponentToWorld();
			}
		}

		/**
		 * Puts the stage's actor, its bodies, every placement and every barrier into the draft, places them in its world,
		 * then dresses it.
		 */
		bool Furnish(FLevelDraft& Draft, const FStageLevel& Stage, FString& OutWhy)
		{
			AActor* Actor = Draft.Spawn(AActor::StaticClass(), Stage.StageName, FTransform::Identity);
			if (!Actor)
			{
				OutWhy = FString::Printf(TEXT("the stage's name %s is taken by an actor every level has"), *Stage.StageName);
				return false;
			}
			USceneComponent* Frame = AddComponent<USceneComponent>(*Actor, FrameName, EComponentMobility::Static);
			Actor->SetRootComponent(Frame);
			TMap<FString, USceneComponent*> Bodies;
			if (!AddBodies(*Actor, *Frame, Stage, Bodies, OutWhy))
			{
				return false;
			}
			for (const FStagePlacement& Placement : Stage.Placements)
			{
				if (!AddPlacement(*Actor, *Frame, Bodies, Placement, OutWhy))
				{
					return false;
				}
			}
			for (const FStageBarrier& Barrier : Stage.Barriers)
			{
				if (!AddBarrier(*Actor, *Frame, Barrier, OutWhy))
				{
					return false;
				}
			}
			Place(*Actor);
			return !Stage.Dress || Stage.Dress(*Actor, OutWhy);
		}

		/** Where a stage whose level is current is furnished again to be dressed: a transient package, never saved. */
		const TCHAR* const RehearsalRoot = TEXT("/Temp/Musee/Dressing/");

		/**
		 * Dresses a stage whose level on disk is already current, for what its dressers find. The systems report their
		 * findings as they place their components (a luminaire that can't be placed, a work without its label), so a make
		 * that writes nothing still furnishes the stage in a draft of its own, lets the systems dress it, and lets the draft
		 * go unsaved: every make of the same inputs reports the same findings.
		 */
		bool Rehearse(const FStageLevel& Stage, FString& OutWhy)
		{
			if (!Stage.Dress)
			{
				return true;
			}
			TUniquePtr<FLevelDraft> Draft = FLevelDraft::Start(FString(RehearsalRoot) + FPackageName::GetShortName(Stage.PackageName), OutWhy);
			return Draft && Furnish(*Draft, Stage, OutWhy);
		}
	}

	FString HashOf(const FStageLevel& Stage)
	{
		FInputHasher Hasher;
		Hasher.Add(TEXT("stage-level")).Add(StageLevelRevision).Add(Stage.StageName).Add(Stage.DependencyHash);
		for (const FStageBody& Body : Stage.Bodies)
		{
			Hasher.Add(Body.Name).Add(Body.Key.ToString()).Add(Body.Parent).Add(Body.LocationMeters).Add(Body.Rotation);
		}
		for (const FStagePlacement& Placement : Stage.Placements)
		{
			Hasher.Add(Placement.Name).Add(Placement.Mesh.ToString()).Add(Placement.LocationMeters).Add(Placement.Rotation)
				.Add(Placement.Body).Add(Placement.Collision.ToString());
			for (const FName Tag : Placement.Tags)
			{
				Hasher.Add(Tag.ToString());
			}
			Hasher.Add(static_cast<int64>(Placement.Instances.Num()));
			for (const FStageInstance& Instance : Placement.Instances)
			{
				Hasher.Add(Instance.LocationMeters).Add(Instance.Rotation);
			}
		}
		for (const FStageBarrier& Barrier : Stage.Barriers)
		{
			Hasher.Add(Barrier.Name).Add(Barrier.Key.ToString()).Add(Barrier.LocationMeters).Add(Barrier.Rotation).Add(Barrier.HalfSizeMeters);
		}
		return Hasher.Finish();
	}

	FMadePackage WriteStageLevel(const FStageLevel& Stage)
	{
		FMadePackage Made{Stage.PackageName, EMadeOutcome::Failed, FString()};
		const FString Hash = HashOf(Stage);
		if (StoredInputHash(Stage.PackageName, FPackageName::GetShortName(Stage.PackageName)) == Hash)
		{
			Made.Outcome = Rehearse(Stage, Made.Why) ? EMadeOutcome::Unchanged : EMadeOutcome::Failed;
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
