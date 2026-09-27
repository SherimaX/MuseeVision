#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FormMake.h"
#include "Kernel/ShellEdges.h"

/**
 * Sweeps off the plan: an architrave drawn on a curved face lies on the face and keeps its true width, a moulding's end
 * chamfered to a plane lies in that plane, and a paraboloid dome rises as z = rise (1 - rho^2).
 */
namespace Musee::Form::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags FaceSweepFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** Engine units in one metre (Foundation's Musee::ToEngine). */
		constexpr double EngineMeters = 100.0;

		/** A point between a curve's stations lies on a chord, within the museum's chord tolerance of the curve (1 mm). */
		constexpr double ChordBound = 1.1e-3;

		/**
		 * A point carried onto a curved face lies within the chord tolerance of it, and stands off it along the chord's
		 * mitred normal: within one and a half tolerances of its true distance.
		 */
		constexpr double OffFaceBound = 1.5e-3;

		/** The oval's inner face, the ellipse 11.0 by 7.5 (Docs/Stages/nympheas.md section 3.1). */
		constexpr double FaceA = 11.0;
		constexpr double FaceB = 7.5;

		/** A rectangle's section, A across and B out, drawn anticlockwise so its solid is inside. */
		FFormProfile Bar(double Across, double Out)
		{
			return FFormProfile::Rectangle(FVector2D(0.0, 0.0), FVector2D(Across, Out));
		}

		const FFormMadePiece* MakeSweep(FAutomationTestBase& Test, const FFormSweep& Sweep, TArray<FFormMadePiece>& Pieces)
		{
			TArray<FMuseeFinding> Findings;
			Musee::FormMaking::Make(FFormDrawing::Of(TEXT("sweep"), TEXT("sweep"), Sweep), Pieces, Findings);
			for (const FMuseeFinding& Finding : Findings)
			{
				Test.AddError(Finding.ToString());
			}
			return Pieces.Num() == 1 ? &Pieces[0] : nullptr;
		}

		void TestSolid(FAutomationTestBase& Test, const TCHAR* What, const FFormMadePiece& Piece)
		{
			const FShellEdges Shell = MeasureShellEdges(*Piece.Mesh);
			Test.TestTrue(FString::Printf(TEXT("%s is one closed solid (%d open, %d non-manifold, %d misoriented)"), What, Shell.Boundary,
				Shell.NonManifold, Shell.Misoriented), Shell.IsClosed() && Shell.EulerCharacteristic() == 2);
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormFaceSweepTest, "Musee.Form.Drawing.FaceSweep", FaceSweepFlags)
	bool FFormFaceSweepTest::RunTest(const FString& Parameters)
	{
		// A 0.16 by 0.075 architrave up both jambs and over the arch of a door 3.0 wide at the oval's east apex, drawn on the
		// oval's inner face (anticlockwise from the west apex, so it looks into the room).
		FFormSweep Architrave;
		Architrave.Section = Bar(0.16, 0.075);
		Architrave.Face = FFormProfile::Ellipse(FVector2D::ZeroVector, FaceA, FaceB, 180.0, false);
		const double Perimeter = UE_DOUBLE_PI * (3.0 * (FaceA + FaceB) - FMath::Sqrt((3.0 * FaceA + FaceB) * (FaceA + 3.0 * FaceB)));
		const double Middle = 0.5 * Perimeter;
		Architrave.Path = FFormProfile::From(FVector2D(Middle - 1.5, 0.0)).LineTo(FVector2D(Middle - 1.5, 3.3))
			.ArcAbout(FVector2D(Middle, 3.3), 1.5, 1.5, 180.0, 0.0).LineTo(FVector2D(Middle + 1.5, 0.0));
		TArray<FFormMadePiece> Pieces;
		const FFormMadePiece* Piece = MakeSweep(*this, Architrave, Pieces);
		if (!Piece)
		{
			return false;
		}
		TestSolid(*this, TEXT("the architrave on the oval's face"), *Piece);
		// Every point lies on the face or stands off it into the room by at most the section's 0.075, never behind it.
		int32 Behind = 0;
		int32 Beyond = 0;
		for (const FVector3d& V : Piece->Mesh->VerticesItr())
		{
			const FVector P = FVector(V) / EngineMeters;
			const FVector2D Gradient(P.X / (FaceA * FaceA), P.Y / (FaceB * FaceB));
			const double Off = (1.0 - (P.X * P.X / (FaceA * FaceA) + P.Y * P.Y / (FaceB * FaceB))) / (2.0 * Gradient.Size());
			Behind += Off < -OffFaceBound ? 1 : 0;
			Beyond += Off > 0.075 + OffFaceBound ? 1 : 0;
		}
		TestEqual(TEXT("nothing of it lies behind the face"), Behind, 0);
		TestEqual(TEXT("nothing stands off it more than its section"), Beyond, 0);
		const UE::Geometry::FAxisAlignedBox3d Bounds = Piece->Mesh->GetBounds();
		TestEqual(TEXT("it rises from the floor to the arch's crown and its width"), Bounds.Max.Z / EngineMeters, 3.3 + 1.5 + 0.16, ChordBound);
		TestEqual(TEXT("its feet stand on the floor"), Bounds.Min.Z, 0.0, 1.0e-9);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormChamferedEndsTest, "Musee.Form.Drawing.ChamferedEnds", FaceSweepFlags)
	bool FFormChamferedEndsTest::RunTest(const FString& Parameters)
	{
		// A bar 40 wide by 70 deep along x in plan, its end cut on a plane leaning 30 degrees from square about the vertical,
		// and its start on one leaning 30 degrees about the bar's width: the ends lie in their planes.
		FFormSweep Bar40;
		Bar40.Section = Bar(0.040, 0.070);
		Bar40.Path = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(1.0, 0.0)}, false);
		const double Lean = FMath::DegreesToRadians(30.0);
		Bar40.EndCut = FVector(FMath::Cos(Lean), FMath::Sin(Lean), 0.0);
		Bar40.StartCut = FVector(-FMath::Cos(Lean), 0.0, -FMath::Sin(Lean));
		TArray<FFormMadePiece> Pieces;
		const FFormMadePiece* Piece = MakeSweep(*this, Bar40, Pieces);
		if (!Piece)
		{
			return false;
		}
		TestSolid(*this, TEXT("the chamfered bar"), *Piece);
		int32 OffPlane = 0;
		for (const FVector3d& V : Piece->Mesh->VerticesItr())
		{
			const FVector P = FVector(V) / EngineMeters;
			const double ToEnd = FVector::DotProduct(P - FVector(1.0, 0.0, 0.0), Bar40.EndCut);
			const double ToStart = FVector::DotProduct(P, Bar40.StartCut);
			OffPlane += (FMath::Abs(ToEnd) > 1.0e-9 && FMath::Abs(ToStart) > 1.0e-9) ? 1 : 0;
		}
		TestEqual(TEXT("every point lies on one of the two cut planes"), OffPlane, 0);
		const UE::Geometry::FAxisAlignedBox3d Bounds = Piece->Mesh->GetBounds();
		TestEqual(TEXT("the end's corner on the path stays at the path's end, its far side cut back"), Bounds.Max.X / EngineMeters, 1.0, 1.0e-9);
		TestEqual(TEXT("the start's top reaches back by the lean over the bar's depth"), Bounds.Min.X / EngineMeters,
			-0.070 * FMath::Tan(Lean), 1.0e-9);

		FFormSweep Flat = Bar40;
		Flat.EndCut = FVector(0.0, 1.0, 0.0);
		TArray<FFormMadePiece> None;
		TArray<FMuseeFinding> Findings;
		Musee::FormMaking::Make(FFormDrawing::Of(TEXT("flat"), TEXT("sweep"), Flat), None, Findings);
		TestTrue(TEXT("an end cut along the bar is a finding"), None.IsEmpty() && Findings.Num() == 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormParaboloidDomeTest, "Musee.Form.Drawing.ParaboloidDome", FaceSweepFlags)
	bool FFormParaboloidDomeTest::RunTest(const FString& Parameters)
	{
		// The oval's glass: within the rim line 9.55 by 6.05, rising 0.60 as z = 0.60 (1 - rho^2), laminated 17.5 mm.
		FFormDome Glass;
		Glass.SemiAxisXMeters = 9.55;
		Glass.SemiAxisYMeters = 6.05;
		Glass.RiseMeters = 0.6;
		Glass.bParaboloid = true;
		Glass.ThicknessMeters = 0.01752;
		TArray<FFormMadePiece> Pieces;
		TArray<FMuseeFinding> Findings;
		Musee::FormMaking::Make(FFormDrawing::Of(TEXT("glass"), TEXT("glass"), Glass), Pieces, Findings);
		if (!TestTrue(TEXT("the glass is one piece, made without findings"), Pieces.Num() == 1 && Findings.IsEmpty()))
		{
			return false;
		}
		TestSolid(*this, TEXT("the paraboloid shell"), Pieces[0]);
		int32 OffTheRise = 0;
		for (const FVector3d& V : Pieces[0].Mesh->VerticesItr())
		{
			const FVector P = FVector(V) / EngineMeters;
			const double Rise = 0.6 * (1.0 - P.X * P.X / (9.55 * 9.55) - P.Y * P.Y / (6.05 * 6.05));
			// The outer face is on the paraboloid; the inner lies the glass's thickness beneath it, along its normal.
			OffTheRise += (P.Z > Rise + 1.0e-9 || P.Z < Rise - 0.02) ? 1 : 0;
		}
		TestEqual(TEXT("every point lies on the paraboloid or within the glass beneath it"), OffTheRise, 0);
		const UE::Geometry::FAxisAlignedBox3d Bounds = Pieces[0].Mesh->GetBounds();
		TestEqual(TEXT("its crown stands its rise above its springing"), Bounds.Max.Z / EngineMeters, 0.6, 1.0e-9);
		return true;
	}
}

#endif
