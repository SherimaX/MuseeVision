#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The roof (§3.4): the travertine-paved slab over the whole Salon, flush with its outer faces and open only over the
 * lanterns; round each opening a stone kerb carrying the outer lantern, two slopes of laminated low-iron glass at 30°
 * to a ridge along the bay, glazed gables, bronze cap bars at the sixths and a cap flashing on the ridge; and over bay
 * 3's lantern the bronze pergola with its four stone planters, where the vines are trained.
 */
namespace Musee::Salon
{
	namespace
	{
		/** The outer lantern over a bay's shaft: kerb, slopes, gables, cap bars and ridge cap. */
		void OuterLantern(TArray<FFormDrawing>& Out, const FString& Name, const FBox2D& L)
		{
			const double K = Plan::KerbWidth;
			Out.Add(PlanSlab(Name + TEXT(".kerb"), TEXT("lantern.kerb"), FFormProfile::Rectangle(L.Min - FVector2D(K, K), L.Max + FVector2D(K, K)),
				{FFormProfile::Rectangle(L.Min, L.Max)}, Plan::RoofTop, Plan::KerbTop, Solid));

			// Drawn on the lantern's cross-section (x south, y up), pushed along the bay.
			const double W = L.Max.Y;
			const double T = Plan::OuterPane;
			const double Pitch = FMath::DegreesToRadians(Plan::OuterPitchDeg);
			const double Ridge = Plan::KerbTop + W * FMath::Tan(Pitch);
			const FVector2D Up(FMath::Sin(Pitch), FMath::Cos(Pitch));
			auto Slope = [&](double Side, double Below, double Above)
			{
				// A strip on the slope between two offsets from the glass's underside, from the kerb's edge to the ridge's plane.
				const FVector2D Foot(Side * W, Plan::KerbTop);
				const FVector2D Normal(Side * Up.X, Up.Y);
				return Polygon({Foot + Normal * Below, Foot + Normal * Above, FVector2D(0.0, Ridge + Above / Up.Y), FVector2D(0.0, Ridge + Below / Up.Y)});
			};
			const double West = L.Min.X - T;
			const double Length = L.Max.X - L.Min.X + 2.0 * T;
			const FMuseeFrame Across = FaceFrame(FVector(West, 0.0, 0.0), FMuseeFrame::East());
			for (const double Side : {-1.0, 1.0})
			{
				const TCHAR* Which = Side < 0.0 ? TEXT("north") : TEXT("south");
				FFormDrawing Glass = FaceSlab(FString::Printf(TEXT("%s.glass.%s"), *Name, Which), TEXT("lantern.outer.glass"), Across, Slope(Side, 0.0, T), {},
					Length, Solid);
				Glass.bNanite = false;
				Out.Add(MoveTemp(Glass));
				for (int32 Bar = 1; Bar < Plan::GridDivisions; ++Bar)
				{
					const double X = FMath::Lerp(L.Min.X, L.Max.X, double(Bar) / Plan::GridDivisions) - 0.5 * Plan::CapBarWidth;
					Out.Add(FaceSlab(FString::Printf(TEXT("%s.bar.%s.%d"), *Name, Which, Bar), TEXT("lantern.outer.bars"),
						FaceFrame(FVector(X, 0.0, 0.0), FMuseeFrame::East()), Slope(Side, T, T + Plan::CapBarProud), {}, Plan::CapBarWidth, Solid));
				}
			}
			const FFormProfile Gable = Polygon({{-W, Plan::KerbTop}, {W, Plan::KerbTop}, {0.0, Ridge}});
			for (const bool bWest : {true, false})
			{
				FFormDrawing Glass = FaceSlab(Name + (bWest ? TEXT(".gable.west") : TEXT(".gable.east")), TEXT("lantern.outer.glass"),
					FaceFrame(FVector(bWest ? West : L.Max.X, 0.0, 0.0), FMuseeFrame::East()), Gable, {}, T, Solid);
				Glass.bNanite = false;
				Out.Add(MoveTemp(Glass));
			}
			// The cap flashing sits down over the apex, to where the slopes' tops fall under its edges.
			const double Half = 0.5 * Plan::RidgeCapWidth;
			const double Apex = Ridge + T / Up.Y;
			const double Seat = Apex - Half * FMath::Tan(Pitch);
			Out.Add(Box(Name + TEXT(".ridge"), TEXT("lantern.outer.bars"), FVector(West, -Half, Seat), FVector(West + Length, Half, Apex + Plan::RidgeCapHeight),
				Solid));
		}

		/** Bay 3's pergola: four posts, a beam along each side, thirteen bars across; the four planters at its corners. */
		void Pergola(TArray<FFormDrawing>& Out)
		{
			const double P = Plan::PostHalf;
			const double Y = Plan::PergolaY;
			for (const double X : Plan::PostX)
			{
				for (const double Side : {-1.0, 1.0})
				{
					Out.Add(Box(FString::Printf(TEXT("pergola.post.%s%s"), Side < 0.0 ? TEXT("n") : TEXT("s"), X < Plan::PostX[1] ? TEXT("w") : TEXT("e")),
						TEXT("pergola"), FVector(X - P, Side * Y - P, Plan::RoofTop), FVector(X + P, Side * Y + P, Plan::BeamBottom), Solid));
				}
			}
			const double B = Plan::BeamHalfWidth;
			for (const double Side : {-1.0, 1.0})
			{
				Out.Add(Box(Side < 0.0 ? TEXT("pergola.beam.north") : TEXT("pergola.beam.south"), TEXT("pergola"),
					FVector(Plan::BeamWest, Side * Y - B, Plan::BeamBottom), FVector(Plan::BeamEast, Side * Y + B, Plan::BeamTop), Solid));
			}
			for (int32 Bar = 0; Bar < Plan::PergolaBars; ++Bar)
			{
				const double X = Plan::PostX[0] + Plan::BarPitch * Bar;
				Out.Add(Box(FString::Printf(TEXT("pergola.bar.%02d"), Bar + 1), TEXT("pergola"), FVector(X - Plan::BarHalfWidth, -Plan::BarHalfLength, Plan::BeamTop),
					FVector(X + Plan::BarHalfWidth, Plan::BarHalfLength, Plan::BarTop), Solid));
			}
			const double Inner = 0.5 * Plan::PlanterSide - Plan::PlanterWall;
			for (const double X : Plan::PlanterX)
			{
				for (const double Side : {-1.0, 1.0})
				{
					FFormBasin Planter;
					Planter.Outline = FFormProfile::Rectangle(FVector2D(-Inner, -Inner), FVector2D(Inner, Inner));
					Planter.DepthMeters = Plan::PlanterHeight - Plan::PlanterWall;
					Planter.FloorMeters = Plan::PlanterWall;
					Planter.WallMeters = Plan::PlanterWall;
					Out.Add(Drawn(FString::Printf(TEXT("pergola.planter.%s%s"), Side < 0.0 ? TEXT("n") : TEXT("s"), X < Plan::PlanterX[1] ? TEXT("w") : TEXT("e")),
						TEXT("pergola.planter"), MoveTemp(Planter), Solid, FMuseeFrame(FVector(X, Side * Plan::PlanterY, Plan::RoofTop))));
				}
			}
		}
	}

	void DrawRoof(TArray<FFormDrawing>& Out)
	{
		TArray<FFormProfile> Openings;
		for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
		{
			const FBox2D L = Lantern(Bay);
			Openings.Add(FFormProfile::Rectangle(L.Min, L.Max));
			OuterLantern(Out, FString::Printf(TEXT("lantern.bay%d.outer"), Bay + 1), L);
		}
		Out.Add(PlanSlab(Pieces::Roof, TEXT("roof.paving"), FFormProfile::Rectangle(FVector2D(Plan::FarOuterX, -Plan::OuterHalf),
			FVector2D(Plan::EndOuterX, Plan::OuterHalf)), MoveTemp(Openings), Plan::RoofSoffit, Plan::RoofTop, Solid));
		Pergola(Out);
	}
}
