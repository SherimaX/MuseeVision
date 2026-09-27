#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FormMakeTesting.h"

/**
 * Lay: flags on a grid with filled joints, ashlar courses in half bond, radial slabs in rings, herringbone boards
 * turned on their module, V-jointed units cut round a hole, and units the stock can't give. Every unit is a closed solid
 * of its own; the filling holds what the units leave.
 */
namespace Musee::Form::Tests::Laying
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** Each unit a closed solid of genus zero: the pieces' Euler characteristic is twice the units. */
		void TestUnits(FAutomationTestBase& Test, const FString& What, const FFormMadePiece& Piece, int32 Units)
		{
			TestClosed(Test, What, Piece, 2 * Units);
		}

		/** The Hall of Light's floor (hall-of-light.md, 3.1): travertine flags 1.20 by 0.80, 2.5 mm joints, 100 mm thick. */
		FFormLaid Flags120By80(const FVector2D& Size)
		{
			FFormLaid Floor;
			Floor.Outline = FFormProfile::Rectangle(FVector2D::ZeroVector, Size);
			Floor.ThicknessMeters = 0.1;
			Floor.JointMeters = 0.0025;
			Floor.Module = {FFormProfile::Rectangle(FVector2D::ZeroVector, FVector2D(1.2, 0.8))};
			Floor.LatticeU = FVector2D(1.2, 0.0);
			Floor.LatticeV = FVector2D(0.0, 0.8);
			return Floor;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormLaidGridTest, "Musee.Form.Drawing.LaidGrid", Flags)
	bool FFormLaidGridTest::RunTest(const FString& Parameters)
	{
		// Two flags by two, joints grouted 3 mm below the face.
		FFormLaid Floor = Flags120By80(FVector2D(2.4, 1.6));
		Floor.FillMeters = 0.003;
		FFormDrawing Drawing = FFormDrawing::Of(TEXT("floor"), TEXT("floor"), Floor);
		Drawing.PartSurfaces.Add(FFormLaid::JointPart, TEXT("floor.joint"));
		const TArray<FFormMadePiece> Pieces = MakeDrawing(*this, Drawing);
		if (!TestEqual(TEXT("the floor is its units and their joints"), Pieces.Num(), 2))
		{
			return false;
		}
		TestUnits(*this, TEXT("four flags"), Pieces[0], 4);
		// Each flag loses half a joint round its inner sides and runs to the outline at its outer sides.
		const double Flag = (1.2 - 0.00125) * (0.8 - 0.00125);
		TestEqual(TEXT("the flags hold four flags' stone"), VolumeCubicMeters(Pieces[0]), 4.0 * Flag * 0.1, 1e-9);
		TestEqual(TEXT("the filling is named for the floor"), Pieces[1].Name, FName(TEXT("floor.joints")));
		TestEqual(TEXT("and finished as grout"), Pieces[1].Surface, FName(TEXT("floor.joint")));
		TestClosed(*this, TEXT("the filling"), Pieces[1], 2);
		// It stands 2 micrometres off the flags and the outline all round its edges (some 16 m of them).
		TestEqual(TEXT("it fills what the flags leave, to 3 mm below the face"), VolumeCubicMeters(Pieces[1]), (2.4 * 1.6 - 4.0 * Flag) * 0.097,
			16.0 * 2.0e-6 * 0.097);

		// The Hall of Light's floor in little: curved ends, two viewing stones' discs, joints filled flush.
		FFormLaid Hall = Flags120By80(FVector2D(6.0, 3.2));
		Hall.Outline = FFormProfile::From(FVector2D(0.0, 0.0)).LineTo(FVector2D(6.0, 0.0)).ArcAbout(FVector2D(4.0, 1.6), 2.563, 2.563, -38.66, 38.66)
			.LineTo(FVector2D(0.0, 3.2)).ArcAbout(FVector2D(2.0, 1.6), 2.563, 2.563, 141.34, 218.66).Close();
		Hall.Holes = {FFormProfile::Ellipse(FVector2D(1.4, 1.6), 0.45, 0.45, 0.0, true), FFormProfile::Ellipse(FVector2D(4.3, 0.8), 0.45, 0.45, 0.0, true)};
		Hall.OriginMeters = FVector2D(0.0, 1.6);
		Hall.FillMeters = 0.0;
		FFormDrawing HallDrawing = FFormDrawing::Of(TEXT("hall.floor"), TEXT("floor"), Hall);
		HallDrawing.PartSurfaces.Add(FFormLaid::JointPart, TEXT("floor"));
		const TArray<FFormMadePiece> HallFloor = MakeDrawing(*this, HallDrawing);
		if (TestEqual(TEXT("the hall's floor is its slabs and their filling"), HallFloor.Num(), 2))
		{
			TestTrue(TEXT("its slabs are closed"), MeasureShellEdges(*HallFloor[0].Mesh).IsClosed());
			TestTrue(TEXT("its filling is closed"), MeasureShellEdges(*HallFloor[1].Mesh).IsClosed());
		}

		// Filled joints need their own surface.
		TArray<FMuseeFinding> Findings;
		MakeDrawing(*this, FFormDrawing::Of(TEXT("floor"), TEXT("floor"), Floor), &Findings);
		TestEqual(TEXT("filled joints without a surface of their own are a finding"), Findings.Num(), 1);

		// Travertine flags from a stock no longer than a metre: every one of the four is too long.
		Floor.FillMeters = -1.0;
		Floor.StockMeters = FVector2D(0.9, 1.0);
		Findings.Reset();
		const TArray<FFormMadePiece> Short = MakeDrawing(*this, FFormDrawing::Of(TEXT("floor"), TEXT("floor"), Floor), &Findings);
		TestEqual(TEXT("open joints make no filling"), Short.Num(), 1);
		TestTrue(TEXT("flags beyond their stock are Form.WithinStock's finding"), Findings.Num() == 1 && Findings[0].Check == TEXT("Form.WithinStock"));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormLaidCoursesTest, "Musee.Form.Drawing.LaidCourses", Flags)
	bool FFormLaidCoursesTest::RunTest(const FString& Parameters)
	{
		// Ashlar in half bond: courses 0.30 high of blocks 0.60 long, each course's joints half a block from the last's.
		FFormLaid Wall;
		Wall.Outline = FFormProfile::Rectangle(FVector2D::ZeroVector, FVector2D(1.2, 0.6));
		Wall.ThicknessMeters = 0.2;
		Wall.JointMeters = 0.005;
		Wall.Arris = FFormArris::Chamfer(0.003);
		Wall.Courses = {{0.3, {0.6}, 0.0}, {0.3, {0.6}, 0.3}};
		const TArray<FFormMadePiece> Pieces = MakeDrawing(*this, FFormDrawing::Of(TEXT("ashlar"), TEXT("stone"), Wall));
		if (TestEqual(TEXT("open joints: the blocks alone"), Pieces.Num(), 1))
		{
			// Two whole blocks below; above, a half, a whole and a half.
			TestUnits(*this, TEXT("five V-jointed blocks in half bond"), Pieces[0], 5);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormLaidRingsTest, "Musee.Form.Drawing.LaidRings", Flags)
	bool FFormLaidRingsTest::RunTest(const FString& Parameters)
	{
		// The Atrium's first ring (atrium.md, 3.3): 12 radial slabs from r 2.290 to 2.940, 6 mm joints, 30 mm marble.
		FFormLaid Floor;
		Floor.Outline = FFormProfile::Ellipse(FVector2D::ZeroVector, 2.94, 2.94, 0.0, false);
		Floor.Holes = {FFormProfile::Ellipse(FVector2D::ZeroVector, 2.29, 2.29, 0.0, false)};
		Floor.ThicknessMeters = 0.03;
		Floor.JointMeters = 0.006;
		Floor.Rings = {{2.29, 2.94, 12, 0.0}};
		const TArray<FFormMadePiece> Pieces = MakeDrawing(*this, FFormDrawing::Of(TEXT("floor.ring-1"), TEXT("floor"), Floor));
		if (TestEqual(TEXT("the ring is one piece of slabs"), Pieces.Num(), 1))
		{
			TestUnits(*this, TEXT("twelve radial slabs"), Pieces[0], 12);
			// The annulus less twelve radial joints of 6 mm across its width, within the chords of its circles.
			const double Expected = (UE_DOUBLE_PI * (2.94 * 2.94 - 2.29 * 2.29) - 12.0 * 0.006 * 0.65) * 0.03;
			TestEqual(TEXT("they hold the ring less its joints"), VolumeCubicMeters(Pieces[0]), Expected, 0.002 * Expected);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormLaidModuleTest, "Musee.Form.Drawing.LaidModule", Flags)
	bool FFormLaidModuleTest::RunTest(const FString& Parameters)
	{
		// Herringbone: boards 0.50 by 0.10, a pair per module, the module laid again along its lattice, turned 45 degrees,
		// cut round a column's round base in the middle of the floor.
		FFormLaid Floor;
		Floor.Outline = FFormProfile::Rectangle(FVector2D(-1.0, -1.0), FVector2D(1.0, 1.0));
		Floor.Holes = {FFormProfile::Ellipse(FVector2D::ZeroVector, 0.3, 0.3, 0.0, false)};
		Floor.ThicknessMeters = 0.022;
		Floor.JointMeters = 0.0005;
		Floor.Arris = FFormArris::Chamfer(0.001);
		Floor.AngleDegrees = 45.0;
		Floor.Module = {FFormProfile::Rectangle(FVector2D(0.0, 0.0), FVector2D(0.5, 0.1)), FFormProfile::Rectangle(FVector2D(0.0, 0.1), FVector2D(0.1, 0.6))};
		Floor.LatticeU = FVector2D(0.1, 0.1);
		Floor.LatticeV = FVector2D(-0.5, 0.5);
		const TArray<FFormMadePiece> Pieces = MakeDrawing(*this, FFormDrawing::Of(TEXT("parquet"), TEXT("oak"), Floor));
		if (!TestEqual(TEXT("the parquet is one piece of boards"), Pieces.Num(), 1))
		{
			return false;
		}
		const FShellEdges Shell = MeasureShellEdges(*Pieces[0].Mesh);
		TestTrue(FString::Printf(TEXT("every board is closed (%d open, %d non-manifold, %d misoriented edges of %d triangles)"), Shell.Boundary,
			Shell.NonManifold, Shell.Misoriented, Shell.Faces), Shell.IsClosed());
		// Every board is laid: the floor less the column's base, less the joints, less the V-joints' chamfers.
		const double Floor2 = 4.0 - UE_DOUBLE_PI * 0.09;
		const double Laid = VolumeCubicMeters(Pieces[0]) / 0.022;
		TestTrue(FString::Printf(TEXT("the boards cover the floor but for their joints (%.4f of %.4f m2)"), Laid, Floor2), Laid < Floor2 && Laid > 0.97 * Floor2);
		const FBox Box = Bounds(Pieces[0]);
		TestTrue(TEXT("they reach the outline and no further"), Box.Max.Equals(FVector(1.0, 1.0, 0.022), 1e-6) && Box.Min.Equals(FVector(-1.0, -1.0, 0.0), 1e-6));
		return true;
	}
}

#endif
