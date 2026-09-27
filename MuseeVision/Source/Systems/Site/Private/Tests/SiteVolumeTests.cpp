#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SitePlan.h"
#include "SitePortClass.h"
#include "SiteReceivedWell.h"
#include "SiteVolume.h"

/**
 * Site's checks of space: two volumes overlap only where a line runs through both, stages stacked on one axis are told
 * apart by their turned profiles, and a received well keeps out everything but the parts its class admits.
 */
namespace Musee::Site::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		TArray<FVector2D> Box(double X0, double Y0, double X1, double Y1)
		{
			return {{X0, Y0}, {X1, Y0}, {X1, Y1}, {X0, Y1}};
		}

		/**
		 * A quarter circle of radius R about (0, Z), from its bottom out to its side, as a polyline: inscribed, its corners
		 * on the arc; or circumscribed, its sides tangent to the arc, so it lies wholly outside the circle.
		 */
		TArray<FVector2D> Arc(double R, double Z, int32 Steps, bool bOutside)
		{
			const double Step = 0.5 * UE_DOUBLE_PI / Steps;
			auto At = [R, Z](double Angle, double Radius) { return FVector2D(Radius * FMath::Sin(Angle), Z - Radius * FMath::Cos(Angle)); };
			TArray<FVector2D> Points = {At(0.0, R)};
			for (int32 Index = bOutside ? 0 : 1; Index < Steps; ++Index)
			{
				Points.Add(bOutside ? At((Index + 0.5) * Step, R / FMath::Cos(0.5 * Step)) : At(Index * Step, R));
			}
			Points.Add(At(0.5 * UE_DOUBLE_PI, R));
			return Points;
		}

		/** A stacked stage: a square prism, and optionally a profile turned about the origin. */
		FSiteFootprint Stacked(double Half, double Bottom, double Top, TArray<FVector2D> Profile = {})
		{
			FSiteFootprint Footprint;
			Footprint.OutlineMeters = Box(-Half, -Half, Half, Half);
			Footprint.BottomMeters = Bottom;
			Footprint.TopMeters = Top;
			Footprint.TurnedProfileMeters = MoveTemp(Profile);
			return Footprint;
		}

		FSiteLayout Placed(TArray<TPair<const TCHAR*, FVector>> Stages)
		{
			FSiteLayout Layout;
			for (const TPair<const TCHAR*, FVector>& Stage : Stages) { Layout.TranslationsMeters.Add(FMuseeKey(Stage.Key), Stage.Value); }
			return Layout;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteOverlapTest, "Musee.Site.Overlap", Flags)
	bool FSiteOverlapTest::RunTest(const FString& Parameters)
	{
		TestEqual(TEXT("two rooms sharing a wall's face touch and don't overlap"), OverlapMeters(Box(0, 0, 4, 4), Box(4, 1, 8, 3)), 0.0, 1e-12);
		TestEqual(TEXT("one standing 0.3 into the other overlaps by 0.3"), OverlapMeters(Box(0, 0, 4, 4), Box(3.7, 1, 8, 3)), 0.3, 1e-9);
		TestEqual(TEXT("a room inside another overlaps by its whole width"), OverlapMeters(Box(0, 0, 4, 4), Box(1, 1, 2, 2)), 1.0, 1e-9);
		// A triangle whose corner pokes 0.2 into a square through its side: no corner of the square lies in it.
		TestEqual(TEXT("a corner through a side is found between the corners"), OverlapMeters(Box(0, 0, 4, 4), {{3.8, 2.0}, {6.0, 0.5}, {6.0, 3.5}}),
			0.2, 1e-6);

		// A sphere of radius 14 about z 22 sits in a canopy that holds it from outside: each side drawn on its own side of the seam.
		TArray<FVector2D> Sphere = Arc(14.0, 22.0, 64, false);
		Sphere.Add({0.0, 22.0});
		TArray<FVector2D> Canopy = Arc(14.0, 22.0, 64, true);
		Canopy.Append({{15.7, 22.0}, {15.7, -0.5}, {0.0, -0.5}});
		TestTrue(TEXT("a bowl in its canopy touches it only on the seam"), OverlapMeters(Canopy, Sphere) <= OverlapToleranceMeters);

		const TMap<FMuseeKey, FSiteFootprint> Stages = {
			{FMuseeKey(TEXT("cube")), Stacked(16.2, -38.2, -0.5)},
			{FMuseeKey(TEXT("atrium")), Stacked(15.7, -0.5, 22.0, Canopy)},
			{FMuseeKey(TEXT("sphere")), Stacked(14.3, 8.0, 22.0, Sphere)},
		};
		const FSiteLayout Layout = Placed({{TEXT("cube"), FVector(54, 0, 0)}, {TEXT("atrium"), FVector(54, 0, 0)}, {TEXT("sphere"), FVector(54, 0, 0)}});
		TestEqual(TEXT("stages stacked on one axis, meeting on their seams, stand apart"), CheckOverlaps(Stages, Layout).Num(), 0);

		const FSiteLayout Raised = Placed({{TEXT("cube"), FVector(54, 0, 0.3)}, {TEXT("atrium"), FVector(54, 0, 0)}, {TEXT("sphere"), FVector(54, 0, -0.3)}});
		const TArray<FMuseeFinding> Found = CheckOverlaps(Stages, Raised);
		TestEqual(TEXT("the cube raised into the atrium and the sphere sunk into its canopy are two overlaps"), Found.Num(), 2);
		for (const FMuseeFinding& Finding : Found)
		{
			TestEqual(TEXT("each is Site.NoOverlap"), Finding.Check, FName(TEXT("Site.NoOverlap")));
		}

		const FSiteLayout Aside = Placed({{TEXT("atrium"), FVector(54, 0, 0)}, {TEXT("sphere"), FVector(54.5, 0, 0)}});
		TestEqual(TEXT("off the axis the turned profiles don't apply, and the prisms overlap"), CheckOverlaps(Stages, Aside).Num(), 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteReceivedWellTest, "Musee.Site.ReceivedWell", Flags)
	bool FSiteReceivedWellTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FSitePortClass, FString> Class = FindPortClass(FMuseeKey(TEXT("iris.4600")));
		if (!TestTrue(TEXT("iris.4600 reads"), Class.HasValue()))
		{
			return false;
		}
		TestEqual(TEXT("its well is a circle less two notches"), Class.GetValue().WellNotches.Num(), 2);
		FSitePort Down;
		Down.Key = FMuseeKey(TEXT("sphere.down"));
		Down.Class = Class.GetValue().Key;
		Down.bHasWell = true;
		Down.Well.MinMeters = FVector(-2.3, -2.3, 7.715);
		Down.Well.MaxMeters = FVector(2.3, 2.3, 23.39);
		Down.Well.Parts = {{TEXT("mast"), TEXT("mast")}};

		// A slab's triangle in plan at a height, as three corners.
		auto Flat = [](FName Name, TArray<FVector2D> Corners, double Z)
		{
			FSiteWellPiece Piece;
			Piece.Name = Name;
			for (const FVector2D& Corner : Corners) { Piece.TrianglesMeters.Add(FVector(Corner, Z)); }
			return Piece;
		};
		const TArray<FSiteWellPiece> Pieces = {
			// A rail's blade in the north notch, 16 mm wide from r 2.265 out.
			Flat(TEXT("neck.rail.north"), {{-0.008, -2.30}, {0.008, -2.30}, {0.0, -2.266}}, 9.0),
			// A floor's corner reaching 0.2 into the well.
			Flat(TEXT("platform.floor"), {{2.10, 0.0}, {3.0, -1.0}, {3.0, 1.0}}, 20.4),
			// The same corner below the well's bottom.
			Flat(TEXT("neck.foot"), {{2.10, 0.0}, {3.0, -1.0}, {3.0, 1.0}}, 7.0),
			// A floor wholly outside the circle.
			Flat(TEXT("platform.deck"), {{2.31, 0.0}, {3.0, -1.0}, {3.0, 1.0}}, 20.4),
			// A radial end face of a ring's segment, standing on a line through the axis from r 2.4 out.
			FSiteWellPiece{TEXT("platform.seat.02"), {FVector(2.4, 0.0, 20.37), FVector(4.1, 0.0, 20.37), FVector(2.4, 0.0, 20.42)}},
			// The mast on the axis, a part the stage names.
			Flat(TEXT("mast.column"), {{-0.2, 0.0}, {0.2, -0.1}, {0.2, 0.1}}, 12.0),
		};
		TArray<FMuseeFinding> Found = CheckReceivedWell(Down, Class.GetValue(), Pieces);
		if (TestEqual(TEXT("only the floor reaching into the well is found"), Found.Num(), 1))
		{
			TestTrue(TEXT("and named"), Found[0].Summary.Contains(TEXT("platform.floor")));
			TestEqual(TEXT("it reaches 2.100 m from the axis"), Found[0].Measurement.Value, 2.1, 1e-9);
		}

		Down.Well.Parts.Add({TEXT("platform.floor"), TEXT("floor")});
		Found = CheckReceivedWell(Down, Class.GetValue(), Pieces);
		TestEqual(TEXT("naming the floor a part the class doesn't admit is still a finding"), Found.Num(), 1);
		TestEqual(TEXT("a mast part is admitted"), PartOf(Down.Well, TEXT("mast.column")), FName(TEXT("mast")));
		return true;
	}
}

#endif
