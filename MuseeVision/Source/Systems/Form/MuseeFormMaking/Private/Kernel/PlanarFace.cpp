#include "Kernel/PlanarFace.h"

#include "Kernel/MeshBuilder.h"
#include "Kernel/Triangulation.h"

namespace Musee::Form
{
	bool AddPlanarFace(FMeshBuilder& Mesh, const FPlaneFrame& Plane, TConstArrayView<FVector2D> Outer,
		TConstArrayView<TArray<FVector2D>> Holes)
	{
		return AddFlatPolygon(Mesh, Outer, Holes, [&Plane](const FVector2D& Point) { return Plane.At(Point); },
			Plane.Normal, [](const FVector2D& Point) { return Point; });
	}

	bool AddFlatPolygon(FMeshBuilder& Mesh, TConstArrayView<FVector2D> Outer, TConstArrayView<TArray<FVector2D>> Holes,
		TFunctionRef<FVector(const FVector2D&)> Place, const FVector& Normal,
		TFunctionRef<FVector2D(const FVector2D&)> UVOf)
	{
		TArray<FIntVector> Triangles;
		if (!TriangulatePolygon(Outer, Holes, Triangles))
		{
			return false;
		}
		const int32 Base = Mesh.NumVertices();
		auto AddPoint = [&Mesh, &Place, &Normal, &UVOf](const FVector2D& Point)
		{
			Mesh.AddVertex(Place(Point), Normal, UVOf(Point));
		};
		for (const FVector2D& Point : Outer)
		{
			AddPoint(Point);
		}
		for (const TArray<FVector2D>& Hole : Holes)
		{
			for (const FVector2D& Point : Hole)
			{
				AddPoint(Point);
			}
		}
		for (const FIntVector& T : Triangles)
		{
			Mesh.AddTriangle(Base + T.X, Base + T.Y, Base + T.Z);
		}
		return true;
	}
}
