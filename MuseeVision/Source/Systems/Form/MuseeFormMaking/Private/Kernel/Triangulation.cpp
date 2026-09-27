#include "Kernel/Triangulation.h"

#include "ConstrainedDelaunay2.h"

namespace Musee::Form
{
	namespace
	{
		using FDelaunay = UE::Geometry::TConstrainedDelaunay2<double>;

		/** Adds a closed loop's points and its edges (each point to the next, the last to the first). */
		void AddLoop(FDelaunay& Delaunay, TConstArrayView<FVector2D> Loop, TArray<UE::Geometry::FIndex2i>& Edges)
		{
			const int32 First = Delaunay.Vertices.Num();
			for (const FVector2D& Point : Loop)
			{
				Delaunay.Vertices.Add(Point);
			}
			for (int32 Index = 0; Index < Loop.Num(); ++Index)
			{
				Edges.Add(UE::Geometry::FIndex2i(First + Index, First + (Index + 1) % Loop.Num()));
			}
		}
	}

	bool TriangulatePolygon(TConstArrayView<FVector2D> Outer, TConstArrayView<TArray<FVector2D>> Holes,
		TArray<FIntVector>& OutTriangles)
	{
		return TriangulatePolygonWithPoints(Outer, Holes, {}, OutTriangles);
	}

	bool TriangulatePolygonWithPoints(TConstArrayView<FVector2D> Outer, TConstArrayView<TArray<FVector2D>> Holes,
		TConstArrayView<FVector2D> Inner, TArray<FIntVector>& OutTriangles)
	{
		OutTriangles.Reset();
		if (Outer.Num() < 3)
		{
			return false;
		}
		FDelaunay Delaunay;
		Delaunay.FillRule = FDelaunay::EFillRule::Odd;
		AddLoop(Delaunay, Outer, Delaunay.Edges);
		for (const TArray<FVector2D>& Hole : Holes)
		{
			if (Hole.Num() < 3)
			{
				return false;
			}
			AddLoop(Delaunay, Hole, Delaunay.HoleEdges);
		}
		for (const FVector2D& Point : Inner)
		{
			Delaunay.Vertices.Add(Point);
		}
		// A failed boundary edge means the loops cross; an added vertex would break the vertex-for-vertex promise.
		if (!Delaunay.Triangulate() || Delaunay.AddedVerticesStartIndex != INDEX_NONE)
		{
			return false;
		}
		OutTriangles.Reserve(Delaunay.Triangles.Num());
		for (const UE::Geometry::FIndex3i& T : Delaunay.Triangles)
		{
			OutTriangles.Add(FIntVector(T.A, T.B, T.C));
		}
		return OutTriangles.Num() > 0;
	}
}
