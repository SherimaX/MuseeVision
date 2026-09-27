#include "NympheasParts.h"
#include "NympheasPlan.h"

/**
 * The fittings: the three guard rails round the pond's opening (§3.6), the body that retracts into the curb's chases
 * (nympheas.guard-rails), drawn down, as at rest; and the four curved oak benches on their travertine plinths (§3.8).
 */
namespace Musee::Nympheas
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** A round bar's section, drawn anticlockwise so its solid is on its left. */
		FFormProfile Round(double Diameter)
		{
			return FFormProfile::Ellipse(FVector2D::ZeroVector, 0.5 * Diameter, 0.5 * Diameter, 0.0, true);
		}

		/** Where the rails ride: at rest each is lowered by the travel, into its chase. */
		double AtRest(double Height) { return Height - Plan::RailTravel; }

		/**
		 * One straight rail between two plan points, with its top and mid rails and its stanchions. The rails stop short of
		 * the chase's ends by the chase's side clearance; the stanchions stand evenly, no more than 1.10 apart, the end ones
		 * inside the rail's ends, from the chase's foot at rest into the top rail's axis.
		 */
		void Rail(TArray<FFormDrawing>& Out, const FString& Name, FVector2D From, FVector2D To)
		{
			const double Clearance = 0.5 * (Plan::ChaseWidth - Plan::TopRailDiameter);
			const FVector2D Along = (To - From).GetSafeNormal();
			From += Along * Clearance;
			To -= Along * Clearance;
			const FMuseeKey Body = Key(TEXT("guard-rails"));
			const double TopAxis = AtRest(Plan::TopRailHeight - 0.5 * Plan::TopRailDiameter);
			struct FBar
			{
				const TCHAR* Part;
				double Diameter;
				double Axis;
			};
			const FBar Bars[] = {{TEXT("top"), Plan::TopRailDiameter, TopAxis}, {TEXT("mid"), Plan::MidRailDiameter, AtRest(Plan::MidRailHeight)}};
			for (const FBar& Bar : Bars)
			{
				FFormSweep Shape;
				Shape.Section = Round(Bar.Diameter);
				Shape.Path = FFormProfile::Polyline({From, To}, false);
				Shape.ElevationMeters = Bar.Axis;
				Out.Add_GetRef(Drawn(*FString::Printf(TEXT("%s.%s"), *Name, Bar.Part), TEXT("guard.rail"), MoveTemp(Shape), Solid)).Body = Body;
			}
			const double Radius = 0.5 * Plan::StanchionDiameter;
			const double Length = FVector2D::Distance(From, To) - 2.0 * Radius;
			const int32 Spans = FMath::Max(1, FMath::CeilToInt32(Length / Plan::StanchionSpacing));
			for (int32 Index = 0; Index <= Spans; ++Index)
			{
				const FVector2D At = From + Along * (Radius + Length * Index / Spans);
				FFormLathe Post;
				Post.Section = FFormProfile::Polyline({FVector2D(0.0, AtRest(0.0)), FVector2D(Radius, AtRest(0.0)), FVector2D(Radius, TopAxis),
					FVector2D(0.0, TopAxis)}, false);
				Out.Add_GetRef(Drawn(*FString::Printf(TEXT("%s.stanchion.%02d"), *Name, Index), TEXT("guard.rail"), MoveTemp(Post), Solid,
					FMuseeFrame(FVector(At.X, At.Y, 0.0)))).Body = Body;
			}
		}

		void GuardRails(TArray<FFormDrawing>& Out)
		{
			const double Stop = Plan::RailStopAtPost;
			const TPair<double, double> Stretches[] = {{Plan::RailWestX, -Plan::PostX - Stop}, {-Plan::PostX + Stop, Plan::PostX - Stop},
				{Plan::PostX + Stop, Plan::RailEastX}};
			for (const bool bNorth : {true, false})
			{
				const double Y = bNorth ? -Plan::RailY : Plan::RailY;
				for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Stretches)); ++Index)
				{
					Rail(Out, FString::Printf(TEXT("guard.%s.%d"), bNorth ? TEXT("north") : TEXT("south"), Index + 1),
						FVector2D(Stretches[Index].Key, Y), FVector2D(Stretches[Index].Value, Y));
				}
			}
			Rail(Out, TEXT("guard.east"), FVector2D(Plan::EastRailX, -Plan::EastRailHalf), FVector2D(Plan::EastRailX, Plan::EastRailHalf));
		}

		/** A rectangle's section with every arris eased to a radius, centred across A, from Bottom to Top. */
		FFormProfile Eased(double Half, double Bottom, double Top, double Radius)
		{
			FFormProfile S = FFormProfile::From(FVector2D(-Half + Radius, Bottom));
			S.LineTo(FVector2D(Half - Radius, Bottom), EFormJoint::Crisp);
			S.ArcAbout(FVector2D(Half - Radius, Bottom + Radius), Radius, Radius, -90.0, 0.0, EFormJoint::Smooth);
			S.ArcAbout(FVector2D(Half - Radius, Top - Radius), Radius, Radius, 0.0, 90.0, EFormJoint::Smooth);
			S.ArcAbout(FVector2D(-Half + Radius, Top - Radius), Radius, Radius, 90.0, 180.0, EFormJoint::Smooth);
			S.ArcAbout(FVector2D(-Half + Radius, Bottom + Radius), Radius, Radius, 180.0, 270.0, EFormJoint::Smooth);
			return S.Close();
		}

		/**
		 * A plinth's section: the toe recess at the floor, the body with its top arrises eased, and the neck set in under
		 * the seat that leaves the shadow gap.
		 */
		FFormProfile PlinthSection()
		{
			const double Body = 0.5 * Plan::SeatWidth - Plan::PlinthSetBack;
			const double Toe = Body - Plan::PlinthToeInset;
			const double Neck = Body - Plan::PlinthNeckInset;
			const double Ease = Plan::PlinthEase;
			FFormProfile S = FFormProfile::From(FVector2D(-Toe, 0.0));
			S.LineTo(FVector2D(Toe, 0.0), EFormJoint::Crisp).LineTo(FVector2D(Toe, Plan::PlinthToeHeight), EFormJoint::Crisp)
				.LineTo(FVector2D(Body, Plan::PlinthToeHeight), EFormJoint::Crisp);
			S.ArcAbout(FVector2D(Body - Ease, Plan::PlinthTop - Ease), Ease, Ease, 0.0, 90.0, EFormJoint::Crisp);
			S.LineTo(FVector2D(Neck, Plan::PlinthTop), EFormJoint::Crisp).LineTo(FVector2D(Neck, Plan::SeatUnderside), EFormJoint::Crisp)
				.LineTo(FVector2D(-Neck, Plan::SeatUnderside), EFormJoint::Crisp).LineTo(FVector2D(-Neck, Plan::PlinthTop), EFormJoint::Crisp);
			S.ArcAbout(FVector2D(-Body + Ease, Plan::PlinthTop - Ease), Ease, Ease, 90.0, 180.0, EFormJoint::Crisp);
			S.LineTo(FVector2D(-Body, Plan::PlinthToeHeight), EFormJoint::Crisp).LineTo(FVector2D(-Toe, Plan::PlinthToeHeight), EFormJoint::Crisp);
			return S.Close();
		}

		/** One bench along its centreline: the oak slab, and a plinth 0.30 in from each end. */
		void Bench(TArray<FFormDrawing>& Out, const TCHAR* Name, TConstArrayView<FVector2D> Line)
		{
			FFormSweep Seat;
			Seat.Section = Eased(0.5 * Plan::SeatWidth, Plan::SeatUnderside, Plan::SeatTop, Plan::SeatEase);
			Seat.Path = SmoothPath(Line);
			Out.Add(Drawn(*FString::Printf(TEXT("bench.%s.seat"), Name), TEXT("bench.seat"), MoveTemp(Seat), Solid));
			const double Length = PolylineLength(Line);
			const TPair<double, double> Ends[] = {{Plan::PlinthFromEnd, Plan::PlinthFromEnd + Plan::PlinthLength},
				{Length - Plan::PlinthFromEnd - Plan::PlinthLength, Length - Plan::PlinthFromEnd}};
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Ends)); ++Index)
			{
				FFormSweep Plinth;
				Plinth.Section = PlinthSection();
				Plinth.Path = SmoothPath(PolylineBetween(Line, Ends[Index].Key, Ends[Index].Value));
				Out.Add(Drawn(*FString::Printf(TEXT("bench.%s.plinth.%d"), Name, Index + 1), TEXT("bench.plinth"), MoveTemp(Plinth), Solid));
			}
		}
	}

	TArray<FVector2D> BenchLine(int32 Bench)
	{
		// B1 (south-west) and B2 (south-east) are drawn; B3 (north-east) and B4 (north-west) mirror B2 and B1 in y.
		TArray<FVector2D> Points;
		const double Sign = Bench < 2 ? 1.0 : -1.0;
		auto Take = [&Points, Sign](const auto& Table)
		{
			for (const auto& Point : Table)
			{
				Points.Add(FVector2D(Point[0], Sign * Point[1]));
			}
		};
		if (Bench == 0 || Bench == 3)
		{
			Take(Plan::BenchSouthWest);
		}
		else
		{
			Take(Plan::BenchSouthEast);
		}
		return CatmullRom(Points, Plan::BenchSubdivisions);
	}

	void DrawFittings(TArray<FFormDrawing>& Out)
	{
		GuardRails(Out);
		for (int32 Index = 0; Index < 4; ++Index)
		{
			Bench(Out, *FString::Printf(TEXT("b%d"), Index + 1), BenchLine(Index));
		}
	}
}
