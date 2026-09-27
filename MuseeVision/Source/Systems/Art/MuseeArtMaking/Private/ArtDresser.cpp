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

		void Label(AActor& Actor, UStaticMeshComponent& Face, UStaticMeshComponent* Linen, const FArtHangEntry& Entry, const FArtWork& Work)
		{
			for (UStaticMeshComponent* Piece : { &Face, Linen })
			{
				if (Piece)
				{
					Piece->ComponentTags.AddUnique(UArtWorkComponent::WorkTag(Work.Id));
					Piece->ComponentTags.AddUnique(UArtWorkComponent::CareTag(Entry.Care));
				}
			}
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
			UStaticMeshComponent* Face = FindPiece(*Stage.StageActor, FacePieceName(Entry.Work));
			if (!Face)
			{
				// The piece source reported why the work wasn't made; there is nothing here to label.
				continue;
			}
			const TValueOrError<FArtWork, FString> Work = Musee::Art::ReadWork(Entry);
			if (Work.HasError())
			{
				OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("Work %s: %s."), *Entry.Work, *Work.GetError())));
				continue;
			}
			Label(*Stage.StageActor, *Face, FindPiece(*Stage.StageActor, LinenPieceName(Entry.Work)), Entry, Work.GetValue());
		}
	}
}
