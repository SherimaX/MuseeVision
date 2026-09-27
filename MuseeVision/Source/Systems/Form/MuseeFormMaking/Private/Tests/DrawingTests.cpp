#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "FormCatalogue.h"
#include "FormMake.h"
#include "Kernel/ShellEdges.h"
#include "MuseeCheck.h"

/**
 * Form's drawing vocabulary, made through the public Make: every enclosing word is one closed solid (its skin shared
 * edge for edge, Euler characteristic 2 less twice its holes), its parts take the surfaces the drawing names, and what
 * can't be drawn is a finding.
 */
namespace Musee::Form::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** The Nymphéas oval's inner face: the ellipse 11.0 m by 7.5 m (salon.md section 3.1). */
		constexpr double OvalA = 11.0;
		constexpr double OvalB = 7.5;

		/** The museum's arched doorway: 3.0 m clear, springing at 3.3 m (Data/Site/PortClasses.json). */
		FFormWallOpening ArchedDoor(double Center)
		{
			FFormWallOpening Door;
			Door.CenterMeters = Center;
			Door.WidthMeters = 3.0;
			Door.HeadMeters = 3.3;
			Door.bArched = true;
			return Door;
		}

		TArray<FFormMadePiece> MakeOne(FAutomationTestBase& Test, const FFormDrawing& Drawing, TArray<FMuseeFinding>* OutFindings = nullptr)
		{
			TArray<FFormMadePiece> Pieces;
			TArray<FMuseeFinding> Findings;
			Musee::FormMaking::Make(Drawing, Pieces, Findings);
			if (!OutFindings)
			{
				for (const FMuseeFinding& Finding : Findings)
				{
					Test.AddError(Finding.ToString());
				}
			}
			else
			{
				*OutFindings = Findings;
			}
			return Pieces;
		}

		/** A closed solid of the given Euler characteristic (2 for a block, 0 for a ring or a block with one hole). */
		void TestSolid(FAutomationTestBase& Test, const FString& What, const FFormMadePiece& Piece, int32 Euler)
		{
			const FShellEdges Shell = MeasureShellEdges(*Piece.Mesh);
			Test.TestTrue(FString::Printf(TEXT("%s is a closed solid (%d open, %d non-manifold, %d misoriented edges of %d triangles)"), *What,
				Shell.Boundary, Shell.NonManifold, Shell.Misoriented, Shell.Faces), Shell.IsClosed());
			Test.TestEqual(FString::Printf(TEXT("%s: Euler characteristic"), *What), Shell.EulerCharacteristic(), Euler);
			Test.TestTrue(FString::Printf(TEXT("%s: Form.Sealed reads it as closed"), *What), Musee::FormMaking::MeasureShell(Piece).IsClosed());
		}

		/** The distinct material slots the mesh's triangles use. */
		TSet<int32> SlotsUsed(const FFormMadePiece& Piece)
		{
			TSet<int32> Used;
			const UE::Geometry::FDynamicMeshMaterialAttribute* Materials = Piece.Mesh->Attributes()->GetMaterialID();
			for (const int32 Triangle : Piece.Mesh->TriangleIndicesItr())
			{
				Used.Add(Materials->GetValue(Triangle));
			}
			return Used;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormStraightWallTest, "Musee.Form.Drawing.StraightWall", Flags)
	bool FFormStraightWallTest::RunTest(const FString& Parameters)
	{
		FFormWall Wall;
		Wall.Plan = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(10.0, 0.0)}, false);
		Wall.ThicknessMeters = 0.6;
		Wall.HeightMeters = 6.0;
		Wall.Openings.Add(ArchedDoor(3.0));
		FFormWallOpening Window;
		Window.CenterMeters = 7.5;
		Window.WidthMeters = 1.2;
		Window.SillMeters = 0.9;
		Window.HeadMeters = 2.4;
		Wall.Openings.Add(Window);
		FFormDrawing Drawing = FFormDrawing::Of(TEXT("wall.south"), TEXT("wall"), Wall);
		Drawing.PartSurfaces.Add(FFormWall::BackPart, TEXT("masonry"));
		Drawing.PartSurfaces.Add(FFormWall::RevealPart, TEXT("reveal"));
		const TArray<FFormMadePiece> Pieces = MakeOne(*this, Drawing);
		if (!TestEqual(TEXT("A wall is one piece"), Pieces.Num(), 1))
		{
			return false;
		}
		// The door notches the wall from the floor; the window pierces it: one through-hole.
		TestSolid(*this, TEXT("a straight wall with a door and a window"), Pieces[0], 0);
		TestEqual(TEXT("its own surface leads"), Pieces[0].Surface, FName(TEXT("wall")));
		TestEqual(TEXT("its back and reveals take their own surfaces"), Pieces[0].SlotSurfaces, TArray<FName>{TEXT("masonry"), TEXT("reveal")});
		TestEqual(TEXT("its triangles use all three surfaces"), SlotsUsed(Pieces[0]).Num(), 3);
		const UE::Geometry::FAxisAlignedBox3d Bounds = Pieces[0].Mesh->GetBounds();
		TestEqual(TEXT("it stands 6 m tall, in engine units"), Bounds.Max.Z - Bounds.Min.Z, 600.0, 1e-3);
		TestEqual(TEXT("it lies 0.6 m thick to its path's right (south)"), Bounds.Max.Y, 60.0, 1e-3);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormCurvedWallTest, "Musee.Form.Drawing.CurvedWall", Flags)
	bool FFormCurvedWallTest::RunTest(const FString& Parameters)
	{
		// The oval's wall: the inner face drawn anticlockwise from the west apex puts the thickness outside the room, and
		// the door at the east apex lies half-way round.
		FFormWall Oval;
		Oval.Plan = FFormProfile::Ellipse(FVector2D::ZeroVector, OvalA, OvalB, 180.0, false);
		Oval.ThicknessMeters = 0.6;
		Oval.HeightMeters = 7.2;
		const TArray<FFormMadePiece> Ring = MakeOne(*this, FFormDrawing::Of(TEXT("oval.wall"), TEXT("wall"), Oval));
		if (Ring.Num() == 1)
		{
			TestSolid(*this, TEXT("the oval's wall, a ring"), Ring[0], 0);
		}
		// From the west apex the path passes the east apex at half its length (Ramanujan's perimeter of the ellipse).
		const UE::Geometry::FAxisAlignedBox3d Bounds = Ring.Num() == 1 ? Ring[0].Mesh->GetBounds() : UE::Geometry::FAxisAlignedBox3d::Empty();
		TestEqual(TEXT("the ring reaches 0.6 m past the oval's east apex"), Bounds.Max.X, (OvalA + 0.6) * 100.0, 0.2);
		const double Perimeter = UE_DOUBLE_PI * (3.0 * (OvalA + OvalB) - FMath::Sqrt((3.0 * OvalA + OvalB) * (OvalA + 3.0 * OvalB)));
		Oval.Openings.Add(ArchedDoor(0.5 * Perimeter));
		const TArray<FFormMadePiece> WithDoor = MakeOne(*this, FFormDrawing::Of(TEXT("oval.wall"), TEXT("wall"), Oval));
		if (WithDoor.Num() == 1)
		{
			TestSolid(*this, TEXT("the oval's wall with its door"), WithDoor[0], 0);
		}

		// An open arc of the ellipse, with ends, and a window in it.
		FFormWall Arc;
		Arc.Plan = FFormProfile::From(FVector2D(-OvalA, 0.0)).ArcAbout(FVector2D::ZeroVector, OvalA, OvalB, 180.0, 90.0);
		Arc.ThicknessMeters = 0.6;
		Arc.HeightMeters = 5.0;
		FFormWallOpening Window;
		Window.CenterMeters = 6.0;
		Window.WidthMeters = 1.5;
		Window.SillMeters = 1.0;
		Window.HeadMeters = 2.5;
		Window.bArched = true;
		Arc.Openings.Add(Window);
		const TArray<FFormMadePiece> Quarter = MakeOne(*this, FFormDrawing::Of(TEXT("arc.wall"), TEXT("wall"), Arc));
		if (Quarter.Num() == 1)
		{
			TestSolid(*this, TEXT("a quarter of the oval with an arched window"), Quarter[0], 0);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormSweepTest, "Musee.Form.Drawing.Sweep", Flags)
	bool FFormSweepTest::RunTest(const FString& Parameters)
	{
		// A closed section (a coping's block) round the pond's ellipse: a ring.
		FFormSweep Coping;
		Coping.Section = FFormProfile::Rectangle(FVector2D(0.0, 0.0), FVector2D(0.44, 0.42));
		Coping.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, 3.9, 2.0, 0.0, false);
		const TArray<FFormMadePiece> Ring = MakeOne(*this, FFormDrawing::Of(TEXT("pond.coping"), TEXT("coping"), Coping));
		if (Ring.Num() == 1)
		{
			TestSolid(*this, TEXT("a coping round an ellipse"), Ring[0], 0);
		}
		// A closed section along an open path with a corner, capped at both ends.
		FFormSweep Rail;
		Rail.Section = FFormProfile::Rectangle(FVector2D(-0.02, 0.0), FVector2D(0.02, 0.05));
		Rail.Path = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(2.0, 0.0), FVector2D(2.0, 1.0)}, false);
		Rail.ElevationMeters = 0.9;
		const TArray<FFormMadePiece> Capped = MakeOne(*this, FFormDrawing::Of(TEXT("rail"), TEXT("rail"), Rail));
		if (Capped.Num() == 1)
		{
			TestSolid(*this, TEXT("a rail round a corner, capped"), Capped[0], 2);
		}
		// The catalogue's cove along the oval's inner face: an open moulding, made.
		const TValueOrError<FFormProfile, FString> Cove = Musee::Form::FindProfile(FMuseeKey(TEXT("cove.quarter-ellipse.1450")));
		if (!TestTrue(TEXT("the cove is in the catalogue"), Cove.HasValue()))
		{
			return false;
		}
		FFormSweep Moulding;
		Moulding.Section = Cove.GetValue();
		Moulding.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, OvalA, OvalB, 180.0, true);
		Moulding.ElevationMeters = 5.0;
		TestEqual(TEXT("the cove runs round the oval as one piece"), MakeOne(*this, FFormDrawing::Of(TEXT("cove"), TEXT("cove"), Moulding)).Num(), 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormBasinTest, "Musee.Form.Drawing.Basin", Flags)
	bool FFormBasinTest::RunTest(const FString& Parameters)
	{
		for (const bool bClockwise : {false, true})
		{
			FFormBasin Basin;
			Basin.Outline = FFormProfile::Ellipse(FVector2D::ZeroVector, 3.9, 2.0, 0.0, bClockwise);
			Basin.DepthMeters = 0.3;
			Basin.FloorMeters = 0.06;
			Basin.WallMeters = 0.003;
			FFormDrawing Drawing = FFormDrawing::Of(TEXT("pond.vessel"), TEXT("vessel"), Basin);
			Drawing.PartSurfaces.Add(FFormBasin::OuterPart, TEXT("tray"));
			const TArray<FFormMadePiece> Pieces = MakeOne(*this, Drawing);
			if (Pieces.Num() == 1)
			{
				TestSolid(*this, bClockwise ? TEXT("a basin drawn clockwise") : TEXT("a basin drawn anticlockwise"), Pieces[0], 2);
				TestEqual(TEXT("the tray is the basin's second surface"), Pieces[0].SlotSurfaces, TArray<FName>{TEXT("tray")});
				const UE::Geometry::FAxisAlignedBox3d Bounds = Pieces[0].Mesh->GetBounds();
				TestEqual(TEXT("its sides stand outside the outline"), Bounds.Max.X, (3.9 + 0.003) * 100.0, 1e-3);
			}
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormStairTest, "Musee.Form.Drawing.Stair", Flags)
	bool FFormStairTest::RunTest(const FString& Parameters)
	{
		// From the risers alone, each going is the pace less two risers.
		FFormStair ByRisers;
		ByRisers.RiseMeters = 3.0;
		ByRisers.Risers = 18;
		const FFormStairSteps Steps = ByRisers.SetOut();
		TestEqual(TEXT("eighteen risers of a sixth of a metre"), Steps.RiserMeters, 3.0 / 18.0, 1e-9);
		TestEqual(TEXT("two risers and a going make the body's pace"), Steps.PaceMeters(), 0.63, 1e-9);
		// From the run alone, the count whose steps come closest to the pace.
		FFormStair ByRun;
		ByRun.RiseMeters = 3.0;
		ByRun.RunMeters = 5.0;
		const FFormStairSteps Chosen = ByRun.SetOut();
		TestTrue(TEXT("the chosen steps make a pace within a centimetre of the body's"), FMath::Abs(Chosen.PaceMeters() - 0.63) < 0.01);
		TestTrue(TEXT("no riser is taller than the body steps"), Chosen.RiserMeters <= 0.22);

		ByRisers.WidthMeters = 2.4;
		ByRisers.WaistMeters = 0.15;
		const TArray<FFormMadePiece> Flight = MakeOne(*this, FFormDrawing::Of(TEXT("stair"), TEXT("stair"), ByRisers));
		if (Flight.Num() == 1)
		{
			TestSolid(*this, TEXT("a flight of eighteen steps"), Flight[0], 2);
		}
		// The oval's stair as built: 36 risers of 0.16111 over 11.862 m, a pace of 0.661 m, past the step rule.
		FFormStair Oval;
		Oval.RiseMeters = 5.8;
		Oval.Risers = 36;
		Oval.RunMeters = 35 * 0.3389;
		Oval.WidthMeters = 2.4;
		Oval.WaistMeters = 0.15;
		TArray<FMuseeFinding> Findings;
		TestEqual(TEXT("the oval's stair is made"), MakeOne(*this, FFormDrawing::Of(TEXT("stair"), TEXT("stair"), Oval), &Findings).Num(), 1);
		TestTrue(TEXT("and found to break the step rule"), Findings.ContainsByPredicate([](const FMuseeFinding& F)
		{
			return F.Check == FName(TEXT("Form.StepsFitTheBody"));
		}));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormCeilingTest, "Musee.Form.Drawing.GlazedCeiling", Flags)
	bool FFormCeilingTest::RunTest(const FString& Parameters)
	{
		FFormCeiling Roof;
		Roof.Outline = FFormProfile::Ellipse(FVector2D::ZeroVector, 11.6, 8.1, 0.0, false);
		Roof.Opening = FFormProfile::Ellipse(FVector2D::ZeroVector, 9.55, 6.05, 0.0, false);
		Roof.ThicknessMeters = 0.3;
		Roof.PaneMeters = 0.0215;
		Roof.PaneLevelMeters = 0.1;
		Roof.GlazingSurface = TEXT("roof.glass");
		const TArray<FFormMadePiece> Pieces = MakeOne(*this, FFormDrawing::Of(TEXT("roof"), TEXT("roof"), Roof));
		if (!TestEqual(TEXT("a glazed ceiling is the slab and its pane"), Pieces.Num(), 2))
		{
			return false;
		}
		TestSolid(*this, TEXT("the roof ring"), Pieces[0], 0);
		TestSolid(*this, TEXT("the pane"), Pieces[1], 2);
		TestEqual(TEXT("the pane is named for its slab"), Pieces[1].Name, FName(TEXT("roof.glazing")));
		TestEqual(TEXT("the pane is its glazing"), Pieces[1].Surface, FName(TEXT("roof.glass")));
		TestFalse(TEXT("glass is not Nanite"), Pieces[1].bNanite);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormReliefTest, "Musee.Form.Drawing.Relief", Flags)
	bool FFormReliefTest::RunTest(const FString& Parameters)
	{
		auto Carving = [](double U, double V) { return 0.02 * (1.0 + FMath::Sin(8.0 * U) * FMath::Cos(20.0 * V)); };
		FFormRelief Panel;
		Panel.SizeMeters = FVector2D(0.6, 0.3);
		Panel.GridMeters = 0.01;
		Panel.BackMeters = 0.03;
		Panel.HeightMeters = Carving;
		const TArray<FFormMadePiece> Flat = MakeOne(*this, FFormDrawing::Of(TEXT("panel"), TEXT("carving"), Panel));
		if (Flat.Num() == 1)
		{
			TestSolid(*this, TEXT("a carved panel"), Flat[0], 2);
		}
		// The frieze on the die of a coping round the pond: a ring of carving.
		FFormRelief Frieze;
		Frieze.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, 3.9, 2.0, 0.0, false);
		Frieze.OffsetMeters = 0.385;
		Frieze.BottomMeters = 0.075;
		Frieze.TopMeters = 0.2725;
		Frieze.GridMeters = 0.02;
		Frieze.BackMeters = 0.02;
		Frieze.HeightMeters = Carving;
		const TArray<FFormMadePiece> Ring = MakeOne(*this, FFormDrawing::Of(TEXT("frieze"), TEXT("frieze"), Frieze));
		if (Ring.Num() == 1)
		{
			TestSolid(*this, TEXT("a frieze round an ellipse"), Ring[0], 0);
			const UE::Geometry::FAxisAlignedBox3d Bounds = Ring[0].Mesh->GetBounds();
			TestTrue(TEXT("the frieze stands out from the die"), Bounds.Max.X > (3.9 + 0.385) * 100.0);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormSolidsTest, "Musee.Form.Drawing.Solids", Flags)
	bool FFormSolidsTest::RunTest(const FString& Parameters)
	{
		FFormLathe Post;
		Post.Section = FFormProfile::From(FVector2D(0.0, 0.0)).LineTo(FVector2D(0.105, 0.0), EFormJoint::Crisp)
			.LineTo(FVector2D(0.105, 0.95), EFormJoint::Crisp).LineTo(FVector2D(0.0, 0.95), EFormJoint::Crisp);
		const TArray<FFormMadePiece> Turned = MakeOne(*this, FFormDrawing::Of(TEXT("post"), TEXT("bronze"), Post));
		if (Turned.Num() == 1)
		{
			TestSolid(*this, TEXT("a turned post"), Turned[0], 2);
		}
		FFormExtrusion Slab;
		Slab.Outline = FFormProfile::Rectangle(FVector2D(-12.0, -8.5), FVector2D(12.0, 8.5));
		Slab.Holes.Add(FFormProfile::Rectangle(FVector2D(-3.9, -1.2), FVector2D(3.6, 1.2)));
		Slab.DepthMeters = 0.4;
		const TArray<FFormMadePiece> Floor = MakeOne(*this, FFormDrawing::Of(TEXT("floor"), TEXT("floor"), Slab));
		if (Floor.Num() == 1)
		{
			TestSolid(*this, TEXT("a slab round the pond's opening"), Floor[0], 0);
		}
		FFormFace Cloth;
		Cloth.Outline = FFormProfile::Ellipse(FVector2D::ZeroVector, 9.55, 6.05, 0.0, false);
		const TArray<FFormMadePiece> Face = MakeOne(*this, FFormDrawing::Of(TEXT("velarium"), TEXT("cloth"), Cloth));
		TestTrue(TEXT("a face is made, open along its outline"), Face.Num() == 1 && !Musee::FormMaking::MeasureShell(Face[0]).IsClosed());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormUnmadeTest, "Musee.Form.Drawing.Unmade", Flags)
	bool FFormUnmadeTest::RunTest(const FString& Parameters)
	{
		FFormWall Wall;
		Wall.Plan = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(4.0, 0.0)}, false);
		Wall.ThicknessMeters = 0.3;
		Wall.HeightMeters = 3.0;
		Wall.Openings.Add(ArchedDoor(2.0));
		FFormDrawing Drawing = FFormDrawing::Of(TEXT("wall.low"), TEXT("wall"), Wall);
		Drawing.Drawer = TEXT("specimen");
		TArray<FMuseeFinding> Findings;
		const TArray<FFormMadePiece> Pieces = MakeOne(*this, Drawing, &Findings);
		TestEqual(TEXT("a door taller than its wall makes nothing"), Pieces.Num(), 0);
		TestTrue(TEXT("and is a finding drawn by its stage"), Findings.Num() == 1 && Findings[0].Check == FName(TEXT("Form.Drawn"))
			&& Findings[0].Drawer == FName(TEXT("specimen")));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormCatalogueTest, "Musee.Form.Catalogue", Flags)
	bool FFormCatalogueTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<TArray<FFormProfile>, FString> All = Musee::Form::ReadProfiles();
		if (!TestTrue(TEXT("the catalogue reads"), All.HasValue()))
		{
			AddError(All.GetError());
			return false;
		}
		TestTrue(TEXT("it holds the oval's cove and skirting"), All.GetValue().Num() >= 2);
		const TValueOrError<FFormProfile, FString> Skirting = Musee::Form::FindProfile(FMuseeKey(TEXT("skirting.flat.100")));
		TestTrue(TEXT("the skirting is found by its key"), Skirting.HasValue() && Skirting.GetValue().Steps.Num() == 3);
		TestTrue(TEXT("a key it doesn't hold is refused"), Musee::Form::FindProfile(FMuseeKey(TEXT("cornice.none"))).HasError());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormSealedTest, "Musee.Form.Sealed", Flags)
	bool FFormSealedTest::RunTest(const FString& Parameters)
	{
		const FMuseeCheck* Sealed = FMuseeCheck::Find(TEXT("Form.Sealed"));
		if (!TestNotNull(TEXT("Form.Sealed is registered"), Sealed))
		{
			return false;
		}
		FFormShells Shells;
		Shells.Stage = FMuseeKey(TEXT("specimen"));
		FFormEnclosure& Room = Shells.Enclosures.AddDefaulted_GetRef();
		Room.Key = FMuseeKey(TEXT("specimen.room"));
		Room.Pieces = {TEXT("wall"), TEXT("cloth"), TEXT("door")};
		FFormPieceShell& Wall = Shells.Pieces.AddDefaulted_GetRef();
		Wall.Piece = TEXT("wall");
		Wall.Faces = 12;
		FFormPieceShell& Cloth = Shells.Pieces.AddDefaulted_GetRef();
		Cloth.Piece = TEXT("cloth");
		Cloth.Faces = 2;
		Cloth.BoundaryEdges = 4;
		FMuseeCheck::FInputs Inputs;
		Inputs.Add(Shells);
		const TValueOrError<TArray<FMuseeFinding>, FString> Found = Sealed->Evaluate(Inputs);
		if (!TestTrue(TEXT("it runs"), Found.HasValue()))
		{
			return false;
		}
		TestEqual(TEXT("an open piece and a missing piece are found; the closed wall is not"), Found.GetValue().Num(), 2);
		return true;
	}
}

#endif
