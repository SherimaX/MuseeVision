#include "ChenghuaiHallTrades.h"
#include "ChenghuaiLattice.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlan.h"
#include "ChenghuaiPlanHalls.h"
#include "ChenghuaiTrades.h"

/**
 * The gates (§3.7; ChenghuaiHouse.cpp:268-463): the 广亮大门's joinery under its roof (columns, beams and struts, the door
 * frame on its ridge line with the pins, the black leaves open, the knockers, the stone drums); the festooned gate (垂花门),
 * its platform, columns, green cantilever beams and hanging posts ending in gilt lotus buds, its band of fretwork, its door
 * frame and the green screen door shut behind; and the screen wall (影壁) facing the gate across the entry yard.
 */
namespace Musee::Chenghuai
{
	namespace
	{
		/** A tapered round shaft standing on a floor, radius R0 at its foot to R1 at its top. */
		FFormProfile Shaft(double R0, double R1, double Height)
		{
			return Section({{0.0, 0.0}, {R0, 0.0}, {R1, Height}, {0.0, Height}}, false);
		}

		void GateColumns(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& S)
		{
			const FBuildingFrame& F = Spec.Frame;
			const double Floor = Spec.Floor;
			const double Half = 0.5 * Plan::GateBase;
			int32 Count = 0;
			for (const double U : Plan::GateColumnsU)
			{
				for (const double V : Plan::GateColumnsV)
				{
					const FString Name = FString::Printf(TEXT("gate.column.%d"), ++Count);
					Block(Out, Name + TEXT(".base"), TEXT("stone.base"), F, U - Half, U + Half, V - Half, V + Half, Floor + Plan::BaseStoneBottom,
						Floor + Plan::BaseStoneTop, EHit::Solid);
					Turned(Out, Name + TEXT(".shaft"), TEXT("timber.red"), F.At(U, V, Floor + Plan::BaseStoneTop),
						Shaft(Plan::GateColumnRadius, Plan::GateColumnTop, S.Zc - Floor - Plan::BaseStoneTop), EHit::Solid);
				}
				// The middle column (中柱) on the ridge line, the door frame standing between the two.
				const double Top = S.Seat[2] - Plan::GateMiddleUnder;
				Turned(Out, FString::Printf(TEXT("gate.middle-column.%d"), Count / 2), TEXT("timber.red"), F.At(U, Plan::GateMiddleV, Floor),
					Shaft(Plan::GateMiddleRadius, Plan::GateMiddleTop, Top - Floor), EHit::Solid);
				// Beams across, front and back halves, under the purlins, and a strut on each at the purlin between.
				const double W = 0.5 * Plan::GateBeamWidth;
				const double H = 0.5 * Plan::GateBeamHeight;
				const double Front = S.Seat[0] - Plan::GateBeamUnder;
				const double Back = S.Seat[4] - Plan::GateBeamUnder;
				Block(Out, FString::Printf(TEXT("gate.beam.%d.front"), Count / 2), TEXT("timber.red"), F, U - W, U + W, Plan::GateBeamFront,
					Plan::GateMiddleV, Front - H, Front + H, EHit::None);
				Block(Out, FString::Printf(TEXT("gate.beam.%d.back"), Count / 2), TEXT("timber.red"), F, U - W, U + W, Plan::GateMiddleV,
					Plan::GateBeamBack, Back - H, Back + H, EHit::None);
				const double Strut = 0.5 * Plan::GateStrut;
				for (int32 Side = 0; Side < 2; ++Side)
				{
					const double V = Plan::GateStrutsV[Side];
					const double Foot = (Side == 0 ? Front : Back) + H;
					Block(Out, FString::Printf(TEXT("gate.strut.%d.%d"), Count / 2, Side + 1), TEXT("timber.red"), F, U - Strut, U + Strut, V - Strut,
						V + Strut, Foot, S.Seat[Side == 0 ? 1 : 3] - Plan::GateStrutUnder, EHit::None);
				}
			}
		}

		void GateDoor(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& S)
		{
			const FBuildingFrame& F = Spec.Frame;
			const double Floor = Spec.Floor;
			const double V = Plan::GateMiddleV;
			const double Axis = 0.5 * (Spec.U0 + Spec.U1);
			const double UA = Axis - 0.5 * Plan::GateDoorClear;
			const double UB = Axis + 0.5 * Plan::GateDoorClear;
			const double Post = Plan::GateDoorPost;
			const double PostHalf = 0.5 * Plan::GateDoorPostDeep;
			const double Head = Floor + Plan::GateDoorHead;
			const double RailA = Axis - Plan::GateRailHalf;
			const double RailB = Axis + Plan::GateRailHalf;
			const double TopRail = S.Seat[2] - Plan::GateTopRailUnder;
			const double Panel = 0.5 * Plan::GatePanel;
			const TCHAR* Red = TEXT("timber.red");
			Block(Out, TEXT("gate.door.post.1"), Red, F, UA - Post, UA, V - PostHalf, V + PostHalf, Floor, Head, EHit::Solid);
			Block(Out, TEXT("gate.door.post.2"), Red, F, UB, UB + Post, V - PostHalf, V + PostHalf, Floor, Head, EHit::Solid);
			Block(Out, TEXT("gate.door.sill"), Red, F, UA - Post, UB + Post, V - 0.5 * Plan::GateSillDeep, V + 0.5 * Plan::GateSillDeep, Floor,
				Floor + Plan::GateSill, EHit::Solid);
			Block(Out, TEXT("gate.door.middle-rail"), Red, F, RailA, RailB, V - PostHalf, V + PostHalf, Head, Head + Plan::GateRail, EHit::Solid);
			Block(Out, TEXT("gate.door.top-rail"), Red, F, RailA, RailB, V - PostHalf, V + PostHalf, TopRail - Plan::GateTopRail, TopRail, EHit::None);
			Block(Out, TEXT("gate.door.panel-over"), Red, F, RailA, RailB, V - Panel, V + Panel, Head + Plan::GateRail, TopRail - Plan::GateTopRail,
				EHit::Solid);
			Block(Out, TEXT("gate.door.side-panel.1"), Red, F, RailA, UA - Post, V - Panel, V + Panel, Floor + Plan::GateSill, Head, EHit::Solid);
			Block(Out, TEXT("gate.door.side-panel.2"), Red, F, UB + Post, RailB, V - Panel, V + Panel, Floor + Plan::GateSill, Head, EHit::Solid);
			// The four plain door pins (门簪) on the middle rail, pointing south out of the gate.
			int32 Pin = 0;
			for (const double X : Plan::GatePinsX)
			{
				TurnedAbout(Out, FString::Printf(TEXT("gate.door.pin.%d"), ++Pin), TEXT("timber.green"), F.At(Axis + X, V - PostHalf, Floor + Plan::GatePinsZ),
					-F.V3(), Section(Plan::PinSection, false), EHit::Solid);
			}
			// The leaves, black, swung open north against the jambs; a knocker's plate and ring on each, facing across the way.
			const double Leaf = Plan::GateLeaf;
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const double Toward = Side == 0 ? 1.0 : -1.0;
				const double Hinge = Side == 0 ? UA : UB;
				const double Face = Hinge + Toward * Plan::GateLeafThick;
				const FString Name = FString::Printf(TEXT("gate.door.leaf.%d"), Side + 1);
				const double V0 = V + Plan::GateLeafFromRail;
				Block(Out, Name, TEXT("timber.black"), F, FMath::Min(Hinge, Face), FMath::Max(Hinge, Face), V0, V0 + Leaf, Floor + Plan::GateLeafFoot,
					Head - Plan::GateLeafHead, EHit::Solid);
				const FVector Across = F.U3() * Toward;
				const FVector Plate = F.At(Face, V0 + Plan::KnockerAlong * Leaf, Floor + Plan::KnockerZ);
				TurnedAbout(Out, Name + TEXT(".knocker"), TEXT("bronze.hardware"), Plate, Across, Section(Plan::KnockerSection, false), EHit::Solid);
				// The ring hangs from the plate in the leaf's plane: a torus about the axis square to the leaf.
				FFormLathe Ring;
				Ring.Section = Circle(FVector2D(Plan::RingRadius, 0.0), Plan::RingTube);
				const FVector Hub = Plate + Across * Plan::RingOut - FVector::UpVector * Plan::RingDrop;
				Draw(Out, Name + TEXT(".ring"), TEXT("bronze.hardware"), MoveTemp(Ring), EHit::Solid,
					FrameOfAxes(Hub, F.V3(), FVector::CrossProduct(Across, F.V3())));
			}
			// The stone drums (门墩) beside the sill, out toward the steps: a base, a box and the lion's block.
			int32 Drum = 0;
			for (const double X : {-Plan::DrumX, Plan::DrumX})
			{
				const FString Name = FString::Printf(TEXT("gate.drum.%d"), ++Drum);
				const FBuildingFrame Stage = FBuildingFrame::Stage();
				const double Base = 0.5 * Plan::DrumBaseWide;
				const double Box = 0.5 * Plan::DrumBoxWide;
				const double Lion = 0.5 * Plan::DrumLionWide;
				Block(Out, Name + TEXT(".base"), TEXT("stone.base"), Stage, X - Base, X + Base, Plan::DrumBaseY1, Plan::DrumBaseY0,
					Floor - Plan::BackBaseUnder, Floor + Plan::DrumBaseTop, EHit::Solid);
				Block(Out, Name + TEXT(".box"), TEXT("stone.base"), Stage, X - Box, X + Box, Plan::DrumBoxY0 - Plan::DrumBoxLong, Plan::DrumBoxY0,
					Floor + Plan::DrumBaseTop, Floor + Plan::DrumBoxTop, EHit::Solid);
				Block(Out, Name + TEXT(".lion"), TEXT("stone.base"), Stage, X - Lion, X + Lion, Plan::DrumLionY0 - Plan::DrumLionLong, Plan::DrumLionY0,
					Floor + Plan::DrumBoxTop, Floor + Plan::DrumLionTop, EHit::Solid);
			}
		}

		/** The 垂花门's platform: a rubbed body on its footing, a 0.30 stone kerb round the top, the floor inside. */
		void ChuiPlatform(TArray<FFormDrawing>& Out)
		{
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			const double X0 = Plan::ChuiX0;
			const double X1 = Plan::ChuiX1;
			const double Y0 = Plan::ChuiY1;
			const double Y1 = Plan::ChuiY0;
			const double Floor = Plan::ChuiFloor;
			const double Foot = Floor - Plan::KerbDepth;
			const double Out5 = Plan::FootingProud;
			const double K = Plan::ChuiKerb;
			Block(Out, TEXT("chuihuamen.platform.footing"), TEXT("stone.kerb"), Stage, X0 - Out5, X1 + Out5, Y0 - Out5, Y1 + Out5, Plan::FootingBottom,
				Plan::FootingTop, EHit::Walkable);
			Block(Out, TEXT("chuihuamen.platform.body"), TEXT("brick.rubbed"), Stage, X0, X1, Y0, Y1, Plan::FootingBottom, Foot, EHit::Solid);
			const TArray<FVector2D> Inside = Rectangle(X0 + K, Y0 + K, X1 - K, Y1 - K);
			Raised(Out, TEXT("chuihuamen.platform.kerb"), TEXT("stone.kerb"), Stage, Rectangle(X0, Y0, X1, Y1), {Inside}, Foot, Floor, EHit::Walkable);
			Raised(Out, TEXT("chuihuamen.platform.floor"), TEXT("floor.chisi"), Stage, Inside, {}, Foot, Floor, EHit::Walkable);
		}

		void Chuihuamen(TArray<FFormDrawing>& Out)
		{
			ChuiPlatform(Out);
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			const FHallSpec FrontSpec = Plan::ChuihuaRoof(true);
			const FHallSpec BackSpec = Plan::ChuihuaRoof(false);
			const FHallSection Front = FHallSection::Of(FrontSpec);
			const FHallSection Back = FHallSection::Of(BackSpec);
			const double Floor = Plan::ChuiFloor;
			const double Base = 0.5 * Plan::ChuiBase;
			int32 Count = 0;
			for (const double X : Plan::ChuiColumnsX)
			{
				++Count;
				for (const bool bWall : {true, false})
				{
					const double Y = bWall ? Plan::ChuiWallY : Plan::ChuiBackY;
					const double Top = (bWall ? Front.Seat[2] : Back.Seat[3]) - Plan::ChuiColumnUnder;
					const FString Name = FString::Printf(TEXT("chuihuamen.column.%d.%s"), Count, bWall ? TEXT("wall") : TEXT("back"));
					Block(Out, Name + TEXT(".base"), TEXT("stone.base"), Stage, X - Base, X + Base, Y - Base, Y + Base, Floor + Plan::BaseStoneBottom,
						Floor + Plan::BaseStoneTop, EHit::Solid);
					Turned(Out, Name + TEXT(".shaft"), TEXT("timber.red"), FVector(X, Y, Floor + Plan::BaseStoneTop),
						Shaft(Plan::ChuiColumnRadius, Plan::ChuiColumnTop, Top - Floor - Plan::BaseStoneTop), EHit::Solid);
				}
				// The cantilever beam (麻叶抱头梁) through the columns out over the hanging post, which hangs from it to its bud.
				const double W = 0.5 * Plan::ChuiBeamWide;
				const double H = 0.5 * Plan::ChuiBeamHigh;
				Block(Out, FString::Printf(TEXT("chuihuamen.beam.%d"), Count), TEXT("timber.green"), Stage, X - W, X + W, Plan::ChuiBeamY0, Plan::ChuiBeamY1,
					Plan::ChuiBeamZ - H, Plan::ChuiBeamZ + H, EHit::None);
				Rod(Out, FString::Printf(TEXT("chuihuamen.hanging-post.%d"), Count), TEXT("timber.red"), FVector(X, Plan::ChuiPostY, Plan::ChuiPostFoot),
					FVector(X, Plan::ChuiPostY, Plan::ChuiPostTop), Plan::ChuiPostRadius, EHit::None);
				Turned(Out, FString::Printf(TEXT("chuihuamen.bud.%d"), Count), TEXT("gilt.bud"), FVector(X, Plan::ChuiPostY, Plan::ChuiBudZ),
					Section(Plan::BudSection, false), EHit::None);
			}
			// The band of step-brocade fretwork between the hanging posts, under the front roof's lintel.
			const double Post = Plan::ChuiPostRadius;
			const double BandA = Plan::ChuiColumnsX[0] + Post;
			const double BandB = Plan::ChuiColumnsX[1] - Post;
			DrawPanel(Out, TEXT("chuihuamen.band"), TEXT("timber.red"), Stage, BandA, Plan::ChuiPostY - 0.5 * Plan::LatticeDepth, Plan::ChuiBandBottom,
				BandB - BandA, Plan::Fretwork, ELattice::StepBrocade, false);
			// The door frame on the wall's line (its leaves taken down by day).
			const double Thick = 0.5 * Plan::ChuiDoorThick;
			const double Y = Plan::ChuiWallY;
			for (int32 Side = 0; Side < 2; ++Side)
			{
				Block(Out, FString::Printf(TEXT("chuihuamen.door.post.%d"), Side + 1), TEXT("timber.red"), Stage, Plan::ChuiDoorPosts[Side][0],
					Plan::ChuiDoorPosts[Side][1], Y - Thick, Y + Thick, Floor, Plan::ChuiDoorHead0, EHit::Solid);
			}
			const double DoorA = Plan::ChuiDoorPosts[0][0];
			const double DoorB = Plan::ChuiDoorPosts[1][1];
			Block(Out, TEXT("chuihuamen.door.head"), TEXT("timber.red"), Stage, DoorA, DoorB, Y - Thick, Y + Thick, Plan::ChuiDoorHead0, Plan::ChuiDoorHead1,
				EHit::Solid);
			Block(Out, TEXT("chuihuamen.door.sill"), TEXT("timber.red"), Stage, DoorA, DoorB, Y - Thick, Y + Thick, Floor, Floor + Plan::ChuiSill, EHit::Solid);
			// The green screen door (屏门), shut, its four panels each with a red disc facing the front court; its head, board and sill.
			const double SY = Plan::ChuiScreenY;
			const double Panel = 0.5 * Plan::ChuiScreenThick;
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const double X0 = Plan::ChuiScreenX0 + Plan::ChuiScreenPitch * Index;
				const double X1 = X0 + Plan::ChuiScreenPanel;
				const FString Name = FString::Printf(TEXT("chuihuamen.screen.panel.%d"), Index + 1);
				Block(Out, Name, TEXT("timber.green"), Stage, X0, X1, SY - Panel, SY + Panel, Plan::ChuiScreenFoot, Plan::ChuiScreenTop, EHit::Solid);
				FFormExtrusion Disc;
				Disc.Outline = Circle(FVector2D::ZeroVector, Plan::ChuiDiscRadius);
				Disc.DepthMeters = Plan::ChuiDiscThick;
				// Laid on the panel's south face, standing out towards the south (+y).
				Draw(Out, Name + TEXT(".disc"), TEXT("timber.red"), MoveTemp(Disc), EHit::Solid,
					FrameOfAxes(FVector(0.5 * (X0 + X1), SY + Panel, Plan::ChuiDiscZ), FVector::ForwardVector, -FVector::UpVector));
			}
			const double Deep = 0.5 * Plan::ChuiScreenHeadDeep;
			const double ScreenA = Plan::ChuiDoorPosts[0][0];
			const double ScreenB = Plan::ChuiDoorPosts[1][1];
			const double HeadTop = Plan::ChuiScreenTop + Plan::ChuiScreenHead;
			Block(Out, TEXT("chuihuamen.screen.head"), TEXT("timber.red"), Stage, ScreenA, ScreenB, SY - Deep, SY + Deep, Plan::ChuiScreenTop, HeadTop,
				EHit::Solid);
			Block(Out, TEXT("chuihuamen.screen.board"), TEXT("timber.red"), Stage, ScreenA, ScreenB, SY - 0.5 * Plan::ChuiScreenBoard,
				SY + 0.5 * Plan::ChuiScreenBoard, HeadTop, Plan::ChuiDoorHead1, EHit::Solid);
			Block(Out, TEXT("chuihuamen.screen.sill"), TEXT("timber.red"), Stage, ScreenA, ScreenB, SY - Deep, SY + Deep, Floor, Plan::ChuiScreenFoot,
				EHit::Solid);
		}

		/** The screen wall (影壁): its stone plinth in three stages, the body, the heart in its frame, the cornice, the coping. */
		void ScreenWall(TArray<FFormDrawing>& Out)
		{
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			const double H = Plan::ScreenHalf;
			const double Back = Plan::ScreenBack;
			const double Face = Plan::ScreenFace;
			int32 Stage3 = 0;
			for (const Plan::FPlinthStage& Part : Plan::ScreenPlinth)
			{
				Block(Out, FString::Printf(TEXT("screen-wall.plinth.%d"), ++Stage3), TEXT("stone.base"), Stage, -H - Part.Proud, H + Part.Proud, Back,
					Face + Part.Proud, Part.Bottom, Part.Top, EHit::Solid);
			}
			const double Body0 = Plan::ScreenPlinth[2].Top;
			Block(Out, TEXT("screen-wall.body"), TEXT("brick.rubbed"), Stage, -H, H, Back, Face, Body0, Plan::ScreenBodyTop, EHit::Solid);
			const double Half = 0.5 * Plan::ScreenHeart;
			const double Z = Plan::ScreenHeartZ;
			const TArray<FVector2D> Heart = Rectangle(-Half, Z - Half, Half, Z + Half);
			const double Frame = Plan::ScreenFrame;
			AlongV(Out, TEXT("screen-wall.heart.frame"), TEXT("brick.rubbed"), Stage, Rectangle(-Half - Frame, Z - Half - Frame, Half + Frame, Z + Half + Frame),
				{Heart}, Face, Face + Plan::ScreenFrameProud, EHit::Solid);
			AlongV(Out, TEXT("screen-wall.heart"), TEXT("screen-wall.heart"), Stage, Heart, {}, Face,
				Face + Plan::ScreenFrameProud - Plan::ScreenHeartBack, EHit::Solid);
			for (int32 K = 0; K < Plan::CorniceCourses; ++K)
			{
				const double Step = Plan::ScreenCorniceStep * (K + 1);
				const double Z0 = Plan::ScreenBodyTop + Plan::CorniceCourse * K;
				Block(Out, FString::Printf(TEXT("screen-wall.cornice.%d"), K + 1), TEXT("brick.rubbed"), Stage, -H - Step, H + Step, Back, Face + Step, Z0,
					Z0 + Plan::CorniceCourse, EHit::Solid);
			}
			const double Front = Face + Plan::ScreenCorniceStep * Plan::CorniceCourses;
			DrawCoping(Out, TEXT("screen-wall.coping"), Stage, -Plan::ScreenCopingHalf, Plan::ScreenCopingHalf, 0.5 * (Back + Front), Front - Back,
				Plan::ScreenCopingZ);
		}
	}

	void DrawGates(TArray<FFormDrawing>& Out)
	{
		const FHallSpec Gate = Plan::GateHall();
		const FHallSection Section = FHallSection::Of(Gate);
		GateColumns(Out, Gate, Section);
		GateDoor(Out, Gate, Section);
		Chuihuamen(Out);
		ScreenWall(Out);
	}
}
