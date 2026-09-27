#include "ChenghuaiParts.h"
#include "ChenghuaiPlanFittings.h"
#include "ChenghuaiTrades.h"

/**
 * The cases (§3.10; ChenghuaiDisplay.cpp:63-204, 510-597), Art's display furniture built by the stage from the sheet's
 * numbers, every case's own solids colliding: the slanted handscroll cases of 臨池 and 舒卷, a phoebe cabinet on a black toe
 * under a silk bed tilted 11° to the reader and a glass hood, the lamp hidden behind a bronze rail at the back; the upright
 * cases against the walls, a plinth with its drawer, a silk deck and back between phoebe ends and mullions, the glass, a
 * canopy whose baffle hides the lamp, a lip hiding the low run in the tall ones, and the scroll cases' bronze hanging rail;
 * and the Ru basin's pedestal under its glass hood.
 */
namespace Musee::Chenghuai
{
	namespace
	{
		FBuildingFrame CaseFrame(const Plan::FCasePlan& Case)
		{
			return FBuildingFrame::Of(FVector2D(Case.OriginX, Case.OriginY), FVector2D(Case.UX, Case.UY), FVector2D(Case.VX, Case.VY));
		}

		FFormDrawing& Glass(FFormDrawing& Drawing)
		{
			Drawing.bNanite = false;
			return Drawing;
		}

		void Slanted(TArray<FFormDrawing>& Out, const Plan::FCasePlan& Case)
		{
			const FBuildingFrame F = CaseFrame(Case);
			const FString Name = FString(Case.Local);
			const double U0 = Case.U0;
			const double U1 = Case.U1;
			const double D = Case.Depth;
			const double Floor = Case.Floor;
			const double Tan = FMath::Tan(FMath::DegreesToRadians(Plan::SlantDegrees));
			const double Front = Floor + Plan::SlantFront;
			const double Back = Front + Tan * D;
			const double Cornice = Floor + Plan::SlantCornice;
			const TCHAR* Wood = TEXT("case.phoebe");
			Block(Out, Name + TEXT(".toe"), TEXT("timber.black"), F, U0 + Plan::ToeIn, U1 - Plan::ToeIn, Plan::ToeBack, D, Floor, Floor + Plan::SlantToe,
				EHit::Solid);
			Block(Out, Name + TEXT(".body"), Wood, F, U0, U1, 0.0, D, Floor + Plan::SlantToe, Floor + Plan::SlantBody, EHit::Solid);
			Block(Out, Name + TEXT(".base-moulding"), Wood, F, U0 - Plan::MouldingProud, U1 + Plan::MouldingProud, -Plan::MouldingProud, D,
				Floor + Plan::SlantToe, Floor + Plan::SlantToe + Plan::MouldingHigh, EHit::Solid);
			Block(Out, Name + TEXT(".cornice"), Wood, F, U0 - Plan::CorniceProud, U1 + Plan::CorniceProud, -Plan::CorniceProud, D, Floor + Plan::SlantBody,
				Cornice, EHit::Solid);
			// The panels on the cabinet's front, one to each metre and a half: a beaded frame round their fields.
			const int32 Panels = FMath::Max(1, FMath::RoundToInt32((U1 - U0) / Plan::CasePanel));
			TArray<TArray<FVector2D>> Fields;
			for (int32 K = 0; K < Panels; ++K)
			{
				const double A = FMath::Lerp(U0, U1, double(K) / Panels) + Plan::BeadIn + Plan::BeadWidth;
				const double B = FMath::Lerp(U0, U1, double(K + 1) / Panels) - Plan::BeadIn - Plan::BeadWidth;
				Fields.Add(Rectangle(A, Floor + Plan::BeadFrom + Plan::BeadWidth, B, Floor + Plan::BeadTo - Plan::BeadWidth));
			}
			AlongV(Out, Name + TEXT(".panels"), Wood, F, Rectangle(U0 + Plan::BeadIn, Floor + Plan::BeadFrom, U1 - Plan::BeadIn, Floor + Plan::BeadTo), Fields,
				-Plan::BeadProud, 0.0, EHit::Solid);
			// The bed: silk over a board tilted to the reader, and its phoebe wedge under it.
			const double VA = Plan::SlantBedFront;
			const double VB = D - Plan::SlantBedBack;
			const double ZA = Floor + Plan::SlantBed;
			const double ZB = ZA + Tan * (VB - VA);
			const double Half = 0.5 * Plan::SlantBedBoard;
			const double In = Plan::ToeIn;
			AlongU(Out, Name + TEXT(".bed"), TEXT("case.silk"), F, {FVector2D(VA, ZA - Half), FVector2D(VB, ZB - Half), FVector2D(VB, ZB + Half),
				FVector2D(VA, ZA + Half)}, {}, U0 + In, U1 - In, EHit::Solid);
			AlongU(Out, Name + TEXT(".bed-wedge"), Wood, F, {FVector2D(VA, Cornice), FVector2D(VB, Cornice), FVector2D(VB, ZB - Half),
				FVector2D(VA, ZA - Half)}, {}, U0 + In, U1 - In, EHit::Solid);
			// The ends, the back upstand, the front rail and the lamp rail.
			const TArray<FVector2D> End = {FVector2D(-Plan::CorniceProud, Cornice), FVector2D(D, Cornice), FVector2D(D, Back + Plan::UpstandOver),
				FVector2D(-Plan::CorniceProud, Front)};
			AlongU(Out, Name + TEXT(".end.1"), Wood, F, End, {}, U0, U0 + Plan::SlantEnd, EHit::Solid);
			AlongU(Out, Name + TEXT(".end.2"), Wood, F, End, {}, U1 - Plan::SlantEnd, U1, EHit::Solid);
			const double E0 = U0 + Plan::SlantEnd;
			const double E1 = U1 - Plan::SlantEnd;
			Block(Out, Name + TEXT(".upstand"), Wood, F, E0, E1, D - Plan::UpstandThick, D, Cornice, Back + Plan::UpstandOver, EHit::Solid);
			Block(Out, Name + TEXT(".front-rail"), Wood, F, E0, E1, -Plan::CorniceProud, Plan::FrontRailDeep - Plan::CorniceProud,
				Front - Plan::FrontRailHigh, Front, EHit::Solid);
			Block(Out, Name + TEXT(".lamp-rail"), TEXT("case.bronze"), F, U0 + Plan::ToeBack, U1 - Plan::ToeBack, D - Plan::LampRailFront,
				D - Plan::LampRailBack, Back - Plan::LampRailBottom, Back - Plan::LampRailTop, EHit::Solid);
			// The glass: the upright front, and the top sloping to the back.
			const double G = Plan::SlantGlass;
			Glass(Block(Out, Name + TEXT(".glass.front"), TEXT("case.glass"), F, E0, E1, 0.0, G, Cornice, Front - Plan::FrontRailHigh, EHit::Solid));
			const double GlassBack = D - Plan::UpstandThick;
			Glass(AlongU(Out, Name + TEXT(".glass.top"), TEXT("case.glass"), F, {FVector2D(0.0, Front), FVector2D(GlassBack, Front + Tan * GlassBack),
				FVector2D(GlassBack, Front + Tan * GlassBack + G), FVector2D(0.0, Front + G)}, {}, E0, E1, EHit::Solid));
		}

		void Upright(TArray<FFormDrawing>& Out, const Plan::FCasePlan& Case)
		{
			const FBuildingFrame F = CaseFrame(Case);
			const FString Name = FString(Case.Local);
			const double U0 = Case.U0;
			const double U1 = Case.U1;
			const double D = Case.Depth;
			const double Floor = Case.Floor;
			const double Deck = Case.Deck;
			const double Canopy = Case.Canopy;
			const TCHAR* Wood = TEXT("case.phoebe");
			const TCHAR* Silk = Case.bCeramics ? TEXT("case.silk.ceramics") : TEXT("case.silk");
			Block(Out, Name + TEXT(".toe"), TEXT("timber.black"), F, U0 + Plan::UprightToeIn, U1 - Plan::UprightToeIn, Plan::ToeIn, D, Floor,
				Floor + Plan::UprightToe, EHit::Solid);
			Block(Out, Name + TEXT(".plinth"), Wood, F, U0, U1, 0.0, D, Floor + Plan::UprightToe, Deck, EHit::Solid);
			Block(Out, Name + TEXT(".plinth-moulding.top"), Wood, F, U0 - Plan::MouldingProud, U1 + Plan::MouldingProud, -Plan::CorniceProud + Plan::BeadProud,
				D, Deck - Plan::MouldingHigh, Deck, EHit::Solid);
			Block(Out, Name + TEXT(".plinth-moulding.foot"), Wood, F, U0 - Plan::BeadProud, U1 + Plan::BeadProud, -Plan::BeadProud, D, Floor + Plan::UprightToe,
				Floor + Plan::UprightToe + Plan::MouldingHigh, EHit::Solid);
			if (Deck - Floor > Plan::DrawerLeast)
			{
				Block(Out, Name + TEXT(".drawer"), Wood, F, U0 + Plan::DrawerIn, U1 - Plan::DrawerIn, -Plan::DrawerProud, 0.0, Floor + Plan::DrawerFrom,
					Deck - Plan::DrawerUnder, EHit::Solid);
			}
			const double E0 = U0 + Plan::UprightEnd;
			const double E1 = U1 - Plan::UprightEnd;
			const double SilkFace = D - Plan::UprightBackBoard - Plan::UprightBackSilk;
			Block(Out, Name + TEXT(".deck"), Silk, F, E0, E1, Plan::UprightDeckIn, D - Plan::UprightDeckIn, Deck, Deck + Plan::UprightDeck, EHit::Solid);
			Block(Out, Name + TEXT(".back"), Silk, F, E0, E1, SilkFace, D - Plan::UprightBackBoard, Deck + Plan::UprightDeck, Canopy, EHit::Solid);
			Block(Out, Name + TEXT(".back-board"), Wood, F, E0, E1, D - Plan::UprightBackBoard, D, Deck, Canopy, EHit::Solid);
			Block(Out, Name + TEXT(".end.1"), Wood, F, U0, E0, 0.0, D, Deck, Canopy, EHit::Solid);
			Block(Out, Name + TEXT(".end.2"), Wood, F, E1, U1, 0.0, D, Deck, Canopy, EHit::Solid);
			for (int32 K = 0; K < Case.MullionCount; ++K)
			{
				const double M = Case.Mullions[K];
				const double Half = 0.5 * Plan::UprightMullion;
				Block(Out, FString::Printf(TEXT("%s.mullion.%d"), *Name, K + 1), Wood, F, M - Half, M + Half, -Plan::MullionBefore, Plan::UprightMullion, Deck,
					Canopy, EHit::Solid);
			}
			Block(Out, Name + TEXT(".canopy"), Wood, F, U0, U1, 0.0, D, Canopy, Case.Top - Plan::UprightCorniceHigh, EHit::Solid);
			Block(Out, Name + TEXT(".canopy-cornice"), Wood, F, U0 - Plan::UprightCornice, U1 + Plan::UprightCornice, -Plan::UprightCornice, D,
				Case.Top - Plan::UprightCorniceHigh, Case.Top, EHit::Solid);
			Block(Out, Name + TEXT(".baffle"), Wood, F, E0, E1, 0.0, Plan::UprightBaffleDeep, Canopy - Plan::UprightBaffle, Canopy, EHit::Solid);
			if (Canopy - Deck > Plan::UprightTall)
			{
				Block(Out, Name + TEXT(".lip"), Wood, F, E0, E1, Plan::UprightDeckIn, Plan::UprightDeckIn + Plan::UprightLipDeep, Deck + Plan::UprightDeck,
					Deck + Plan::UprightDeck + Plan::UprightLip, EHit::Solid);
			}
			Glass(Block(Out, Name + TEXT(".glass"), TEXT("case.glass"), F, E0, E1, Plan::UprightGlassAt, Plan::UprightGlassAt + Plan::UprightGlass, Deck,
				Canopy - Plan::UprightBaffle, EHit::Solid));
			if (Case.bScrolls)
			{
				const double V = D - Plan::HangingRailBack;
				const double Z = Canopy - Plan::HangingRailUnder;
				Rod(Out, Name + TEXT(".hanging-rail"), TEXT("case.bronze"), F.At(E0, V, Z), F.At(E1, V, Z), Plan::HangingRail, EHit::Solid);
			}
		}

		/** The Ru basin's case: a phoebe pedestal on a black toe, its capped top and silk deck, a glass hood over it. */
		void Pedestal(TArray<FFormDrawing>& Out)
		{
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			const double X = Plan::PedestalX;
			const double Y = Plan::PedestalY;
			const double H = Plan::PedestalHalf;
			const double Floor = Plan::PedestalFloor;
			const double Top = Floor + Plan::PedestalHigh;
			const double Hood = Top + Plan::PedestalHood;
			Block(Out, TEXT("tianqing.pedestal.toe"), TEXT("timber.black"), Stage, X - H + Plan::ToeIn, X + H - Plan::ToeIn, Y - H + Plan::ToeIn, Y + H - Plan::ToeIn,
				Floor, Floor + Plan::UprightToe, EHit::Solid);
			Block(Out, TEXT("tianqing.pedestal"), TEXT("case.phoebe"), Stage, X - H, X + H, Y - H, Y + H, Floor + Plan::UprightToe, Top - Plan::PedestalCap,
				EHit::Solid);
			const double Cap = H + Plan::PedestalCapProud;
			Block(Out, TEXT("tianqing.pedestal.cap"), TEXT("case.phoebe"), Stage, X - Cap, X + Cap, Y - Cap, Y + Cap, Top - Plan::PedestalCap, Top,
				EHit::Solid);
			const double In = H - Plan::PedestalDeckIn;
			Block(Out, TEXT("tianqing.pedestal.deck"), TEXT("case.silk.ceramics"), Stage, X - In, X + In, Y - In, Y + In, Top, Top + Plan::UprightDeck,
				EHit::Solid);
			const double G = Plan::UprightGlass;
			Glass(Raised(Out, TEXT("tianqing.pedestal.hood"), TEXT("case.glass"), Stage, Rectangle(X - H, Y - H, X + H, Y + H),
				{Rectangle(X - H + G, Y - H + G, X + H - G, Y + H - G)}, Top, Hood - G, EHit::Solid));
			Glass(Block(Out, TEXT("tianqing.pedestal.hood.top"), TEXT("case.glass"), Stage, X - H, X + H, Y - H, Y + H, Hood - G, Hood, EHit::Solid));
		}
	}

	void DrawCases(TArray<FFormDrawing>& Out)
	{
		for (const Plan::FCasePlan& Case : Plan::Cases)
		{
			if (Case.bSlanted)
			{
				Slanted(Out, Case);
			}
			else
			{
				Upright(Out, Case);
			}
		}
		Pedestal(Out);
	}
}
