#include "ArtPieces.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Objects/ObjectMaking.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/**
		 * The piece's own frame: at its foot's centre, y its front (the plan's facing, a bearing from the stage's +x
		 * towards +y), z the bed's up, x to the viewer's right as the viewer faces the front.
		 */
		FMuseeFrame WareFrame(const FArtHangEntry& Entry, const FMuseeFrame& Bed)
		{
			const double Bearing = FMath::DegreesToRadians(static_cast<double>(Entry.Facing_Deg));
			const FVector Up = Bed.DirectionToParent(FVector::UpVector);
			const FVector Level(FMath::Cos(Bearing), FMath::Sin(Bearing), 0.0);
			const FVector Front = (Level - Up * (Up | Level)).GetSafeNormal();
			const FVector Right = FVector::CrossProduct(Front, Up);
			const FVector Foot(Entry.Centre[0], Entry.Centre[1], Entry.Centre[2]);
			return FMuseeFrame(Foot, FQuat(FMatrix(Right, Front, Up, FVector::ZeroVector)));
		}
	}

	void MakeWare(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const TValueOrError<FArtWareForms, FString>& Forms = Making.Forms();
		if (Forms.HasError())
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: it can't be turned: %s."), *Entry.Work, *Forms.GetError())));
			return;
		}
		const bool bSplit = Plan.IsSplit();
		const FMuseeFrame Frame = WareFrame(Entry, Making.Stage.FindAnchor(Entry.Anchor, MountAnchorKind)->Frame);
		FFormMadePiece Decorated = ArtPiece(FName(ObjectName(Entry, bSplit)), FName(ObjectWord(Entry.Support)),
			{ Canvas.Layer, Canvas.Finish.Working, Canvas.Finish.History }, Frame, true);
		FFormMadePiece Ground = ArtPiece(PartName(Entry, bSplit, TEXT("ground")), FName(ObjectWord(Entry.Support)), Canvas.Ground, Frame, true);
		BuildWare(Forms.GetValue(), Canvas.Ware, Decorated.Mesh, Ground.Mesh);
		OutPieces.Add(MoveTemp(Decorated));
		if (Ground.Mesh->TriangleCount() > 0)
		{
			OutPieces.Add(MoveTemp(Ground));
		}
	}
}
