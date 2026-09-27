#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "FormMake.h"
#include "Kernel/ShellEdges.h"
#include "Misc/AutomationTest.h"

/**
 * What the tests of Form's words ask of a made piece: that it is made without a finding, that it is one closed solid of
 * the right shape (its Euler characteristic: 2 for a block, 0 for a ring), the volume it holds, and the surfaces its
 * triangles use.
 */
namespace Musee::Form::Tests
{
	/** Makes one drawing; its findings are the test's errors, unless the test takes them. */
	inline TArray<FFormMadePiece> MakeDrawing(FAutomationTestBase& Test, const FFormDrawing& Drawing, TArray<FMuseeFinding>* OutFindings = nullptr)
	{
		TArray<FFormMadePiece> Pieces;
		TArray<FMuseeFinding> Findings;
		Musee::FormMaking::Make(Drawing, Pieces, Findings);
		if (OutFindings)
		{
			*OutFindings = Findings;
			return Pieces;
		}
		for (const FMuseeFinding& Finding : Findings)
		{
			Test.AddError(Finding.ToString());
		}
		return Pieces;
	}

	/** A piece whose skin is closed and consistently turned, with the given Euler characteristic over all its shells. */
	inline void TestClosed(FAutomationTestBase& Test, const FString& What, const FFormMadePiece& Piece, int32 Euler)
	{
		const FShellEdges Shell = MeasureShellEdges(*Piece.Mesh);
		Test.TestTrue(FString::Printf(TEXT("%s is closed (%d open, %d non-manifold, %d misoriented edges of %d triangles)"), *What,
			Shell.Boundary, Shell.NonManifold, Shell.Misoriented, Shell.Faces), Shell.IsClosed());
		Test.TestEqual(FString::Printf(TEXT("%s: Euler characteristic"), *What), Shell.EulerCharacteristic(), Euler);
	}

	/** The volume a closed piece holds, cubic metres (its mesh is in engine centimetres). */
	inline double VolumeCubicMeters(const FFormMadePiece& Piece)
	{
		double Six = 0.0;
		for (const int32 Triangle : Piece.Mesh->TriangleIndicesItr())
		{
			const UE::Geometry::FIndex3i T = Piece.Mesh->GetTriangle(Triangle);
			Six += FVector::DotProduct(Piece.Mesh->GetVertex(T.A), FVector::CrossProduct(Piece.Mesh->GetVertex(T.B), Piece.Mesh->GetVertex(T.C)));
		}
		return FMath::Abs(Six) / 6.0 * 1.0e-6;
	}

	/** The engine-unit bounds of a piece, in metres. */
	inline FBox Bounds(const FFormMadePiece& Piece)
	{
		const UE::Geometry::FAxisAlignedBox3d Box = Piece.Mesh->GetBounds();
		return FBox(FVector(Box.Min) * 0.01, FVector(Box.Max) * 0.01);
	}

	/** The area of a piece's triangles in one material slot facing within a degree of a direction, square metres. */
	inline double FacingArea(const FFormMadePiece& Piece, int32 Slot, const FVector& Direction)
	{
		double Area = 0.0;
		const UE::Geometry::FDynamicMeshMaterialAttribute* Materials = Piece.Mesh->Attributes()->GetMaterialID();
		for (const int32 Triangle : Piece.Mesh->TriangleIndicesItr())
		{
			const UE::Geometry::FIndex3i T = Piece.Mesh->GetTriangle(Triangle);
			const FVector A = Piece.Mesh->GetVertex(T.A);
			// The engine's front face: cross(c - a, b - a).
			const FVector Normal = FVector::CrossProduct(Piece.Mesh->GetVertex(T.C) - A, Piece.Mesh->GetVertex(T.B) - A);
			if (Materials->GetValue(Triangle) == Slot && FVector::DotProduct(Normal.GetSafeNormal(), Direction) > FMath::Cos(FMath::DegreesToRadians(1.0)))
			{
				Area += 0.5 * Normal.Size() * 1.0e-4;
			}
		}
		return Area;
	}
}
