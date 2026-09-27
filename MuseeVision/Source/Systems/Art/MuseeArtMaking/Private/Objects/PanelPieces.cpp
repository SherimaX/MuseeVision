#include "ArtPieces.h"
#include "Canvas/CanvasMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "FormMake.h"
#include "FormMesh.h"
#include "MuseeCollision.h"
#include "Objects/ObjectMaking.h"
#include "Plan/EntryContext.h"
#include "Support/ArtSupport.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** A box in a mesh's frame, from its least corner to its most, each face's UV0 in metres across it. */
		void AddBox(FFormMesh& Mesh, const FVector& Least, const FVector& Most)
		{
			for (int32 Axis = 0; Axis < 3; ++Axis)
			{
				const int32 U = (Axis + 1) % 3;
				const int32 V = (Axis + 2) % 3;
				for (const double Side : { -1.0, 1.0 })
				{
					FVector Normal = FVector::ZeroVector;
					Normal[Axis] = Side;
					const double Us[4] = { Least[U], Most[U], Most[U], Least[U] };
					const double Vs[4] = { Least[V], Least[V], Most[V], Most[V] };
					int32 Corners[4];
					for (int32 Corner = 0; Corner < 4; ++Corner)
					{
						FVector Point = FVector::ZeroVector;
						Point[Axis] = Side < 0.0 ? Least[Axis] : Most[Axis];
						Point[U] = Us[Corner];
						Point[V] = Vs[Corner];
						Corners[Corner] = Mesh.AddVertex(Point, Normal, FVector2D(Us[Corner] - Least[U], Vs[Corner] - Least[V]));
					}
					Mesh.AddQuad(Corners[0], Corners[1], Corners[2], Corners[3]);
				}
			}
		}

		/**
		 * Where one set of a cradle's parts stands across a span: evenly, the outermost the edge distance in from the span's
		 * ends, no further apart than their spacing; their middles, metres from the span's start.
		 */
		TArray<double> Stations(double Span, double Edge, double Width, double Spacing)
		{
			const double First = Edge + 0.5 * Width;
			const double Run = Span - 2.0 * First;
			if (Run <= 0.0)
			{
				return { 0.5 * Span };
			}
			const int32 Gaps = FMath::Max(1, FMath::CeilToInt32(Run / Spacing));
			TArray<double> Out;
			for (int32 Gap = 0; Gap <= Gaps; ++Gap)
			{
				Out.Add(First + Run * Gap / Gaps);
			}
			return Out;
		}

		/**
		 * The cradle on the panel's back, in the panel's frame (x along the wall, y out of it, z up, its lower-left corner at
		 * the origin): its members along the grain (the panel's longer side), from the cradle's back to the board, and its
		 * slides across them between the outermost members, flush with the back, through their slots.
		 */
		void BuildCradle(const FArtPanelConstruction& Panel, const FVector2D& Size, double Back, UE::Geometry::FDynamicMesh3& Out)
		{
			const bool bGrainUp = Size.Y >= Size.X;
			const double Along = bGrainUp ? Size.Y : Size.X;
			const double Across = bGrainUp ? Size.X : Size.Y;
			// A point across the grain, out of the wall and along the grain, in the panel's frame.
			const auto Point = [bGrainUp](double C, double Y, double A) { return bGrainUp ? FVector(C, Y, A) : FVector(A, Y, C); };
			const double Board = Back + Panel.CradleDepthMeters;
			FFormMesh Mesh;
			const TArray<double> Members = Stations(Across, Panel.EdgeMeters, Panel.MemberWidthMeters, Panel.MemberSpacingMeters);
			const double Member = 0.5 * Panel.MemberWidthMeters;
			for (const double C : Members)
			{
				AddBox(Mesh, Point(C - Member, Back, 0.0), Point(C + Member, Board, Along));
			}
			const double Slide = 0.5 * Panel.SlideWidthMeters;
			for (const double A : Stations(Along, Panel.EdgeMeters, Panel.SlideWidthMeters, Panel.SlideSpacingMeters))
			{
				AddBox(Mesh, Point(Members[0] - Member, Back, A - Slide), Point(Members.Last() + Member, Back + Panel.SlideDepthMeters, A + Slide));
			}
			Mesh.Write(Out);
		}

		/** The work's layer, finished as its support's record says. */
		FLayerFinish LayerOf(const FCanvasPlan& Canvas)
		{
			return { Canvas.Layer, Canvas.Finish.Working, Canvas.Finish.History };
		}
	}

	void MakePanel(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const TValueOrError<FArtPanelConstruction, FString>& Read = Making.Panels();
		if (Read.HasError())
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its panel can't be made: %s."), *Entry.Work, *Read.GetError())));
			return;
		}
		if (!OnStraightWall(Making, Entry, OutFindings))
		{
			return;
		}
		const FArtPanelConstruction& Panel = Read.GetValue();
		const bool bSplit = Plan.IsSplit();
		const FMuseeFrame& Wall = Making.Stage.FindAnchor(Entry.Anchor, WallAnchorKind)->Frame;
		const FVector2D Size = Entry.SizeMeters();
		// The panel's own frame: its lower-left corner on the wall's plane, x along the wall, y out of it, z up.
		const FMuseeFrame Corner(Wall.PointToParent(FVector(Entry.Along, 0.0, Entry.Height)), Wall.Rotation);
		// From the wall out: the cradle's back on the hanging gap, the board on the cradle, the ground and paint on the board.
		const double Back = HangingGapMeters;
		const double BoardFront = Back + Panel.CradleDepthMeters + Panel.BoardMeters;
		const double Face = BoardFront + Panel.GroundMeters;
		if (IsFramed(Entry))
		{
			const TValueOrError<FArtMoulding, FString>& Moulding = Making.Moulding(Entry.FrameStyle);
			if (Moulding.HasError())
			{
				OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: its frame can't be made: %s."), *Entry.Work, *Moulding.GetError())));
				return;
			}
			const FVector Along = Wall.DirectionToParent(FVector::ForwardVector);
			const FVector Down = -Wall.DirectionToParent(FVector::UpVector);
			const FVector TopLeft = Wall.PointToParent(FVector(Entry.Along, 0.0, Entry.Height + Size.Y));
			// The lip rests on the paint; the back stands on the hanging gap, as the cradle's does.
			FFormDrawing Frame = FrameDrawing(Moulding.GetValue(), Size, FrameAlong(TopLeft, Along, Down), FFrameDepth{ Face, Back },
				PartName(Entry, bSplit, TEXT("frame")), Entry.Frame);
			Frame.Collision = Collision::Solid.Profile();
			Frame.Drawer = TEXT("Art");
			Musee::FormMaking::Make(Frame, OutPieces, OutFindings);
		}
		OutPieces.Add(SheetPiece(FName(ObjectName(Entry, bSplit)), FName(ObjectWord(Entry.Support)), LayerOf(Canvas), Corner, Size, Panel.GroundMeters, Face,
			false));
		OutPieces.Add(SheetPiece(PartName(Entry, bSplit, TEXT("board")), Entry.Wall, {}, Corner, Size, Panel.BoardMeters, BoardFront, false));
		FFormMadePiece Cradle = ArtPiece(PartName(Entry, bSplit, TEXT("cradle")), Entry.Wall, {}, Corner, true);
		BuildCradle(Panel, Size, Back, Cradle.Mesh);
		OutPieces.Add(MoveTemp(Cradle));
	}

	void MakeTapestry(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		if (!OnStraightWall(Making, Entry, OutFindings))
		{
			return;
		}
		const FMuseeFrame& Wall = Making.Stage.FindAnchor(Entry.Anchor, WallAnchorKind)->Frame;
		const double Weave = FindSupport(Entry.Support, Entry.Kind)->ThicknessMeters;
		// Its lower-left corner on the wall's plane; its back hangs offWall out of it, on the stage's batten and lining.
		const FMuseeFrame Corner(Wall.PointToParent(FVector(Entry.Along, 0.0, Entry.Height)), Wall.Rotation);
		OutPieces.Add(SheetPiece(FName(ObjectName(Entry, Plan.IsSplit())), FName(ObjectWord(Entry.Support)), LayerOf(Canvas), Corner, Entry.SizeMeters(), Weave,
			Entry.OffWall + Weave, false));
	}
}
