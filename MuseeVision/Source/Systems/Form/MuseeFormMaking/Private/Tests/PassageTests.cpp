#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FormMake.h"
#include "Kernel/ShellEdges.h"

/**
 * Passages through walls: a straight passage through a curved wall runs its jambs parallel to its axis, as the
 * Rotunda's doorways are cut through the drum; and a wall between two faces fills the gap between a flat port face and a
 * drum, running out to a feather where the two touch, as the joint's wedge masonry does.
 */
namespace Musee::Form::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags PassageFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** Engine units in one metre (Foundation's Musee::ToEngine). */
		constexpr double Engine = 100.0;

		/**
		 * A point between a curve's stations lies on a chord, within the museum's chord tolerance of the curve (1 mm); and a
		 * distance along a curved face is measured on its chords, which fall short of the true arc by as much again round
		 * half a drum.
		 */
		constexpr double OnChord = 1.5e-3;

		/** The Rotunda's drum: inner face r 10.00, outer r 11.20 (Docs/Stages/rotunda.md section 2). */
		constexpr double DrumInner = 10.0;
		constexpr double DrumDepth = 1.2;

		/** doorway.3000x6000.arched: 3.00 clear, springing 4.50 (Data/Site/PortClasses.json). */
		FFormWallOpening RotundaDoor(double Center)
		{
			FFormWallOpening Door;
			Door.CenterMeters = Center;
			Door.WidthMeters = 3.0;
			Door.HeadMeters = 4.5;
			Door.bArched = true;
			Door.bStraight = true;
			return Door;
		}

		const FFormMadePiece* MakeWall(FAutomationTestBase& Test, const FFormWall& Wall, TArray<FFormMadePiece>& Pieces)
		{
			TArray<FMuseeFinding> Findings;
			Musee::FormMaking::Make(FFormDrawing::Of(TEXT("wall"), TEXT("wall"), Wall), Pieces, Findings);
			for (const FMuseeFinding& Finding : Findings)
			{
				Test.AddError(Finding.ToString());
			}
			return Pieces.Num() == 1 ? &Pieces[0] : nullptr;
		}

		void TestClosed(FAutomationTestBase& Test, const TCHAR* What, const FFormMadePiece& Piece, int32 Euler)
		{
			const FShellEdges Shell = MeasureShellEdges(*Piece.Mesh);
			Test.TestTrue(FString::Printf(TEXT("%s is a closed solid (%d open, %d non-manifold, %d misoriented)"), What, Shell.Boundary,
				Shell.NonManifold, Shell.Misoriented), Shell.IsClosed());
			Test.TestEqual(FString::Printf(TEXT("%s: Euler characteristic"), What), Shell.EulerCharacteristic(), Euler);
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormStraightPassageTest, "Musee.Form.Drawing.StraightPassage", PassageFlags)
	bool FFormStraightPassageTest::RunTest(const FString& Parameters)
	{
		// The drum's inner face drawn anticlockwise from the west, so its depth lies outside; the east door half-way round.
		FFormWall Drum;
		Drum.Plan = FFormProfile::Ellipse(FVector2D::ZeroVector, DrumInner, DrumInner, 180.0, false);
		Drum.ThicknessMeters = DrumDepth;
		Drum.HeightMeters = 9.47;
		Drum.Openings.Add(RotundaDoor(UE_DOUBLE_PI * DrumInner));
		TArray<FFormMadePiece> Pieces;
		const FFormMadePiece* Piece = MakeWall(*this, Drum, Pieces);
		if (!Piece)
		{
			return false;
		}
		TestClosed(*this, TEXT("the drum with a straight door"), *Piece, 0);
		// Below the springing nothing of the drum stands within the clear width, and the jambs lie on the planes y ±1.50
		// from the inner face (x 9.887) to the outer (x 11.099): the rotunda sheet's jamb/face intersections.
		int32 Inside = 0;
		bool bInnerJamb = false;
		bool bOuterJamb = false;
		for (const FVector3d& V : Piece->Mesh->VerticesItr())
		{
			const FVector P = FVector(V) / Engine;
			if (P.X > 9.0 && P.Z > -1.0e-9 && P.Z < 4.49)
			{
				Inside += FMath::Abs(P.Y) < 1.5 - OnChord ? 1 : 0;
				bInnerJamb |= FVector2D::Distance(FVector2D(P.X, FMath::Abs(P.Y)), FVector2D(FMath::Sqrt(100.0 - 2.25), 1.5)) < OnChord;
				bOuterJamb |= FVector2D::Distance(FVector2D(P.X, FMath::Abs(P.Y)), FVector2D(FMath::Sqrt(125.44 - 2.25), 1.5)) < OnChord;
			}
		}
		TestEqual(TEXT("nothing stands in the clear opening below the springing"), Inside, 0);
		TestTrue(TEXT("the jambs meet the inner face at x 9.887"), bInnerJamb);
		TestTrue(TEXT("and the outer face at x 11.099"), bOuterJamb);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormWallBetweenFacesTest, "Musee.Form.Drawing.WallBetweenFaces", PassageFlags)
	bool FFormWallBetweenFacesTest::RunTest(const FString& Parameters)
	{
		// Albion's north port plane (here y 0, faced from the north) tangent to the drum's outer cylinder R 11.20 (its axis at
		// y 11.20): the wedges run from nothing on the axis to 0.611 at |x| 3.65 (albion.md section 2), round the rough
		// opening, 4.10 wide and arched from 4.50, from z -1.20 to 7.50.
		const double Radius = 11.2;
		const double Half = 3.65;
		const double Reach = FMath::RadiansToDegrees(FMath::Asin((Half + 0.1) / Radius));
		FFormWall Wedge;
		Wedge.Plan = FFormProfile::Polyline({FVector2D(-Half, 0.0), FVector2D(Half, 0.0)}, false);
		Wedge.Back = FFormProfile::From(FVector2D(-(Half + 0.1), Radius - FMath::Sqrt(Radius * Radius - FMath::Square(Half + 0.1))))
			.ArcAbout(FVector2D(0.0, Radius), Radius, Radius, -90.0 - Reach, -90.0 + Reach);
		Wedge.BaseMeters = -1.2;
		Wedge.HeightMeters = 8.7;
		FFormWallOpening Rough;
		Rough.CenterMeters = Half;
		Rough.WidthMeters = 4.1;
		Rough.SillMeters = 1.15;
		Rough.HeadMeters = 5.7;
		Rough.bArched = true;
		Rough.bStraight = true;
		Wedge.Openings.Add(Rough);
		TArray<FFormMadePiece> Pieces;
		const FFormMadePiece* Piece = MakeWall(*this, Wedge, Pieces);
		if (!Piece)
		{
			return false;
		}
		TestClosed(*this, TEXT("the wedge masonry round the rough opening"), *Piece, 0);
		double Deepest = 0.0;
		double Thinnest = TNumericLimits<double>::Max();
		int32 Outside = 0;
		for (const FVector3d& V : Piece->Mesh->VerticesItr())
		{
			const FVector P = FVector(V) / Engine;
			const double Drum = Radius - FMath::Sqrt(Radius * Radius - P.X * P.X);
			Outside += (P.Y < -1.0e-9 || P.Y > Drum + OnChord) ? 1 : 0;
			Deepest = FMath::Max(Deepest, P.Y);
			Thinnest = FMath::Min(Thinnest, P.Y > 0.0 ? P.Y : Thinnest);
		}
		TestEqual(TEXT("every point lies between the port plane and the drum"), Outside, 0);
		TestEqual(TEXT("the wedge is 0.611 deep at |x| 3.65"), Deepest, Radius - FMath::Sqrt(Radius * Radius - Half * Half), OnChord);
		TestTrue(TEXT("and runs out to a feather on the axis"), Thinnest < 1.0e-3);
		return true;
	}
}

#endif
