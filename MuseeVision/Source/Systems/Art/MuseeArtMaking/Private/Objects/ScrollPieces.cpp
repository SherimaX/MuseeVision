#include "ArtPieces.h"
#include "FormExtrusion.h"
#include "FormMake.h"
#include "MuseeCollision.h"
#include "Objects/ObjectMaking.h"
#include "Objects/ScrollDrawing.h"
#include "Plan/EntryContext.h"

namespace Musee::ArtMaking
{
	FFormDrawing ArtDrawing(FName Name, FName Surface, FFormExtrusion&& Shape, const FMuseeFrame& Frame)
	{
		FFormDrawing Drawing = FFormDrawing::Of(Name, Surface, MoveTemp(Shape));
		Drawing.Frame = Frame;
		Drawing.Collision = Collision::Solid.Profile();
		Drawing.Drawer = TEXT("Art");
		return Drawing;
	}

	FFormDrawing Rod(FName Name, FName Surface, const FVector& Start, const FVector& Axis, const FVector& Across, double Length, double Radius)
	{
		FFormExtrusion Circle;
		Circle.Outline = FFormProfile::Ellipse(FVector2D::ZeroVector, Radius, Radius, 0.0, false);
		Circle.DepthMeters = Length;
		// The drawing's z is x cross y: with x across the axis and y the axis cross x, z is the axis.
		return ArtDrawing(Name, Surface, MoveTemp(Circle), FrameAlong(Start, Across, FVector::CrossProduct(Axis, Across)));
	}

	FFormDrawing MountSheet(FName Name, FName Surface, const FScrollWall& Wall, const FVector2D& Corner, const FVector2D& Size, const FVector2D& Window,
		const FVector2D& Painting, double Face, double Thickness)
	{
		FFormExtrusion Silk;
		Silk.Outline = FFormProfile::Rectangle(FVector2D::ZeroVector, Size);
		Silk.Holes.Add(FFormProfile::Rectangle(Window, Window + Painting));
		Silk.DepthMeters = Thickness;
		return ArtDrawing(Name, Surface, MoveTemp(Silk), FrameAlong(Wall.At(Corner.X, Face, Corner.Y), Wall.Along, Wall.Up));
	}

	namespace
	{
		/**
		 * The top stick: a half-round the mount's width, its flat against the mount's back with its middle on the mount's top
		 * edge, its round towards the wall.
		 */
		FFormDrawing Stick(FName Name, FName Surface, const FScrollWall& Wall, const FVector& Start, double Length, double Radius)
		{
			FFormExtrusion Half;
			// In the drawing's (x up, y towards the wall): the half disc on the wall's side of its flat.
			Half.Outline = FFormProfile::From(FVector2D(Radius, 0.0)).ArcAbout(FVector2D::ZeroVector, Radius, Radius, 0.0, 180.0).Close();
			Half.DepthMeters = Length;
			return ArtDrawing(Name, Surface, MoveTemp(Half), FrameAlong(Start, Wall.Up, -Wall.Out));
		}
	}

	void MakeHangingScroll(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const TValueOrError<FArtScrollFittings, FString>& Read = Making.Scrolls();
		if (Read.HasError())
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its scroll can't be mounted: %s."), *Entry.Work, *Read.GetError())));
			return;
		}
		if (!OnStraightWall(Making, Entry, OutFindings))
		{
			return;
		}
		const FArtScrollFittings& Fit = Read.GetValue();
		const FArtScrollMount& Mount = Entry.Mount;
		const bool bSplit = Plan.IsSplit();
		const FScrollWall Wall(Making.Stage.FindAnchor(Entry.Anchor, WallAnchorKind)->Frame);
		const FVector2D Painting = Entry.SizeMeters();
		// The mount round the painting, in the wall's (along, up): a border either side, a band and the earth below, a band
		// and the heaven above.
		const FVector2D MountAt(Entry.Along - Mount.Border, Entry.Height - Mount.Band - Mount.Earth);
		const FVector2D MountSize(Painting.X + 2.0 * Mount.Border, Painting.Y + 2.0 * Mount.Band + Mount.Earth + Mount.Heaven);
		const FVector2D Window(Mount.Border, Mount.Earth + Mount.Band);
		const double Back = Entry.OffWall;
		const double Face = Back + Fit.MountMeters;
		TArray<FFormDrawing> Drawings;
		Drawings.Add(MountSheet(PartName(Entry, bSplit, TEXT("mount")), Entry.Frame, Wall, MountAt, MountSize, Window, Painting, Face, Fit.MountMeters));
		const double Top = MountAt.Y + MountSize.Y;
		Drawings.Add(Stick(PartName(Entry, bSplit, TEXT("stick")), Entry.Frame, Wall, Wall.At(MountAt.X, Back, Top), MountSize.X, Fit.StickRadiusMeters));
		if (Mount.Roller)
		{
			// The roller's axis in the mount's middle, its underside the mount's foot; its knobs on its ends.
			const FVector Foot = Wall.At(MountAt.X, 0.5 * (Back + Face), MountAt.Y + Fit.RollerRadiusMeters);
			Drawings.Add(Rod(PartName(Entry, bSplit, TEXT("roller")), Entry.Frame, Foot, Wall.Along, Wall.Up, MountSize.X, Fit.RollerRadiusMeters));
			Drawings.Add(Rod(PartName(Entry, bSplit, TEXT("knob.left")), Mount.Knob, Foot - Wall.Along * Fit.KnobLengthMeters, Wall.Along, Wall.Up,
				Fit.KnobLengthMeters, Fit.KnobRadiusMeters));
			Drawings.Add(Rod(PartName(Entry, bSplit, TEXT("knob.right")), Mount.Knob, Foot + Wall.Along * MountSize.X, Wall.Along, Wall.Up,
				Fit.KnobLengthMeters, Fit.KnobRadiusMeters));
		}
		// The cord: from its two eyes on the stick's top up to the hook above the stick's middle.
		const FVector Hook = Wall.At(MountAt.X + 0.5 * MountSize.X, Back, Top + Mount.Hook);
		const double Eyes[] = { Fit.CordEyeShare, 1.0 - Fit.CordEyeShare };
		const TCHAR* const Strands[] = { TEXT("cord.left"), TEXT("cord.right") };
		for (int32 Strand = 0; Strand < 2; ++Strand)
		{
			const FVector Eye = Wall.At(MountAt.X + Eyes[Strand] * MountSize.X, Back, Top + Fit.StickRadiusMeters);
			const FVector Run = Hook - Eye;
			Drawings.Add(Rod(PartName(Entry, bSplit, Strands[Strand]), Mount.Cord, Eye, Run.GetSafeNormal(), Wall.Out, Run.Size(), Fit.CordRadiusMeters));
		}
		Musee::FormMaking::Make(Drawings, OutPieces, OutFindings);
		// The painting laid into the mount's window, flush with it.
		const FMuseeFrame Corner(Wall.At(Entry.Along, 0.0, Entry.Height), Wall.Wall.Rotation);
		OutPieces.Add(SheetPiece(FName(ObjectName(Entry, bSplit)), FName(ObjectWord(Entry.Support)), { Canvas.Layer, Canvas.Finish.Working, Canvas.Finish.History },
			Corner, Painting, Fit.MountMeters, Face, false));
	}
}
