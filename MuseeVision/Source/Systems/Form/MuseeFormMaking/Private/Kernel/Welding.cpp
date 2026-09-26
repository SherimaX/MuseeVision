#include "Kernel/Welding.h"

#include "Kernel/MeshBuilder.h"

namespace Musee::Form
{
	namespace
	{
		/**
		 * The cell of a grid whose cells are one tolerance wide, so a point within the tolerance of another lies in the
		 * same cell or a neighbouring one.
		 */
		FInt64Vector CellOf(const FVector& P, double CellMeters)
		{
			return FInt64Vector(FMath::FloorToInt64(P.X / CellMeters), FMath::FloorToInt64(P.Y / CellMeters),
				FMath::FloorToInt64(P.Z / CellMeters));
		}

		/** The lowest-numbered point within the tolerance of P in the 27 cells around its own, or INDEX_NONE. */
		int32 FindPoint(const TMap<FInt64Vector, TArray<int32>>& Cells, const TArray<FVector>& Points, const FVector& P,
			const FInt64Vector& Cell, double ToleranceMeters)
		{
			int32 Found = INDEX_NONE;
			const double Reach = ToleranceMeters * ToleranceMeters;
			for (int64 X = -1; X <= 1; ++X)
			{
				for (int64 Y = -1; Y <= 1; ++Y)
				{
					for (int64 Z = -1; Z <= 1; ++Z)
					{
						const TArray<int32>* Members = Cells.Find(Cell + FInt64Vector(X, Y, Z));
						if (!Members)
						{
							continue;
						}
						for (const int32 Point : *Members)
						{
							if ((Found == INDEX_NONE || Point < Found) && FVector::DistSquared(Points[Point], P) <= Reach)
							{
								Found = Point;
							}
						}
					}
				}
			}
			return Found;
		}
	}

	FWeld WeldVertices(const FMeshBuilder& Mesh, double ToleranceMeters)
	{
		FWeld Weld;
		Weld.PointOfVertex.Reserve(Mesh.NumVertices());
		TMap<FInt64Vector, TArray<int32>> Cells;
		const double CellMeters = FMath::Max(ToleranceMeters, UE_DOUBLE_SMALL_NUMBER);
		for (const FVector& P : Mesh.AllPositions())
		{
			const FInt64Vector Cell = CellOf(P, CellMeters);
			int32 Point = FindPoint(Cells, Weld.Points, P, Cell, ToleranceMeters);
			if (Point == INDEX_NONE)
			{
				Point = Weld.Points.Add(P);
				Cells.FindOrAdd(Cell).Add(Point);
			}
			Weld.PointOfVertex.Add(Point);
		}
		return Weld;
	}
}
