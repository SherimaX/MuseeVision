#include "ChenghuaiLattice.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlanFittings.h"
#include "ChenghuaiTrades.h"

/**
 * The rooms' furnishing (§3.10; ChenghuaiDisplay.cpp:209-443, ChenghuaiHouse.cpp:658-772): the main hall's open screens
 * (落地罩); the Ming tables and official's hat chairs in phoebe, and 林泉's recessed-leg painting table and rose chairs in
 * huanghuali; the scholars' inkstones, brush pots and water droppers; the seventeen plain silk lanterns; the name boards
 * on their walls and 澄懷堂's on its painted lintel, and the couplet's shells round the hall's middle columns.
 */
namespace Musee::Chenghuai
{
	namespace
	{
		const TCHAR* const Phoebe = TEXT("furniture");
		const TCHAR* const Huanghuali = TEXT("furniture.huanghuali");

		void Screen(TArray<FFormDrawing>& Out, int32 Index, double X)
		{
			const FBuildingFrame F = FBuildingFrame::Of(FVector2D(X, Plan::ScreenY0), FVector2D(0.0, -1.0), FVector2D(1.0, 0.0));
			const FString Name = FString::Printf(TEXT("chenghuaitang.screen.%d"), Index);
			const TCHAR* Wood = TEXT("screen.phoebe");
			const double Length = Plan::ScreenY0 - Plan::ScreenY1;
			const double Floor = Plan::ScreenFloor;
			const double Top = Plan::ScreenTop;
			const double Post = Plan::ScreenPost;
			const double Leg = Plan::ScreenLeg;
			const double Frieze = Top - Plan::ScreenFrieze;
			const double Half = 0.5 * Plan::ScreenThick;
			Block(Out, Name + TEXT(".post.1"), Wood, F, 0.0, Post, -Half, Half, Floor, Top, EHit::Solid);
			Block(Out, Name + TEXT(".post.2"), Wood, F, Length - Post, Length, -Half, Half, Floor, Top, EHit::Solid);
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const double A = Side == 0 ? Post : Length - Post - Leg;
				const double B = A + Leg;
				const double LegPost = Plan::ScreenLegPost;
				const double P0 = Side == 0 ? B : A - LegPost;
				const FString Part = FString::Printf(TEXT("%s.leg.%d"), *Name, Side + 1);
				Block(Out, Part + TEXT(".post"), Wood, F, P0, P0 + LegPost, -0.5 * Plan::ScreenLegThick, 0.5 * Plan::ScreenLegThick, Floor, Frieze, EHit::Solid);
				Block(Out, Part + TEXT(".base"), Wood, F, FMath::Min(A, P0), FMath::Max(B, P0 + LegPost), -0.5 * Plan::ScreenBaseThick,
					0.5 * Plan::ScreenBaseThick, Floor, Floor + Plan::ScreenLegBase, EHit::Solid);
				DrawPanel(Out, Part + TEXT(".fretwork"), Wood, F, A, -0.5 * Plan::LatticeDepth, Floor + Plan::ScreenLegBase, Leg,
					Frieze - Floor - Plan::ScreenLegBase, ELattice::StepBrocade, false);
			}
			Block(Out, Name + TEXT(".frieze-rail"), Wood, F, Post, Length - Post, -0.5 * Plan::ScreenLegThick, 0.5 * Plan::ScreenLegThick,
				Frieze - Plan::ScreenRail, Frieze, EHit::Solid);
			Block(Out, Name + TEXT(".head"), Wood, F, 0.0, Length, -Half, Half, Top - Plan::ScreenHead, Top, EHit::Solid);
			DrawPanel(Out, Name + TEXT(".frieze"), Wood, F, Post, -0.5 * Plan::LatticeDepth, Frieze, Length - 2.0 * Post, Plan::ScreenFrieze - Plan::ScreenHead,
				ELattice::Lantern, false);
			const double SA = Post + Leg + Plan::ScreenRail;
			const double SB = Length - Post - Leg - Plan::ScreenRail - Plan::SpandrelWide;
			for (int32 Side = 0; Side < 2; ++Side)
			{
				DrawPanel(Out, FString::Printf(TEXT("%s.spandrel.%d"), *Name, Side + 1), Wood, F, Side == 0 ? SA : SB, -0.5 * Plan::LatticeDepth,
					Frieze - Plan::ScreenRail - Plan::SpandrelHigh, Plan::SpandrelWide, Plan::SpandrelHigh, ELattice::Grid, false);
			}
		}

		/** A Ming table: its top, everted ends where it has them, legs set in, aprons along its long sides, low stretchers at its ends. */
		void Table(TArray<FFormDrawing>& Out, const Plan::FTablePlan& Table)
		{
			const bool bAlongX = Table.X1 - Table.X0 >= Table.Y1 - Table.Y0;
			const FBuildingFrame F = bAlongX ? FBuildingFrame::Of(FVector2D(Table.X0, Table.Y0), FVector2D(1.0, 0.0), FVector2D(0.0, 1.0))
				: FBuildingFrame::Of(FVector2D(Table.X0, Table.Y0), FVector2D(0.0, 1.0), FVector2D(1.0, 0.0));
			const double L = bAlongX ? Table.X1 - Table.X0 : Table.Y1 - Table.Y0;
			const double W = bAlongX ? Table.Y1 - Table.Y0 : Table.X1 - Table.X0;
			const double Floor = Table.Floor;
			const double Top = Floor + Table.Height;
			const double Under = Top - Plan::TableTop;
			const FString Name = FString(Table.Local);
			Block(Out, Name + TEXT(".top"), Phoebe, F, 0.0, L, 0.0, W, Under, Top, EHit::Solid);
			if (Table.bEverted)
			{
				// Each end turns up: a step 0.09 long, its tip 0.05, rising 0.06 in two courses.
				const double Step = 0.5 * Plan::EvertedRise;
				for (int32 End = 0; End < 2; ++End)
				{
					const double S = End == 0 ? 1.0 : -1.0;
					const double U = End == 0 ? 0.0 : L;
					AlongV(Out, FString::Printf(TEXT("%s.end.%d"), *Name, End + 1), Phoebe, F, {FVector2D(U, Top), FVector2D(U + S * Plan::EvertedLong, Top),
						FVector2D(U + S * Plan::EvertedLong, Top + Step), FVector2D(U + S * Plan::EvertedTip, Top + Step),
						FVector2D(U + S * Plan::EvertedTip, Top + Plan::EvertedRise), FVector2D(U, Top + Plan::EvertedRise)}, {}, 0.0, W, EHit::Solid);
				}
			}
			const double Leg = Plan::TableLeg;
			const double In = Plan::TableLegIn;
			const double Side = Plan::TableLegSide;
			int32 Count = 0;
			for (const double U : {In, L - In - Leg})
			{
				for (const double V : {Side, W - Side - Leg})
				{
					Block(Out, FString::Printf(TEXT("%s.leg.%d"), *Name, ++Count), Phoebe, F, U, U + Leg, V, V + Leg, Floor, Under, EHit::Solid);
				}
			}
			for (int32 K = 0; K < 2; ++K)
			{
				// Each apron runs between the legs along a long side, flush with the legs' outer faces.
				const double V = K == 0 ? Side : W - Side - Plan::ApronThick;
				Block(Out, FString::Printf(TEXT("%s.apron.%d"), *Name, K + 1), Phoebe, F, In, L - In, V, V + Plan::ApronThick, Under - Plan::ApronHigh, Under,
					EHit::Solid);
				const double U = K == 0 ? In : L - In - Leg;
				Block(Out, FString::Printf(TEXT("%s.stretcher.%d"), *Name, K + 1), Phoebe, F, U, U + Leg, Side + Leg, W - Side - Leg, Floor + Plan::StretcherZ0,
					Floor + Plan::StretcherZ1, EHit::Solid);
			}
		}

		/** A southern official's hat chair (南官帽椅) in round members, facing its plan direction. */
		void Chair(TArray<FFormDrawing>& Out, const Plan::FChairPlan& Chair)
		{
			const FVector2D Facing(Chair.FacingX, Chair.FacingY);
			// u across the chair, v from its front back to its crest.
			const FBuildingFrame F = FBuildingFrame::Of(FVector2D(Chair.X, Chair.Y), FVector2D(-Facing.Y, Facing.X), -Facing);
			const FString Name = FString(Chair.Local);
			const double Hw = 0.5 * Plan::ChairWide;
			const double Hd = 0.5 * Plan::ChairDeep;
			const double R = Plan::ChairMember;
			const double Floor = Chair.Floor;
			const double Seat = Floor + Plan::ChairSeat;
			const double Arm = Seat + Plan::ChairArm;
			const double Crest = Floor + Plan::ChairCrest;
			const double Front = -Hd + R + Plan::ChairLegIn;
			const double Back = Hd - R - Plan::ChairLegIn;
			auto Lean = [&](double Z) { return Back + Plan::ChairLean * FMath::Clamp((Z - Seat) / (Crest - Seat), 0.0, 1.0); };
			const double Frame = Plan::ChairFrame;
			Raised(Out, Name + TEXT(".seat-frame"), Phoebe, F, Rectangle(-Hw, -Hd, Hw, Hd), {Rectangle(-Hw + Frame, -Hd + Frame, Hw - Frame, Hd - Frame)},
				Seat - Plan::ChairFrameDeep, Seat, EHit::Solid);
			Block(Out, Name + TEXT(".seat"), Phoebe, F, -Hw + Frame, Hw - Frame, -Hd + Frame, Hd - Frame, Seat - Plan::ChairBoardUnder - Plan::ChairBoard,
				Seat - Plan::ChairBoardUnder, EHit::Solid);
			for (int32 Side = 0; Side < 2; ++Side)
			{
				const double S = Side == 0 ? -1.0 : 1.0;
				const double U = S * (Hw - R - Plan::ChairLegIn);
				const FString Part = FString::Printf(TEXT("%s.side.%d"), *Name, Side + 1);
				Rod(Out, Part + TEXT(".front-leg"), Phoebe, F.At(U, Front, Floor), F.At(U, Front, Arm), R, EHit::Solid);
				Rod(Out, Part + TEXT(".back-leg"), Phoebe, F.At(U, Back, Floor), F.At(U, Back, Seat), R, EHit::Solid);
				Rod(Out, Part + TEXT(".back-post"), Phoebe, F.At(U, Back, Seat), F.At(U, Lean(Crest), Crest), R, EHit::Solid);
				Rod(Out, Part + TEXT(".arm"), Phoebe, F.At(U, Front, Arm), F.At(U, Lean(Arm), Arm + Plan::ChairArmRise), Plan::ChairArmR, EHit::Solid);
				Rod(Out, Part + TEXT(".stretcher"), Phoebe, F.At(U, Front, Floor + Plan::ChairSideStretcher), F.At(U, Back, Floor + Plan::ChairSideStretcher),
					Plan::ChairStretcherR, EHit::Solid);
			}
			const double Inner = Hw - R - Plan::ChairLegIn;
			Rod(Out, Name + TEXT(".crest"), Phoebe, F.At(-Inner, Lean(Crest), Crest), F.At(Inner, Lean(Crest), Crest), Plan::ChairCrestR, EHit::Solid);
			Rod(Out, Name + TEXT(".back-stretcher"), Phoebe, F.At(-Inner, Back, Floor + Plan::ChairBackStretcher), F.At(Inner, Back, Floor + Plan::ChairBackStretcher),
				Plan::ChairStretcherR, EHit::Solid);
			Block(Out, Name + TEXT(".footrest"), Phoebe, F, -Inner, Inner, Front - Plan::FootrestHalf, Front + Plan::FootrestHalf, Floor + Plan::FootrestZ0,
				Floor + Plan::FootrestZ1, EHit::Solid);
			Block(Out, Name + TEXT(".apron"), Phoebe, F, -Hw + Frame, Hw - Frame, -Hd + Plan::ChairApronIn, -Hd + Plan::ChairApronIn + Plan::ApronThick,
				Seat - Plan::ChairApronHigh, Seat - Plan::ChairFrameDeep, EHit::Solid);
			// The S-curved splat, carved as a board on its curve from the seat's back rail to the crest.
			const FBuildingFrame Side = F.Within(0.0, 0.0, FVector2D(0.0, 1.0), FVector2D(1.0, 0.0));
			TArray<FVector2D> Curve;
			const int32 Stations = UE_ARRAY_COUNT(Plan::SplatBow);
			for (int32 K = 0; K < Stations; ++K)
			{
				const double Z = FMath::Lerp(Seat, Crest, double(K) / (Stations - 1));
				Curve.Add(FVector2D(Lean(Z) + Plan::SplatBow[K], Z));
			}
			ReliefAlongV(Out, Name + TEXT(".splat"), Phoebe, Side, Curve, -0.5 * Plan::ChairSplat, 0.5 * Plan::ChairSplat, Plan::MemberGrid, Plan::ReliefBack,
				[](double, double) { return Plan::ChairSplatThick - Plan::ReliefBack; });
		}

		/** 林泉's painting table (画案): the top on four round legs set in, a plain apron with spandrels, two stretchers at each end. */
		void PaintingTable(TArray<FFormDrawing>& Out)
		{
			const FBuildingFrame F = FBuildingFrame::Of(FVector2D(Plan::PaintingX, Plan::PaintingY), FVector2D(1.0, 0.0), FVector2D(0.0, 1.0));
			const double HL = 0.5 * Plan::PaintingLong;
			const double HW = 0.5 * Plan::PaintingDeep;
			const double Floor = Plan::PaintingFloor;
			const double Top = Floor + Plan::PaintingHigh;
			const double Under = Top - Plan::TableTop;
			const double LU = HL - Plan::PaintingLegEnd;
			const double LV = HW - Plan::PaintingLegSide;
			const double R = Plan::PaintingLegR;
			Block(Out, TEXT("linquan.table.top"), Huanghuali, F, -HL, HL, -HW, HW, Under, Top, EHit::Solid);
			int32 Count = 0;
			for (const double U : {-LU, LU})
			{
				for (const double V : {-LV, LV})
				{
					Turned(Out, FString::Printf(TEXT("linquan.table.leg.%d"), ++Count), Huanghuali, F.At(U, V, Floor),
						Section({{0.0, 0.0}, {R, 0.0}, {R, Under - Floor}, {0.0, Under - Floor}}, false), EHit::Solid);
				}
			}
			// The apron along each long side, run into the legs, a spandrel under it inside each leg.
			const double A0 = Under - Plan::ApronHigh;
			const double Spandrel = Plan::PaintingSpandrel;
			const TArray<FVector2D> Apron = {FVector2D(-LU - R, Under), FVector2D(-LU - R, A0 - Spandrel), FVector2D(-LU + R, A0 - Spandrel),
				FVector2D(-LU + R + Spandrel, A0), FVector2D(LU - R - Spandrel, A0), FVector2D(LU - R, A0 - Spandrel), FVector2D(LU + R, A0 - Spandrel),
				FVector2D(LU + R, Under)};
			for (const double V : {-LV, LV})
			{
				const double T = 0.5 * Plan::ApronThick;
				AlongV(Out, FString::Printf(TEXT("linquan.table.apron.%d"), V < 0.0 ? 1 : 2), Huanghuali, F, Apron, {}, V - T, V + T, EHit::Solid);
			}
			Count = 0;
			for (const double U : {-LU, LU})
			{
				for (const double Z : Plan::PaintingStretchers)
				{
					Rod(Out, FString::Printf(TEXT("linquan.table.stretcher.%d"), ++Count), Huanghuali, F.At(U, -LV, Floor + Z), F.At(U, LV, Floor + Z),
						Plan::PaintingStretcherR, EHit::Solid);
				}
			}
		}

		/** A rose chair (玫瑰椅): a board seat on four round legs, a low straight back and arms, a low rail round three sides. */
		void RoseChair(TArray<FFormDrawing>& Out, int32 Index, double X)
		{
			// u across, v from the front back: the chair faces south, so its back is north.
			const FBuildingFrame F = FBuildingFrame::Of(FVector2D(X, Plan::RoseChairY), FVector2D(1.0, 0.0), FVector2D(0.0, -1.0));
			const FString Name = FString::Printf(TEXT("linquan.rose-chair.%d"), Index);
			const double Hw = 0.5 * Plan::RoseWide - Plan::RoseMember;
			const double Hd = 0.5 * Plan::RoseDeep - Plan::RoseMember;
			const double R = Plan::RoseMember;
			const double Floor = Plan::PaintingFloor;
			const double Seat = Floor + Plan::RoseSeat;
			const double Rail = Seat + Plan::RoseRail;
			Block(Out, Name + TEXT(".seat"), Huanghuali, F, -Hw - R, Hw + R, -Hd - R, Hd + R, Seat - Plan::RoseBoard, Seat, EHit::Solid);
			int32 Count = 0;
			for (const double U : {-Hw, Hw})
			{
				Rod(Out, FString::Printf(TEXT("%s.front-leg.%d"), *Name, ++Count), Huanghuali, F.At(U, -Hd, Floor), F.At(U, -Hd, Floor + Plan::RoseArm), R,
					EHit::Solid);
				Rod(Out, FString::Printf(TEXT("%s.back-leg.%d"), *Name, Count), Huanghuali, F.At(U, Hd, Floor), F.At(U, Hd, Floor + Plan::RoseBack), R,
					EHit::Solid);
				Rod(Out, FString::Printf(TEXT("%s.arm.%d"), *Name, Count), Huanghuali, F.At(U, -Hd, Floor + Plan::RoseArm), F.At(U, Hd, Floor + Plan::RoseArm), R,
					EHit::Solid);
				Rod(Out, FString::Printf(TEXT("%s.rail.side.%d"), *Name, Count), Huanghuali, F.At(U, -Hd, Rail), F.At(U, Hd, Rail), R, EHit::Solid);
				Rod(Out, FString::Printf(TEXT("%s.stretcher.side.%d"), *Name, Count), Huanghuali, F.At(U, -Hd, Floor + Plan::RoseStretchers[1]),
					F.At(U, Hd, Floor + Plan::RoseStretchers[1]), R, EHit::Solid);
			}
			Rod(Out, Name + TEXT(".back-rail"), Huanghuali, F.At(-Hw, Hd, Floor + Plan::RoseBack), F.At(Hw, Hd, Floor + Plan::RoseBack), R, EHit::Solid);
			Rod(Out, Name + TEXT(".rail.back"), Huanghuali, F.At(-Hw, Hd, Rail), F.At(Hw, Hd, Rail), R, EHit::Solid);
			Rod(Out, Name + TEXT(".stretcher.front"), Huanghuali, F.At(-Hw, -Hd, Floor + Plan::RoseStretchers[0]), F.At(Hw, -Hd, Floor + Plan::RoseStretchers[0]),
				R, EHit::Solid);
			Rod(Out, Name + TEXT(".stretcher.back"), Huanghuali, F.At(-Hw, Hd, Floor + Plan::RoseStretchers[2]), F.At(Hw, Hd, Floor + Plan::RoseStretchers[2]),
				R, EHit::Solid);
		}

		void DeskThings(TArray<FFormDrawing>& Out)
		{
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			for (const Plan::FDeskThings& Things : Plan::DeskThings)
			{
				const FString Name = FString(Things.Local);
				const double Z = Things.Top;
				Block(Out, Name + TEXT(".inkstone-box"), Phoebe, Stage, Things.InkX - 0.5 * Plan::InkBoxWide, Things.InkX + 0.5 * Plan::InkBoxWide,
					Things.InkY - 0.5 * Plan::InkBoxLong, Things.InkY + 0.5 * Plan::InkBoxLong, Z, Z + Plan::InkBoxHigh, EHit::Solid);
				Block(Out, Name + TEXT(".inkstone"), TEXT("desk.inkstone"), Stage, Things.InkX - 0.5 * Plan::InkstoneWide, Things.InkX + 0.5 * Plan::InkstoneWide,
					Things.InkY - 0.5 * Plan::InkstoneLong, Things.InkY + 0.5 * Plan::InkstoneLong, Z, Z + Plan::InkstoneHigh, EHit::Solid);
				const double R = Plan::PotR;
				const double H = Plan::PotHigh;
				Turned(Out, Name + TEXT(".brush-pot"), TEXT("desk.stoneware"), FVector(Things.PotX, Things.PotY, Z),
					Section({{0.0, 0.0}, {R, 0.0}, {R, H}, {R - Plan::PotWall, H}, {R - Plan::PotWall, Plan::PotBed}, {0.0, Plan::PotBed}}, false), EHit::Solid);
				const double D = Plan::DropperR;
				Turned(Out, Name + TEXT(".water-dropper"), TEXT("desk.stoneware"), FVector(Things.DropperX, Things.DropperY, Z),
					Section({{0.0, 0.0}, {D, 0.0, true}, {D, 0.5 * Plan::DropperHigh, true}, {0.5 * D, Plan::DropperHigh, true}, {0.0, Plan::DropperHigh}}, false),
					EHit::Solid);
			}
			const Plan::FDeskThings& Table = Plan::DeskThings[1];
			Turned(Out, TEXT("linquan.table.rock-stand"), Phoebe, FVector(Plan::RockStandX, Plan::RockStandY, Table.Top),
				Section({{0.0, 0.0}, {Plan::RockStandR, 0.0}, {Plan::RockStandR, Plan::RockStandHigh}, {0.0, Plan::RockStandHigh}}, false), EHit::Solid);
		}

		/** A plain hexagonal silk lantern (纱灯) hung from its hook on a bronze rod: black plates, posts and a rail, silk panels. */
		void Lantern(TArray<FFormDrawing>& Out, int32 Index, const Plan::FLanternPlan& L)
		{
			const FString Name = FString::Printf(TEXT("lantern.%d"), Index);
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			const FVector2D Centre(L.X, L.Y);
			const double Crown = L.Hook - L.Rod;
			const double Top = Crown - Plan::LanternCrown;
			const double Z1 = Top - Plan::LanternPlate;
			const double Z0 = Z1 - L.Height;
			const TCHAR* Black = TEXT("timber.black");
			Rod(Out, Name + TEXT(".rod"), TEXT("bronze.hardware"), FVector(L.X, L.Y, Crown), FVector(L.X, L.Y, L.Hook), Plan::LanternRod, EHit::None);
			const TArray<FVector2D> Plate = RegularPolygon(Centre, L.Radius + Plan::LanternPlateOut, 6, 0.0);
			const TArray<FVector2D> Small = RegularPolygon(Centre, L.Radius - Plan::LanternCrownIn, 6, 0.0);
			Raised(Out, Name + TEXT(".crown"), Black, Stage, Small, {}, Top, Crown, EHit::None);
			Raised(Out, Name + TEXT(".top"), Black, Stage, Plate, {}, Z1, Top, EHit::None);
			Raised(Out, Name + TEXT(".base"), Black, Stage, Plate, {}, Z0 - Plan::LanternPlate, Z0, EHit::None);
			Raised(Out, Name + TEXT(".foot"), Black, Stage, Small, {}, Z0 - Plan::LanternPlate - Plan::LanternCrown, Z0 - Plan::LanternPlate, EHit::None);
			// The silk's six panels, one hexagonal band 4 mm thick; the posts at its corners; the rail round its middle.
			const double Apothem = FMath::Cos(UE_DOUBLE_PI / 6.0);
			const TArray<FVector2D> Outer = RegularPolygon(Centre, L.Radius, 6, 0.0);
			FFormDrawing& Silk = Raised(Out, Name + TEXT(".silk"), TEXT("lantern.silk"), Stage, Outer,
				{RegularPolygon(Centre, L.Radius - Plan::LanternSilk / Apothem, 6, 0.0)}, Z0, Z1, EHit::None);
			Silk.bNanite = false;
			for (int32 K = 0; K < 6; ++K)
			{
				const FVector2D P = Outer[K];
				const double H = 0.5 * Plan::LanternPost;
				Block(Out, FString::Printf(TEXT("%s.post.%d"), *Name, K + 1), Black, Stage, P.X - H, P.X + H, P.Y - H, P.Y + H, Z0, Z1, EHit::None);
			}
			const double Mid = 0.5 * (Z0 + Z1);
			Raised(Out, Name + TEXT(".rail"), Black, Stage, RegularPolygon(Centre, L.Radius + Plan::LanternRailDeep / Apothem, 6, 0.0), {Outer},
				Mid - 0.5 * Plan::LanternRailHigh, Mid + 0.5 * Plan::LanternRailHigh, EHit::None);
		}

		void Plaques(TArray<FFormDrawing>& Out)
		{
			for (const Plan::FPlaquePlan& Plaque : Plan::Plaques)
			{
				const FVector Facing(Plaque.FacingX, Plaque.FacingY, 0.0);
				const FVector Face(Plaque.X, Plaque.Y, Plaque.Z);
				const FVector Across = FVector::CrossProduct(FVector::UpVector, Facing);
				Bar(Out, FString::Printf(TEXT("%s.plaque"), Plaque.Local), TEXT("timber.black"), Face - Facing * Plan::PlaqueBoard, Face, Across, Plaque.Width,
					Plaque.Height, EHit::Solid);
			}
			Block(Out, TEXT("chenghuaitang.plaque-lintel"), TEXT("timber.painted"), FBuildingFrame::Stage(), Plan::HallLintelX0, Plan::HallLintelX1,
				Plan::HallLintelY - Plan::HallLintelHalf, Plan::HallLintelY + Plan::HallLintelHalf, Plan::HallLintelZ0, Plan::HallLintelZ1, EHit::None);
			// The couplet's shells, each hugging its column over 1.9 radians centred on the south.
			int32 Count = 0;
			for (const double X : Plan::CoupletX)
			{
				const double R0 = Plan::CoupletColumn + Plan::CoupletGap;
				const double R1 = R0 + Plan::CoupletShell;
				const double Z0 = Plan::CoupletZ0;
				const double Z1 = Plan::CoupletZ1;
				FFormLathe Shell;
				Shell.Section = Section({{R0, 0.0}, {R1, 0.0}, {R1, Z1 - Z0}, {R0, Z1 - Z0}}, true);
				const double Half = FMath::RadiansToDegrees(0.5 * Plan::CoupletSpan);
				// The south is +y, 90° from the east towards the south.
				Shell.FromDegrees = 90.0 - Half;
				Shell.ToDegrees = 90.0 + Half;
				Draw(Out, FString::Printf(TEXT("chenghuaitang.couplet.%d"), ++Count), TEXT("timber.black"), MoveTemp(Shell), EHit::Solid,
					FMuseeFrame(FVector(X, Plan::CoupletY, Z0)));
			}
		}
	}

	void DrawFurniture(TArray<FFormDrawing>& Out)
	{
		int32 Index = 0;
		for (const double X : Plan::ScreensX)
		{
			Screen(Out, ++Index, X);
		}
		for (const Plan::FTablePlan& Each : Plan::Tables)
		{
			Table(Out, Each);
		}
		for (const Plan::FChairPlan& Each : Plan::Chairs)
		{
			Chair(Out, Each);
		}
		PaintingTable(Out);
		Index = 0;
		for (const double X : Plan::RoseChairsX)
		{
			RoseChair(Out, ++Index, X);
		}
		DeskThings(Out);
		Index = 0;
		for (const Plan::FLanternPlan& Each : Plan::Lanterns)
		{
			Lantern(Out, ++Index, Each);
		}
		Plaques(Out);
	}
}
