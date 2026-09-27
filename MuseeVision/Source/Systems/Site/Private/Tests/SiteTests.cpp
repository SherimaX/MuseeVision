#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SiteBlind.h"
#include "SiteDescription.h"
#include "SitePlan.h"
#include "SitePortClass.h"

/**
 * Site's connectors: the plan's records read, stages placed by translation alone from their ports, the checks of a
 * mating, the blind of an unmated port, and the stage contract's own check of a description.
 */
namespace Musee::Site::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		const TCHAR* const Arched = TEXT("doorway.3000x4800.arched");

		/** A doorway port on a stage's face, facing a way given as a yaw in degrees. */
		FSitePort Port(const TCHAR* Key, const FVector& Origin, double FacingDegrees, const TCHAR* Class = Arched)
		{
			FSitePort P;
			P.Key = FMuseeKey(Key);
			P.Class = FMuseeKey(Class);
			P.Frame = FMuseeFrame(Origin, FRotator(0.0, FacingDegrees, 0.0).Quaternion());
			P.WallDepthMeters = 0.6;
			return P;
		}

		FSitePlan Plan(TArray<FSiteMating> Matings)
		{
			FSitePlan P;
			P.Anchor = FMuseeKey(TEXT("oval"));
			P.Arrival.Stage = FMuseeKey(TEXT("salon"));
			P.Arrival.PointMeters = {1.0, 2.0, 0.0};
			P.Arrival.FacingDegrees = 180.0;
			P.Matings = MoveTemp(Matings);
			return P;
		}

		FSiteMating Mate(const TCHAR* A, const TCHAR* B, double Passage)
		{
			FSiteMating M;
			M.A = FMuseeKey(A);
			M.B = FMuseeKey(B);
			M.PassageMeters = Passage;
			return M;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteRecordsTest, "Musee.Site.Records", Flags)
	bool FSiteRecordsTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FSitePlan, FString> SitePlan = ReadPlan();
		if (TestTrue(TEXT("the site plan reads"), SitePlan.HasValue()))
		{
			TestEqual(TEXT("the Rotunda anchors it (D-012)"), SitePlan.GetValue().Anchor, FMuseeKey(TEXT("rotunda")));
		}
		else
		{
			AddError(SitePlan.GetError());
		}
		const TValueOrError<FSitePortClass, FString> Door = FindPortClass(FMuseeKey(Arched));
		TestTrue(TEXT("the arched doorway's crown is 4.8 m"), Door.HasValue() && FMath::IsNearlyEqual(Door.GetValue().CrownMeters(), 4.8));
		const TValueOrError<FSiteBlindDoors, FString> Doors = ReadBlindDoors();
		TestTrue(TEXT("the blind doors are the palette's bronze"), Doors.HasValue() && Doors.GetValue().Material == FName(TEXT("bronze.cusn10")));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSitePlanSolvesTest, "Musee.Site.PlanSolves", Flags)
	bool FSitePlanSolvesTest::RunTest(const FString& Parameters)
	{
		// The oval's east port faces east at (11.9, 0, 0); the salon's west port faces west at its own (-60.6, 0, 0).
		TMap<FMuseeKey, TArray<FSitePort>> Ports;
		Ports.Add(FMuseeKey(TEXT("oval")), {Port(TEXT("oval.east"), FVector(11.9, 0.0, 0.0), 0.0), Port(TEXT("oval.south"), FVector(0.0, 8.1, 0.0), 90.0)});
		Ports.Add(FMuseeKey(TEXT("salon")), {Port(TEXT("salon.west"), FVector(-60.6, 0.0, 0.0), 180.0)});
		TArray<FMuseeFinding> Findings;
		const FSiteLayout Layout = Solve(Plan({Mate(TEXT("oval.east"), TEXT("salon.west"), 0.5), Mate(TEXT("oval.south"), TEXT("garden.north"), 0.0)}),
			Ports, Findings);
		TestEqual(TEXT("a clean plan has no findings"), Findings.Num(), 0);
		TestEqual(TEXT("the anchor stands at the origin"), Layout.TranslationsMeters.FindRef(FMuseeKey(TEXT("oval"))), FVector::ZeroVector);
		TestEqual(TEXT("the salon stands where its port meets the oval's across the passage"),
			Layout.TranslationsMeters.FindRef(FMuseeKey(TEXT("salon"))), FVector(11.9 + 0.5 + 60.6, 0.0, 0.0));
		TestTrue(TEXT("the mated ports are known"), Layout.MatedPorts.Contains(FMuseeKey(TEXT("salon.west"))));
		TestFalse(TEXT("a port whose neighbour isn't built stays blind"), Layout.MatedPorts.Contains(FMuseeKey(TEXT("oval.south"))));
		TestTrue(TEXT("the arrival is placed with its stage"), Layout.bHasArrival
			&& Layout.ArrivalMeters.Equals(FVector(11.9 + 0.5 + 60.6 + 1.0, 2.0, 0.0), 1e-9));

		// Ports that face the same way can't meet.
		Ports[FMuseeKey(TEXT("salon"))][0] = Port(TEXT("salon.west"), FVector(-60.6, 0.0, 0.0), 0.0);
		Findings.Reset();
		const FSiteLayout Askew = Solve(Plan({Mate(TEXT("oval.east"), TEXT("salon.west"), 0.0)}), Ports, Findings);
		TestTrue(TEXT("ports that don't face each other are found"), Findings.Num() == 1 && Findings[0].Check == FName(TEXT("Site.PortsMate")));
		TestFalse(TEXT("and don't place the stage"), Askew.TranslationsMeters.Contains(FMuseeKey(TEXT("salon"))));

		// A loop that returns off by more than a millimetre.
		Ports[FMuseeKey(TEXT("salon"))] = {Port(TEXT("salon.west"), FVector(-60.6, 0.0, 0.0), 180.0), Port(TEXT("salon.south"), FVector(-30.0, 7.6, 0.0), 90.0)};
		Ports.Add(FMuseeKey(TEXT("court")), {Port(TEXT("court.north"), FVector(0.0, -5.0, 0.0), 270.0), Port(TEXT("court.west"), FVector(-5.0, 0.0, 0.0), 180.0)});
		Ports[FMuseeKey(TEXT("oval"))].Add(Port(TEXT("oval.east-lower"), FVector(11.9, 20.0, 0.0), 0.0));
		Findings.Reset();
		const FSiteLayout Loop = Solve(Plan({Mate(TEXT("oval.east"), TEXT("salon.west"), 0.0), Mate(TEXT("salon.south"), TEXT("court.north"), 0.0),
			Mate(TEXT("oval.east-lower"), TEXT("court.west"), 0.0)}), Ports, Findings);
		TestTrue(TEXT("a loop that doesn't close is found"), Findings.ContainsByPredicate([](const FMuseeFinding& F)
		{
			return F.Check == FName(TEXT("Site.LoopsClose"));
		}));
		TestEqual(TEXT("every stage of the loop is still placed"), Loop.TranslationsMeters.Num(), 3);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteBlindTest, "Musee.Site.Blind", Flags)
	bool FSiteBlindTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FSitePortClass, FString> Class = FindPortClass(FMuseeKey(Arched));
		const TValueOrError<FSiteBlindDoors, FString> Doors = ReadBlindDoors();
		if (!TestTrue(TEXT("the records read"), Class.HasValue() && Doors.HasValue()))
		{
			return false;
		}
		const FSitePort West = Port(TEXT("oval.west"), FVector(-11.6, 0.0, 0.0), 180.0);
		TArray<FFormDrawing> Drawings;
		FString Why;
		if (!TestTrue(TEXT("an unmated door gets its blind"), Blind(West, Class.GetValue(), Doors.GetValue(), Drawings, Why)))
		{
			AddError(Why);
			return false;
		}
		if (!TestEqual(TEXT("two leaves"), Drawings.Num(), 2))
		{
			return false;
		}
		TestEqual(TEXT("named for the port"), Drawings[0].Name, FName(TEXT("blind.west.left")));
		for (const FFormDrawing& Leaf : Drawings)
		{
			TestEqual(TEXT("each leaf is the palette's bronze"), Leaf.Material, FName(TEXT("bronze.cusn10")));
			TestEqual(TEXT("drawn by Site"), Leaf.Drawer, FName(TEXT("Site")));
			TestTrue(TEXT("an extrusion"), Leaf.Shape.IsType<FFormExtrusion>());
			// The leaf's frame stands inside the wall: behind the port's face by the set-back and the leaf, facing out.
			const FVector Inward = Leaf.Frame.OriginMeters - West.Frame.OriginMeters;
			TestEqual(TEXT("set back inside the wall"), FVector::DotProduct(Inward, West.Facing()),
				-(Doors.GetValue().SetBackMeters + Doors.GetValue().LeafMeters), 1e-9);
			TestTrue(TEXT("pushed out towards the port's face"),
				Leaf.Frame.DirectionToParent(FVector::UpVector).Equals(West.Facing(), 1e-9));
		}
		FSitePort Thin = West;
		Thin.WallDepthMeters = 0.1;
		Drawings.Reset();
		TestFalse(TEXT("a wall too thin for the leaves is refused"), Blind(Thin, Class.GetValue(), Doors.GetValue(), Drawings, Why));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteDescribedTest, "Musee.Site.Described", Flags)
	bool FSiteDescribedTest::RunTest(const FString& Parameters)
	{
		FSiteDescription Description;
		Description.Stage = FMuseeKey(TEXT("oval"));
		Description.Ports.Add(Port(TEXT("oval.east"), FVector::ZeroVector, 0.0));
		Description.Pieces.Add(FFormDrawing::Of(TEXT("floor"), TEXT("floor"), FFormExtrusion()));
		FFormEnclosure& Room = Description.Enclosures.AddDefaulted_GetRef();
		Room.Key = FMuseeKey(TEXT("oval.room"));
		Room.Pieces = {TEXT("floor")};
		TestEqual(TEXT("a sound description passes"), CheckDescription(Description).Num(), 0);

		Description.Ports.Add(Port(TEXT("salon.west"), FVector::ZeroVector, 0.0));
		Description.Pieces.Add(FFormDrawing::Of(TEXT("floor"), TEXT("floor"), FFormExtrusion()));
		Room.Pieces.Add(TEXT("vault"));
		TestEqual(TEXT("another stage's port, a repeated name and an undrawn piece are three breaks"), CheckDescription(Description).Num(), 3);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteRidingBodiesTest, "Musee.Site.RidingBodies", Flags)
	bool FSiteRidingBodiesTest::RunTest(const FString& Parameters)
	{
		FSiteDescription Description;
		Description.Stage = FMuseeKey(TEXT("lift"));
		const FMuseeKey Car(TEXT("lift.car"));
		const FMuseeKey Leaf(TEXT("lift.car.leaf"));
		Description.Anchors.Add({Car, TEXT("Body"), FMuseeFrame()});
		Description.Anchors.Add({Leaf, TEXT("Body"), FMuseeFrame(FVector(-2.0, 0.0, 0.0))});
		Description.BodyParents.Add(Leaf, Car);
		TestEqual(TEXT("a leaf riding its car is sound"), CheckDescription(Description).Num(), 0);

		Description.BodyParents.Add(Car, Leaf);
		TestEqual(TEXT("a car riding its own leaf is a loop, found once for each"), CheckDescription(Description).Num(), 2);

		Description.BodyParents.Reset();
		Description.BodyParents.Add(Leaf, FMuseeKey(TEXT("lift.shaft")));
		TestEqual(TEXT("riding a body the stage doesn't declare is a break"), CheckDescription(Description).Num(), 1);
		return true;
	}
}

#endif
