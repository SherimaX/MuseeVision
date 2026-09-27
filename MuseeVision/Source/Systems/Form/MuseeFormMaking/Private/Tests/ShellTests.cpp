#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Kernel/Extrude.h"
#include "Kernel/Lathe.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/PlanRuns.h"
#include "Kernel/Profile.h"
#include "Kernel/ShellEdges.h"
#include "Kernel/Sweep.h"
#include "Stations/Stations.h"
#include "Tests/FormTestShapes.h"

namespace Musee::Form::Tests
{
	namespace
	{
		void TestClosed(FAutomationTestBase& Test, const FString& What, const FShellEdges& Shell, int32 EulerCharacteristic)
		{
			Test.TestTrue(FString::Printf(TEXT("%s is closed (boundary %d, non-manifold %d, misoriented %d)"), *What,
				Shell.Boundary, Shell.NonManifold, Shell.Misoriented), Shell.IsClosed());
			Test.TestEqual(FString::Printf(TEXT("%s: Euler characteristic"), *What), Shell.EulerCharacteristic(),
				EulerCharacteristic);
		}

		/** A baluster's section, closed along its axis: foot, bulb, neck and cap, drawn with the solid on the left. */
		FProfileStations Baluster()
		{
			return FProfile(FVector2D(0.0, 0.0))
				.LineTo(FVector2D(0.10, 0.0))
				.LineTo(FVector2D(0.10, 0.08))
				.Arc(FVector2D(0.04, 0.30), 0.09, 0.14, -UE_DOUBLE_HALF_PI, UE_DOUBLE_HALF_PI)
				.LineTo(FVector2D(0.035, 0.60))
				.LineTo(FVector2D(0.08, 0.62))
				.LineTo(FVector2D(0.08, 0.70))
				.LineTo(FVector2D(0.0, 0.70))
				.Close()
				.Resolve(0.001);
		}

		/** A closed loop profile of a round well, in plan metres. */
		FProfileStations Circle(const FVector2D& Center, double Radius)
		{
			return FProfile(Center + FVector2D(Radius, 0.0))
				.Arc(Center, Radius, 0.0, UE_DOUBLE_TWO_PI)
				.Close()
				.Resolve(0.001);
		}

		FProfileStations Rectangle(const FVector2D& Low, const FVector2D& High)
		{
			return FProfile(Low)
				.LineTo(FVector2D(High.X, Low.Y))
				.LineTo(High)
				.LineTo(FVector2D(Low.X, High.Y))
				.Close()
				.Resolve(0.001);
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormSweptShellClosedTest, "Musee.Form.SweptShellClosed",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormSweptShellClosedTest::RunTest(const FString& Parameters)
	{
		const FProfileStations Section = CoveBlock().Resolve(0.001);

		// A moulding along two walls meeting at a right angle, capped at both ends: one closed block.
		const TArray<FVector2D> Walls = {FVector2D(0, 0), FVector2D(4, 0), FVector2D(4, 3)};
		const TArray<TArray<FSweepFrame>> Runs = PlanRuns(Walls, false, 2.5);
		FMeshBuilder Open;
		for (const TArray<FSweepFrame>& Run : Runs)
		{
			Sweep(Open, Run, Section);
		}
		const FShellEdges Uncapped = MeasureShellEdges(Open);
		TestEqual(TEXT("Uncapped, the moulding is open round both ends"), Uncapped.Boundary, 2 * Section.NumSegments());
		FMeshBuilder Capped = Open;
		TestTrue(TEXT("The start cap is made"), Cap(Capped, Runs[0][0], Section, false));
		TestTrue(TEXT("The end cap is made"), Cap(Capped, Runs.Last().Last(), Section, true));
		TestClosed(*this, TEXT("The capped moulding"), MeasureShellEdges(Capped), 2);

		// The same section round a closed 4 m by 3 m room: a ring.
		const TArray<FVector2D> Room = {FVector2D(0, 0), FVector2D(4, 0), FVector2D(4, 3), FVector2D(0, 3)};
		FMeshBuilder Ring;
		for (const TArray<FSweepFrame>& Run : PlanRuns(Room, true, 2.5))
		{
			Sweep(Ring, Run, Section);
		}
		TestClosed(*this, TEXT("The moulding round the room"), MeasureShellEdges(Ring), 0);

		// A baluster turned about its axis, its section closed along the axis: one closed solid.
		FMeshBuilder Turned;
		const FProfileStations Balustrade = Baluster();
		Lathe(Turned, Balustrade, FLatheAxis(), LatheStations(Balustrade, 0.0, UE_DOUBLE_TWO_PI));
		TestClosed(*this, TEXT("The baluster"), MeasureShellEdges(Turned), 2);

		// A slab 6 m by 4 m, 0.3 m thick, round a stair well and a service shaft (both drawn the same way round as
		// the slab): a solid with two holes through it.
		FMeshBuilder Slab;
		const FPlaneFrame Plan;
		const FProfileStations Well = Circle(FVector2D(2.0, 2.0), 0.9);
		const FProfileStations Shaft = Rectangle(FVector2D(4.2, 1.0), FVector2D(5.2, 2.0));
		const FProfileStations Outline = Rectangle(FVector2D(0, 0), FVector2D(6, 4));
		TestTrue(TEXT("The slab is extruded"), Extrude(Slab, Plan, Outline, {Well, Shaft}, -0.3, true));
		TestClosed(*this, TEXT("The slab"), MeasureShellEdges(Slab), -2);
		int32 Outward = 0;
		for (int32 Vertex = 0; Vertex < Slab.NumVertices(); ++Vertex)
		{
			const FVector P = Slab.Position(Vertex);
			const FVector ToAxis = FVector(2.0 - P.X, 2.0 - P.Y, 0.0);
			if (FMath::IsNearlyEqual(ToAxis.Size(), 0.9, 1e-9) && FMath::Abs(Slab.Normal(Vertex).Z) < 1e-9)
			{
				Outward += FVector::DotProduct(Slab.Normal(Vertex), ToAxis) > 0.0 ? 0 : 1;
			}
		}
		TestEqual(TEXT("Well sides not facing into the well"), Outward, 0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormSharedStationsTest, "Musee.Form.SharedStations",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormSharedStationsTest::RunTest(const FString& Parameters)
	{
		// A 5 m drum under its dome, meeting at the springing. The drum's stations include two door edges.
		const FProfileStations Drum = FProfile(FVector2D(5.0, 0.0)).LineTo(FVector2D(5.0, 10.0)).Resolve(0.001);
		const FProfileStations Dome = FProfile(FVector2D(5.0, 10.0))
			.Arc(FVector2D(0.0, 10.0), 5.0, 0.0, UE_DOUBLE_HALF_PI)
			.Resolve(0.001);
		const TArray<double> DoorEdges = {0.2, 0.5};
		const TArray<double> Shared = LatheStations(Drum, 0.0, UE_DOUBLE_TWO_PI, 0.001, DoorEdges);
		const int32 Rim = Shared.Num() - 1;

		// Taking the same stations, the two meet vertex for vertex: the only open edges are the drum's foot.
		FMeshBuilder Together;
		Lathe(Together, Drum, FLatheAxis(), Shared);
		Lathe(Together, Dome, FLatheAxis(), Shared);
		const FShellEdges Joined = MeasureShellEdges(Together);
		TestEqual(TEXT("Open edges when drum and dome share stations: only the foot"), Joined.Boundary, Rim);
		TestEqual(TEXT("Non-manifold edges"), Joined.NonManifold, 0);
		TestEqual(TEXT("Misoriented edges"), Joined.Misoriented, 0);

		// A dome stationed on its own (without the door edges) meets the drum with cracks along the springing.
		FMeshBuilder Apart;
		Lathe(Apart, Drum, FLatheAxis(), Shared);
		Lathe(Apart, Dome, FLatheAxis(), LatheStations(Dome, 0.0, UE_DOUBLE_TWO_PI));
		TestTrue(TEXT("Stations not shared leave the springing open"), MeasureShellEdges(Apart).Boundary > Rim);
		return true;
	}
}

#endif
