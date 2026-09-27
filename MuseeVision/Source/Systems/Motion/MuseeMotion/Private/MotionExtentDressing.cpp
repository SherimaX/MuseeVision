#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "MotionDressing.h"
#include "MotionExtent.h"
#include "MotionSchedule.h"

// The pieces of variable length at making time: the parts Motion drew (MuseeMotionMaking) are found by their piece tags
// and given what moves them: a carriage under each telescope stage, an instanced column for a column's turns.

namespace Musee::Motion::Dressing
{
	namespace
	{
		/** The component the extent's anchored end rides on: its body's, or the stage's root. */
		USceneComponent* AnchorOf(const FMuseeStageContext& Stage, const FMotionExtentSpec& Spec)
		{
			return Spec.Body.IsNone() ? Stage.StageActor->GetRootComponent() : FindTagged(*Stage.StageActor, Spec.Body.ToName());
		}

		UMotionExtent* PlaceExtent(AActor& Actor, const FMotionExtentSpec& Spec)
		{
			UMotionExtent* Extent = NewObject<UMotionExtent>(&Actor, ObjectName(TEXT("Extent"), Spec.Key));
			Extent->ComponentTags.Add(Spec.Key.ToName());
			Actor.AddInstanceComponent(Extent);
			Extent->RegisterComponent();
			return Extent;
		}

		void DressTelescope(const FMuseeStageContext& Stage, const FMotionExtentSpec& Spec, USceneComponent& Anchor, double DrawnLength,
			TArray<FMuseeFinding>& OutFindings)
		{
			AActor& Actor = *Stage.StageActor;
			TArray<USceneComponent*> Carriages;
			for (int32 Index = 0; Index < Spec.Telescope.Num(); ++Index)
			{
				USceneComponent* Part = FindTagged(Actor, PieceTag(Spec.StagePiece(Index)));
				if (!Part)
				{
					OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Extent %s: its stage %d (%s) was not made."), *Spec.Key.ToString(), Index + 1,
						*Spec.StagePiece(Index).ToString())));
					return;
				}
				// The carriage stands where the stage is drawn, so the stage keeps its pose in it and slides with it.
				USceneComponent* Carriage = NewObject<USceneComponent>(&Actor, FName(ObjectName(TEXT("Carriage"), Spec.Key).ToString() + FString::Printf(TEXT("_%d"), Index + 1)));
				Carriage->SetMobility(EComponentMobility::Movable);
				Adopt(Actor, *Carriage, Anchor);
				Part->SetMobility(EComponentMobility::Movable);
				Part->AttachToComponent(Carriage, FAttachmentTransformRules::KeepWorldTransform);
				Carriages.Add(Carriage);
			}
			PlaceExtent(Actor, Spec)->ConfigureTelescope(Spec, Carriages, DrawnLength);
		}

		void DressTurns(const FMuseeStageContext& Stage, const FMotionExtentSpec& Spec, double DrawnLength, TArray<FMuseeFinding>& OutFindings)
		{
			AActor& Actor = *Stage.StageActor;
			UStaticMeshComponent* Turn = Cast<UStaticMeshComponent>(FindTagged(Actor, PieceTag(Spec.TurnPiece())));
			if (!Turn || !Turn->GetAttachParent())
			{
				OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Extent %s: its turn (%s) was not made."), *Spec.Key.ToString(), *Spec.TurnPiece().ToString())));
				return;
			}
			// The turn Motion drew is the column's pattern: every turn is an instance of it, and the drawn one goes.
			UInstancedStaticMeshComponent* Column = NewObject<UInstancedStaticMeshComponent>(&Actor, ObjectName(TEXT("Column"), Spec.Key));
			Column->SetStaticMesh(Turn->GetStaticMesh());
			for (int32 Slot = 0; Slot < Turn->GetNumMaterials(); ++Slot)
			{
				Column->SetMaterial(Slot, Turn->GetMaterial(Slot));
			}
			Column->SetCollisionProfileName(Turn->GetCollisionProfileName());
			Column->SetMobility(EComponentMobility::Movable);
			Column->ComponentTags = Turn->ComponentTags;
			const FTransform FirstTurn = Turn->GetRelativeTransform();
			Adopt(Actor, *Column, *Turn->GetAttachParent());
			Actor.RemoveInstanceComponent(Turn);
			Turn->DestroyComponent();
			PlaceExtent(Actor, Spec)->ConfigureTurns(Spec, Column, FirstTurn, DrawnLength);
		}
	}

	void DressExtents(const FMuseeStageContext& Stage, const FMotionSchedule& Schedule, TArray<FMuseeFinding>& OutFindings)
	{
		for (const FMotionExtentSpec& Spec : Schedule.Extents)
		{
			USceneComponent* Anchor = AnchorOf(Stage, Spec);
			if (!Anchor)
			{
				OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Extent %s: its body %s has no tagged scene component."), *Spec.Key.ToString(), *Spec.Body.ToString())));
				continue;
			}
			const double DrawnLength = Spec.Length.At(DrawnValue(Schedule, Spec.Length.Channel));
			if (!Spec.Telescope.IsEmpty())
			{
				DressTelescope(Stage, Spec, *Anchor, DrawnLength, OutFindings);
			}
			else
			{
				DressTurns(Stage, Spec, DrawnLength, OutFindings);
			}
		}
	}
}
