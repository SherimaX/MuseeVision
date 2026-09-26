#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Kernel/MeshBuilder.h"
#include "Kernel/Welding.h"
#include "Tests/FormTestShapes.h"

namespace Musee::Form::Tests
{
	namespace
	{
		/** A 1 m square in the plan at height Z, facing Facing, with plan UVs; returns its first vertex. */
		int32 AddSquare(FMeshBuilder& Mesh, double X, double Z, const FVector& Facing)
		{
			const int32 First = Mesh.AddVertex(FVector(X, 0, Z), Facing, FVector2D(X, 0));
			Mesh.AddVertex(FVector(X + 1, 0, Z), Facing, FVector2D(X + 1, 0));
			Mesh.AddVertex(FVector(X + 1, 1, Z), Facing, FVector2D(X + 1, 1));
			Mesh.AddVertex(FVector(X, 1, Z), Facing, FVector2D(X, 1));
			Mesh.AddQuad(First, First + 1, First + 2, First + 3);
			return First;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormWindingAndDegeneratesTest, "Musee.Form.WindingAndDegenerates",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormWindingAndDegeneratesTest::RunTest(const FString& Parameters)
	{
		// Given either way round, a triangle is wound to face along its normals.
		FMeshBuilder Mesh;
		const FVector Up = FVector::UpVector;
		Mesh.AddVertex(FVector(0, 0, 0), Up, FVector2D(0, 0));
		Mesh.AddVertex(FVector(1, 0, 0), Up, FVector2D(1, 0));
		Mesh.AddVertex(FVector(0, 1, 0), Up, FVector2D(0, 1));
		const int32 Below = Mesh.AddVertex(FVector(0, 0, 0), -Up, FVector2D(0, 0));
		Mesh.AddVertex(FVector(1, 0, 0), -Up, FVector2D(1, 0));
		Mesh.AddVertex(FVector(0, 1, 0), -Up, FVector2D(0, 1));
		TestTrue(TEXT("Counter-clockwise triangle kept"), Mesh.AddTriangle(0, 1, 2));
		TestTrue(TEXT("Clockwise triangle kept"), Mesh.AddTriangle(0, 2, 1));
		TestTrue(TEXT("Downward triangle kept"), Mesh.AddTriangle(Below, Below + 1, Below + 2));
		TestTrue(TEXT("Given counter-clockwise, it faces up"), FVector::DotProduct(FaceNormal(Mesh, 0), Up) > 0.999999);
		TestTrue(TEXT("Given clockwise, it faces up"), FVector::DotProduct(FaceNormal(Mesh, 1), Up) > 0.999999);
		TestTrue(TEXT("With downward normals, it faces down"), FVector::DotProduct(FaceNormal(Mesh, 2), -Up) > 0.999999);

		// Triangles that carry no surface are dropped; a thin one that does is kept.
		const int32 Line = Mesh.AddVertex(FVector(2, 0, 0), Up, FVector2D(2, 0));
		Mesh.AddVertex(FVector(3, 0, 0), Up, FVector2D(3, 0));
		Mesh.AddVertex(FVector(5, 0, 0), Up, FVector2D(5, 0));
		const int32 Near = Mesh.AddVertex(FVector(0, 0, 0.5e-6), Up, FVector2D(0, 0));
		const int32 Thin = Mesh.AddVertex(FVector(2, 1.0e-5, 0), Up, FVector2D(2, 0));
		TestFalse(TEXT("Collinear corners are dropped"), Mesh.AddTriangle(Line, Line + 1, Line + 2));
		TestFalse(TEXT("Corners that would weld are dropped"), Mesh.AddTriangle(0, 1, Near));
		TestTrue(TEXT("A sliver five micrometres high has a surface and is kept"), Mesh.AddTriangle(0, 1, Thin));
		TestEqual(TEXT("Dropped triangles are counted"), Mesh.NumDroppedTriangles(), 2);

		// A quad splits along its shorter diagonal.
		FMeshBuilder Sheared;
		const int32 A = Sheared.AddVertex(FVector(0, 0, 0), Up, FVector2D(0, 0));
		const int32 B = Sheared.AddVertex(FVector(2, 0, 0), Up, FVector2D(2, 0));
		const int32 C = Sheared.AddVertex(FVector(4, 1, 0), Up, FVector2D(4, 1));
		const int32 D = Sheared.AddVertex(FVector(2, 1, 0), Up, FVector2D(2, 1));
		Sheared.AddQuad(A, B, C, D);
		auto Holds = [&Sheared](int32 Triangle, int32 Vertex)
		{
			const FIntVector& T = Sheared.Triangle(Triangle);
			return T.X == Vertex || T.Y == Vertex || T.Z == Vertex;
		};
		TestTrue(TEXT("The sheared quad splits along B-D"), Holds(0, B) && Holds(0, D) && Holds(1, B) && Holds(1, D));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormSlotsAndWeldingTest, "Musee.Form.SlotsAndWelding",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormSlotsAndWeldingTest::RunTest(const FString& Parameters)
	{
		FMeshBuilder Mesh(2);
		TestEqual(TEXT("The first named slot follows the piece's own"), Mesh.UseSlot(TEXT("stone")), 1);
		const int32 First = AddSquare(Mesh, 0.0, 0.0, FVector::UpVector);
		TestEqual(TEXT("A second slot"), Mesh.UseSlot(TEXT("bronze")), 2);
		AddSquare(Mesh, 1.0, 0.0, FVector::UpVector);
		TestEqual(TEXT("A slot used again keeps its index"), Mesh.UseSlot(TEXT("stone")), 1);
		Mesh.SetUV(First, 1, FVector2D(0.25, 0.75));
		Mesh.SetColor(First + 1, FLinearColor(0.2f, 0.4f, 0.6f, 1.0f));
		TestTrue(TEXT("A second UV channel is kept"), Mesh.UV(First, 1).Equals(FVector2D(0.25, 0.75)));
		TestTrue(TEXT("Uncoloured vertices are white"), Mesh.HasColors() && Mesh.Color(First) == FLinearColor::White);

		FMeshBuilder Other(2);
		Other.UseSlot(TEXT("glass"));
		AddSquare(Other, 5.0, 0.0, -FVector::UpVector);
		Other.UseSlot(TEXT("bronze"));
		AddSquare(Other, 6.0, 0.0, -FVector::UpVector);
		Mesh.Append(Other);
		TestEqual(TEXT("Slots joined by name"), Mesh.Slots().Num(), 4);
		TestTrue(TEXT("Appended glass keeps its slot"), Mesh.Slots()[Mesh.TriangleSlot(4)] == FName(TEXT("glass")));
		TestEqual(TEXT("Appended bronze joins the bronze slot"), Mesh.TriangleSlot(6), 2);
		TestTrue(TEXT("Appended vertices are white"), Mesh.Color(Mesh.NumVertices() - 1) == FLinearColor::White);

		// Each pair of squares shares an edge through separate vertices: welded, each pair stands on six points.
		const FWeld Weld = WeldVertices(Mesh);
		TestEqual(TEXT("Points under the sixteen vertices of two pairs of squares"), Weld.Points.Num(), 12);
		FMeshBuilder Close;
		Close.AddVertex(FVector(0, 0, 0), FVector::UpVector, FVector2D::ZeroVector);
		Close.AddVertex(FVector(0.5e-6, 0, 0), FVector::UpVector, FVector2D::ZeroVector);
		Close.AddVertex(FVector(2.5e-6, 0, 0), FVector::UpVector, FVector2D::ZeroVector);
		const FWeld Micro = WeldVertices(Close);
		TestEqual(TEXT("Half a micrometre welds; two and a half do not"), Micro.Points.Num(), 2);
		TestEqual(TEXT("The welded vertex stands on the first point"), Micro.PointOfVertex[1], 0);
		return true;
	}
}

#endif
