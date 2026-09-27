#include "ArtHangReading.h"
#include "ArtPieces.h"
#include "ArtWork.h"
#include "ArtWorkComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The tag Studio gives a piece's mesh component: `musee.piece:<name>` (Docs/Stages contract). */
		FString PieceTag(FName Piece)
		{
			return FString::Printf(TEXT("musee.piece:%s"), *Piece.ToString());
		}

		/** The stage actor's pieces named Piece, or, with bParts, named Piece or any of its parts `<Piece>.<part>`. */
		TArray<UStaticMeshComponent*> FindPieces(const AActor& Actor, FName Piece, bool bParts)
		{
			TArray<UStaticMeshComponent*> Meshes;
			Actor.GetComponents(Meshes);
			const FString Whole = PieceTag(Piece);
			const FString Part = Whole + TEXT(".");
			return Meshes.FilterByPredicate([&](const UStaticMeshComponent* Mesh)
			{
				return Mesh->ComponentTags.ContainsByPredicate([&](FName Tag)
				{
					const FString Text = Tag.ToString();
					return Text == Whole || (bParts && Text.StartsWith(Part));
				});
			});
		}

		/**
		 * An object's pieces: the object's own piece and its parts (a canvas's face, linen and frame; a panel's glass; a
		 * print, its mount, glazing and frame; ware and its ground), or what the stage built for the work.
		 */
		TArray<UStaticMeshComponent*> PiecesOf(const AActor& Actor, const FArtHangEntry& Entry, bool bSplit)
		{
			return FindPieces(Actor, FName(ObjectName(Entry, bSplit)), true);
		}

		/**
		 * The object's label and care class, the component its pieces hang from: set where its first piece is, on what
		 * that piece hung from (the stage's root or its moving body), so the pieces keep their places and their motion.
		 */
		void Label(AActor& Actor, const TArray<UStaticMeshComponent*>& Pieces, const FArtHangEntry& Entry, bool bSplit, const FArtWork& Work)
		{
			// Named apart from the pieces' components, whose names may spell the work's the same way.
			const FString Name = bSplit ? FString::Printf(TEXT("Label_%s_%d"), *Work.Id, Entry.Canvas) : FString::Printf(TEXT("Label_%s"), *Work.Id);
			UArtWorkComponent* Carrier = NewObject<UArtWorkComponent>(&Actor, MakeUniqueObjectName(&Actor, UArtWorkComponent::StaticClass(), FName(Name)));
			Carrier->SetupAttachment(Pieces[0]->GetAttachParent());
			Carrier->Describe(Work.Id, Entry.Care, Work.Caption());
			Actor.AddInstanceComponent(Carrier);
			Carrier->RegisterComponent();
			Carrier->SetWorldTransform(Pieces[0]->GetComponentTransform());
			for (UStaticMeshComponent* Piece : Pieces)
			{
				Piece->AttachToComponent(Carrier, FAttachmentTransformRules::KeepWorldTransform);
				Piece->ComponentTags.AddUnique(UArtWorkComponent::WorkTag(Entry.Work));
				Piece->ComponentTags.AddUnique(UArtWorkComponent::CareTag(Entry.Care));
			}
		}
	}

	void DressWorks(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings)
	{
		FArtHang Hang;
		if (!Stage.StageActor || !ReadStageHang(Stage, Hang, OutFindings))
		{
			return;
		}
		for (const FArtHangEntry& Entry : Hang.Works)
		{
			const bool bSplit = Hang.Works.ContainsByPredicate([&Entry](const FArtHangEntry& Other) { return &Other != &Entry && Other.Work == Entry.Work; });
			const TArray<UStaticMeshComponent*> Pieces = PiecesOf(*Stage.StageActor, Entry, bSplit);
			if (Pieces.IsEmpty())
			{
				// A work Art makes and didn't was reported by the piece source; a built work is the stage's to make.
				if (Entry.IsBuilt())
				{
					OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("Work %s: the stage built no piece %s for it to caption."), *Entry.Work,
						*BuiltPieceName(Entry, bSplit).ToString())));
				}
				continue;
			}
			const TValueOrError<FArtWork, FString> Work = Musee::Art::ReadWork(Entry);
			if (Work.HasError())
			{
				OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("Work %s: %s."), *Entry.Work, *Work.GetError())));
				continue;
			}
			Label(*Stage.StageActor, Pieces, Entry, bSplit, Work.GetValue());
		}
	}
}
