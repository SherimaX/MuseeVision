#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Drawing/Draw.h"
#include "FormMake.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/ShellEdges.h"

/**
 * Curved form: sweeps wound up a helix (a stair's string, plumb, and a handrail over a flight and its landing, square to
 * the rake), coffers set into a dome in its development, and a pane cut on great circles and bent onto a sphere. Each is
 * one closed solid and lies where the geometry says, within the chord tolerance.
 */
namespace Musee::Form::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags CurvedFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** Engine units in one metre (Foundation's Musee::ToEngine). */
		constexpr double CurvedEngine = 100.0;

		/** A point carried onto a curve lies within the chord tolerance (1 mm) of it; half as much again for rounding. */
		constexpr double CurvedOnCurve = 1.5e-3;

		const FFormMadePiece* CurvedMake(FAutomationTestBase& Test, const FFormDrawing& Drawing, TArray<FFormMadePiece>& Pieces)
		{
			TArray<FMuseeFinding> Findings;
			Musee::FormMaking::Make(Drawing, Pieces, Findings);
			for (const FMuseeFinding& Finding : Findings)
			{
				Test.AddError(Finding.ToString());
			}
			return Pieces.Num() == 1 ? &Pieces[0] : nullptr;
		}

		void CurvedClosed(FAutomationTestBase& Test, const TCHAR* What, const FFormMadePiece& Piece, int32 Euler)
		{
			const FShellEdges Shell = MeasureShellEdges(*Piece.Mesh);
			Test.TestTrue(FString::Printf(TEXT("%s is a closed solid (%d open, %d non-manifold, %d misoriented)"), What, Shell.Boundary,
				Shell.NonManifold, Shell.Misoriented), Shell.IsClosed());
			Test.TestEqual(FString::Printf(TEXT("%s: Euler characteristic"), What), Shell.EulerCharacteristic(), Euler);
		}

		/** Every vertex of a made piece, in metres in its frame. */
		TArray<FVector> CurvedPoints(const FFormMadePiece& Piece)
		{
			TArray<FVector> Points;
			for (const FVector3d& V : Piece.Mesh->VerticesItr())
			{
				Points.Add(FVector(V) / CurvedEngine);
			}
			return Points;
		}

		/** The Rotunda's string: r 2.20 in, 0.15 wide, a quarter turn rising 1.0 m, its board 0.52 deep under its top. */
		FFormSweep QuarterTurnString()
		{
			const double Radius = 2.2;
			FFormSweep String;
			String.Section = FFormProfile::Rectangle(FVector2D(0.0, -0.52), FVector2D(0.15, 0.0));
			String.Path = FFormProfile::From(FVector2D(Radius, 0.0)).ArcAbout(FVector2D::ZeroVector, Radius, Radius, 0.0, 90.0);
			String.Rise = FFormProfile::From(FVector2D(0.0, 0.0)).LineTo(FVector2D(0.5 * UE_DOUBLE_PI * Radius, 1.0));
			String.bPlumb = true;
			return String;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormHelicalSweepTest, "Musee.Form.Drawing.HelicalSweep", CurvedFlags)
	bool FFormHelicalSweepTest::RunTest(const FString& Parameters)
	{
		// A plumb string wound a quarter turn: its board stands between r 2.05 and 2.20 at every height.
		TArray<FFormMadePiece> Pieces;
		const FFormMadePiece* String = CurvedMake(*this, FFormDrawing::Of(TEXT("stair.string"), TEXT("string"), QuarterTurnString()), Pieces);
		if (!String)
		{
			return false;
		}
		CurvedClosed(*this, TEXT("a plumb string up a helix"), *String, 2);
		double Nearest = TNumericLimits<double>::Max();
		double Furthest = 0.0;
		double Highest = -TNumericLimits<double>::Max();
		double Lowest = TNumericLimits<double>::Max();
		for (const FVector& P : CurvedPoints(*String))
		{
			const double Radius = FVector2D(P.X, P.Y).Size();
			Nearest = FMath::Min(Nearest, Radius);
			Furthest = FMath::Max(Furthest, Radius);
			Highest = FMath::Max(Highest, P.Z);
			Lowest = FMath::Min(Lowest, P.Z);
		}
		TestEqual(TEXT("its inner face stands at r 2.05, plumb"), Nearest, 2.05, CurvedOnCurve);
		TestEqual(TEXT("its outer face at r 2.20"), Furthest, 2.2, CurvedOnCurve);
		// The rise is read along the path's chords, which fall short of the arc by the tolerance over three radii (0.015 % here).
		TestEqual(TEXT("its top climbs 1.0 m over the quarter turn"), Highest, 1.0, 1e-3);
		TestEqual(TEXT("its board hangs 0.52 under its start"), Lowest, -0.52, 1e-6);

		// A plumb board's level face is square to the helix it lies on, at every radius across it (FSweepFrame's rake).
		FMeshBuilder Board;
		FString Why;
		if (!TestTrue(TEXT("the string draws"), Musee::Form::Drawing::DrawSweep(QuarterTurnString(), Board, Why)))
		{
			return false;
		}
		const double Climb = 1.0 / (0.5 * UE_DOUBLE_PI);
		double Worst = 0.0;
		for (int32 Vertex = 0; Vertex < Board.NumVertices(); ++Vertex)
		{
			const FVector& N = Board.Normal(Vertex);
			if (FMath::Abs(N.Z) < 0.5)
			{
				continue;
			}
			const FVector& P = Board.Position(Vertex);
			const double Angle = FMath::Atan2(P.Y, P.X);
			const double Radius = FVector2D(P.X, P.Y).Size();
			const FVector Helix = FVector(-FMath::Sin(Angle) * Radius, FMath::Cos(Angle) * Radius, Climb).GetSafeNormal();
			Worst = FMath::Max(Worst, FMath::Abs(FVector::DotProduct(N, Helix)));
		}
		TestTrue(FString::Printf(TEXT("the top and bottom faces' normals are square to the helix at their radius (worst %.2e)"), Worst), Worst < 2e-3);

		// A handrail square to its rake, climbing 1.0 over 2.0 m and then level over a landing: one closed rail, mitred at the
		// landing's edge.
		FFormSweep Rail;
		Rail.Section = FFormProfile::Rectangle(FVector2D(-0.03, -0.0225), FVector2D(0.03, 0.0225));
		Rail.Path = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(4.0, 0.0)}, false);
		Rail.Rise = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(2.0, 1.0), FVector2D(4.0, 1.0)}, false);
		TArray<FFormMadePiece> RailPieces;
		const FFormMadePiece* Handrail = CurvedMake(*this, FFormDrawing::Of(TEXT("stair.handrail"), TEXT("handrail"), Rail), RailPieces);
		if (!Handrail)
		{
			return false;
		}
		CurvedClosed(*this, TEXT("a raked handrail running on level over a landing"), *Handrail, 2);
		double RailTop = -TNumericLimits<double>::Max();
		for (const FVector& P : CurvedPoints(*Handrail))
		{
			RailTop = FMath::Max(RailTop, P.Z);
		}
		TestEqual(TEXT("over the landing its top lies level at 1.0225"), RailTop, 1.0225, 1e-6);

		// A rise can't run back on itself.
		FFormSweep Back = Rail;
		Back.Rise = FFormProfile::Polyline({FVector2D(0.0, 0.0), FVector2D(2.0, 1.0), FVector2D(1.0, 2.0)}, false);
		TArray<FFormMadePiece> None;
		TArray<FMuseeFinding> Findings;
		Musee::FormMaking::Make(FFormDrawing::Of(TEXT("stair.handrail"), TEXT("handrail"), Back), None, Findings);
		TestTrue(TEXT("a rise that runs back is a finding"), None.IsEmpty() && Findings.Num() == 1);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormCofferOnDomeTest, "Musee.Form.Drawing.CofferOnDome", CurvedFlags)
	bool FFormCofferOnDomeTest::RunTest(const FString& Parameters)
	{
		// A dome's inner face, the sphere R 10 about (0, 0, 10), its meridian drawn down from 50 degrees to 10 so it faces
		// the room; U true on the parallel at 30 degrees. A coffer's frame sunk 0.20 into it round a cell 2.0 by 2.0.
		const double Radius = 10.0;
		const FVector Centre(0.0, 0.0, 10.0);
		const double Reference = Radius * FMath::Cos(FMath::DegreesToRadians(30.0));
		FFormProfile Meridian = FFormProfile::From(FVector2D(Radius * FMath::Cos(FMath::DegreesToRadians(50.0)), 10.0 + Radius * FMath::Sin(FMath::DegreesToRadians(50.0))));
		Meridian.ArcAbout(FVector2D(0.0, 10.0), Radius, Radius, 50.0, 10.0);
		FFormSweep Coffer;
		Coffer.Section = FFormProfile::Rectangle(FVector2D(0.0, -0.20), FVector2D(0.30, 0.0));
		Coffer.Path = FFormProfile::Rectangle(FVector2D(0.0, 1.0), FVector2D(2.0, 3.0));
		FFormDrawing Drawing = FFormDrawing::Of(TEXT("dome.coffer"), TEXT("coffer"), Coffer);
		Drawing.Ground = FFormGround::Revolved(Meridian, Reference);
		TArray<FFormMadePiece> Pieces;
		const FFormMadePiece* Piece = CurvedMake(*this, Drawing, Pieces);
		if (!Piece)
		{
			return false;
		}
		CurvedClosed(*this, TEXT("a coffer's frame set into a dome"), *Piece, 0);
		const double Turn = 2.0 / Reference;
		int32 Astray = 0;
		int32 OnFirstMeridian = 0;
		double Deepest = 0.0;
		for (const FVector& P : CurvedPoints(*Piece))
		{
			const double Distance = FVector::Distance(P, Centre);
			const double Angle = FMath::Atan2(P.Y, P.X);
			Astray += (Distance < Radius - CurvedOnCurve || Distance > Radius + 0.20 + CurvedOnCurve || Angle < -1e-9 || Angle > Turn + 1e-9) ? 1 : 0;
			OnFirstMeridian += FMath::Abs(Angle) < 1e-9 ? 1 : 0;
			Deepest = FMath::Max(Deepest, Distance - Radius);
		}
		TestEqual(TEXT("every point lies between the rib face and the coffer's back, within the cell's meridians"), Astray, 0);
		TestTrue(TEXT("its outer side lies in the cell's meridian plane"), OnFirstMeridian > 2);
		TestEqual(TEXT("it sinks 0.20 into the dome"), Deepest, 0.20, CurvedOnCurve);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormSphericalPaneTest, "Musee.Form.Drawing.SphericalPane", CurvedFlags)
	bool FFormSphericalPaneTest::RunTest(const FString& Parameters)
	{
		// The Atrium's canopy: the sphere R 14.000 about (0, 0, 22), seen from below; a triangular pane 24 mm thick outside it,
		// drawn on the plane tangent at its south pole.
		const double Radius = 14.0;
		const FVector Centre(0.0, 0.0, 22.0);
		const FMuseeFrame Tangent(FVector(0.0, 0.0, 8.0), FQuat(FVector::ForwardVector, UE_DOUBLE_PI));
		const FVector2D Corners[3] = {FVector2D(0.0, 0.0), FVector2D(1.4, 0.0), FVector2D(0.7, 1.2)};
		FFormExtrusion Pane;
		Pane.Outline = FFormProfile::Polyline(Corners, true);
		Pane.DepthMeters = 0.024;
		FFormDrawing Drawing = FFormDrawing::Of(TEXT("canopy.pane"), TEXT("glass"), Pane);
		Drawing.Ground = FFormGround::OnSphere(Tangent, Radius);
		TArray<FFormMadePiece> Pieces;
		const FFormMadePiece* Piece = CurvedMake(*this, Drawing, Pieces);
		if (!Piece)
		{
			return false;
		}
		CurvedClosed(*this, TEXT("a pane bent onto a sphere"), *Piece, 2);
		// Each edge lies in the plane through the centre and its two corners' rays: the glazier's cut, square to the glass.
		FVector Rays[3];
		for (int32 Corner = 0; Corner < 3; ++Corner)
		{
			Rays[Corner] = (Tangent.PointToParent(FVector(Corners[Corner].X, Corners[Corner].Y, 0.0)) - Centre).GetSafeNormal();
		}
		const FVector Inside = (Rays[0] + Rays[1] + Rays[2]).GetSafeNormal();
		int32 Astray = 0;
		int32 OnCut = 0;
		for (const FVector& P : CurvedPoints(*Piece))
		{
			const double Distance = FVector::Distance(P, Centre);
			Astray += (Distance < Radius - 1e-6 || Distance > Radius + 0.024 + 1e-6) ? 1 : 0;
			for (int32 Edge = 0; Edge < 3; ++Edge)
			{
				FVector Cut = FVector::CrossProduct(Rays[Edge], Rays[(Edge + 1) % 3]).GetSafeNormal();
				Cut = FVector::DotProduct(Cut, Inside) >= 0.0 ? Cut : -Cut;
				const double Side = FVector::DotProduct(P - Centre, Cut);
				Astray += Side < -1e-6 ? 1 : 0;
				OnCut += FMath::Abs(Side) < 1e-6 ? 1 : 0;
			}
		}
		TestEqual(TEXT("every point lies within the glass's two spheres and its three cuts"), Astray, 0);
		TestTrue(TEXT("its edges lie on the cuts"), OnCut >= 6);
		return true;
	}
}

#endif
