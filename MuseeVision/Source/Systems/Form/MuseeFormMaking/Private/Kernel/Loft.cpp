#include "Kernel/Loft.h"

#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/Tolerances.h"

namespace Musee::Form
{
	namespace
	{
		/** The rows and columns of a loft's grid, with the metres run along and around it to each point. */
		struct FLoftMeasure
		{
			const FLoftGrid& Grid;
			int32 Points = 0;
			int32 Sections = 0;
			TArray<TArray<double>> Along;
			TArray<TArray<double>> Around;

			explicit FLoftMeasure(const FLoftGrid& InGrid)
				: Grid(InGrid), Points(InGrid.Rings[0].Num()), Sections(InGrid.Rings.Num())
			{
				Along.SetNum(Sections);
				Around.SetNum(Sections);
				for (int32 K = 0; K < Sections; ++K)
				{
					Around[K].Add(0.0);
					for (int32 I = 1; I <= Points; ++I)
					{
						Around[K].Add(Around[K].Last() + FVector::Distance(At(K, I - 1), At(K, I)));
					}
					for (int32 I = 0; I < Points; ++I)
					{
						Along[K].Add(K == 0 ? 0.0 : Along[K - 1][I] + FVector::Distance(At(K - 1, I), At(K, I)));
					}
				}
			}

			const FVector& At(int32 K, int32 I) const { return Grid.Rings[K][I % Points]; }

			/** The metres round section K to column C, counting on past a full turn for a patch that runs over the seam. */
			double V(int32 K, int32 C) const { return Around[K][C % Points] + (C / Points) * Around[K][Points]; }

			/** The outward face normal of the quad from point I to I + 1 between sections K and K + 1, scaled by its size. */
			FVector QuadNormal(int32 K, int32 I) const
			{
				const FVector AroundDirection = (At(K, I + 1) - At(K, I)) + (At(K + 1, I + 1) - At(K + 1, I));
				const FVector UpDirection = (At(K + 1, I) - At(K, I)) + (At(K + 1, I + 1) - At(K, I + 1));
				return FVector::CrossProduct(AroundDirection, UpDirection);
			}

			FVector Centre(int32 K) const
			{
				FVector Sum = FVector::ZeroVector;
				for (const FVector& Point : Grid.Rings[K])
				{
					Sum += Point;
				}
				return Sum / Points;
			}
		};

		/** One smooth patch of the side, between two arrises round it and two along it, added as a grid of shared vertices. */
		void SidePatch(FMeshBuilder& Mesh, const FLoftMeasure& Loft, int32 FirstRow, int32 LastRow, int32 FirstColumn, int32 Columns)
		{
			TArray<int32> Vertices;
			for (int32 K = FirstRow; K <= LastRow; ++K)
			{
				for (int32 J = 0; J < Columns; ++J)
				{
					FVector Normal = FVector::ZeroVector;
					for (int32 QK = FMath::Max(K - 1, FirstRow); QK <= FMath::Min(K, LastRow - 1); ++QK)
					{
						for (int32 QJ = FMath::Max(J - 1, 0); QJ <= FMath::Min(J, Columns - 2); ++QJ)
						{
							Normal += Loft.QuadNormal(QK, FirstColumn + QJ);
						}
					}
					const FVector& Point = Loft.At(K, FirstColumn + J);
					if (Normal.IsNearlyZero(UE_DOUBLE_SMALL_NUMBER))
					{
						// A patch that has closed to a line here: the point faces away from its section's middle.
						Normal = (Point - Loft.Centre(K)) * FVector(1.0, 1.0, 0.0);
					}
					Vertices.Add(Mesh.AddVertex(Point, Normal, FVector2D(Loft.Along[K][(FirstColumn + J) % Loft.Points], Loft.V(K, FirstColumn + J))));
				}
			}
			auto Index = [&Vertices, FirstRow, Columns](int32 K, int32 J) { return Vertices[(K - FirstRow) * Columns + J]; };
			for (int32 K = FirstRow; K < LastRow; ++K)
			{
				for (int32 J = 0; J + 1 < Columns; ++J)
				{
					Mesh.AddQuad(Index(K, J), Index(K + 1, J), Index(K + 1, J + 1), Index(K, J + 1));
				}
			}
		}

		/** Closes an end with its flat section, facing along Normal; an end closed to a point needs none. */
		bool End(FMeshBuilder& Mesh, TConstArrayView<FVector> Ring, const FVector& Normal)
		{
			TArray<FVector2D> Outline;
			for (const FVector& Point : Ring)
			{
				const FVector2D Flat(Point.X, Point.Y);
				if (Outline.IsEmpty() || FVector2D::Distance(Outline.Last(), Flat) > WeldToleranceMeters)
				{
					Outline.Add(Flat);
				}
			}
			while (Outline.Num() > 1 && FVector2D::Distance(Outline.Last(), Outline[0]) <= WeldToleranceMeters)
			{
				Outline.Pop();
			}
			if (Outline.Num() < 3)
			{
				return true;
			}
			const double Z = Ring[0].Z;
			return AddFlatPolygon(Mesh, Outline, {}, [Z](const FVector2D& P) { return FVector(P.X, P.Y, Z); }, Normal,
				[](const FVector2D& P) { return P; });
		}
	}

	bool Loft(FMeshBuilder& Mesh, const FLoftGrid& Grid, FName SideSlot, FName EndSlot)
	{
		if (Grid.Rings.Num() < 2 || Grid.Rings[0].Num() < 3)
		{
			return false;
		}
		const FLoftMeasure Measure(Grid);
		TArray<int32> Rows = {0};
		for (int32 K = 1; K + 1 < Measure.Sections; ++K)
		{
			if (Grid.CrispAcross.IsValidIndex(K) && Grid.CrispAcross[K])
			{
				Rows.Add(K);
			}
		}
		Rows.Add(Measure.Sections - 1);
		TArray<int32> Arrises;
		for (int32 I = 0; I < Measure.Points; ++I)
		{
			if (Grid.CrispAround.IsValidIndex(I) && Grid.CrispAround[I])
			{
				Arrises.Add(I);
			}
		}
		Mesh.UseSlot(SideSlot);
		for (int32 Row = 0; Row + 1 < Rows.Num(); ++Row)
		{
			if (Arrises.IsEmpty())
			{
				SidePatch(Mesh, Measure, Rows[Row], Rows[Row + 1], 0, Measure.Points + 1);
				continue;
			}
			for (int32 A = 0; A < Arrises.Num(); ++A)
			{
				const int32 From = Arrises[A];
				const int32 Span = (Arrises[(A + 1) % Arrises.Num()] - From + Measure.Points) % Measure.Points;
				SidePatch(Mesh, Measure, Rows[Row], Rows[Row + 1], From, (Span == 0 ? Measure.Points : Span) + 1);
			}
		}
		Mesh.UseSlot(EndSlot);
		return End(Mesh, Grid.Rings[0], -FVector::UpVector) && End(Mesh, Grid.Rings.Last(), FVector::UpVector);
	}
}
