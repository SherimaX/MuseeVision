#include "CubeParts.h"

/**
 * Where the shaft meets the room (§3.6): the dark bronze sleeve in two courses through the service void and the box's
 * roof, with a gilt line over their joint; the housing the ceiling iris moves and lies open in (its inner wall, the iris's
 * seat; its roof, which the sleeve stands on; its outer wall on the ceiling modules' backs); fourteen flats hanging the
 * soffit ring from the roof outside the blades' sweep; and the gilt soffit ring that hides the open blades from the room,
 * with its two beads under it. A bead is drawn as the part of its torus below the ring's soffit, so it meets the ring
 * face to face; the inner bead is cut where the rails' tail brackets pass it.
 */
namespace Musee::Cube
{
	namespace
	{
		/** The part of a torus (major radius, tube radius, centre height) below a plane a little over its centre, as a section. */
		FFormProfile BeadSection(const double (&Bead)[3], double Plane)
		{
			const FVector2D Centre(Bead[0], Bead[2]);
			const double Tube = Bead[1];
			const double Rise = FMath::RadiansToDegrees(FMath::Asin((Plane - Centre.Y) / Tube));
			const double Chord = Tube * FMath::Cos(FMath::DegreesToRadians(Rise));
			return FFormProfile::From(FVector2D(Centre.X - Chord, Plane)).ArcAbout(Centre, Tube, Tube, 180.0 - Rise, 360.0 + Rise).Close();
		}

		void Sleeve(TArray<FFormDrawing>& Out)
		{
			using namespace Plan;
			Out.Add(Drawn(FString::Printf(TEXT("%svoid"), Pieces::Sleeve), TEXT("sleeve"), Ring(SleeveInner, SleeveOuter, VoidCourse[0], VoidCourse[1]), Solid));
			Out.Add(Drawn(FString::Printf(TEXT("%sroof"), Pieces::Sleeve), TEXT("sleeve"), Ring(SleeveInner, SleeveOuter, RoofCourse[0], RoofCourse[1]), Solid));
			Out.Add(Drawn(TEXT("sleeve.line"), TEXT("shaft.gilt"), Ring(SleeveLineInner, SleeveInner, SleeveLine[0], SleeveLine[1]), Solid));
		}

		void Housing(TArray<FFormDrawing>& Out)
		{
			using namespace Plan;
			Out.Add(Drawn(Pieces::HousingInner, TEXT("housing"), Ring(HousingInner, SeatRadius, HousingInnerWall[0], HousingInnerWall[1]), Solid));
			Out.Add(Drawn(Pieces::HousingRoof, TEXT("housing"), Ring(HousingInner, HousingOuter + HousingSkin, HousingRoof[0], HousingRoof[1]), Solid));
			Out.Add(Drawn(TEXT("housing.outer"), TEXT("housing"), Ring(HousingOuter, HousingOuter + HousingSkin, HousingOuterWall[0], HousingOuterWall[1]),
				Solid));
			// The hangers stand at plan angles 12.857 + 25.714k: halfway between the blades' pins, clear of their sweep.
			for (int32 Index = 0; Index < Hangers; ++Index)
			{
				const double Bearing = 90.0 + 180.0 / Hangers + 360.0 * Index / Hangers;
				Out.Add(Drawn(FString::Printf(TEXT("housing.hanger.%d"), Index + 1), TEXT("housing"),
					Prism(AtBearing(Bearing, HangerInner, HangerOuter, -HangerHalf, HangerHalf), Plan::Hanger[0], Plan::Hanger[1]), Solid,
					FMuseeFrame(FVector(0.0, 0.0, Plan::Hanger[0]))));
			}
		}

		void Soffit(TArray<FFormDrawing>& Out)
		{
			using namespace Plan;
			TArray<FVector2D> Points;
			for (const auto& Point : Plan::SoffitRing)
			{
				Points.Add(FVector2D(Point[0], Point[1]));
			}
			FFormLathe Annulus;
			Annulus.Section = FFormProfile::Polyline(Points, true);
			Out.Add(Drawn(Pieces::SoffitRing, TEXT("soffit"), MoveTemp(Annulus), Solid));

			FFormLathe Outer;
			Outer.Section = BeadSection(OuterBead, Ceiling);
			Out.Add(Drawn(TEXT("soffit.bead.outer"), TEXT("soffit"), MoveTemp(Outer), Solid));
			// The inner bead stops either side of each tail bracket, cut radially at the bead's outer edge.
			const double Gap = HalfAngle(BracketHalf, InnerBead[0] + InnerBead[1]);
			for (int32 Half = 0; Half < 2; ++Half)
			{
				FFormLathe Inner;
				Inner.Section = BeadSection(InnerBead, Ceiling);
				Inner.FromDegrees = PlanAngle(RailBearings[Half] + Gap);
				Inner.ToDegrees = PlanAngle(RailBearings[(Half + 1) % 2] - Gap + (Half == 0 ? 0.0 : 360.0));
				Out.Add(Drawn(FString::Printf(TEXT("soffit.bead.inner.%s"), Half == 0 ? TEXT("east") : TEXT("west")), TEXT("soffit"), MoveTemp(Inner), Solid));
			}
		}
	}

	void DrawCeiling(TArray<FFormDrawing>& Out)
	{
		Sleeve(Out);
		Housing(Out);
		Soffit(Out);
	}
}
