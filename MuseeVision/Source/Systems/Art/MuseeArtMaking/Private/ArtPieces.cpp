#include "ArtPieces.h"

#include "ArtHangReading.h"
#include "ArtWork.h"
#include "Canvas/CanvasMesh.h"
#include "Canvas/WallPath.h"
#include "FormMake.h"
#include "Frame/Moulding.h"
#include "MuseeCollision.h"
#include "MuseeRecords.h"

namespace Musee::ArtMaking
{
	namespace
	{
		const FName CanvasSurface(TEXT("canvas"));
		const FName LinenSurface(TEXT("canvas.linen"));
		const FName NoFrame(TEXT("none"));

		/** The linen a canvas is woven of, bare where it turns round the stretcher: the palette's linen. */
		const FName LinenMaterial(TEXT("accent.linen"));

		/** What every canvas of a stage is made with: the paint's finish, and the frame's moulding once a canvas needs it. */
		struct FMaking
		{
			const FMuseeStageContext& Stage;
			const FArtHang& Hang;
			FPaintFinish Paint;
			TOptional<TValueOrError<FArtMoulding, FString>> Moulding;

			const TValueOrError<FArtMoulding, FString>& ReadMoulding()
			{
				if (!Moulding.IsSet())
				{
					Moulding.Emplace(ReadRecord<FArtMoulding>(FArtMoulding::Record));
				}
				return Moulding.GetValue();
			}
		};

		/** The wall a work hangs on: its anchor's straight face, or the curved path the plan gives it. */
		FWallPath WallOf(const FMaking& Making, const FMuseeKey& Anchor, bool& bOutCurved)
		{
			const FMuseeFrame& Frame = Making.Stage.FindAnchor(Anchor)->Frame;
			const FArtWall* Curved = Making.Hang.Walls.FindByPredicate([&Anchor](const FArtWall& W) { return W.Anchor == Anchor; });
			bOutCurved = Curved != nullptr;
			return Curved ? FWallPath(Frame, Curved->Path) : FWallPath(Frame);
		}

		/** The work's own frame: at its start edge on the floor line of the wall's face, x to the viewer's right, y into the room. */
		FMuseeFrame WorkFrame(const FWallPath::FStation& Start)
		{
			const FVector Right = FVector::CrossProduct(Start.Out, FVector::UpVector).GetSafeNormal();
			return FMuseeFrame(Start.Point, FQuat(FMatrix(Right, Start.Out, FVector::UpVector, FVector::ZeroVector)));
		}

		/** The canvas's top-left corner on the wall's face in the stage, x along its top edge, y down, z out of the wall. */
		FMuseeFrame TopLeftOf(const FMuseeFrame& Work, double TopMeters)
		{
			const FVector Across = Work.DirectionToParent(FVector::ForwardVector);
			const FVector Down = Work.DirectionToParent(-FVector::UpVector);
			const FVector Out = Work.DirectionToParent(FVector::RightVector);
			return FMuseeFrame(Work.PointToParent(FVector(0.0, 0.0, TopMeters)), FQuat(FMatrix(Across, Down, Out, FVector::ZeroVector)));
		}

		/** The moulded frame round a canvas, in the finish of the plan's frame surface; false with a finding when it can't be. */
		bool MakeFrame(FMaking& Making, const FArtHangEntry& Entry, bool bSplit, bool bCurved, const FMuseeFrame& Work,
			TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
		{
			const TValueOrError<FArtMoulding, FString>& Moulding = Making.ReadMoulding();
			if (bCurved || Moulding.HasError())
			{
				OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its frame can't be made: %s."), *Entry.Work,
					bCurved ? TEXT("a moulded frame is made on a straight wall") : *Moulding.GetError())));
				return false;
			}
			const FVector2D Size(Entry.Size[0], Entry.Size[1]);
			FFormDrawing Drawing = FrameDrawing(Moulding.GetValue(), Size, TopLeftOf(Work, Entry.Height + Size.Y), FramePieceName(Entry, bSplit), Entry.Frame);
			Drawing.Collision = Collision::Solid.Profile();
			Drawing.Drawer = TEXT("Art");
			const int32 Before = OutPieces.Num();
			Musee::FormMaking::Make(Drawing, OutPieces, OutFindings);
			return OutPieces.Num() > Before;
		}

		/** One canvas of a work: its painted face, its linen and its frame, on its wall. */
		void MakeCanvas(FMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
		{
			const FArtHangEntry& Entry = *Canvas.Entry;
			bool bCurved = false;
			const FWallPath Wall = WallOf(Making, Entry.Anchor, bCurved);
			const double Width = Entry.Size[0];
			if (!Wall.Faults().IsEmpty() || Entry.Along + Width > Wall.Length())
			{
				OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its wall %s: %s."), *Entry.Work, *Entry.Anchor.ToString(),
					Wall.Faults().IsEmpty() ? TEXT("the canvas runs past the wall's end") : *Wall.Faults())));
				return;
			}
			const FWallPath::FStation Start = Wall.At(Entry.Along);
			FCanvasSpec Spec{ Entry.Along, Width, Entry.Height, Entry.Size[1], true, Canvas.ImageOffsetMeters };
			// The image reads left to right as seen from the room, whichever way the wall's path runs.
			Spec.bReadsWithPath = FVector::DotProduct(Start.Along, FVector::CrossProduct(Start.Out, FVector::UpVector)) > 0.0;
			const FMuseeFrame Frame = WorkFrame(Spec.bReadsWithPath ? Start : Wall.At(Entry.Along + Width));
			const bool bSplit = Plan.IsSplit();
			if (!Entry.Frame.IsNone() && Entry.Frame != NoFrame && !MakeFrame(Making, Entry, bSplit, bCurved, Frame, OutPieces, OutFindings))
			{
				return;
			}
			FFormMadePiece Face, Linen;
			BuildCanvas(Wall, Spec, Frame, Face.Mesh, Linen.Mesh);
			Face.Name = FacePieceName(Entry, bSplit);
			Face.Surface = CanvasSurface;
			Face.Material = Musee::Art::PaintMaterial(Entry.Work);
			Face.Working = Making.Paint.Working;
			Face.History = Making.Paint.History;
			Linen.Name = LinenPieceName(Entry, bSplit);
			Linen.Surface = LinenSurface;
			Linen.Material = LinenMaterial;
			Linen.Working = Making.Paint.Working;
			Linen.History = Making.Paint.History;
			for (FFormMadePiece* Piece : { &Face, &Linen })
			{
				Piece->Collision = Collision::Solid.Profile();
				Piece->bNanite = true;
				Piece->Frame = Frame;
				Piece->Drawer = TEXT("Art");
				OutPieces.Add(MoveTemp(*Piece));
			}
		}
	}

	FString CanvasName(const FArtHangEntry& Entry, bool bSplit)
	{
		return bSplit ? FString::Printf(TEXT("canvas.%s.%d"), *Entry.Work, Entry.Canvas) : FString::Printf(TEXT("canvas.%s"), *Entry.Work);
	}

	FName FacePieceName(const FArtHangEntry& Entry, bool bSplit)
	{
		return FName(CanvasName(Entry, bSplit));
	}

	FName LinenPieceName(const FArtHangEntry& Entry, bool bSplit)
	{
		return FName(CanvasName(Entry, bSplit) + TEXT(".linen"));
	}

	FName FramePieceName(const FArtHangEntry& Entry, bool bSplit)
	{
		return FName(CanvasName(Entry, bSplit) + TEXT(".frame"));
	}

	void GatherArt(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		FArtHang Hang;
		if (!ReadStageHang(Stage, Hang, OutFindings))
		{
			return;
		}
		FMaking Making{ Stage, Hang };
		TArray<FWorkPlan> Works;
		if (!PlanWorks(Stage, Hang, Works, Making.Paint, OutFindings))
		{
			return;
		}
		for (const FWorkPlan& Plan : Works)
		{
			for (const FCanvasPlan& Canvas : Plan.Canvases)
			{
				MakeCanvas(Making, Plan, Canvas, OutPieces, OutFindings);
			}
		}
	}
}
