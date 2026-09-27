#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Kernel/MeshBuilder.h"
#include "Kernel/Patch.h"
#include "Stations/Stations.h"
#include "Tests/FormTestShapes.h"

namespace Musee::Form::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormPatchNormalsTest, "Musee.Form.PatchNormals",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormPatchNormalsTest::RunTest(const FString& Parameters)
	{
		// An eighth of a 4 m sphere, seen from inside (a vault cell): longitude u and latitude v on stations.
		const double Radius = 4.0;
		auto Sphere = [Radius](double U, double V)
		{
			return Radius * FVector(FMath::Cos(U) * FMath::Cos(V), FMath::Sin(U) * FMath::Cos(V), FMath::Sin(V));
		};
		const TArray<double> Longitudes = ArcStations(Radius, 0.0, UE_DOUBLE_HALF_PI, 0.001);
		const TArray<double> Latitudes = ArcStations(Radius, 0.0, 1.2, 0.001);
		const FPatchGrid Grid = FPatchGrid::Sample(Longitudes, Latitudes, Sphere);
		FMeshBuilder Mesh;
		Patch(Mesh, Grid, [](const FVector& P) { return -P; });
		TestEqual(TEXT("One vertex per grid point"), Mesh.NumVertices(), Longitudes.Num() * Latitudes.Num());

		// Inside the grid, central differences on a sphere give its exact normal; at the edges, one-sided ones lean by
		// half a step along each edge they lie on, so within a step even at a corner. Every normal faces the centre,
		// and every triangle faces with its normals.
		const double Step = FMath::Max(Longitudes[1] - Longitudes[0], Latitudes[1] - Latitudes[0]);
		int32 Inexact = 0;
		int32 Astray = 0;
		for (int32 I = 0; I < Grid.CountU; ++I)
		{
			for (int32 J = 0; J < Grid.CountV; ++J)
			{
				const int32 Vertex = I * Grid.CountV + J;
				const FVector Truth = -Mesh.Position(Vertex).GetSafeNormal();
				const bool bEdge = I == 0 || J == 0 || I == Grid.CountU - 1 || J == Grid.CountV - 1;
				const double Error = AngleBetween(Mesh.Normal(Vertex), Truth);
				Inexact += bEdge ? 0 : (Error < 1e-9 ? 0 : 1);
				Astray += Error < Step ? 0 : 1;
			}
		}
		TestEqual(TEXT("Inner normals not the sphere's own"), Inexact, 0);
		TestEqual(TEXT("Normals further than a step from the sphere's"), Astray, 0);
		int32 Backwards = 0;
		for (int32 Triangle = 0; Triangle < Mesh.NumTriangles(); ++Triangle)
		{
			const FIntVector& T = Mesh.Triangle(Triangle);
			Backwards += FVector::DotProduct(FaceNormal(Mesh, Triangle), Mesh.Normal(T.X)) > 0.0 ? 0 : 1;
		}
		TestEqual(TEXT("Triangles facing away from their normals"), Backwards, 0);

		// UVs are metres along the grid lines: along the equator U reaches a quarter of the circumference, less the
		// chords' shortfall (under the tolerance per chord).
		const double Equator = Mesh.UV((Grid.CountU - 1) * Grid.CountV).X;
		const double Quarter = UE_DOUBLE_HALF_PI * Radius;
		TestTrue(*FString::Printf(TEXT("Equator U %.6f m against the arc's %.6f m"), Equator, Quarter),
			Equator <= Quarter && Quarter - Equator < 0.001 * (Grid.CountU - 1));
		return true;
	}
}

#endif
