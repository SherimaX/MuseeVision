#include "ArtPieces.h"

#include "ArtHangReading.h"
#include "ArtWork.h"
#include "Canvas/CanvasMesh.h"
#include "Canvas/WallPath.h"
#include "FormMake.h"
#include "Frame/Moulding.h"
#include "Glass/GlassPane.h"
#include "MuseeCollision.h"
#include "MuseeRecords.h"
#include "Objects/ObjectMaking.h"
#include "Plan/EntryContext.h"

namespace Musee::ArtMaking
{
	namespace
	{
		const FName CanvasSurface(TEXT("canvas"));
		const FName LinenSurface(TEXT("canvas.linen"));
		const FName NoFrame(TEXT("none"));

		/**
		 * The linen a canvas is woven of, bare where it turns round the stretcher: the palette's linen, stretched on the
		 * stretcher's bars and as new, since the tacking edges and back are out of the light and out of reach.
		 */
		const FName LinenMaterial(TEXT("accent.linen"));
		const FName LinenWorking(TEXT("stretched"));
		const FName LinenHistory(TEXT("new"));

		/** The wall a work hangs on: its anchor's straight face, or the curved path the plan gives it. */
		FWallPath WallOf(const FObjectMaking& Making, const FMuseeKey& Anchor, bool& bOutCurved)
		{
			const FMuseeFrame& Frame = Making.Stage.FindAnchor(Anchor, WallAnchorKind)->Frame;
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
		bool MakeFrame(FObjectMaking& Making, const FArtHangEntry& Entry, bool bSplit, bool bCurved, const FMuseeFrame& Work,
			TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
		{
			const TValueOrError<FArtMoulding, FString>& Moulding = Making.Moulding(Entry.FrameStyle);
			if (bCurved || Moulding.HasError())
			{
				OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its frame can't be made: %s."), *Entry.Work,
					bCurved ? TEXT("a moulded frame is made on a straight wall") : *Moulding.GetError())));
				return false;
			}
			const FVector2D Size(Entry.Size[0], Entry.Size[1]);
			// The lip rests on the painted face; the back stands on the hanging gap, as the stretcher's does.
			const FFrameDepth Depth{ StretcherDepthMeters, HangingGapMeters };
			FFormDrawing Drawing = FrameDrawing(Moulding.GetValue(), Size, TopLeftOf(Work, Entry.Height + Size.Y), Depth, FramePieceName(Entry, bSplit), Entry.Frame);
			Drawing.Collision = Collision::Solid.Profile();
			Drawing.Drawer = TEXT("Art");
			const int32 Before = OutPieces.Num();
			Musee::FormMaking::Make(Drawing, OutPieces, OutFindings);
			return OutPieces.Num() > Before;
		}

		/** One canvas of a work: its painted face, its linen and its frame, on its wall. */
		void MakeCanvas(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
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
			Face.Material = Canvas.Layer;
			Face.Working = Canvas.Finish.Working;
			Face.History = Canvas.Finish.History;
			Linen.Name = LinenPieceName(Entry, bSplit);
			Linen.Surface = LinenSurface;
			Linen.Material = LinenMaterial;
			Linen.Working = LinenWorking;
			Linen.History = LinenHistory;
			for (FFormMadePiece* Piece : { &Face, &Linen })
			{
				Piece->Collision = Collision::Solid.Profile();
				Piece->bNanite = true;
				Piece->Frame = Frame;
				Piece->Drawer = TEXT("Art");
				OutPieces.Add(MoveTemp(*Piece));
			}
		}
		/**
		 * One stained-glass panel of a work, set in its Glazing light: the image's glass, the image's size, standing on
		 * the light's glass line with its lower-left corner at the plan's along and height.
		 */
		void MakeGlass(const FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces)
		{
			const FArtHangEntry& Entry = *Canvas.Entry;
			const FMuseeFrame& Light = Making.Stage.FindAnchor(Entry.Anchor, GlazingAnchorKind)->Frame;
			FFormMadePiece Pane;
			BuildGlassPane(FGlassPaneSpec{ Entry.SizeMeters() }, Pane.Mesh);
			Pane.Name = GlassPieceName(Entry, Plan.IsSplit());
			Pane.Surface = Entry.Wall;
			Pane.Material = Canvas.Layer;
			Pane.Working = Canvas.Finish.Working;
			Pane.History = Canvas.Finish.History;
			Pane.Collision = Collision::Solid.Profile();
			// Glass is translucent, which Nanite can't render.
			Pane.bNanite = false;
			Pane.Frame = Light.FrameToParent(FMuseeFrame(FVector(Entry.Along, 0.0, Entry.Height)));
			Pane.Drawer = TEXT("Art");
			OutPieces.Add(MoveTemp(Pane));
		}
	}

	const TCHAR* ObjectWord(FName Support)
	{
		if (Support == Supports::Canvas) { return TEXT("canvas"); }
		if (Support == Supports::Glass) { return TEXT("glass"); }
		if (Support == Supports::Paper) { return TEXT("paper"); }
		if (Support == Supports::Plate) { return TEXT("plate"); }
		if (Support == Supports::Earthenware || Support == Supports::Ceramic) { return TEXT("ware"); }
		if (Support == Supports::Cast) { return TEXT("cast"); }
		if (Support == Supports::Silk) { return TEXT("silk"); }
		if (Support == Supports::Panel) { return TEXT("panel"); }
		if (Support == Supports::Tapestry) { return TEXT("tapestry"); }
		return TEXT("work");
	}

	FString ObjectName(const FArtHangEntry& Entry, bool bSplit)
	{
		// Ceramic cast from its scan is named as a cast; turned, as ware.
		const TCHAR* Word = Entry.IsBuilt() ? TEXT("work") : IsCast(Entry) ? TEXT("cast") : ObjectWord(Entry.Support);
		return bSplit ? FString::Printf(TEXT("%s.%s.%d"), Word, *Entry.Work, Entry.Canvas) : FString::Printf(TEXT("%s.%s"), Word, *Entry.Work);
	}

	FName PartName(const FArtHangEntry& Entry, bool bSplit, const TCHAR* Part)
	{
		return FName(ObjectName(Entry, bSplit) + TEXT(".") + Part);
	}

	namespace
	{
		/** One object of a work, made as its support is made. */
		void MakeObject(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
			TArray<FMuseeFinding>& OutFindings)
		{
			const FArtHangEntry& Entry = *Canvas.Entry;
			if (IsCast(Entry))
			{
				MakeCast(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else if (IsTurned(Entry))
			{
				MakeVessel(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else if (Entry.Support == Supports::Glass && IsTransparency(Making.Stage, Entry))
			{
				MakeTransparency(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else if (Entry.Support == Supports::Glass)
			{
				MakeGlass(Making, Plan, Canvas, OutPieces);
			}
			else if (Entry.Support == Supports::Earthenware)
			{
				MakeWare(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else if (Entry.Support == Supports::Canvas)
			{
				MakeCanvas(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else if (Entry.Support == Supports::Panel)
			{
				MakePanel(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else if (Entry.Support == Supports::Tapestry)
			{
				MakeTapestry(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else if (Entry.Kind == Kinds::HangingScroll)
			{
				MakeHangingScroll(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else if (Entry.Kind == Kinds::Book)
			{
				MakeBook(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else if (Entry.Kind == Kinds::Leaf)
			{
				MakeLeaf(Making, Plan, Canvas, OutPieces);
			}
			else if (Making.Stage.FindAnchor(Entry.Anchor, WallAnchorKind))
			{
				MakePrint(Making, Plan, Canvas, OutPieces, OutFindings);
			}
			else
			{
				MakeCard(Making, Plan, Canvas, OutPieces);
			}
		}
	}

	FString CanvasName(const FArtHangEntry& Entry, bool bSplit)
	{
		return ObjectName(Entry, bSplit);
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

	FName GlassPieceName(const FArtHangEntry& Entry, bool bSplit)
	{
		return FName(ObjectName(Entry, bSplit));
	}

	FName BuiltPieceName(const FArtHangEntry& Entry, bool bSplit)
	{
		return FName(ObjectName(Entry, bSplit));
	}

	void GatherArt(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		FArtHang Hang;
		if (!ReadStageHang(Stage, Hang, OutFindings))
		{
			return;
		}
		FObjectMaking Making{ Stage, Hang };
		TArray<FWorkPlan> Works;
		if (!PlanWorks(Stage, Hang, Works, OutFindings))
		{
			return;
		}
		for (const FWorkPlan& Plan : Works)
		{
			for (const FCanvasPlan& Canvas : Plan.Canvases)
			{
				MakeObject(Making, Plan, Canvas, OutPieces, OutFindings);
			}
		}
	}
}
