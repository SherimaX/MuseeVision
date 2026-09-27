#include "ChenghuaiHallTrades.h"
#include "ChenghuaiLattice.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlan.h"
#include "ChenghuaiPlanHalls.h"
#include "ChenghuaiTrades.h"

/**
 * The ground's stonework in the house (§3.2, §3.8; ChenghuaiHouse.cpp:535-649): the flights of steps with their side
 * stones; the porch's white stone paving to the drum; the courts' square paving and their paths of brick on edge, each cut
 * round every platform's footing, flight, threshold and bed; the inner court's four kerbed beds and the rear court's two
 * tree pits (their soil Nature's); the fish bowl on the crossing and the pomegranates' tubs; the galleries' floors round
 * the inner court's corners, their own columns and their hanging fretwork.
 */
namespace Musee::Chenghuai
{
	namespace
	{
		/** A flight's own frame: v down it, away from its platform's edge; u across it. */
		FBuildingFrame FlightFrame(const Plan::FFlightPlan& Flight)
		{
			const FVector2D Out(Flight.OutX, Flight.OutY);
			return FBuildingFrame::Of(FVector2D(Flight.EdgeX, Flight.EdgeY), FVector2D(-Out.Y, Out.X), Out);
		}

		double RunOf(const Plan::FFlightPlan& Flight)
		{
			return (Flight.Risers - 1) * Flight.Going;
		}

		/** The plan's axis-square box of a building frame's rectangle (every frame of the sheet is square to the plan). */
		FBox2D PlanBox(const FBuildingFrame& F, double U0, double U1, double V0, double V1)
		{
			FBox2D Box(ForceInit);
			for (const FVector2D& Corner : {F.Plan(U0, V0), F.Plan(U1, V0), F.Plan(U1, V1), F.Plan(U0, V1)})
			{
				Box += Corner;
			}
			return Box;
		}

		FBox2D Rect(double X0, double X1, double Y0, double Y1)
		{
			return FBox2D(FVector2D(FMath::Min(X0, X1), FMath::Min(Y0, Y1)), FVector2D(FMath::Max(X0, X1), FMath::Max(Y0, Y1)));
		}

		double Mirror(double X)
		{
			return 2.0 * Plan::CourtAxis - X;
		}

		/** Everything the courts' paving is cut round: the platforms and their footings, the flights, the thresholds, the beds. */
		TArray<FBox2D> Obstacles()
		{
			TArray<FBox2D> Boxes;
			const double Foot = Plan::FootingProud;
			for (const FHallSpec& Spec : Buildings())
			{
				if (Spec.bPlatform)
				{
					Boxes.Add(PlanBox(Spec.Frame, Spec.PlatformU0 - Foot, Spec.PlatformU1 + Foot, Spec.PlatformFront - Foot, Spec.PlatformBack + Foot));
				}
			}
			for (const Plan::FFlightPlan& Flight : Plan::Flights)
			{
				const double Side = 0.5 * Flight.Width + (Flight.bSideStones ? Plan::SideStone : 0.0);
				Boxes.Add(PlanBox(FlightFrame(Flight), -Side, Side, 0.0, RunOf(Flight) + Plan::SideStoneReach));
			}
			Boxes.Add(Rect(Plan::ChuiX0 - Foot, Plan::ChuiX1 + Foot, Plan::ChuiY0 + Foot, Plan::ChuiY1 - Foot));
			const double Plinth = Plan::ScreenPlinth[0].Proud;
			Boxes.Add(Rect(-Plan::ScreenHalf - Plinth, Plan::ScreenHalf + Plinth, Plan::ScreenBack, Plan::ScreenFace + Plinth));
			for (const double Side : {1.0, -1.0})
			{
				auto X = [Side](double West) { return Side > 0.0 ? West : Mirror(West); };
				Boxes.Add(Rect(X(Plan::GalleryLowerX0), X(Plan::GalleryKerbX1), Plan::GalleryLowerY0, Plan::GalleryLowerY1));
				Boxes.Add(Rect(X(Plan::GalleryLowerX0), X(Plan::GalleryUpperX1), Plan::GalleryUpperY0, Plan::GalleryUpperY1));
				Boxes.Add(Rect(X(Plan::GalleryLowerX1), X(Plan::GalleryUpperX1), Plan::GalleryCornerKerbY0, Plan::GalleryCornerKerbY1));
			}
			for (const Plan::FWallPlan& Wall : Plan::Walls)
			{
				for (const Plan::FWallHolePlan& Hole : Wall.Holes)
				{
					const bool bSill = Hole.Kind == Plan::EWallHole::MoonGate;
					if (Hole.Kind == Plan::EWallHole::Window)
					{
						continue;
					}
					const double Past = bSill ? Plan::MoonSillPast : Plan::ThresholdPast;
					const double Wider = bSill ? 0.0 : Plan::ThresholdWider;
					const double A0 = bSill ? Plan::MoonY - 0.5 * Plan::MoonSillWidth : FMath::Min(Hole.A0, Hole.A1) - Wider;
					const double A1 = bSill ? Plan::MoonY + 0.5 * Plan::MoonSillWidth : FMath::Max(Hole.A0, Hole.A1) + Wider;
					Boxes.Add(Wall.bAlongX ? Rect(A0, A1, Wall.Face0 - Past, Wall.Face1 + Past) : Rect(Wall.Face0 - Past, Wall.Face1 + Past, A0, A1));
				}
			}
			for (const Plan::FPlanRect& Bed : Plan::InnerBeds)
			{
				Boxes.Add(Rect(Bed.X0, Bed.X1, Bed.Y0, Bed.Y1));
			}
			for (const Plan::FPlanPoint& Pit : Plan::Pits)
			{
				const double Half = 0.5 * Plan::PitSide;
				Boxes.Add(Rect(Pit.X - Half, Pit.X + Half, Pit.Y - Half, Pit.Y + Half));
			}
			return Boxes;
		}

		void Flight(TArray<FFormDrawing>& Out, const Plan::FFlightPlan& Flight)
		{
			if (Flight.Risers < 2)
			{
				return;
			}
			const FBuildingFrame F = FlightFrame(Flight);
			const double Riser = (Flight.Top - Flight.Ground) / Flight.Risers;
			const double Run = RunOf(Flight);
			const double Bed = Flight.Ground - Plan::FlightBed;
			// The treads' section down the flight: each tread one going, one riser under the last; the top riser is the kerb's.
			TArray<FVector2D> Section = {FVector2D(0.0, Bed), FVector2D(Run, Bed)};
			for (int32 Tread = 1; Tread < Flight.Risers; ++Tread)
			{
				const double Z = Flight.Ground + Tread * Riser;
				Section.Add(FVector2D(Run - (Tread - 1) * Flight.Going, Z));
				Section.Add(FVector2D(Run - Tread * Flight.Going, Z));
			}
			const FString Name = FString::Printf(TEXT("flight.%s"), Flight.Local);
			const double Half = 0.5 * Flight.Width;
			AlongU(Out, Name, TEXT("stone.step"), F, Section, {}, -Half, Half, EHit::Walkable);
			if (!Flight.bSideStones)
			{
				return;
			}
			// The sloping side stones (垂带), from the platform's floor at its edge down to the ground past the foot.
			const TArray<FVector2D> Side = {FVector2D(0.0, Bed), FVector2D(Run + Plan::SideStoneReach, Bed),
				FVector2D(Run + Plan::SideStoneReach, Flight.Ground + Plan::SideStoneToe), FVector2D(0.0, Flight.Top)};
			AlongU(Out, Name + TEXT(".side.1"), TEXT("stone.step"), F, Side, {}, -Half - Plan::SideStone, -Half, EHit::Solid);
			AlongU(Out, Name + TEXT(".side.2"), TEXT("stone.step"), F, Side, {}, Half, Half + Plan::SideStone, EHit::Solid);
		}

		/** The porch's paving: from the gate's footing and gable ends, round its front flight, to the mating arc on the drum. */
		void Porch(TArray<FFormDrawing>& Out)
		{
			const double Inner = Plan::PorchInner;
			const double Steps = Plan::PorchStepsHalf;
			const double Clear = Plan::PorchGateClear;
			TArray<FVector2D> Outline = {FVector2D(-Inner, Clear), FVector2D(-Steps, Clear), FVector2D(-Steps, Plan::PorchStepsReach),
				FVector2D(Steps, Plan::PorchStepsReach), FVector2D(Steps, Clear), FVector2D(Inner, Clear)};
			for (int32 K = 0; K <= Plan::DrumArcStations; ++K)
			{
				const double X = FMath::Lerp(Inner, -Inner, double(K) / Plan::DrumArcStations);
				Outline.Add(FVector2D(X, Plan::DrumCentreY - FMath::Sqrt(FMath::Square(Plan::DrumRadius) - X * X)));
			}
			Raised(Out, TEXT("porch.paving"), TEXT("porch.paving"), FBuildingFrame::Stage(), Outline, {}, -Plan::PorchPavingDepth, 0.0, EHit::Walkable);
		}

		/** The courts' paving and paths, each cut round the obstacles (and the paths already laid). */
		void Paving(TArray<FFormDrawing>& Out)
		{
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			TArray<FBox2D> Covered = Obstacles();
			int32 Path = 0;
			for (const Plan::FPlanRect& Rect : Plan::CourtPaths)
			{
				const double X0 = FMath::Min(Rect.X0, Rect.X1);
				const double X1 = FMath::Max(Rect.X0, Rect.X1);
				const double Y0 = FMath::Min(Rect.Y0, Rect.Y1);
				const double Y1 = FMath::Max(Rect.Y0, Rect.Y1);
				RaisedRegions(Out, FString::Printf(TEXT("court.path.%d"), ++Path), TEXT("brick.on-edge"), Stage, OpenRegions(X0, X1, Y0, Y1, Covered),
					Plan::PathLevel - Plan::PavingDepth, Plan::PathLevel, EHit::Walkable);
				Covered.Add(FBox2D(FVector2D(X0, Y0), FVector2D(X1, Y1)));
			}
			for (const Plan::FCourtExtent& Court : Plan::Courts)
			{
				RaisedRegions(Out, FString::Printf(TEXT("%s.paving"), Court.Local), TEXT("court.paving"), Stage,
					OpenRegions(Court.X0, Court.X1, Court.Y1, Court.Y0, Covered), Plan::CourtLevel - Plan::PavingDepth, Plan::CourtLevel, EHit::Walkable);
			}
		}

		/** A kerb of stone round a bed or a pit, from its bed to its top, the soil inside Nature's. */
		void Kerbed(TArray<FFormDrawing>& Out, const FString& Name, double X0, double X1, double Y0, double Y1, double Width, double Top)
		{
			const TArray<FVector2D> Inside = Rectangle(X0 + Width, Y0 + Width, X1 - Width, Y1 - Width);
			Raised(Out, Name, TEXT("stone.kerb"), FBuildingFrame::Stage(), Rectangle(X0, Y0, X1, Y1), {Inside}, Plan::KerbBed, Top, EHit::Walkable);
		}

		void BedsAndThings(TArray<FFormDrawing>& Out)
		{
			int32 Index = 0;
			for (const Plan::FPlanRect& Bed : Plan::InnerBeds)
			{
				Kerbed(Out, FString::Printf(TEXT("inner-court.bed.%d.kerb"), ++Index), FMath::Min(Bed.X0, Bed.X1), FMath::Max(Bed.X0, Bed.X1),
					FMath::Min(Bed.Y0, Bed.Y1), FMath::Max(Bed.Y0, Bed.Y1), Plan::BedKerb, Plan::BedKerbTop);
			}
			Index = 0;
			for (const Plan::FPlanPoint& Pit : Plan::Pits)
			{
				const double Half = 0.5 * Plan::PitSide;
				Kerbed(Out, FString::Printf(TEXT("rear-court.pit.%d.kerb"), ++Index), Pit.X - Half, Pit.X + Half, Pit.Y - Half, Pit.Y + Half, Plan::PitKerb,
					Plan::PitKerbTop);
			}
			Turned(Out, TEXT("inner-court.fish-bowl"), TEXT("fish-bowl"), FVector(Plan::BowlX, Plan::BowlY, Plan::PathLevel), Section(Plan::BowlSection, false),
				EHit::Solid);
			Index = 0;
			for (const Plan::FPlanPoint& Tub : Plan::Tubs)
			{
				Turned(Out, FString::Printf(TEXT("inner-court.tub.%d"), ++Index), TEXT("tub"), FVector(Tub.X, Tub.Y, Plan::CourtLevel),
					Section(Plan::TubSection, false), EHit::Solid);
			}
		}

		/** The galleries' floors round the inner court's northern corners, their own columns, and the fretwork on their court side. */
		void Galleries(TArray<FFormDrawing>& Out)
		{
			const FBuildingFrame Stage = FBuildingFrame::Stage();
			const double Kerb = Plan::KerbDepth;
			for (const bool bEast : {false, true})
			{
				auto X = [bEast](double West) { return bEast ? Mirror(West) : West; };
				const FString Side = bEast ? TEXT("east") : TEXT("west");
				auto Box = [&](const FString& Part, const TCHAR* Surface, double X0, double X1, double Y0, double Y1, double Z0, double Z1)
				{
					Block(Out, FString::Printf(TEXT("gallery.%s.%s"), *Side, *Part), Surface, Stage, FMath::Min(X(X0), X(X1)), FMath::Max(X(X0), X(X1)), Y0, Y1, Z0,
						Z1, EHit::Walkable);
				};
				const double Lower = Plan::GalleryLower;
				const double Upper = Plan::GalleryUpper;
				Box(TEXT("lower.body"), TEXT("brick.rubbed"), Plan::GalleryLowerX0, Plan::GalleryKerbX1, Plan::GalleryLowerY0, Plan::GalleryLowerY1,
					Plan::FootingBottom, Lower - Kerb);
				Box(TEXT("lower.floor"), TEXT("floor.chisi"), Plan::GalleryLowerX0, Plan::GalleryLowerX1, Plan::GalleryLowerY0, Plan::GalleryLowerY1,
					Lower - Kerb, Lower);
				Box(TEXT("lower.kerb"), TEXT("stone.kerb"), Plan::GalleryLowerX1, Plan::GalleryKerbX1, Plan::GalleryLowerY0, Plan::GalleryLowerY1,
					Plan::KerbBed, Lower);
				Box(TEXT("upper.body"), TEXT("brick.rubbed"), Plan::GalleryLowerX0, Plan::GalleryUpperX1, Plan::GalleryUpperY0, Plan::GalleryUpperY1,
					Plan::FootingBottom, Upper - Kerb);
				Box(TEXT("upper.floor"), TEXT("floor.chisi"), Plan::GalleryLowerX0, Plan::GalleryUpperX1, Plan::GalleryUpperY0, Plan::GalleryUpperY1,
					Upper - Kerb, Upper);
				Box(TEXT("upper.kerb"), TEXT("stone.kerb"), Plan::GalleryLowerX1, Plan::GalleryUpperX1, Plan::GalleryCornerKerbY0,
					Plan::GalleryCornerKerbY1, Plan::KerbBed, Upper);
				int32 Count = 0;
				for (const Plan::FGalleryColumn& Column : Plan::GalleryColumns)
				{
					DrawColumn(Out, FString::Printf(TEXT("gallery.%s.column.%d"), *Side, ++Count), Stage, X(Column.X), Column.Y, Column.Floor,
						Plan::GalleryColumnTop, Plan::GalleryColumnD, TEXT("timber.red"), Plan::GalleryColumnBase);
				}
				// The hanging fretwork under the lintels, 0.12 in from each column, its top at the lintel's underside.
				const double Top = Plan::GalleryColumnTop - (Plan::LintelPerDiameter * Plan::GalleryColumnD + Plan::LintelOver);
				Count = 0;
				for (const Plan::FPlanRun& Run : Plan::GalleryFretwork)
				{
					const FVector2D A(X(Run.X0), Run.Y0);
					const FVector2D B(X(Run.X1), Run.Y1);
					const FVector2D Along = (B - A).GetSafeNormal();
					const FBuildingFrame Line = FBuildingFrame::Of(A, Along, FVector2D(-Along.Y, Along.X));
					const double Length = FVector2D::Distance(A, B) - 2.0 * Plan::FretworkIn;
					DrawPanel(Out, FString::Printf(TEXT("gallery.%s.fretwork.%d"), *Side, ++Count), TEXT("timber.red"), Line, Plan::FretworkIn,
						-0.5 * Plan::LatticeDepth, Top - Plan::Fretwork, Length, Plan::Fretwork, ELattice::StepBrocade, false);
				}
			}
		}
	}

	void DrawFlights(TArray<FFormDrawing>& Out)
	{
		for (const Plan::FFlightPlan& Each : Plan::Flights)
		{
			Flight(Out, Each);
		}
	}

	void DrawCourts(TArray<FFormDrawing>& Out)
	{
		DrawFlights(Out);
		Porch(Out);
		Paving(Out);
		BedsAndThings(Out);
		Galleries(Out);
	}
}
