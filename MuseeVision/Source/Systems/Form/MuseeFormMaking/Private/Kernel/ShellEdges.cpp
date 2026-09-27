#include "Kernel/ShellEdges.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Welding.h"

namespace Musee::Form
{
	namespace
	{
		/** How often the triangles run an edge from its lower point to its higher, and back. */
		struct FEdgeUse
		{
			int32 Rising = 0;
			int32 Falling = 0;
		};

		uint64 EdgeKey(int32 Low, int32 High)
		{
			return (uint64(uint32(Low)) << 32) | uint64(uint32(High));
		}
	}

	FShellEdges MeasureShellEdges(TConstArrayView<FIntVector> PointTriangles)
	{
		FShellEdges Result;
		TMap<uint64, FEdgeUse> Uses;
		TSet<int32> Used;
		for (const FIntVector& T : PointTriangles)
		{
			const int32 Corners[3] = {T.X, T.Y, T.Z};
			if (Corners[0] == Corners[1] || Corners[1] == Corners[2] || Corners[2] == Corners[0])
			{
				++Result.Collapsed;
				continue;
			}
			++Result.Faces;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 From = Corners[Corner];
				const int32 To = Corners[(Corner + 1) % 3];
				Used.Add(From);
				FEdgeUse& Use = Uses.FindOrAdd(EdgeKey(FMath::Min(From, To), FMath::Max(From, To)));
				++(From < To ? Use.Rising : Use.Falling);
			}
		}
		Result.Points = Used.Num();
		Result.Edges = Uses.Num();
		for (const TPair<uint64, FEdgeUse>& Pair : Uses)
		{
			const int32 Low = int32(Pair.Key >> 32);
			const int32 High = int32(Pair.Key & 0xffffffffull);
			const int32 Total = Pair.Value.Rising + Pair.Value.Falling;
			if (Total == 1)
			{
				++Result.Boundary;
				Result.BoundaryEdges.Add(Pair.Value.Rising == 1 ? FIntPoint(Low, High) : FIntPoint(High, Low));
			}
			else if (Total > 2)
			{
				++Result.NonManifold;
			}
			else if (Pair.Value.Rising != 1)
			{
				++Result.Misoriented;
			}
		}
		return Result;
	}

	FShellEdges MeasureShellEdges(const FMeshBuilder& Mesh, const FWeld& Weld)
	{
		TArray<FIntVector> PointTriangles;
		PointTriangles.Reserve(Mesh.NumTriangles());
		for (const FIntVector& T : Mesh.AllTriangles())
		{
			PointTriangles.Add(FIntVector(Weld.PointOfVertex[T.X], Weld.PointOfVertex[T.Y], Weld.PointOfVertex[T.Z]));
		}
		return MeasureShellEdges(PointTriangles);
	}

	FShellEdges MeasureShellEdges(const FMeshBuilder& Mesh, double WeldMeters)
	{
		return MeasureShellEdges(Mesh, WeldVertices(Mesh, WeldMeters));
	}

	FShellEdges MeasureShellEdges(const UE::Geometry::FDynamicMesh3& Mesh)
	{
		TArray<FIntVector> PointTriangles;
		PointTriangles.Reserve(Mesh.TriangleCount());
		for (const int32 Triangle : Mesh.TriangleIndicesItr())
		{
			const UE::Geometry::FIndex3i T = Mesh.GetTriangle(Triangle);
			PointTriangles.Add(FIntVector(T.A, T.B, T.C));
		}
		return MeasureShellEdges(PointTriangles);
	}
}
