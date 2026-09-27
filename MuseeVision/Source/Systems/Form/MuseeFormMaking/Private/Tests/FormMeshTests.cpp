#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "FormMesh.h"

/**
 * The mesh facade the making modules draw on: what it writes is the kernel's contract. Coincident vertices weld into
 * one point, the tangent space fills normal layers 0 to 2, UV0 is metres, and each triangle's MaterialID is its slot.
 */
namespace Musee::Form::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormMeshFacadeTest, "Musee.Form.Mesh", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	bool FFormMeshFacadeTest::RunTest(const FString& Parameters)
	{
		// Two square metres of floor side by side, each drawn with its own four vertices; the second in its own slot.
		FFormMesh Mesh;
		for (int32 Square = 0; Square < 2; ++Square)
		{
			if (Square == 1)
			{
				TestEqual(TEXT("a named slot follows the piece's own"), Mesh.UseSlot(TEXT("border")), 1);
			}
			const double X = Square;
			const int32 A = Mesh.AddVertex(FVector(X, 0.0, 0.0), FVector::UpVector, FVector2D(X, 0.0));
			const int32 B = Mesh.AddVertex(FVector(X + 1.0, 0.0, 0.0), FVector::UpVector, FVector2D(X + 1.0, 0.0));
			const int32 C = Mesh.AddVertex(FVector(X + 1.0, 1.0, 0.0), FVector::UpVector, FVector2D(X + 1.0, 1.0));
			const int32 D = Mesh.AddVertex(FVector(X, 1.0, 0.0), FVector::UpVector, FVector2D(X, 1.0));
			Mesh.AddQuad(A, B, C, D);
		}
		TestFalse(TEXT("a triangle with no area is dropped"), Mesh.AddTriangle(0, 1, 1));
		Mesh.Translate(FVector(0.0, 0.0, 2.0));
		TestEqual(TEXT("the mesh moves"), Mesh.Position(0).Z, 2.0);

		UE::Geometry::FDynamicMesh3 Out;
		Mesh.Write(Out);
		TestEqual(TEXT("the shared edge's vertices weld: six points"), Out.VertexCount(), 6);
		TestEqual(TEXT("four triangles"), Out.TriangleCount(), 4);
		TestEqual(TEXT("positions are engine units"), Out.GetVertex(0).Z, 200.0, 1.0e-9);
		const UE::Geometry::FDynamicMeshAttributeSet* Attributes = Out.Attributes();
		if (!TestTrue(TEXT("normals, tangents and bitangents, and UV0 with the piece layout"), Attributes && Attributes->NumNormalLayers() == 3
			&& Attributes->NumUVLayers() == 2))
		{
			return false;
		}
		TSet<int32> Slots;
		for (const int32 Triangle : Out.TriangleIndicesItr())
		{
			Slots.Add(Attributes->GetMaterialID()->GetValue(Triangle));
			const FVector3f Tangent = Attributes->PrimaryTangents()->GetElement(Attributes->PrimaryTangents()->GetTriangle(Triangle).A);
			TestTrue(TEXT("the tangent runs along increasing U"), Tangent.Equals(FVector3f(1.0f, 0.0f, 0.0f), 1.0e-5f));
		}
		TestEqual(TEXT("the triangles keep their slots"), Slots.Num(), 2);
		TestEqual(TEXT("the slots are named"), Mesh.Slots()[1], FName(TEXT("border")));
		return true;
	}
}

#endif
