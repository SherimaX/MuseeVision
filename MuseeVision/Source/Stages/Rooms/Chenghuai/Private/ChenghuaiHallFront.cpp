#include "ChenghuaiHallTrades.h"
#include "ChenghuaiLattice.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlan.h"
#include "FormOpening.h"

/**
 * A hall's front (装修) (§3.6; ChenghuaiHall.cpp:278-418, 1024-1119), filled bay by bay on its lattice line: the frame
 * (槛框) of posts against the columns and a head rail under the lintel; partition doors (隔扇) on a sill, four to a wide bay,
 * the middle pair swung into the room in an open bay, with a transom (横披) of lattice panels over a door head where there
 * is room; support-and-remove windows (支摘窗) on a brick sill wall, each half split into a plain lower sash and a patterned
 * upper one; or a brick bay. Its lattice and open door pairs are the room's openings.
 */
namespace Musee::Chenghuai::Hall
{
	namespace
	{
		/** The front's heights: its columns' top, the lintel's underside (the head), and the door's top. */
		struct FFrontLine
		{
			double V = 0.0;
			double Head = 0.0;
			double ColumnR = 0.0;
			double DoorTop = 0.0;
			bool bTransom = false;
		};

		FFrontLine LineOf(const FHallSpec& Spec, const FHallSection& S)
		{
			FFrontLine Line;
			Line.V = Spec.FacadeV;
			const bool bVeranda = Spec.FacadeV > 0.0;
			const double ColumnTop = bVeranda ? S.InnerColumnTop(S.PurlinAt(Spec.FacadeV)) : S.Zc;
			Line.Head = ColumnTop - (Plan::LintelPerDiameter * Spec.ColD + Plan::LintelOver);
			Line.ColumnR = 0.5 * Spec.ColD + (bVeranda ? Plan::VerandaColumnPast : 0.0);
			const double F = Spec.Floor;
			Line.DoorTop = FMath::Max(FMath::Min(F + Plan::DoorTopMost, Line.Head - Plan::DoorUnderHead), F + Plan::DoorTopLeast);
			Line.bTransom = Line.Head - Plan::FramePost - (Line.DoorTop + Plan::DoorHead) >= Plan::TransomLeast;
			if (!Line.bTransom)
			{
				Line.DoorTop = Line.Head - Plan::FramePost;
			}
			return Line;
		}

		/** A bay's span between its frame posts' outer faces: the gable's inner face or the column's face each side. */
		void BaySpan(const FHallSpec& Spec, const FFrontLine& Line, int32 Bay, double& OutA, double& OutB)
		{
			const bool bFirst = Bay == 0 && HasGable(Spec, false);
			const bool bLast = Bay + 2 == Spec.ColU.Num() && HasGable(Spec, true);
			OutA = bFirst ? Spec.U0 + Spec.GableT : Spec.ColU[Bay] + Line.ColumnR;
			OutB = bLast ? Spec.U1 - Spec.GableT : Spec.ColU[Bay + 1] - Line.ColumnR;
		}

		int32 LeavesIn(double Width)
		{
			return Width > Plan::FourLeafBay ? 4 : 2;
		}

		/** A drawing frame facing out of the room through the front at (u, z): x along the front, y up, z towards -v. */
		FMuseeFrame Facing(const FHallSpec& Spec, double U, double V, double Z)
		{
			const FBuildingFrame& F = Spec.Frame;
			return FrameOfAxes(F.At(U, V, Z), F.U3() * F.Hand(), FVector::UpVector);
		}

		void WallBay(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FFrontLine& Line, const FString& Bay, double A, double B)
		{
			const FBuildingFrame& F = Spec.Frame;
			const double V = Line.V;
			const double F0 = Spec.Floor;
			const double Half = 0.5 * Plan::BayWall;
			const double BaseHalf = 0.5 * Plan::BayWallBase;
			Block(Out, Named(Spec, Bay + TEXT(".base")), Spec.WallBase, F, A, B, V - BaseHalf, V + BaseHalf, F0 - Plan::BackBaseUnder, F0 + Plan::WallBase,
				EHit::Solid);
			Block(Out, Named(Spec, Bay + TEXT(".wall")), Spec.WallBody, F, A, B, V - Half, V + Half - Plan::Skin, F0 + Plan::WallBase, Line.Head,
				EHit::Solid);
			Block(Out, Named(Spec, Bay + TEXT(".skin")), TEXT("room.plaster"), F, A, B, V + Half - Plan::Skin, V + Half, F0, Line.Head, EHit::Solid);
		}

		void Doors(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FFrontLine& Line, const FString& Bay, double IA, double IB, bool bOpen,
			ELattice Pattern)
		{
			const FBuildingFrame& F = Spec.Frame;
			const double V = Line.V;
			const double F0 = Spec.Floor;
			const double Sill = 0.5 * Plan::DoorSillThick;
			Block(Out, Named(Spec, Bay + TEXT(".sill")), Spec.Timber, F, IA, IB, V - Sill, V + Sill, F0, F0 + Plan::DoorSill, EHit::Solid);
			if (Line.bTransom)
			{
				const double Top = Line.DoorTop + Plan::DoorHead;
				const double Under = Line.Head - Plan::FramePost;
				Block(Out, Named(Spec, Bay + TEXT(".door-head")), Spec.Timber, F, IA, IB, V - Sill, V + Sill, Line.DoorTop, Top, EHit::Solid);
				const int32 Panels = FMath::Max(1, FMath::RoundToInt32((IB - IA) / Plan::TransomPanel));
				const double Clear = Plan::TransomPanelClear;
				for (int32 Index = 0; Index < Panels; ++Index)
				{
					const double TA = FMath::Lerp(IA, IB, double(Index) / Panels);
					const double TB = FMath::Lerp(IA, IB, double(Index + 1) / Panels);
					DrawPanel(Out, Named(Spec, FString::Printf(TEXT("%s.transom.%d"), *Bay, Index + 1)), Spec.Timber, F, TA + Clear,
						V - 0.5 * Plan::LatticeDepth, Top, TB - TA - 2.0 * Clear, Under - Top, Pattern, true);
					if (Index > 0)
					{
						const double M = 0.5 * Plan::TransomMullion;
						Block(Out, Named(Spec, FString::Printf(TEXT("%s.mullion.%d"), *Bay, Index)), Spec.Timber, F, TA - M, TA + M, V - 0.5 * Plan::FrameThick,
							V + 0.5 * Plan::FrameThick, Top, Under, EHit::Solid);
					}
				}
			}
			const int32 Leaves = LeavesIn(IB - IA);
			const double Width = (IB - IA) / Leaves;
			const double Clear = Plan::LeafClear;
			const double Z = F0 + Plan::DoorSill;
			const double Height = Line.DoorTop - Z;
			for (int32 K = 0; K < Leaves; ++K)
			{
				const FString Leaf = Named(Spec, FString::Printf(TEXT("%s.leaf.%d"), *Bay, K + 1));
				const double LA = IA + Width * K;
				const bool bLeft = K == Leaves / 2 - 1;
				const bool bSwung = bOpen && (bLeft || K == Leaves / 2);
				if (!bSwung)
				{
					DrawLeaf(Out, Leaf, Spec.Timber, F, LA + Clear, V - 0.5 * Plan::LeafThick, Z, Width - 2.0 * Clear, Height, Pattern);
					continue;
				}
				// Swung 90° into the room about its outer stile: the leaf now runs along v, back from its hinge.
				const double Hinge = bLeft ? LA + Clear : LA + Width - Clear;
				const FBuildingFrame Swung = F.Within(Hinge, V + Plan::LeafHinge, FVector2D(0.0, 1.0), FVector2D(bLeft ? 1.0 : -1.0, 0.0));
				DrawLeaf(Out, Leaf, Spec.Timber, Swung, 0.0, 0.0, Z, Width - 2.0 * Clear, Height, Pattern);
			}
		}

		void Windows(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FFrontLine& Line, const FString& Bay, double IA, double IB,
			ELattice Pattern)
		{
			const FBuildingFrame& F = Spec.Frame;
			const double V = Line.V;
			const double F0 = Spec.Floor;
			const double SillTop = F0 + Plan::SillBoardTop;
			Block(Out, Named(Spec, Bay + TEXT(".sill-wall")), TEXT("brick.rubbed"), F, IA, IB, V - 0.5 * Plan::SillWall, V + 0.5 * Plan::SillWall,
				F0 - Plan::BackBaseUnder, F0 + Plan::SillWallTop, EHit::Solid);
			Block(Out, Named(Spec, Bay + TEXT(".sill-board")), Spec.Timber, F, IA, IB, V - 0.5 * Plan::SillBoard, V + 0.5 * Plan::SillBoard,
				F0 + Plan::SillWallTop, SillTop, EHit::Solid);
			const double Frame = 0.5 * Plan::FrameThick;
			const double W0 = SillTop + Plan::WindowSill;
			const double W1 = Line.Head - Plan::FramePost;
			Block(Out, Named(Spec, Bay + TEXT(".window-sill")), Spec.Timber, F, IA, IB, V - Frame, V + Frame, SillTop, W0, EHit::Solid);
			const double Middle = 0.5 * (IA + IB);
			const double Post = 0.5 * Plan::WindowPost;
			Block(Out, Named(Spec, Bay + TEXT(".window-post")), Spec.Timber, F, Middle - Post, Middle + Post, V - Frame, V + Frame, W0, W1, EHit::Solid);
			const double Split = W0 + Plan::WindowSplit * (W1 - W0);
			const double Rail = 0.5 * Plan::WindowRail;
			const double Clear = Plan::PanelClear;
			for (int32 Half = 0; Half < 2; ++Half)
			{
				const double A = Half == 0 ? IA : Middle + Post;
				const double B = Half == 0 ? Middle - Post : IB;
				const FString Sash = FString::Printf(TEXT("%s.window.%d"), *Bay, Half + 1);
				Block(Out, Named(Spec, Sash + TEXT(".rail")), Spec.Timber, F, A, B, V - 0.5 * Plan::WindowRailDeep, V + 0.5 * Plan::WindowRailDeep,
					Split - Rail, Split + Rail, EHit::Solid);
				DrawPanel(Out, Named(Spec, Sash + TEXT(".lower")), Spec.Timber, F, A + Clear, V - 0.5 * Plan::LatticeDepth, W0 + Clear,
					B - A - 2.0 * Clear, Split - Rail - W0 - 2.0 * Clear, ELattice::Grid, true);
				DrawPanel(Out, Named(Spec, Sash + TEXT(".upper")), Spec.Timber, F, A + Clear, V - 0.5 * Plan::LatticeDepth, Split + Rail + Clear,
					B - A - 2.0 * Clear, W1 - Split - Rail - 2.0 * Clear, Pattern, true);
			}
		}
	}

	void DrawFront(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section)
	{
		if (Spec.FacadeV < 0.0)
		{
			return;
		}
		const FFrontLine Line = LineOf(Spec, Section);
		const FBuildingFrame& F = Spec.Frame;
		for (int32 Index = 0; Index + 1 < Spec.ColU.Num() && Index < Spec.Bays.Num(); ++Index)
		{
			const EBay Kind = Spec.Bays[Index];
			if (Kind == EBay::Open)
			{
				continue;
			}
			double A = 0.0;
			double B = 0.0;
			BaySpan(Spec, Line, Index, A, B);
			const FString Bay = FString::Printf(TEXT("bay.%d"), Index + 1);
			if (Kind == EBay::Wall)
			{
				WallBay(Out, Spec, Line, Bay, A, B);
				continue;
			}
			// The frame: a post against each column (or gable), the head rail under the lintel.
			const double Post = Plan::FramePost;
			const double Frame = 0.5 * Plan::FrameThick;
			const double V = Line.V;
			Block(Out, Named(Spec, Bay + TEXT(".post.1")), Spec.Timber, F, A, A + Post, V - Frame, V + Frame, Spec.Floor, Line.Head, EHit::Solid);
			Block(Out, Named(Spec, Bay + TEXT(".post.2")), Spec.Timber, F, B - Post, B, V - Frame, V + Frame, Spec.Floor, Line.Head, EHit::Solid);
			Block(Out, Named(Spec, Bay + TEXT(".head-rail")), Spec.Timber, F, A + Post, B - Post, V - Frame, V + Frame, Line.Head - Post, Line.Head,
				EHit::Solid);
			const bool bOpen = Kind == EBay::DoorsOpen;
			const ELattice Pattern = bOpen ? Spec.OpenBayLattice : Spec.Lattice;
			if (Kind == EBay::Windows)
			{
				Windows(Out, Spec, Line, Bay, A + Post, B - Post, Pattern);
			}
			else
			{
				Doors(Out, Spec, Line, Bay, A + Post, B - Post, bOpen, Pattern);
			}
		}
	}

	namespace
	{
		FFormOpening& Opening(TArray<FFormOpening>& Out, const FHallSpec& Spec, const FString& Local, EFormOpeningKind Kind, double U, double Z,
			const FVector2D& Size)
		{
			FFormOpening& Made = Out.AddDefaulted_GetRef();
			Made.Key = Key(Spec.Room + TEXT(".") + Local);
			Made.Kind = Kind;
			Made.Enclosure = Key(Spec.Room);
			Made.Frame = Facing(Spec, U, Spec.FacadeV, Z);
			Made.SizeMeters = Size;
			return Made;
		}
	}

	void DrawFrontOpenings(const FHallSpec& Spec, TArray<FFormOpening>& Out)
	{
		if (Spec.FacadeV < 0.0 || Spec.Room.IsEmpty())
		{
			return;
		}
		const FFrontLine Line = LineOf(Spec, FHallSection::Of(Spec));
		double First = 0.0;
		double Last = 0.0;
		double Ignored = 0.0;
		BaySpan(Spec, Line, 0, First, Ignored);
		BaySpan(Spec, Line, FMath::Min(Spec.Bays.Num(), Spec.ColU.Num() - 1) - 1, Ignored, Last);
		// The paper lattice over the whole front, facing out of the room: the room's one Glazed opening.
		Opening(Out, Spec, TEXT("lattice"), EFormOpeningKind::Glazed, 0.5 * (First + Last), 0.5 * (Spec.Floor + Line.Head),
			FVector2D(Last - First, Line.Head - Spec.Floor)).Glazing = Surface(TEXT("lattice.paper"));
		// Each open door pair, the room's way in and out.
		for (int32 Index = 0; Index + 1 < Spec.ColU.Num() && Index < Spec.Bays.Num(); ++Index)
		{
			if (Spec.Bays[Index] != EBay::DoorsOpen)
			{
				continue;
			}
			double A = 0.0;
			double B = 0.0;
			BaySpan(Spec, Line, Index, A, B);
			const double Inside = B - A - 2.0 * Plan::FramePost;
			const double Pair = 2.0 * Inside / LeavesIn(Inside);
			const double Sill = Spec.Floor + Plan::DoorSill;
			Opening(Out, Spec, FString::Printf(TEXT("door.%d"), Index + 1), EFormOpeningKind::Open, 0.5 * (A + B), 0.5 * (Sill + Line.DoorTop),
				FVector2D(Pair, Line.DoorTop - Sill));
		}
	}
}

namespace Musee::Chenghuai
{
	void HallOpenings(const FHallSpec& Spec, TArray<FFormOpening>& Out)
	{
		Hall::DrawFrontOpenings(Spec, Out);
	}
}
