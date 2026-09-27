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
		FFormMesh Mesh;
		BuildStone({ Kind, Entry.SizeMeters().Y, Stone->FootRadiusMeters, Musee::Seed(FMuseeKey(Entry.Work)) }, Mesh);
		if (Mesh.IsEmpty())
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its rock leaves no surface."), *Entry.Work)));
			return;
		}
		const FMuseeFrame Frame = RoundFrame(Entry, Making.Stage.FindAnchor(Entry.Anchor, MountAnchorKind)->Frame);
		FFormMadePiece Piece = ArtPiece(FName(ObjectName(Entry, Plan.IsSplit())), Supports::Stone, Canvas.Finish, Frame, true);
		Mesh.Write(Piece.Mesh);
		OutPieces.Add(MoveTemp(Piece));
	}
}
