#include "ChenghuaiLattice.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlan.h"
#include "ChenghuaiTrades.h"

/**
 * The free-standing walls (§3.5; ChenghuaiParts.cpp:16-340, ChenghuaiHouse.cpp:467-531): each on a base course of rubbed
 * brick, its body in its faces' finishes (grey brick, or the white 粉墙), under a tiled coping; its doors with their
 * thresholds, frames and leaves folded open; its leak windows (漏窗), each a panel with a shaped hole, a rim and a lattice
 * of fired-clay bars; the moon gate (月洞门) with its ring of rubbed brick and its stone sill; and the porch's low walls on
 * the Rotunda's drum.
 */
namespace Musee::Chenghuai
{
	namespace
	{
		/** A wall's own frame: u along its run, v across it (the plan's y for a wall along x; x for one along y), z up. */
		FBuildingFrame WallFrame(const Plan::FWallPlan& W)
		{
			return W.bAlongX ? FBuildingFrame::Stage() : FBuildingFrame::Of(FVector2D::ZeroVector, FVector2D(0.0, -1.0), FVector2D(1.0, 0.0));
		}

		/** Where a plan coordinate along the wall's run falls on its u. */
		double UOf(const Plan::FWallPlan& W, double A)
		{
			return W.bAlongX ? A : -A;
		}

		/** A wall's layers across its thickness: its body in the low face's finish, and a skin of the high face's where they differ. */
		struct FLayer
		{
			const TCHAR* Surface;
			double V0;
			double V1;
			const TCHAR* Part;
		};

		TArray<FLayer> Layers(const Plan::FWallPlan& W, double Past = 0.0)
		{
			if (FCString::Strcmp(W.FaceLow, W.FaceHigh) == 0)
			{
				return {{W.FaceLow, W.Face0 - Past, W.Face1 + Past, TEXT("body")}};
			}
			return {{W.FaceLow, W.Face0 - Past, W.Face1 - Plan::Skin, TEXT("body")}, {W.FaceHigh, W.Face1 - Plan::Skin, W.Face1 + Past, TEXT("skin")}};
		}

		void Layered(TArray<FFormDrawing>& Out, const FString& Name, const Plan::FWallPlan& W, const FBuildingFrame& F, TConstArrayView<FVector2D> Outline,
			TConstArrayView<TArray<FVector2D>> Holes)
		{
			for (const FLayer& Layer : Layers(W))
			{
				AlongV(Out, Name + TEXT(".") + Layer.Part, Layer.Surface, F, Outline, Holes, Layer.V0, Layer.V1, EHit::Solid);
			}
		}

		/** An opening's span along the wall's u. */
		struct FHole
		{
			Plan::FWallHolePlan Plan;
			double A = 0.0;
			double B = 0.0;
		};

		/** The moon gate's outline: a circle about (Centre, Z) cut flat at z Cut, its arc run over the top from the cut's right end. */
		TArray<FVector2D> MoonOutline(double Centre, double Radius, double Cut)
		{
			const double Half = FMath::Acos(FMath::Clamp((Plan::MoonZ - Cut) / Radius, -1.0, 1.0));
			// From the cut's right end (angle -90° + Half from +u, anticlockwise) round to its left end.
			const double From = -UE_DOUBLE_HALF_PI + Half;
			const double To = 1.5 * UE_DOUBLE_PI - Half;
			TArray<FVector2D> Points;
			for (int32 K = 0; K <= Plan::MoonStations; ++K)
			{
				const double T = FMath::Lerp(From, To, double(K) / Plan::MoonStations);
				Points.Add(FVector2D(Centre + Radius * FMath::Cos(T), Plan::MoonZ + Radius * FMath::Sin(T)));
			}
			return Points;
		}

		void LeakWindow(TArray<FFormDrawing>& Out, const Plan::FWallPlan& W, const FBuildingFrame& F, const FHole& Hole, int32 Index)
		{
			const FString Name = FString::Printf(TEXT("%s.window.%d"), W.Local, Index);
			const FVector2D Centre(0.5 * (Hole.A + Hole.B), 0.5 * (Hole.Plan.Z0 + Hole.Plan.Z1));
			const double Radius = Plan::LeakRadius * FMath::Min(Hole.B - Hole.A, Hole.Plan.Z1 - Hole.Plan.Z0);
			const TArray<FVector2D> Shape = LeakShape(Hole.Plan.Leak, Centre, Radius);
			// The panel filling the window's rectangle, pierced by the shape.
			Layered(Out, Name + TEXT(".panel"), W, F, Rectangle(Hole.A, Hole.Plan.Z0, Hole.B, Hole.Plan.Z1), {Shape});
			// The rim round the shape (its inside the shape moved in by the rim's width), and the lattice's bars on the wall's centre
			// plane, their outline the shape itself so every opening lies inside it; the rim, deeper, covers their meeting.
			const FLoops Openings = LeakOpenings(Hole.Plan.Leak, Shape, Centre, Radius, Musee::Seed(Key(Name)));
			const double Mid = 0.5 * (W.Face0 + W.Face1);
			TArray<FVector2D> RimInside;
			const double Ratio = (Radius - Plan::LeakRim / FMath::Cos(UE_DOUBLE_PI / Shape.Num())) / Radius;
			for (const FVector2D& Corner : Shape)
			{
				RimInside.Add(Centre + (Corner - Centre) * Ratio);
			}
			AlongV(Out, Name + TEXT(".rim"), TEXT("leak-window.lattice"), F, Shape, {RimInside}, Mid - 0.5 * Plan::LeakRimDepth, Mid + 0.5 * Plan::LeakRimDepth,
				EHit::Solid);
			AlongV(Out, Name + TEXT(".lattice"), TEXT("leak-window.lattice"), F, Shape, Openings, Mid - 0.5 * Plan::LeakBarDepth,
				Mid + 0.5 * Plan::LeakBarDepth, EHit::Solid);
		}

		void Door(TArray<FFormDrawing>& Out, const Plan::FWallPlan& W, const FBuildingFrame& F, const FHole& Hole, int32 Index)
		{
			const FString Name = FString::Printf(TEXT("%s.door.%d"), W.Local, Index);
			const double Top = Plan::CourtLevel + Plan::ThresholdOver;
			const double Head = Hole.Plan.Z1;
			Block(Out, Name + TEXT(".threshold"), TEXT("stone.step"), F, Hole.A - Plan::ThresholdWider, Hole.B + Plan::ThresholdWider,
				W.Face0 - Plan::ThresholdPast, W.Face1 + Plan::ThresholdPast, -Plan::ThresholdBed, Top, EHit::Walkable);
			const bool bStone = Hole.Plan.Kind == Plan::EWallHole::StoneDoor;
			const TCHAR* Frame = bStone ? TEXT("stone.step") : TEXT("timber.red");
			const double Mid = 0.5 * (W.Face0 + W.Face1);
			// A stone surround lines the whole reveal; a timber frame stands on the wall's centre.
			const double V0 = bStone ? W.Face0 - Plan::Skin : Mid - 0.5 * Plan::DoorPost;
			const double V1 = bStone ? W.Face1 + Plan::Skin : Mid + 0.5 * Plan::DoorPost;
			Block(Out, Name + TEXT(".post.1"), Frame, F, Hole.A, Hole.A + Plan::DoorPost, V0, V1, Top, Head, EHit::Solid);
			Block(Out, Name + TEXT(".post.2"), Frame, F, Hole.B - Plan::DoorPost, Hole.B, V0, V1, Top, Head, EHit::Solid);
			Block(Out, Name + TEXT(".head"), Frame, F, Hole.A, Hole.B, V0, V1, Head - Plan::DoorHeadRail, Head, EHit::Solid);
			if (bStone)
			{
				return;
			}
			// The leaves, folded open against the reveals on the wall's +v side.
			const TCHAR* Leaves = Hole.Plan.Kind == Plan::EWallHole::ScreenDoor ? TEXT("timber.green") : TEXT("timber.red");
			const double Leaf = 0.5 * (Hole.B - Hole.A - 2.0 * Plan::DoorPost);
			const double L0 = Mid + 0.5 * Plan::DoorPost;
			Block(Out, Name + TEXT(".leaf.1"), Leaves, F, Hole.A + Plan::DoorPost, Hole.A + Plan::DoorPost + Plan::DoorLeaf, L0, L0 + Leaf,
				Top + Plan::LeafClear, Head - Plan::DoorHeadRail - Plan::LeafClear, EHit::Solid);
			Block(Out, Name + TEXT(".leaf.2"), Leaves, F, Hole.B - Plan::DoorPost - Plan::DoorLeaf, Hole.B - Plan::DoorPost, L0, L0 + Leaf,
				Top + Plan::LeafClear, Head - Plan::DoorHeadRail - Plan::LeafClear, EHit::Solid);
		}

		/** The panel round the moon gate's ring, the ring of rubbed brick lining it and standing proud of both faces, the sill. */
		void MoonGate(TArray<FFormDrawing>& Out, const Plan::FWallPlan& W, const FBuildingFrame& F, const FHole& Hole, double PanelTop)
		{
			const FString Name = FString::Printf(TEXT("%s.moon-gate"), W.Local);
			const double Centre = UOf(W, Plan::MoonY);
			const double Outer = Plan::MoonRadius + Plan::MoonRing;
			const TArray<FVector2D> Around = MoonOutline(Centre, Outer, Plan::MoonCut);
			Layered(Out, Name + TEXT(".panel"), W, F, Rectangle(Hole.A, Plan::WallFoot, Hole.B, PanelTop), {Around});
			TArray<FVector2D> Ring = Around;
			const TArray<FVector2D> Inside = MoonOutline(Centre, Plan::MoonRadius, Plan::MoonCut);
			for (int32 Index = Inside.Num() - 1; Index >= 0; --Index)
			{
				Ring.Add(Inside[Index]);
			}
			AlongV(Out, Name + TEXT(".ring"), TEXT("brick.rubbed"), F, Ring, {}, W.Face0 - Plan::MoonRingProud, W.Face1 + Plan::MoonRingProud, EHit::Solid);
			const double Sill = 0.5 * Plan::MoonSillWidth;
			Block(Out, Name + TEXT(".sill"), TEXT("stone.step"), F, Centre - Sill, Centre + Sill, W.Face0 - Plan::MoonSillPast, W.Face1 + Plan::MoonSillPast,
				Plan::MoonSillBottom, Plan::MoonSillTop, EHit::Walkable);
		}

		void Wall(TArray<FFormDrawing>& Out, const Plan::FWallPlan& W)
		{
			const FBuildingFrame F = WallFrame(W);
			const double U0 = FMath::Min(UOf(W, W.A0), UOf(W, W.A1));
			const double U1 = FMath::Max(UOf(W, W.A0), UOf(W, W.A1));
			TArray<FHole> Holes;
			for (const Plan::FWallHolePlan& Plan : W.Holes)
			{
				Holes.Add({Plan, FMath::Min(UOf(W, Plan.A0), UOf(W, Plan.A1)), FMath::Max(UOf(W, Plan.A0), UOf(W, Plan.A1))});
			}
			Holes.Sort([](const FHole& A, const FHole& B) { return A.A < B.A; });
			// The body's elevation, notched up from its base at each door and round the moon gate's panel; the windows' holes.
			const double MoonTop = FMath::Min(W.Height, Plan::MoonZ + Plan::MoonRadius + Plan::MoonRing + Plan::MoonMargin);
			TArray<FVector2D> Body = {FVector2D(U0, W.Base)};
			TArray<TArray<FVector2D>> WindowHoles;
			TArray<TPair<double, double>> Gaps;
			int32 Doors = 0;
			int32 Windows = 0;
			for (FHole& Hole : Holes)
			{
				switch (Hole.Plan.Kind)
				{
				case Plan::EWallHole::Window:
					WindowHoles.Add(Rectangle(Hole.A, Hole.Plan.Z0, Hole.B, Hole.Plan.Z1));
					LeakWindow(Out, W, F, Hole, ++Windows);
					break;
				case Plan::EWallHole::MoonGate:
				{
					const double Centre = UOf(W, Plan::MoonY);
					const double Half = Plan::MoonRadius + Plan::MoonRing + Plan::MoonMargin;
					Hole.A = Centre - Half;
					Hole.B = Centre + Half;
					Body.Append({FVector2D(Hole.A, W.Base), FVector2D(Hole.A, MoonTop), FVector2D(Hole.B, MoonTop), FVector2D(Hole.B, W.Base)});
					Gaps.Add({Hole.A, Hole.B});
					MoonGate(Out, W, F, Hole, MoonTop);
					break;
				}
				default:
					Body.Append({FVector2D(Hole.A, W.Base), FVector2D(Hole.A, Hole.Plan.Z1), FVector2D(Hole.B, Hole.Plan.Z1), FVector2D(Hole.B, W.Base)});
					Gaps.Add({Hole.A, Hole.B});
					Door(Out, W, F, Hole, ++Doors);
					break;
				}
			}
			Body.Append({FVector2D(U1, W.Base), FVector2D(U1, W.Height), FVector2D(U0, W.Height)});
			Layered(Out, FString(W.Local), W, F, Body, WindowHoles);
			// The base course in runs between the doors, standing proud of both faces.
			double From = U0;
			int32 Run = 0;
			Gaps.Add({U1, U1});
			for (const TPair<double, double>& Gap : Gaps)
			{
				if (Gap.Key - From > Plan::WallBaseProud)
				{
					Block(Out, FString::Printf(TEXT("%s.base.%d"), W.Local, ++Run), W.BaseSurface, F, From, Gap.Key, W.Face0 - Plan::WallBaseProud,
						W.Face1 + Plan::WallBaseProud, Plan::WallFoot, W.Base, EHit::Solid);
				}
				From = Gap.Value;
			}
			if (W.bCoping)
			{
				DrawCoping(Out, FString(W.Local) + TEXT(".coping"), F, U0, U1, 0.5 * (W.Face0 + W.Face1), W.Face1 - W.Face0, W.Height);
			}
		}

		/** A porch wall from the site's south face to the drum, its end cut on the drum's cylinder, and its coping. */
		void PorchWall(TArray<FFormDrawing>& Out, double Side)
		{
			const FString Name = FString::Printf(TEXT("porch.wall.%s"), Side < 0.0 ? TEXT("west") : TEXT("east"));
			auto OnDrum = [](double X) { return Plan::DrumCentreY - FMath::Sqrt(FMath::Square(Plan::DrumRadius) - X * X); };
			auto Outline = [&](double Inner, double Outer)
			{
				TArray<FVector2D> Points = {FVector2D(Inner, Plan::SiteSouth), FVector2D(Outer, Plan::SiteSouth)};
				constexpr int32 Stations = 8;
				for (int32 K = 0; K <= Stations; ++K)
				{
					const double X = FMath::Lerp(Outer, Inner, double(K) / Stations);
					Points.Add(FVector2D(X, OnDrum(X)));
				}
				return Points;
			};
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			Raised(Out, Name, TEXT("brick.rubbed"), Stage, Outline(Side * Plan::PorchInner, Side * Plan::PorchOuter), {}, Plan::PorchWallBottom,
				Plan::PorchWallTop, EHit::Solid);
			const double Proud = Plan::PorchCopingProud;
			Raised(Out, Name + TEXT(".coping"), TEXT("porch.coping"), Stage, Outline(Side * (Plan::PorchInner - Proud), Side * (Plan::PorchOuter + Proud)), {},
				Plan::PorchWallTop, Plan::PorchCopingTop, EHit::Solid);
		}
	}

	void DrawCoping(TArray<FFormDrawing>& Out, const FString& Name, const FBuildingFrame& F, double A0, double A1, double Centre, double Thick,
		double Z)
	{
		const double Corbel = 0.5 * (Thick + Plan::CopingCorbelWider);
		Block(Out, Name + TEXT(".corbel"), TEXT("brick.rubbed"), F, A0, A1, Centre - Corbel, Centre + Corbel, Z - Plan::CopingCorbel, Z, EHit::Solid);
		const double Hf = 0.5 * Thick + Plan::CopingOverhang;
		const FVector2D Crown(Centre, Z + Plan::CopingCrown);
		const FVector2D Low(Centre - Hf, Z + Plan::CopingEaves);
		const FVector2D High(Centre + Hf, Z + Plan::CopingEaves);
		AlongU(Out, Name + TEXT(".bed"), TEXT("roof.bed"), F, {FVector2D(Centre - Hf, Z), FVector2D(Centre + Hf, Z), High, Crown, Low}, {}, A0, A1,
			EHit::Solid);
		// 合瓦 rows down both sides from 0.06 off the crown to the eave; each side's curve walked towards +v, so the rows stand up on it.
		const Plan::FTileStyle Style = Plan::He;
		const double Grid = Style.Pitch / Plan::TileStationsPerRow;
		const int32 Steps = FMath::FloorToInt32((A1 - A0) / Grid);
		const double To = A0 + (Steps - UE_KINDA_SMALL_NUMBER) * Grid;
		const double Slope = FVector2D::Distance(Low, Crown);
		for (const bool bHigh : {false, true})
		{
			const TArray<FVector2D> Curve = bHigh ? TArray<FVector2D>{Crown, High} : TArray<FVector2D>{Low, Crown};
			ReliefAlongU(Out, Name + (bHigh ? TEXT(".tiles.2") : TEXT(".tiles.1")), Style.Surface, F, Curve, A0, To, Grid, Plan::ReliefBack,
				[bHigh, Slope, A0, Style, Hf](double Distance, double U)
				{
					const double FromEave = bHigh ? Slope - Distance : Distance;
					if ((Slope - FromEave) / Slope * Hf < Plan::CopingRowsFrom)
					{
						return 0.0;
					}
					return TileHeight(Style, false, U - A0 - Style.PanHalf, FromEave);
				});
		}
		FFormSweep Ridge;
		Ridge.Section = Section(Plan::CopingRidge, true);
		Ridge.Path = FFormProfile::Polyline({F.Plan(A0, Centre), F.Plan(A1, Centre)}, false);
		Ridge.ElevationMeters = Crown.Y;
		Draw(Out, Name + TEXT(".ridge"), TEXT("roof.ridge"), MoveTemp(Ridge), EHit::None);
	}

	void DrawWalls(TArray<FFormDrawing>& Out)
	{
		for (const Plan::FWallPlan& W : Plan::Walls)
		{
			Wall(Out, W);
		}
		PorchWall(Out, -1.0);
		PorchWall(Out, 1.0);
	}
}
