#include "ArtPieces.h"
#include "FormMake.h"
#include "MuseeCollision.h"
#include "MuseeRecords.h"
#include "Objects/ObjectMaking.h"
#include "Plan/EntryContext.h"
#include "Sheet/SheetMesh.h"
#include "Support/ArtSupport.h"

namespace Musee::ArtMaking
{
	FMuseeFrame FrameAlong(const FVector& Origin, const FVector& X, const FVector& Y)
	{
		return FMuseeFrame(Origin, FQuat(FMatrix(X, Y, FVector::CrossProduct(X, Y), FVector::ZeroVector)));
	}

	FFormMadePiece SheetPiece(FName Name, FName Surface, const FLayerFinish& Material, const FMuseeFrame& Frame, const FVector2D& Size, double Thickness,
		double Front, bool bGlass, double ImageLeftMeters)
	{
		FFormMadePiece Piece = ArtPiece(Name, Surface, Material, Frame, !bGlass);
		BuildSheet(FSheetSpec{ Size, Thickness, Front, ImageLeftMeters }, Piece.Mesh);
		return Piece;
	}

	namespace
	{
		/**
		 * Where a mounted work lies on its Wall anchor: the work at its place in the plan, and its mount (a mount it lies
		 * on, or a window mat it shows through) centred on it, in the wall's (along, up), on the mount's face. The mount's
		 * face is the anchor's plane, a hanging plane in the room (the Hall of Light's stems), unless the plan hangs the
		 * work against the wall itself: then the mount's back stands the plan's offWall off it, where the frame's back
		 * rests (Albion's works on paper in their frames).
		 */
		struct FMounted
		{
			const FMuseeFrame& Wall;
			FVector2D WorkAt;
			FVector2D WorkSize;
			FVector2D MountAt;
			FVector2D MountSize;
			/** How far the mount's face stands out of the anchor's plane. */
			double Face = 0.0;

			FMounted(const FMuseeFrame& InWall, const FArtHangEntry& Entry)
				: Wall(InWall), WorkAt(Entry.Along, Entry.Height), WorkSize(Entry.SizeMeters()), MountSize(Entry.MatSize[0], Entry.MatSize[1]),
				Face(Entry.OffWall > 0.0f ? Entry.OffWall + Entry.MatDepth : 0.0)
			{
				MountAt = WorkAt + 0.5 * (WorkSize - MountSize);
			}

			/** The wall's axes at a point of the mount's face. */
			FMuseeFrame At(const FVector2D& Point) const { return FMuseeFrame(Wall.PointToParent(FVector(Point.X, Face, Point.Y)), Wall.Rotation); }
		};

		/**
		 * The mount: a board of its size and depth, its face on the wall anchor's plane, its back the depth behind; a window
		 * mat has the work's window cut through it.
		 */
		FFormDrawing MountDrawing(const FArtHangEntry& Entry, bool bSplit, const FMounted& Mounted, const TCHAR* Part, bool bWindow)
		{
			FFormExtrusion Board;
			Board.Outline = FFormProfile::Rectangle(FVector2D::ZeroVector, Mounted.MountSize);
			if (bWindow)
			{
				const FVector2D Window = Mounted.WorkAt - Mounted.MountAt;
				Board.Holes.Add(FFormProfile::Rectangle(Window, Window + Mounted.WorkSize));
			}
			Board.DepthMeters = Entry.MatDepth;
			FFormDrawing Drawing = FFormDrawing::Of(PartName(Entry, bSplit, Part), Entry.Mat, MoveTemp(Board));
			// Drawn across the wall and up it, and pushed into the wall.
			const FVector Along = Mounted.Wall.DirectionToParent(FVector::ForwardVector);
			const FVector Up = Mounted.Wall.DirectionToParent(FVector::UpVector);
			Drawing.Frame = FrameAlong(Mounted.At(Mounted.MountAt).OriginMeters, Along, Up);
			Drawing.Collision = Collision::Solid.Profile();
			Drawing.Drawer = TEXT("Art");
			return Drawing;
		}

		/** The frame round a mount, its lip on the face given and its back at the depth given; false, with a finding, if it can't be. */
		bool MakeMountFrame(FObjectMaking& Making, const FArtHangEntry& Entry, bool bSplit, const FMounted& Mounted, const FFrameDepth& Depth,
			TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
		{
			if (!IsFramed(Entry))
			{
				return true;
			}
			const TValueOrError<FArtMoulding, FString>& Moulding = Making.Moulding(Entry.FrameStyle);
			if (Moulding.HasError())
			{
				OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its frame can't be made: %s."), *Entry.Work, *Moulding.GetError())));
				return false;
			}
			const FVector Along = Mounted.Wall.DirectionToParent(FVector::ForwardVector);
			const FVector Down = -Mounted.Wall.DirectionToParent(FVector::UpVector);
			const FVector TopLeft = Mounted.At(Mounted.MountAt + FVector2D(0.0, Mounted.MountSize.Y)).OriginMeters;
			FFormDrawing Frame = FrameDrawing(Moulding.GetValue(), Mounted.MountSize, FrameAlong(TopLeft, Along, Down), Depth,
				PartName(Entry, bSplit, TEXT("frame")), Entry.Frame);
			Frame.Collision = Collision::Solid.Profile();
			Frame.Drawer = TEXT("Art");
			Musee::FormMaking::Make(Frame, OutPieces, OutFindings);
			return true;
		}
	}

	bool OnStraightWall(const FObjectMaking& Making, const FArtHangEntry& Entry, TArray<FMuseeFinding>& OutFindings)
	{
		if (!Making.Hang.Walls.ContainsByPredicate([&Entry](const FArtWall& W) { return W.Anchor == Entry.Anchor; }))
		{
			return true;
		}
		OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: it hangs flat, on a straight wall; %s is curved."), *Entry.Work,
			*Entry.Anchor.ToString())));
		return false;
	}

	const TValueOrError<FArtMoulding, FString>& FObjectMaking::Moulding(FName FrameStyle)
	{
		const FString Record = FArtMoulding::RecordFor(FrameStyle);
		if (const TValueOrError<FArtMoulding, FString>* Read = Mouldings.Find(Record))
		{
			return *Read;
		}
		return Mouldings.Add(Record, ReadRecord<FArtMoulding>(Record));
	}

	const TValueOrError<FArtWareForms, FString>& FObjectMaking::Forms()
	{
		if (!WareForms.IsSet())
		{
			WareForms.Emplace(ReadRecord<FArtWareForms>(FArtWareForms::Record));
		}
		return WareForms.GetValue();
	}

	const TValueOrError<FArtScrollFittings, FString>& FObjectMaking::Scrolls()
	{
		if (!ScrollFittings.IsSet())
		{
			ScrollFittings.Emplace(ReadRecord<FArtScrollFittings>(FArtScrollFittings::Record));
		}
		return ScrollFittings.GetValue();
	}

	const TValueOrError<FArtPanelConstruction, FString>& FObjectMaking::Panels()
	{
		if (!PanelConstruction.IsSet())
		{
			PanelConstruction.Emplace(ReadRecord<FArtPanelConstruction>(FArtPanelConstruction::Record));
		}
		return PanelConstruction.GetValue();
	}

	const TValueOrError<TArray<FArtBook>, FString>& FObjectMaking::Books()
	{
		if (!Bindings.IsSet())
		{
			Bindings.Emplace(ReadRecords<FArtBook>(FArtBook::Record));
		}
		return Bindings.GetValue();
	}

	FFormMadePiece ArtPiece(FName Name, FName Surface, const FLayerFinish& Material, const FMuseeFrame& Frame, bool bNanite)
	{
		FFormMadePiece Piece;
		Piece.Name = Name;
		Piece.Surface = Surface;
		Piece.Material = Material.Palette;
		Piece.Working = Material.Working;
		Piece.History = Material.History;
		Piece.Collision = Collision::Solid.Profile();
		Piece.bNanite = bNanite;
		Piece.Frame = Frame;
		Piece.Drawer = TEXT("Art");
		return Piece;
	}

	void MakePrint(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const bool bSplit = Plan.IsSplit();
		if (!OnStraightWall(Making, Entry, OutFindings))
		{
			return;
		}
		const FMounted Mounted(Making.Stage.FindAnchor(Entry.Anchor, WallAnchorKind)->Frame, Entry);
		const double Paper = FindSupport(Entry.Support)->ThicknessMeters;
		// The glazing lies on the print; the frame's lip on the glazing, its back flush with the mount's.
		const double Face = Paper + Canvas.GlazingMeters;
		if (!MakeMountFrame(Making, Entry, bSplit, Mounted, FFrameDepth{ Face, -Entry.MatDepth }, OutPieces, OutFindings))
		{
			return;
		}
		Musee::FormMaking::Make(MountDrawing(Entry, bSplit, Mounted, TEXT("mount"), false), OutPieces, OutFindings);
		const FLayerFinish Layer{ Canvas.Layer, Canvas.Finish.Working, Canvas.Finish.History };
		OutPieces.Add(SheetPiece(FName(ObjectName(Entry, bSplit)), FName(ObjectWord(Entry.Support)), Layer, Mounted.At(Mounted.WorkAt), Mounted.WorkSize, Paper,
			Paper, false));
		if (Entry.Glazed)
		{
			OutPieces.Add(SheetPiece(PartName(Entry, bSplit, TEXT("glazing")), Entry.Glazing, {}, Mounted.At(Mounted.MountAt), Mounted.MountSize,
				Canvas.GlazingMeters, Face, true));
		}
	}

	void MakeTransparency(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const bool bSplit = Plan.IsSplit();
		if (!OnStraightWall(Making, Entry, OutFindings))
		{
			return;
		}
		const FMounted Mounted(Making.Stage.FindAnchor(Entry.Anchor, WallAnchorKind)->Frame, Entry);
		// From the room inwards: the glazing on the window mat's face, the mat, the plate behind its window, the pane
		// behind the plate.
		const double Glazing = Canvas.GlazingMeters;
		const double PlateFront = -Entry.MatDepth;
		const double BackingFront = PlateFront - Canvas.PlateMeters;
		const double Back = BackingFront - Canvas.BackingMeters;
		if (!MakeMountFrame(Making, Entry, bSplit, Mounted, FFrameDepth{ Glazing, Back }, OutPieces, OutFindings))
		{
			return;
		}
		Musee::FormMaking::Make(MountDrawing(Entry, bSplit, Mounted, TEXT("mat"), true), OutPieces, OutFindings);
		const FLayerFinish Layer{ Canvas.Layer, Canvas.Finish.Working, Canvas.Finish.History };
		OutPieces.Add(SheetPiece(FName(ObjectName(Entry, bSplit)), Entry.Wall, Layer, Mounted.At(Mounted.WorkAt), Mounted.WorkSize, Canvas.PlateMeters,
			PlateFront, true));
		if (!Entry.Backing.IsNone())
		{
			OutPieces.Add(SheetPiece(PartName(Entry, bSplit, TEXT("backing")), Entry.Backing, {}, Mounted.At(Mounted.MountAt), Mounted.MountSize,
				Canvas.BackingMeters, BackingFront, true));
		}
		if (Entry.Glazed)
		{
			OutPieces.Add(SheetPiece(PartName(Entry, bSplit, TEXT("glazing")), Entry.Glazing, {}, Mounted.At(Mounted.MountAt), Mounted.MountSize, Glazing,
				Glazing, true));
		}
	}

	void MakeCard(const FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const bool bSplit = Plan.IsSplit();
		const FMuseeFrame& Reader = Making.Stage.FindAnchor(Entry.Anchor, MountAnchorKind)->Frame;
		const FVector2D Size = Entry.SizeMeters();
		const double Thickness = FindSupport(Entry.Support)->ThicknessMeters;
		// Centred on the reader's face: its lower-left half its size to the viewer's left and down the face.
		const FMuseeFrame Frame(Reader.PointToParent(FVector(-0.5 * Size.X, 0.0, -0.5 * Size.Y)), Reader.Rotation);
		const FLayerFinish Layer{ Canvas.Layer, Canvas.Finish.Working, Canvas.Finish.History };
		OutPieces.Add(SheetPiece(FName(ObjectName(Entry, bSplit)), FName(ObjectWord(Entry.Support)), Layer, Frame, Size, Thickness, Thickness, false));
		if (Entry.Glazed)
		{
			OutPieces.Add(SheetPiece(PartName(Entry, bSplit, TEXT("glazing")), Entry.Glazing, {}, Frame, Size, Canvas.GlazingMeters, Thickness + Canvas.GlazingMeters, true));
		}
	}
}
