#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FormCatalogue.h"
#include "FormMake.h"
#include "FormOpening.h"
#include "Kernel/ShellEdges.h"

/**
 * Form's newer words, made through the public Make: a landform and a step dressed to a helicoid (heightfields), a scrim
 * sagging as a membrane does, bronze capitals cast from the catalogue's typeface, walls with segmental and pointed heads
 * and a top that follows an arc, and openings between two enclosures. Each is closed where it encloses and lies where
 * its geometry says.
 */
namespace Musee::Form::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags LandFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** Engine units in one metre (Foundation's Musee::ToEngine). */
		constexpr double LandEngine = 100.0;

		/** A point on a chord of a drawn curve lies within the chord tolerance of it, with rounding to spare. */
		constexpr double LandOnChord = 1.5e-3;

		TArray<FFormMadePiece> LandMake(FAutomationTestBase& Test, const FFormDrawing& Drawing, TArray<FMuseeFinding>* OutFindings = nullptr)
		{
			TArray<FFormMadePiece> Pieces;
			TArray<FMuseeFinding> Findings;
			Musee::FormMaking::Make(Drawing, Pieces, Findings);
			for (const FMuseeFinding& Finding : Findings)
			{
				if (!OutFindings)
				{
					Test.AddError(Finding.ToString());
				}
			}
			if (OutFindings)
			{
				*OutFindings = Findings;
			}
			return Pieces;
		}

		void LandClosed(FAutomationTestBase& Test, const TCHAR* What, const FFormMadePiece& Piece, int32 Euler)
		{
			const FShellEdges Shell = MeasureShellEdges(*Piece.Mesh);
			Test.TestTrue(FString::Printf(TEXT("%s is closed (%d open, %d non-manifold, %d misoriented edges of %d triangles)"), What, Shell.Boundary,
				Shell.NonManifold, Shell.Misoriented, Shell.Faces), Shell.IsClosed());
			Test.TestEqual(FString::Printf(TEXT("%s: Euler characteristic"), What), Shell.EulerCharacteristic(), Euler);
		}

		TArray<FVector> LandPoints(const FFormMadePiece& Piece)
		{
			TArray<FVector> Points;
			for (const FVector3d& V : Piece.Mesh->VerticesItr())
			{
				Points.Add(FVector(V) / LandEngine);
			}
			return Points;
		}

		/** A pond's bank: from the garden's ground at z 0.02 on the edge (r 3.0), falling 0.45 in to the bed at -0.85. */
		double BankTop(double X, double Y)
		{
			const double Fall = FMath::Clamp((3.0 - FVector2D(X, Y).Size()) / 0.45, 0.0, 1.0);
			return FMath::Lerp(0.02, -0.85, Fall);
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormHeightfieldTest, "Musee.Form.Drawing.Heightfield", LandFlags)
	bool FFormHeightfieldTest::RunTest(const FString& Parameters)
	{
		FFormHeightfield Bank;
		Bank.Outline = FFormProfile::Ellipse(FVector2D::ZeroVector, 3.0, 3.0, 0.0, true);
		Bank.TopMeters = [](double X, double Y) { return BankTop(X, Y); };
		Bank.BottomMeters = [](double, double) { return -1.0; };
		Bank.GridMeters = 0.2;
		const TArray<FFormMadePiece> Pieces = LandMake(*this, FFormDrawing::Of(TEXT("pond.bed"), TEXT("bed"), Bank));
		if (!TestEqual(TEXT("a landform is one piece"), Pieces.Num(), 1))
		{
			return false;
		}
		LandClosed(*this, TEXT("a pond's bank and bed"), Pieces[0], 2);
		int32 OffSurface = 0;
		for (const FVector& P : LandPoints(Pieces[0]))
		{
			OffSurface += (P.Z > -0.999 && FMath::Abs(P.Z - BankTop(P.X, P.Y)) > 1e-6) ? 1 : 0;
		}
		TestEqual(TEXT("every point of its top lies on the landform"), OffSurface, 0);

		// A spandrel step: level on top, its underside dressed to a helicoid falling 0.05 m a degree, over an annular sector.
		FFormHeightfield Step;
		Step.Outline = FFormProfile::From(FVector2D(6.48, 0.0)).ArcAbout(FVector2D::ZeroVector, 6.48, 6.48, 0.0, 5.0, EFormJoint::Crisp);
		Step.Outline.LineTo(FVector2D(2.35 * FMath::Cos(FMath::DegreesToRadians(5.0)), 2.35 * FMath::Sin(FMath::DegreesToRadians(5.0))), EFormJoint::Crisp)
			.ArcAbout(FVector2D::ZeroVector, 2.35, 2.35, 5.0, 0.0, EFormJoint::Crisp);
		Step.Outline.Close();
		Step.TopMeters = [](double, double) { return -0.30; };
		Step.BottomMeters = [](double X, double Y) { return -0.70 + 0.05 * FMath::RadiansToDegrees(FMath::Atan2(Y, X)); };
		Step.GridMeters = 0.1;
		const TArray<FFormMadePiece> Steps = LandMake(*this, FFormDrawing::Of(TEXT("stair.tread"), TEXT("tread"), Step));
		if (TestEqual(TEXT("a step is one piece"), Steps.Num(), 1))
		{
			LandClosed(*this, TEXT("a step dressed to a helicoid"), Steps[0], 2);
		}

		// A top that falls through its underside can't be made.
		FFormHeightfield Through = Step;
		Through.TopMeters = [](double, double) { return -0.60; };
		TArray<FMuseeFinding> Findings;
		TestTrue(TEXT("a top under its underside is a finding"), LandMake(*this, FFormDrawing::Of(TEXT("stair.tread"), TEXT("tread"), Through), &Findings).IsEmpty()
			&& Findings.Num() == 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormClothTest, "Musee.Form.Drawing.Cloth", LandFlags)
	bool FFormClothTest::RunTest(const FString& Parameters)
	{
		// The Salon's scrim: linen 2.3 m square, 0.5 mm thick, hanging 30 mm at its middle (salon.md section 3.4).
		FFormCloth Scrim;
		Scrim.SizeMeters = FVector2D(2.3, 2.3);
		Scrim.SagMeters = 0.03;
		Scrim.ThicknessMeters = 0.0005;
		Scrim.GridMeters = 0.05;
		const TArray<FFormMadePiece> Pieces = LandMake(*this, FFormDrawing::Of(TEXT("lantern.scrim"), TEXT("scrim"), Scrim));
		if (!TestEqual(TEXT("a cloth is one piece"), Pieces.Num(), 1))
		{
			return false;
		}
		LandClosed(*this, TEXT("a scrim"), Pieces[0], 2);
		double Lowest = 0.0;
		double Highest = -1.0;
		double QuarterDepth = 1.0;
		for (const FVector& P : LandPoints(Pieces[0]))
		{
			Lowest = FMath::Min(Lowest, P.Z);
			Highest = FMath::Max(Highest, P.Z);
			if (FMath::IsNearlyEqual(P.X, 0.6, 1e-6) && FMath::IsNearlyEqual(P.Y, 1.15, 1e-6))
			{
				QuarterDepth = FMath::Min(QuarterDepth, -P.Z);
			}
		}
		TestEqual(TEXT("its underside hangs 30.5 mm at the middle"), Lowest, -0.0305, 1e-6);
		TestEqual(TEXT("it is held level on its frame"), Highest, 0.0, 1e-9);
		// A membrane loaded all over is fuller than a parabola across its middle: 0.6 m in from the frame its top has fallen
		// 23.94 mm, where a parabola of the same depth would have fallen 23.14 mm.
		TestEqual(TEXT("0.6 m in, it hangs as a membrane does"), QuarterDepth, 0.023936, 1e-5);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormLetteringTest, "Musee.Form.Drawing.Lettering", LandFlags)
	bool FFormLetteringTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FFormTypeface, FString> Face = FindTypeface(FMuseeKey(TEXT("centaur.capitals")));
		if (!TestTrue(TEXT("the catalogue holds Centaur's capitals"), Face.HasValue()))
		{
			AddError(Face.GetError());
			return false;
		}
		TestNotNull(TEXT("with the motto's letters"), Face.GetValue().Find(TEXT('N')));
		// The Rotunda's motto's first words: capitals 0.26 on the circle r 8.53, tops to the dial, 8 mm proud, 2.5 mm bevel, the
		// words parted by a gilt triangular stop in a gap of 1.15 caps (rotunda.md section 3.1).
		FFormLettering Motto;
		Motto.Text = TEXT("HORAS");
		Motto.Text.AppendChar(FFormLettering::Interpunct);
		Motto.Text += TEXT("NON");
		Motto.Typeface = Face.GetValue();
		Motto.CapMeters = 0.26;
		Motto.TrackingCaps = 0.16;
		Motto.WordSpaceCaps = 0.6;
		Motto.StopGapCaps = 1.15;
		Motto.StopHalfWidthCaps = 0.12;
		Motto.StopTopCaps = 0.60;
		Motto.StopPointCaps = 0.36;
		Motto.StopApexMeters = 0.008;
		Motto.BaselineRadiusMeters = 8.53;
		Motto.CentreDegrees = 90.0;
		Motto.bTopsInward = true;
		Motto.ProudMeters = 0.008;
		Motto.BevelMeters = 0.0025;
		FFormDrawing Drawing = FFormDrawing::Of(TEXT("floor.motto"), TEXT("motto"), Motto);
		Drawing.PartSurfaces.Add(FFormLettering::StopPart, TEXT("motto.stop"));
		const TArray<FFormMadePiece> Pieces = LandMake(*this, Drawing);
		if (!TestEqual(TEXT("a line of letters is one piece"), Pieces.Num(), 1))
		{
			return false;
		}
		// H, S and N twice are solid; O twice, R and A each have a counter; the stop is a pyramid: nine closed shells, four rings.
		LandClosed(*this, TEXT("HORAS, a stop and NON cast in bronze"), Pieces[0], 9 * 2 - 4 * 2);
		TestEqual(TEXT("the stop takes its own surface"), Pieces[0].SlotSurfaces, TArray<FName>{TEXT("motto.stop")});
		double Top = 0.0;
		double Inner = TNumericLimits<double>::Max();
		double Outer = 0.0;
		for (const FVector& P : LandPoints(Pieces[0]))
		{
			Top = FMath::Max(Top, P.Z);
			const double Radius = FVector2D(P.X, P.Y).Size();
			Inner = FMath::Min(Inner, Radius);
			Outer = FMath::Max(Outer, Radius);
		}
		TestEqual(TEXT("the letters stand 8 mm proud"), Top, 0.008, 1e-9);
		TestTrue(FString::Printf(TEXT("their tops reach in towards the cap line r 8.27 (%.3f)"), Inner), Inner > 8.25 && Inner < 8.29);
		TestTrue(FString::Printf(TEXT("and their feet sit on the baseline r 8.53 (%.3f)"), Outer), Outer > 8.52 && Outer < 8.56);

		FFormLettering Missing = Motto;
		Missing.Text = TEXT("HORAZ");
		TArray<FMuseeFinding> Findings;
		TestTrue(TEXT("a letter the face lacks is a finding"), LandMake(*this, FFormDrawing::Of(TEXT("floor.motto"), TEXT("motto"), Missing), &Findings).IsEmpty()
			&& Findings.Num() == 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormArchedHeadsTest, "Musee.Form.Drawing.ArchedHeads", LandFlags)
	bool FFormArchedHeadsTest::RunTest(const FString& Parameters)
	{
		// A segmental window 3.0 wide of radius 3.0 (rise 0.402) and a pointed one 2.0 wide of radius 2.0 (an equilateral
		// arch, rise 1.732), both springing at 3.0 over a sill at 1.0, in a straight wall 0.6 thick.
		FFormWallOpening Segmental;
		Segmental.CenterMeters = 3.0;
		Segmental.WidthMeters = 3.0;
		Segmental.SillMeters = 1.0;
		Segmental.HeadMeters = 3.0;
		Segmental.bArched = true;
		Segmental.ArchRadiusMeters = 3.0;
		FFormWallOpening Pointed = Segmental;
		Pointed.CenterMeters = 7.5;
		Pointed.WidthMeters = 2.0;
		Pointed.ArchRadiusMeters = 2.0;
		Pointed.bPointed = true;
		TestEqual(TEXT("the segmental arch rises 3 - sqrt(6.75)"), Segmental.RiseMeters(), 3.0 - FMath::Sqrt(6.75), 1e-12);
		TestEqual(TEXT("the equilateral arch rises sqrt(3)"), Pointed.RiseMeters(), FMath::Sqrt(3.0), 1e-12);
		FFormWall Wall;
		Wall.Plan = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(10.0, 0.0)}, false);
		Wall.ThicknessMeters = 0.6;
		Wall.HeightMeters = 6.0;
		Wall.Openings = {Segmental, Pointed};
		const TArray<FFormMadePiece> Pieces = LandMake(*this, FFormDrawing::Of(TEXT("wall"), TEXT("wall"), Wall));
		if (!TestEqual(TEXT("a wall is one piece"), Pieces.Num(), 1))
		{
			return false;
		}
		LandClosed(*this, TEXT("a wall pierced by a segmental and a pointed window"), Pieces[0], 2 - 2 * 2);
		int32 InOpenings = 0;
		bool bSegmentalCrown = false;
		bool bPointedCrown = false;
		for (const FVector& P : LandPoints(Pieces[0]))
		{
			for (const FFormWallOpening* O : {&Segmental, &Pointed})
			{
				const double Across = P.X - O->CenterMeters;
				const bool bWithin = FMath::Abs(Across) < O->HalfWidthMeters() - LandOnChord && P.Z > O->SillMeters + 1e-6;
				InOpenings += (bWithin && P.Z < O->HeadAt(Across) - LandOnChord) ? 1 : 0;
			}
			bSegmentalCrown |= FVector2D::Distance(FVector2D(P.X, P.Z), FVector2D(3.0, Segmental.CrownMeters())) < 1e-6;
			bPointedCrown |= FVector2D::Distance(FVector2D(P.X, P.Z), FVector2D(7.5, Pointed.CrownMeters())) < 1e-6;
		}
		TestEqual(TEXT("nothing stands within either opening"), InOpenings, 0);
		TestTrue(TEXT("the segmental head has its crown"), bSegmentalCrown);
		TestTrue(TEXT("the pointed head comes to its point"), bPointedCrown);

		// The upper course of the Atrium's portal (3.5 over its bed at 6.0), which a segmental arch R 5.8 about z 2.2, springing
		// from jambs ±4.95 at 5.223, passes straight through: it opens on the arch's chord at its bed, ±4.382, up to the crown.
		FFormWallOpening Through;
		Through.CenterMeters = 6.05;
		Through.WidthMeters = 9.9;
		Through.HeadMeters = 2.2 + FMath::Sqrt(5.8 * 5.8 - 4.95 * 4.95) - 6.0;
		Through.bArched = true;
		Through.ArchRadiusMeters = 5.8;
		Through.bStraight = true;
		TestEqual(TEXT("an arch springing under the bed opens on its chord there"), Through.SillHalfWidthMeters(), FMath::Sqrt(5.8 * 5.8 - 3.8 * 3.8), 1e-9);
		FFormWall Course;
		Course.Plan = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(12.1, 0.0)}, false);
		Course.ThicknessMeters = 0.75;
		Course.HeightMeters = 3.5;
		Course.Openings = {Through};
		const TArray<FFormMadePiece> Courses = LandMake(*this, FFormDrawing::Of(TEXT("portal.upper"), TEXT("dress"), Course));
		if (TestEqual(TEXT("the course is one piece"), Courses.Num(), 1))
		{
			LandClosed(*this, TEXT("a course the arch passes through from its bed"), Courses[0], 2);
			int32 UnderArch = 0;
			for (const FVector& P : LandPoints(Courses[0]))
			{
				const double Across = P.X - Through.CenterMeters;
				UnderArch += (FMath::Abs(Across) < Through.SillHalfWidthMeters() - LandOnChord && P.Z < Through.HeadAt(Across) - LandOnChord) ? 1 : 0;
			}
			TestEqual(TEXT("nothing of the course stands under the arch"), UnderArch, 0);
		}

		// A door under a lunette in the same stretch of wall: the lunette a semicircle R 1.5 springing at the door's head, 2.0,
		// opening from a sill at 3.0 on its chord there. The door notches the wall; the lunette pierces it.
		FFormWallOpening Door;
		Door.CenterMeters = 5.0;
		Door.WidthMeters = 2.0;
		Door.HeadMeters = 2.0;
		FFormWallOpening Lunette;
		Lunette.CenterMeters = 5.0;
		Lunette.WidthMeters = 3.0;
		Lunette.SillMeters = 3.0;
		Lunette.HeadMeters = 2.0;
		Lunette.bArched = true;
		FFormWall Stacked;
		Stacked.Plan = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(10.0, 0.0)}, false);
		Stacked.ThicknessMeters = 0.3;
		Stacked.HeightMeters = 4.0;
		Stacked.Openings = {Door, Lunette};
		const TArray<FFormMadePiece> StackedPieces = LandMake(*this, FFormDrawing::Of(TEXT("lining"), TEXT("lining"), Stacked));
		if (TestEqual(TEXT("a wall with a lunette over its door is one piece"), StackedPieces.Num(), 1))
		{
			LandClosed(*this, TEXT("a wall with a lunette over its door"), StackedPieces[0], 0);
		}

		// A lining whose top follows a vault's arc: R 5.42 about (5.0, 2.20), from its springings at 4.1 to its crown.
		const double Radius = 5.42;
		const double Half = 5.0;
		const double Springing = FMath::RadiansToDegrees(FMath::Acos(Half / Radius));
		FFormWall Lining;
		Lining.Plan = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(10.0, 0.0)}, false);
		Lining.ThicknessMeters = 0.05;
		Lining.Top = FFormProfile::From(FVector2D(0.0, 2.2 + Radius * FMath::Sin(FMath::DegreesToRadians(Springing))))
			.ArcAbout(FVector2D(5.0, 2.2), Radius, Radius, 180.0 - Springing, Springing);
		const TArray<FFormMadePiece> Linings = LandMake(*this, FFormDrawing::Of(TEXT("lining"), TEXT("lining"), Lining));
		if (TestEqual(TEXT("a lining is one piece"), Linings.Num(), 1))
		{
			LandClosed(*this, TEXT("a lining under a vault"), Linings[0], 2);
			int32 AboveArc = 0;
			double Crown = 0.0;
			for (const FVector& P : LandPoints(Linings[0]))
			{
				const double Arc = 2.2 + FMath::Sqrt(Radius * Radius - FMath::Square(P.X - 5.0));
				AboveArc += P.Z > Arc + 1e-6 ? 1 : 0;
				Crown = FMath::Max(Crown, P.Z);
			}
			TestEqual(TEXT("its top never rises past the vault's arc"), AboveArc, 0);
			TestEqual(TEXT("and meets it at the crown"), Crown, 2.2 + Radius, LandOnChord);
		}
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormOpeningBetweenTest, "Musee.Form.Openings", LandFlags)
	bool FFormOpeningBetweenTest::RunTest(const FString& Parameters)
	{
		FFormOpening Door;
		Door.Kind = EFormOpeningKind::Passage;
		Door.Enclosure = FMuseeKey(TEXT("rotunda.hall"));
		TestFalse(TEXT("an opening names no enclosure beyond until it is given one"), Door.IsBetweenEnclosures());
		Door.Beyond = FMuseeKey(TEXT("rotunda.stair"));
		TestTrue(TEXT("a passage joins its enclosure to the one beyond"), Door.IsBetweenEnclosures());
		TestFalse(TEXT("an opening lies in its plane unless it is bent"), Door.IsCurved());
		FFormOpening Drum;
		Drum.RadiusMeters = 14.0;
		TestTrue(TEXT("a drum's glazing is bent round its drum"), Drum.IsCurved());
		return true;
	}
}

#endif
