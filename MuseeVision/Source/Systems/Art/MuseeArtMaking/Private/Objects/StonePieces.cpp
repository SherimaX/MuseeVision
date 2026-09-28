#include "ArtPieces.h"
#include "FormMesh.h"
#include "MuseeKey.h"
#include "MuseeRecords.h"
#include "Objects/ObjectMaking.h"
#include "Stone/StoneMesh.h"

namespace Musee::ArtMaking
{
	const TValueOrError<FArtStones, FString>& FObjectMaking::Stones()
	{
		if (!StoneRecord.IsSet())
		{
			StoneRecord.Emplace(ReadRecord<FArtStones>(FArtStones::Record));
		}
		return StoneRecord.GetValue();
	}

	void MakeStone(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const TValueOrError<FArtStones, FString>& Stones = Making.Stones();
		const FArtStone* Stone = Stones.HasValue() ? Stones.GetValue().Find(Entry.Work) : nullptr;
		const FArtStoneKind* Kind = Stone ? Stones.GetValue().FindKind(Stone->Kind) : nullptr;
		if (!Kind)
		{
			const FString Why = Stones.HasError() ? Stones.GetError() : FString::Printf(TEXT("Data/%s gives it no kind of rock"), FArtStones::Record);
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: it can't be formed: %s."), *Entry.Work, *Why)));
			return;
		}
		const FStoneSpec Spec{ Kind, Entry.SizeMeters().Y, Stone->FootRadiusMeters, Musee::Seed(FMuseeKey(Entry.Work)) };
		FFormMesh Mesh;
		BuildStone(Spec, Mesh);
		if (Mesh.IsEmpty())
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its rock leaves no surface."), *Entry.Work)));
			return;
		}
		const bool bSplit = Plan.IsSplit();
		const FMuseeFrame Ground = RoundFrame(Entry, Making.Stage.FindAnchor(Entry.Anchor, MountAnchorKind)->Frame);
		double Raised = 0.0;
		if (Entry.Stand)
		{
			// Its stand is carved to it (座): the rock sits down into the recess cut to its foot.
			const FArtStoneStand& StandRecord = Stones.GetValue().Stand;
			FStoneStandForm Stand = CarveStoneStand(Mesh, StoneCellMeters(Spec), StandRecord);
			if (Stand.Mesh.IsEmpty())
			{
				OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its stand can't be carved to it."), *Entry.Work)));
				return;
			}
			Raised = Stand.RecessFloorMeters;
			FFormMadePiece StandPiece = ArtPiece(PartName(Entry, bSplit, TEXT("stand")), TEXT("stand"),
				{ StandRecord.Palette, StandRecord.Working, StandRecord.History }, Ground, true);
			Stand.Mesh.Write(StandPiece.Mesh);
			OutPieces.Add(MoveTemp(StandPiece));
		}
		const FMuseeFrame OnStand = Ground.FrameToParent(FMuseeFrame(FVector(0.0, 0.0, Raised)));
		FFormMadePiece Piece = ArtPiece(FName(ObjectName(Entry, bSplit)), Supports::Stone, Canvas.Finish, OnStand, true);
		Mesh.Write(Piece.Mesh);
		OutPieces.Add(MoveTemp(Piece));
	}
}
