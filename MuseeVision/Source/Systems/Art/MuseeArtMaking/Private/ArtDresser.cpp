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
		FName PieceTag(FName Piece)
		{
			return FName(FString::Printf(TEXT("musee.piece:%s"), *Piece.ToString()));
		}

		UStaticMeshComponent* FindPiece(const AActor& Actor, FName Piece)
		{
			TArray<UStaticMeshComponent*> Meshes;
			Actor.GetComponents(Meshes);
			const FName Tag = PieceTag(Piece);
			UStaticMeshComponent* const* Found = Meshes.FindByPredicate([Tag](const UStaticMeshComponent* M) { return M->ComponentHasTag(Tag); });
			return Found ? *Found : nullptr;
		}

		/** Tags every piece of a canvas with its work and care class. */
		void Tag(const AActor& Actor, const FArtHangEntry& Entry, bool bSplit)
		{
			for (const FName Piece : { FacePieceName(Entry, bSplit), LinenPieceName(Entry, bSplit), FramePieceName(Entry, bSplit) })
			{
				if (UStaticMeshComponent* Mesh = FindPiece(Actor, Piece))
				{
					Mesh->ComponentTags.AddUnique(UArtWorkComponent::WorkTag(Entry.Work));
					Mesh->ComponentTags.AddUnique(UArtWorkComponent::CareTag(Entry.Care));
				}
			}
		}

		/** The work's label and care class, on the face of its first canvas. */
		void Label(AActor& Actor, UStaticMeshComponent& Face, const FArtHangEntry& Entry, const FArtWork& Work)
		{
			UArtWorkComponent* Carrier = NewObject<UArtWorkComponent>(&Actor, FName(FString::Printf(TEXT("Work_%s"), *Work.Id)));
			Carrier->SetupAttachment(&Face);
			Carrier->Describe(Work.Id, Entry.Care, Work.Caption());
			Actor.AddInstanceComponent(Carrier);
			Carrier->RegisterComponent();
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
			UStaticMeshComponent* Face = FindPiece(*Stage.StageActor, FacePieceName(Entry, bSplit));
			if (!Face)
			{
				// The piece source reported why the canvas wasn't made; there is nothing here to label.
				continue;
			}
			Tag(*Stage.StageActor, Entry, bSplit);
			if (Entry.Canvas != 0)
			{
				continue;
			}
			const TValueOrError<FArtWork, FString> Work = Musee::Art::ReadWork(Entry);
			if (Work.HasError())
			{
				OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("Work %s: %s."), *Entry.Work, *Work.GetError())));
				continue;
			}
			Label(*Stage.StageActor, *Face, Entry, Work.GetValue());
		}
	}
}
