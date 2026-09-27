#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/FormMakeTesting.h"

/**
 * Members: lofts (a tapered herm, a post with stopped chamfers, a turned taper whose arcs are divided alike) and square
 * lathes (an abacus, a half abacus against its wall, an octagonal die). Each is one closed solid holding the volume of
 * the solid the trade would cut.
 */
namespace Musee::Form::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		FFormLoftSection Section(FFormProfile Outline, double At, EFormJoint Joint = EFormJoint::Auto)
		{
			FFormLoftSection Out;
			Out.Outline = MoveTemp(Outline);
			Out.AtMeters = At;
			Out.Joint = Joint;
			return Out;
		}

		FFormProfile Square(double Half)
		{
			return FFormProfile::Rectangle(FVector2D(-Half, -Half), FVector2D(Half, Half));
		}

		/** A square post's section with its four arrises chamfered by C (zero: the square, each chamfer a point). */
		FFormProfile Chamfered(double Half, double C)
		{
			const FVector2D Points[8] = {{-Half + C, -Half}, {Half - C, -Half}, {Half, -Half + C}, {Half, Half - C}, {Half - C, Half},
				{-Half + C, Half}, {-Half, Half - C}, {-Half, -Half + C}};
			return FFormProfile::Polyline(Points, true);
		}

		FFormProfile Circle(double Radius)
		{
			return FFormProfile::Ellipse(FVector2D::ZeroVector, Radius, Radius, 0.0, false);
		}

		/** A section (radius, height) from the axis out and up and back to the axis: a block turned round its plan. */
		FFormProfile Block(double Radius, double Height)
		{
			return FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(Radius, 0.0), FVector2D(Radius, Height), FVector2D(0.0, Height)}, false);
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormLoftTest, "Musee.Form.Drawing.Loft", Flags)
	bool FFormLoftTest::RunTest(const FString& Parameters)
	{
		// The Classical Hall's herm shaft: 0.26 square at 0.13 widening to 0.32 at 1.17 (classical-hall.md, 3.8).
		FFormLoft Herm;
		Herm.Sections = {Section(Square(0.13), 0.13), Section(Square(0.16), 1.17)};
		const TArray<FFormMadePiece> Shaft = MakeDrawing(*this, FFormDrawing::Of(TEXT("herm.shaft"), TEXT("herm"), Herm));
		if (TestEqual(TEXT("the herm's shaft is one piece"), Shaft.Num(), 1))
		{
			TestClosed(*this, TEXT("the herm's shaft"), Shaft[0], 2);
			const double A = 0.26 * 0.26;
			const double B = 0.32 * 0.32;
			TestEqual(TEXT("it holds a frustum's volume"), VolumeCubicMeters(Shaft[0]), 1.04 / 3.0 * (A + B + FMath::Sqrt(A * B)), 1e-9);
		}

		// Albion's lectern post (albion.md, 3.6): 0.11 square with its arrises chamfered 10 mm between stops 50 mm long.
		constexpr double Half = 0.055;
		constexpr double C = 0.01;
		constexpr double Stop = 0.05;
		constexpr double Height = 0.83;
		FFormLoft Post;
		Post.Sections = {Section(Chamfered(Half, 0.0), 0.0), Section(Chamfered(Half, C), Stop, EFormJoint::Crisp),
			Section(Chamfered(Half, C), Height - Stop, EFormJoint::Crisp), Section(Chamfered(Half, 0.0), Height)};
		const TArray<FFormMadePiece> Stopped = MakeDrawing(*this, FFormDrawing::Of(TEXT("lectern.post"), TEXT("lectern"), Post));
		if (TestEqual(TEXT("the stop-chamfered post is one piece"), Stopped.Num(), 1))
		{
			TestClosed(*this, TEXT("the stop-chamfered post"), Stopped[0], 2);
			// Each chamfer takes a triangle of C squared over two along its run, and a pyramid of a third of that at each stop.
			const double Expected = 4.0 * Half * Half * Height - 2.0 * C * C * (Height - 2.0 * Stop) - 8.0 * (C * C / 6.0) * Stop;
			TestEqual(TEXT("it holds the square post less its chamfers and stops"), VolumeCubicMeters(Stopped[0]), Expected, 1e-9);
		}

		// A turned taper drawn as two circles of different radii: their arcs are divided alike.
		FFormLoft Taper;
		Taper.Sections = {Section(Circle(0.2), 0.0), Section(Circle(0.1), 0.5)};
		const TArray<FFormMadePiece> Cone = MakeDrawing(*this, FFormDrawing::Of(TEXT("taper"), TEXT("taper"), Taper));
		if (TestEqual(TEXT("the taper is one piece"), Cone.Num(), 1))
		{
			TestClosed(*this, TEXT("the taper"), Cone[0], 2);
			TestEqual(TEXT("it holds a cone frustum's volume, within its chords"), VolumeCubicMeters(Cone[0]),
				UE_DOUBLE_PI * 0.5 / 3.0 * (0.04 + 0.02 + 0.01), 0.01 * UE_DOUBLE_PI * 0.5 / 3.0 * 0.07);
		}

		// Sections not drawn alike can't be lofted.
		FFormLoft Unlike;
		Unlike.Sections = {Section(Square(0.1), 0.0), Section(Circle(0.1), 0.5)};
		TArray<FMuseeFinding> Findings;
		MakeDrawing(*this, FFormDrawing::Of(TEXT("unlike"), TEXT("unlike"), Unlike), &Findings);
		TestEqual(TEXT("unlike sections are a finding"), Findings.Num(), 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormSquareLatheTest, "Musee.Form.Drawing.SquareLathe", Flags)
	bool FFormSquareLatheTest::RunTest(const FString& Parameters)
	{
		// Albion's abacus is 0.90 square (half 0.45): a square lathe of a block 0.45 from the axis.
		FFormLathe Abacus;
		Abacus.Section = Block(0.45, 0.125);
		Abacus.Sides = 4;
		const TArray<FFormMadePiece> Whole = MakeDrawing(*this, FFormDrawing::Of(TEXT("abacus"), TEXT("capital"), Abacus));
		if (TestEqual(TEXT("the abacus is one piece"), Whole.Num(), 1))
		{
			TestClosed(*this, TEXT("the abacus"), Whole[0], 2);
			TestEqual(TEXT("it holds 0.90 square by 0.125"), VolumeCubicMeters(Whole[0]), 0.9 * 0.9 * 0.125, 1e-9);
			TestTrue(TEXT("its sides stand square to x and y, 0.45 from the axis"), Bounds(Whole[0]).Max.Equals(FVector(0.45, 0.45, 0.125), 1e-9));
		}

		// A half abacus against a wall: turned from east through south to west, its cut face closing it.
		FFormLathe Half = Abacus;
		Half.Section.Close();
		Half.FromDegrees = 0.0;
		Half.ToDegrees = 180.0;
		const TArray<FFormMadePiece> Court = MakeDrawing(*this, FFormDrawing::Of(TEXT("abacus.half"), TEXT("capital"), Half));
		if (TestEqual(TEXT("the half abacus is one piece"), Court.Num(), 1))
		{
			TestClosed(*this, TEXT("the half abacus"), Court[0], 2);
			TestEqual(TEXT("it holds half the square"), VolumeCubicMeters(Court[0]), 0.5 * 0.9 * 0.9 * 0.125, 1e-9);
			TestEqual(TEXT("it lies to the south of its wall"), Bounds(Court[0]).Min.Y, 0.0, 1e-9);
		}

		// An octagonal die: eight sides, each 0.3 from the axis.
		FFormLathe Die;
		Die.Section = Block(0.3, 0.5);
		Die.Sides = 8;
		const TArray<FFormMadePiece> Octagon = MakeDrawing(*this, FFormDrawing::Of(TEXT("die"), TEXT("die"), Die));
		if (TestEqual(TEXT("the octagonal die is one piece"), Octagon.Num(), 1))
		{
			TestClosed(*this, TEXT("the octagonal die"), Octagon[0], 2);
			TestEqual(TEXT("it holds an octagon's prism"), VolumeCubicMeters(Octagon[0]), 8.0 * 0.09 * FMath::Tan(UE_DOUBLE_PI / 8.0) * 0.5, 1e-9);
		}
		return true;
	}
}

#endif
