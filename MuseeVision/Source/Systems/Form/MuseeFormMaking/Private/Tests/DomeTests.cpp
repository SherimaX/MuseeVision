#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "FormMake.h"
#include "Kernel/ShellEdges.h"

/**
 * The dome: a shell is one closed solid whose faces take the surfaces the dome names, standing on its springing at its
 * own frame; a dome of no thickness is one sheet facing the room beneath it; a shell thicker than its tightest curve
 * is a finding.
 */
namespace Musee::Form::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags DomeFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** Engine units in one metre (Foundation's Musee::ToEngine). */
		constexpr double EngineUnits = 100.0;

		/** A glass dome over the Nymphéas oval's opening: 9.55 m by 6.05 m, rising 1.2 m from the curb at 6.35 m. */
		FFormDome OvalDome(double Thickness)
		{
			FFormDome Dome;
			Dome.Frame.OriginMeters = FVector(0.0, 0.0, 6.35);
			Dome.SemiAxisXMeters = 9.55;
			Dome.SemiAxisYMeters = 6.05;
			Dome.RiseMeters = 1.2;
			Dome.ThicknessMeters = Thickness;
			Dome.OculusFraction = 0.4;
			Dome.Surface = TEXT("dome.outer-glass");
			Dome.InnerSurface = TEXT("dome.inner-glass");
			Dome.OculusSurface = TEXT("dome.eye");
			return Dome;
		}

		TArray<FFormMadePiece> MakeDome(const FFormDome& Dome, TArray<FMuseeFinding>& OutFindings)
		{
			TArray<FFormMadePiece> Pieces;
			Musee::FormMaking::Make(FFormDrawing::Of(TEXT("dome"), TEXT("dome"), Dome), Pieces, OutFindings);
			return Pieces;
		}

		TSet<FName> SurfacesOf(const FFormMadePiece& Piece)
		{
			TSet<FName> Out;
			const UE::Geometry::FDynamicMeshMaterialAttribute* Materials = Piece.Mesh->Attributes()->GetMaterialID();
			for (const int32 Triangle : Piece.Mesh->TriangleIndicesItr())
			{
				Out.Add(Piece.SurfaceOfSlot(Materials->GetValue(Triangle)));
			}
			return Out;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormDomeShellTest, "Musee.Form.Drawing.Dome", DomeFlags)
	bool FFormDomeShellTest::RunTest(const FString& Parameters)
	{
		TArray<FMuseeFinding> Findings;
		const TArray<FFormMadePiece> Pieces = MakeDome(OvalDome(0.05), Findings);
		if (!TestEqual(TEXT("a shell is one piece, made without findings"), Pieces.Num() * 10 + Findings.Num(), 10))
		{
			return false;
		}
		const FShellEdges Shell = MeasureShellEdges(*Pieces[0].Mesh);
		TestTrue(FString::Printf(TEXT("the shell is closed (%d open, %d non-manifold, %d misoriented edges)"), Shell.Boundary, Shell.NonManifold,
			Shell.Misoriented), Shell.IsClosed());
		TestEqual(TEXT("the shell is one solid without holes"), Shell.EulerCharacteristic(), 2);
		const TSet<FName> Surfaces = SurfacesOf(Pieces[0]);
		TestTrue(TEXT("its faces take the dome's three surfaces"), Surfaces.Num() == 3 && Surfaces.Contains(TEXT("dome.outer-glass"))
			&& Surfaces.Contains(TEXT("dome.inner-glass")) && Surfaces.Contains(TEXT("dome.eye")));
		const UE::Geometry::FAxisAlignedBox3d Bounds = Pieces[0].Mesh->GetBounds();
		TestEqual(TEXT("it springs at its frame"), Bounds.Min.Z, 6.35 * EngineUnits, 1.0e-6);
		TestEqual(TEXT("its crown stands its rise above"), Bounds.Max.Z, (6.35 + 1.2) * EngineUnits, 1.0e-6);
		TestEqual(TEXT("its plan reaches its semi-axis along x"), Bounds.Max.X, 9.55 * EngineUnits, 1.0e-6);
		TestEqual(TEXT("and along y"), Bounds.Max.Y, 6.05 * EngineUnits, 1.0e-6);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormDomeSheetTest, "Musee.Form.Drawing.DomeSheet", DomeFlags)
	bool FFormDomeSheetTest::RunTest(const FString& Parameters)
	{
		TArray<FMuseeFinding> Findings;
		FFormDome Velarium = OvalDome(0.0);
		Velarium.InnerSurface = NAME_None;
		const TArray<FFormMadePiece> Pieces = MakeDome(Velarium, Findings);
		if (!TestEqual(TEXT("a sheet is one piece"), Pieces.Num(), 1))
		{
			return false;
		}
		const UE::Geometry::FDynamicMesh3& Mesh = *Pieces[0].Mesh;
		TestFalse(TEXT("a sheet is open along its springing"), MeasureShellEdges(Mesh).IsClosed());
		const TSet<FName> Surfaces = SurfacesOf(Pieces[0]);
		TestTrue(TEXT("with no inner surface named, it takes the outer's, and its eye its own"), Surfaces.Num() == 2
			&& Surfaces.Contains(TEXT("dome.outer-glass")) && Surfaces.Contains(TEXT("dome.eye")));
		int32 FacingOut = 0;
		for (const int32 Triangle : Mesh.TriangleIndicesItr())
		{
			// Unreal's front face: cross(c - a, b - a).
			const UE::Geometry::FIndex3i T = Mesh.GetTriangle(Triangle);
			const FVector A = Mesh.GetVertex(T.A);
			const FVector B = Mesh.GetVertex(T.B);
			const FVector C = Mesh.GetVertex(T.C);
			const FVector Face = FVector::CrossProduct(C - A, B - A);
			const FVector Middle = (A + B + C) / 3.0 / EngineUnits - FVector(0.0, 0.0, 6.35);
			const FVector Outward(Middle.X / FMath::Square(9.55), Middle.Y / FMath::Square(6.05), Middle.Z / FMath::Square(1.2));
			FacingOut += FVector::DotProduct(Face, Outward) > 0.0 ? 1 : 0;
		}
		TestEqual(TEXT("every triangle faces the room beneath"), FacingOut, 0);

		TArray<FMuseeFinding> Folding;
		const TArray<FFormMadePiece> None = MakeDome(OvalDome(1.0), Folding);
		TestTrue(TEXT("a shell thicker than its tightest curve is a finding"), None.IsEmpty() && Folding.Num() == 1);
		return true;
	}
}

#endif
