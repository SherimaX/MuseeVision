#include "AlbionParts.h"

/**
 * The fittings (§3.8): the bronze picture rail bay by bay on the side walls, on its brackets, with ball ends; the two oak
 * benches; the four oak table cases, their baize beds under glazed boxes; the lectern in Morris's bay; the tapestry's
 * batten on its painted brackets, and its linen lining. The electroliers are Light's fittings (§13), the works and their
 * rods Art's, the blind doors Site's.
 */
namespace Musee::Albion
{
	namespace
	{
		/**
		 * The picture rail on the side walls at 4.20, 0.035 off the face, bay by bay between the pilasters (0.03 clear of
		 * each, 0.05 from the end walls), with ball ends; its brackets about every 1.4, at least two a bay.
		 */
		void PictureRail(TArray<FFormDrawing>& Out)
		{
			for (const bool bWest : {true, false})
			{
				const double S = bWest ? -1.0 : 1.0;
				const double Face = S * Plan::SideInnerX;
				const double RailX = Face - S * Plan::RailOff;
				const TCHAR* Side = bWest ? TEXT("west") : TEXT("east");
				for (int32 Bay = 1; Bay <= Plan::Lines - 1; ++Bay)
				{
					const double Clear = Plan::PilasterHalf + Plan::RailClearOfPilaster;
					const double From = Plan::LineY(Bay) + (Bay == 1 ? Plan::RailClearOfEndWall : Clear);
					const double To = Plan::LineY(Bay + 1) - (Bay == Plan::Lines - 1 ? Plan::RailClearOfEndWall : Clear);
					const double Knob = Plan::RailBallRadius;
					const FString Name = FString::Printf(TEXT("rail.%s.%d"), Side, Bay);
					Out.Add(Rod(Name, TEXT("picture-rail"), FVector(RailX, From + Knob, Plan::RailZ), FVector(RailX, To - Knob, Plan::RailZ), Plan::RailRadius,
						Overhead));
					Out.Add(Ball(Name + TEXT(".ball.1"), TEXT("picture-rail"), FVector(RailX, From + Knob, Plan::RailZ), Knob, Overhead));
					Out.Add(Ball(Name + TEXT(".ball.2"), TEXT("picture-rail"), FVector(RailX, To - Knob, Plan::RailZ), Knob, Overhead));
					const double Length = To - From;
					const int32 Brackets = FMath::Max(2, FMath::RoundToInt32(Length / Plan::RailBracketPitch));
					for (int32 Index = 0; Index < Brackets; ++Index)
					{
						const double Y = From + Length * (Index + 0.5) / Brackets;
						const FString Bracket = FString::Printf(TEXT("%s.bracket.%d"), *Name, Index + 1);
						const double Plate = Face - S * Plan::RailPlate[1];
						const double PlateHalf = 0.5 * Plan::RailPlate[0];
						Out.Add(Box(Bracket + TEXT(".plate"), TEXT("picture-rail"), FVector(FMath::Min(Face, Plate), Y - PlateHalf, Plan::RailPlate[2]),
							FVector(FMath::Max(Face, Plate), Y + PlateHalf, Plan::RailZ), Overhead));
						const double ArmHalf = 0.5 * Plan::RailArm[0];
						Out.Add(Box(Bracket + TEXT(".arm"), TEXT("picture-rail"), FVector(FMath::Min(Plate, RailX), Y - ArmHalf, Plan::RailArm[1]),
							FVector(FMath::Max(Plate, RailX), Y + ArmHalf, Plan::RailArm[2]), Overhead));
					}
				}
			}
		}

		/** One of the top's two boards, (x, z): 0.25 wide, its outer top arris rounded 12 mm. Side -1 is the west board. */
		FFormProfile BenchBoard(double CX, double Side)
		{
			const double Outer = CX + Side * Plan::BenchHalfDepth;
			const double Low = Plan::BenchTop - Plan::BenchBoard;
			const double R = Plan::BenchArris;
			FFormProfile P = FFormProfile::From(FVector2D(CX, Low));
			P.LineTo(FVector2D(Outer, Low), EFormJoint::Crisp).LineTo(FVector2D(Outer, Plan::BenchTop - R), EFormJoint::Crisp);
			const double Start = Side < 0.0 ? 180.0 : 0.0;
			P.ArcAbout(FVector2D(Outer - Side * R, Plan::BenchTop - R), R, R, Start, 90.0, EFormJoint::Smooth);
			P.LineTo(FVector2D(CX, Plan::BenchTop), EFormJoint::Smooth);
			return P.Close();
		}

		/**
		 * A bench long along y: two boards on two ends set on edge 0.30 in from its ends, each on two feet, cleats under the
		 * top inside the ends, and the stretcher wedged through mortises in the ends.
		 */
		void Bench(bool bWest, TArray<FFormDrawing>& Out)
		{
			const double CX = bWest ? -Plan::BenchX : Plan::BenchX;
			const double CY = Plan::BenchY;
			const FString Name = bWest ? TEXT("bench.west") : TEXT("bench.east");
			for (const double Side : {-1.0, 1.0})
			{
				FFormExtrusion Board;
				Board.Outline = BenchBoard(CX, Side);
				Board.DepthMeters = 2.0 * Plan::BenchHalfLength;
				Out.Add(Drawn(Name + (Side < 0.0 ? TEXT(".board.west") : TEXT(".board.east")), TEXT("bench"), MoveTemp(Board), Solid,
					AcrossY(CY + Plan::BenchHalfLength)));
			}
			const double Low = Plan::BenchTop - Plan::BenchBoard;
			for (const double End : {-1.0, 1.0})
			{
				const TCHAR* Which = End < 0.0 ? TEXT("north") : TEXT("south");
				const double Y = CY + End * (Plan::BenchHalfLength - Plan::BenchEndIn);
				const double T = 0.5 * Plan::BenchEndThick;
				FFormExtrusion Piece;
				Piece.Outline = FFormProfile::Rectangle(FVector2D(CX - Plan::BenchEndHalf, Plan::BenchEndFoot), FVector2D(CX + Plan::BenchEndHalf, Low));
				Piece.Holes.Add(FFormProfile::Rectangle(FVector2D(CX - Plan::StretcherHalf[0], Plan::StretcherZ - Plan::StretcherHalf[1]),
					FVector2D(CX + Plan::StretcherHalf[0], Plan::StretcherZ + Plan::StretcherHalf[1])));
				Piece.DepthMeters = Plan::BenchEndThick;
				Out.Add(Drawn(FString::Printf(TEXT("%s.end.%s"), *Name, Which), TEXT("bench"), MoveTemp(Piece), Solid, AcrossY(Y + T)));
				for (const double Foot : {-1.0, 1.0})
				{
					const double FX = CX + Foot * Plan::BenchFootX;
					Out.Add(Box(FString::Printf(TEXT("%s.foot.%s.%s"), *Name, Which, Foot < 0.0 ? TEXT("west") : TEXT("east")), TEXT("bench"),
						FVector(FX - 0.5 * Plan::BenchFoot[0], Y - 0.5 * Plan::BenchFoot[1], 0.0), FVector(FX + 0.5 * Plan::BenchFoot[0], Y + 0.5 * Plan::BenchFoot[1],
						Plan::BenchEndFoot), Solid));
				}
				// The cleat under the top, against the end's inner face.
				const double Inner = Y - End * T;
				const double Beyond = Inner - End * Plan::BenchCleat[1];
				Out.Add(Box(FString::Printf(TEXT("%s.cleat.%s"), *Name, Which), TEXT("bench"),
					FVector(CX - 0.5 * Plan::BenchCleat[0], FMath::Min(Inner, Beyond), Low - Plan::BenchCleat[2]),
					FVector(CX + 0.5 * Plan::BenchCleat[0], FMath::Max(Inner, Beyond), Low), Solid));
			}
			Out.Add(Box(Name + TEXT(".stretcher"), TEXT("bench"),
				FVector(CX - Plan::StretcherHalf[0], CY - Plan::StretcherHalfLength, Plan::StretcherZ - Plan::StretcherHalf[1]),
				FVector(CX + Plan::StretcherHalf[0], CY + Plan::StretcherHalfLength, Plan::StretcherZ + Plan::StretcherHalf[1]), Solid));
		}

		/**
		 * A table case: four turned legs on their square blocks, stretchers low down, the apron under the deck, the deck with
		 * its edge proud of the apron, the baize bed, and the glazed box of oak corner posts and top rails, glass all round
		 * and over. Its collision is its own solids.
		 */
		void Case(const FString& Name, const FVector2D& C, TArray<FFormDrawing>& Out)
		{
			const double LX = Plan::CaseHalfX - Plan::LegIn;
			const double LY = Plan::CaseHalfY - Plan::LegIn;
			const double B = Plan::LegBlockHalf;
			const FVector2D Corners[] = {FVector2D(-LX, -LY), FVector2D(LX, -LY), FVector2D(LX, LY), FVector2D(-LX, LY)};
			TArray<FFormProfile> BlockHoles;
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const FVector2D At = C + Corners[Index];
				Out.Add(Lathe(FString::Printf(TEXT("%s.leg.%d"), *Name, Index + 1), TEXT("case"), FVector(At.X, At.Y, 0.0), Turned(Plan::LegPoints), Solid));
				Out.Add(Box(FString::Printf(TEXT("%s.block.%d"), *Name, Index + 1), TEXT("case"), FVector(At.X - B, At.Y - B, Plan::LegTurnTop),
					FVector(At.X + B, At.Y + B, Plan::LegBlockTop), Solid));
				BlockHoles.Add(FFormProfile::Rectangle(At - FVector2D(B, B), At + FVector2D(B, B)));
			}
			// Stretchers: along each long side between the legs, and across the middle between them.
			const double SW = 0.5 * Plan::CaseStretcher[0];
			const double SH = 0.5 * Plan::CaseStretcher[1];
			const double Z = Plan::CaseStretcherZ;
			for (const double Side : {-1.0, 1.0})
			{
				const double X = C.X + Side * LX;
				Out.Add(Box(FString::Printf(TEXT("%s.stretcher.%s"), *Name, Side < 0.0 ? TEXT("west") : TEXT("east")), TEXT("case"),
					FVector(X - SW, C.Y - LY, Z - SH), FVector(X + SW, C.Y + LY, Z + SH), Solid));
			}
			Out.Add(Box(Name + TEXT(".stretcher.across"), TEXT("case"), FVector(C.X - LX + SW, C.Y - SW, Z - SH), FVector(C.X + LX - SW, C.Y + SW, Z + SH), Solid));

			// The apron: its boards' outer faces flush with the blocks, between them, from 0.38 to the deck.
			const double OX = LX + B;
			const double OY = LY + B;
			const double T = Plan::ApronMeters;
			Out.Add(Box(Name + TEXT(".apron.west"), TEXT("case"), FVector(C.X - OX, C.Y - LY + B, Plan::ApronFoot), FVector(C.X - OX + T, C.Y + LY - B, Plan::DeckFoot), Solid));
			Out.Add(Box(Name + TEXT(".apron.east"), TEXT("case"), FVector(C.X + OX - T, C.Y - LY + B, Plan::ApronFoot), FVector(C.X + OX, C.Y + LY - B, Plan::DeckFoot), Solid));
			Out.Add(Box(Name + TEXT(".apron.north"), TEXT("case"), FVector(C.X - LX + B, C.Y - OY, Plan::ApronFoot), FVector(C.X + LX - B, C.Y - OY + T, Plan::DeckFoot), Solid));
			Out.Add(Box(Name + TEXT(".apron.south"), TEXT("case"), FVector(C.X - LX + B, C.Y + OY - T, Plan::ApronFoot), FVector(C.X + LX - B, C.Y + OY, Plan::DeckFoot), Solid));

			FFormExtrusion Deck;
			Deck.Outline = FFormProfile::Rectangle(C - FVector2D(Plan::CaseHalfX, Plan::CaseHalfY), C + FVector2D(Plan::CaseHalfX, Plan::CaseHalfY));
			Deck.Holes = BlockHoles;
			Deck.DepthMeters = Plan::DeckTop - Plan::DeckFoot;
			Out.Add(Drawn(Name + TEXT(".deck"), TEXT("case"), MoveTemp(Deck), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::DeckFoot))));

			// The glazed box: posts on the blocks, the glass on the posts' centre lines, the rails over it.
			const double P = 0.5 * Plan::CasePost;
			const double PX = OX - P;
			const double PY = OY - P;
			const double RailFoot = Plan::CaseTop - Plan::CaseRailHigh;
			const double G = 0.5 * Plan::CaseGlassMeters;
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const FVector2D At = C + FVector2D(FMath::Sign(Corners[Index].X) * PX, FMath::Sign(Corners[Index].Y) * PY);
				Out.Add(Box(FString::Printf(TEXT("%s.post.%d"), *Name, Index + 1), TEXT("case"), FVector(At.X - P, At.Y - P, Plan::LegBlockTop),
					FVector(At.X + P, At.Y + P, RailFoot), Solid));
			}
			Out.Add(Box(Name + TEXT(".rail.west"), TEXT("case"), FVector(C.X - PX - P, C.Y - OY, RailFoot), FVector(C.X - PX + P, C.Y + OY, Plan::CaseTop), Solid));
			Out.Add(Box(Name + TEXT(".rail.east"), TEXT("case"), FVector(C.X + PX - P, C.Y - OY, RailFoot), FVector(C.X + PX + P, C.Y + OY, Plan::CaseTop), Solid));
			Out.Add(Box(Name + TEXT(".rail.north"), TEXT("case"), FVector(C.X - PX + P, C.Y - PY - P, RailFoot), FVector(C.X + PX - P, C.Y - PY + P, Plan::CaseTop), Solid));
			Out.Add(Box(Name + TEXT(".rail.south"), TEXT("case"), FVector(C.X - PX + P, C.Y + PY - P, RailFoot), FVector(C.X + PX - P, C.Y + PY + P, Plan::CaseTop), Solid));
			auto Pane = [&](const FString& Which, const FVector& Min, const FVector& Max)
			{
				Out.Add(Glass(Box(Name + TEXT(".glass.") + Which, TEXT("case.glass"), Min, Max, Solid)));
			};
			Pane(TEXT("west"), FVector(C.X - PX - G, C.Y - PY + P, Plan::DeckTop), FVector(C.X - PX + G, C.Y + PY - P, RailFoot));
			Pane(TEXT("east"), FVector(C.X + PX - G, C.Y - PY + P, Plan::DeckTop), FVector(C.X + PX + G, C.Y + PY - P, RailFoot));
			Pane(TEXT("north"), FVector(C.X - PX + P, C.Y - PY - G, Plan::DeckTop), FVector(C.X + PX - P, C.Y - PY + G, RailFoot));
			Pane(TEXT("south"), FVector(C.X - PX + P, C.Y + PY - G, Plan::DeckTop), FVector(C.X + PX - P, C.Y + PY + G, RailFoot));
			Pane(TEXT("top"), FVector(C.X - PX + P, C.Y - PY + P, Plan::CaseTop - Plan::CaseGlassMeters), FVector(C.X + PX - P, C.Y + PY - P, Plan::CaseTop));

			// The baize bed inside the glass, cut round the blocks at its corners.
			const double BX = PX - G;
			const double BY = PY - G;
			const double KX = LX - B;
			const double KY = LY - B;
			FFormExtrusion Baize;
			Baize.Outline = Polygon({C + FVector2D(-KX, -BY), C + FVector2D(KX, -BY), C + FVector2D(KX, -KY), C + FVector2D(BX, -KY), C + FVector2D(BX, KY),
				C + FVector2D(KX, KY), C + FVector2D(KX, BY), C + FVector2D(-KX, BY), C + FVector2D(-KX, KY), C + FVector2D(-BX, KY), C + FVector2D(-BX, -KY),
				C + FVector2D(-KX, -KY)});
			Baize.DepthMeters = Plan::BaizeTop - Plan::DeckTop;
			Out.Add(Drawn(Name + TEXT(".baize"), TEXT("case.baize"), MoveTemp(Baize), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::DeckTop))));
		}

		/**
		 * The lectern, the reader facing east: its cross feet on four pads, the post (drawn square at its mean, §13), the
		 * collar, the desk sloping 18° with its ledge, and two brackets each side from the post's head to the desk.
		 */
		void Lectern(TArray<FFormDrawing>& Out)
		{
			const FVector2D C(Plan::LecternX, Plan::LecternY);
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const double Angle = FMath::DegreesToRadians(90.0 * Index);
				const FVector2D At = C + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Plan::PadReach;
				Out.Add(Box(FString::Printf(TEXT("lectern.pad.%d"), Index + 1), TEXT("lectern"), FVector(At.X - Plan::PadHalf, At.Y - Plan::PadHalf, 0.0),
					FVector(At.X + Plan::PadHalf, At.Y + Plan::PadHalf, Plan::PadMeters), Solid));
			}
			const double L = Plan::FootHalfLength;
			const double W = Plan::FootHalfWidth;
			FFormExtrusion Cross;
			Cross.Outline = Polygon({C + FVector2D(-L, -W), C + FVector2D(-W, -W), C + FVector2D(-W, -L), C + FVector2D(W, -L), C + FVector2D(W, -W),
				C + FVector2D(L, -W), C + FVector2D(L, W), C + FVector2D(W, W), C + FVector2D(W, L), C + FVector2D(-W, L), C + FVector2D(-W, W), C + FVector2D(-L, W)});
			Cross.DepthMeters = Plan::FootHigh;
			Out.Add(Drawn(TEXT("lectern.feet"), TEXT("lectern"), MoveTemp(Cross), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::PadMeters))));

			const double Post = 0.25 * (Plan::PostFootWidth + Plan::PostHeadWidth);
			const double FeetTop = Plan::PadMeters + Plan::FootHigh;
			Out.Add(Box(TEXT("lectern.post"), TEXT("lectern"), FVector(C.X - Post, C.Y - Post, FeetTop), FVector(C.X + Post, C.Y + Post, Plan::CollarFoot), Solid));
			Out.Add(Box(TEXT("lectern.collar"), TEXT("lectern"), FVector(C.X - Plan::CollarHalf, C.Y - Plan::CollarHalf, Plan::CollarFoot),
				FVector(C.X + Plan::CollarHalf, C.Y + Plan::CollarHalf, Plan::CollarTop), Solid));

			// The desk in section (x, z): its top from the near edge (west) up to the far edge's 1.20, 0.03 thick.
			const double Slope = FMath::DegreesToRadians(Plan::DeskSlopeDeg);
			const FVector2D Rise(FMath::Cos(Slope), FMath::Sin(Slope));
			const FVector2D Normal(-FMath::Sin(Slope), FMath::Cos(Slope));
			const FVector2D Far(C.X + 0.5 * Plan::DeskDepth * Rise.X, Plan::DeskFarTop);
			const FVector2D Near = Far - Rise * Plan::DeskDepth;
			auto OnDesk = [&](double U, double Proud) { return Near + Rise * U + Normal * Proud; };
			const double HalfWidth = 0.5 * Plan::DeskWidth;
			FFormExtrusion Desk;
			Desk.Outline = Section({OnDesk(0.0, 0.0), OnDesk(Plan::DeskDepth, 0.0), OnDesk(Plan::DeskDepth, -Plan::DeskBoard), OnDesk(0.0, -Plan::DeskBoard)});
			Desk.DepthMeters = Plan::DeskWidth;
			Out.Add(Drawn(TEXT("lectern.desk"), TEXT("lectern"), MoveTemp(Desk), Solid, AcrossY(C.Y + HalfWidth)));
			FFormExtrusion Ledge;
			const double Back = Plan::LedgeUp - Plan::LedgeWidth;
			Ledge.Outline = Section({OnDesk(Back, 0.0), OnDesk(Plan::LedgeUp, 0.0), OnDesk(Plan::LedgeUp, Plan::LedgeProud), OnDesk(Back, Plan::LedgeProud)});
			Ledge.DepthMeters = Plan::DeskWidth;
			Out.Add(Drawn(TEXT("lectern.ledge"), TEXT("lectern"), MoveTemp(Ledge), Solid, AcrossY(C.Y + HalfWidth)));

			int32 Index = 0;
			for (const double Side : {-1.0, 1.0})
			{
				for (const double Reach : Plan::BracketUp)
				{
					const FVector2D Under = OnDesk(Reach, -Plan::DeskBoard);
					Out.Add(Beam(FString::Printf(TEXT("lectern.bracket.%d"), ++Index), TEXT("lectern"), FVector(C.X, C.Y + Side * Plan::BracketFootY, Plan::PostTop),
						FVector(Under.X, C.Y + Side * Plan::BracketHeadY, Under.Y), Plan::LecternBracket[0], Plan::LecternBracket[1], Solid));
				}
			}
		}

		/**
		 * The tapestry's batten in its sleeve, its top at the tapestry's top edge, on two painted iron brackets from the
		 * south wall; and the tapestry's linen lining, behind the weave whose face stands 0.05 off the wall.
		 */
		void Tapestry(TArray<FFormDrawing>& Out)
		{
			const double Wall = Plan::EndInnerY;
			const double Lining = Plan::TapestryFace + Plan::TapestryWeave;
			const double Batten = Lining + Plan::LiningLinen;
			const double BattenFoot = Plan::BattenTop - Plan::BattenHigh;
			Out.Add(Box(TEXT("tapestry.lining"), TEXT("tapestry.lining"), FVector(-Plan::TapestryHalf, Lining, Plan::TapestryFoot),
				FVector(Plan::TapestryHalf, Batten, Plan::BattenTop), Solid));
			Out.Add(Box(TEXT("tapestry.batten"), TEXT("tapestry.batten"), FVector(-Plan::BattenHalf, Batten, BattenFoot),
				FVector(Plan::BattenHalf, Batten + Plan::BattenThick, Plan::BattenTop), Overhead));
			const double T = Plan::BattenBracketMeters;
			for (const double Side : {-1.0, 1.0})
			{
				// In section (y, z): the plate up the wall under the batten and the arm out beneath it.
				FFormExtrusion Bracket;
				Bracket.Outline = Section({FVector2D(Wall, BattenFoot - Plan::BattenBracketPlate), FVector2D(Wall, BattenFoot), FVector2D(Batten, BattenFoot),
					FVector2D(Batten, BattenFoot - T), FVector2D(Wall - T, BattenFoot - T), FVector2D(Wall - T, BattenFoot - Plan::BattenBracketPlate)});
				Bracket.DepthMeters = 2.0 * Plan::BattenBracketHalf;
				Out.Add(Drawn(Side < 0.0 ? TEXT("tapestry.bracket.west") : TEXT("tapestry.bracket.east"), TEXT("iron.cast"), MoveTemp(Bracket), Overhead,
					AcrossX(Side * Plan::BattenBracketX - Plan::BattenBracketHalf)));
			}
		}
	}

	void DrawFittings(TArray<FFormDrawing>& Out)
	{
		PictureRail(Out);
		Bench(true, Out);
		Bench(false, Out);
		for (const double SX : {-1.0, 1.0})
		{
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::CaseY)); ++Index)
			{
				const FString Name = FString::Printf(TEXT("case.%s%d"), SX < 0.0 ? TEXT("w") : TEXT("e"), Index == 0 ? 3 : 5);
				Case(Name, FVector2D(SX * Plan::CaseX, Plan::CaseY[Index]), Out);
			}
		}
		Lectern(Out);
		Tapestry(Out);
	}
}
