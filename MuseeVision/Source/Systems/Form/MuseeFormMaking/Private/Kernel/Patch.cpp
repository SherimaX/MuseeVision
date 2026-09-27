#include "Kernel/Patch.h"

#include "Kernel/MeshBuilder.h"

namespace Musee::Form
{
	namespace
	{
		/**
		 * The normal at a grid point from the grid's differences across it, facing the hint. Where the differences
		 * give no direction (a pole, where a whole row meets at one point), the hint is the normal.
		 */
		FVector GridNormal(const FPatchGrid& Grid, int32 I, int32 J, const FVector& Hint)
		{
			const FVector AcrossU = Grid.At(FMath::Min(I + 1, Grid.CountU - 1), J) - Grid.At(FMath::Max(I - 1, 0), J);
			const FVector AcrossV = Grid.At(I, FMath::Min(J + 1, Grid.CountV - 1)) - Grid.At(I, FMath::Max(J - 1, 0));
			FVector Normal = FVector::CrossProduct(AcrossU, AcrossV);
			if (!Normal.Normalize(UE_DOUBLE_SMALL_NUMBER * AcrossU.SizeSquared() * AcrossV.SizeSquared()))
			{
				return Hint.GetSafeNormal();
			}
			return FVector::DotProduct(Normal, Hint) < 0.0 ? -Normal : Normal;
		}
	}

	FPatchGrid FPatchGrid::Sample(TConstArrayView<double> StationsU, TConstArrayView<double> StationsV,
		TFunctionRef<FVector(double U, double V)> Surface)
	{
		FPatchGrid Grid;
		Grid.CountU = StationsU.Num();
		Grid.CountV = StationsV.Num();
		Grid.Points.Reserve(Grid.CountU * Grid.CountV);
		for (const double U : StationsU)
		{
			for (const double V : StationsV)
			{
				Grid.Points.Add(Surface(U, V));
			}
		}
		return Grid;
	}

	void Patch(FMeshBuilder& Mesh, const FPatchGrid& Grid, TFunctionRef<FVector(const FVector&)> FacingHint)
	{
		// Metres along each grid line: the U of point (I, J) is the length of its line of constant J up to I.
		TArray<double> AlongU;
		TArray<double> AlongV;
		AlongU.SetNumZeroed(Grid.Points.Num());
		AlongV.SetNumZeroed(Grid.Points.Num());
		for (int32 I = 0; I < Grid.CountU; ++I)
		{
			for (int32 J = 0; J < Grid.CountV; ++J)
			{
				const int32 Here = I * Grid.CountV + J;
				const FVector& P = Grid.At(I, J);
				AlongU[Here] = I == 0 ? 0.0 : AlongU[Here - Grid.CountV] + FVector::Distance(Grid.At(I - 1, J), P);
				AlongV[Here] = J == 0 ? 0.0 : AlongV[Here - 1] + FVector::Distance(Grid.At(I, J - 1), P);
			}
		}
		Patch(Mesh, Grid, FacingHint, [&Grid, &AlongU, &AlongV](int32 I, int32 J)
		{
			return FVector2D(AlongU[I * Grid.CountV + J], AlongV[I * Grid.CountV + J]);
		});
	}

	void Patch(FMeshBuilder& Mesh, const FPatchGrid& Grid, TFunctionRef<FVector(const FVector&)> FacingHint,
		TFunctionRef<FVector2D(int32 I, int32 J)> UVAt)
	{
		if (Grid.CountU < 2 || Grid.CountV < 2)
		{
			return;
		}
		const int32 Base = Mesh.NumVertices();
		for (int32 I = 0; I < Grid.CountU; ++I)
		{
			for (int32 J = 0; J < Grid.CountV; ++J)
			{
				const FVector& P = Grid.At(I, J);
				Mesh.AddVertex(P, GridNormal(Grid, I, J, FacingHint(P)), UVAt(I, J));
			}
		}
		auto Index = [Base, &Grid](int32 I, int32 J) { return Base + I * Grid.CountV + J; };
		for (int32 I = 0; I + 1 < Grid.CountU; ++I)
		{
			for (int32 J = 0; J + 1 < Grid.CountV; ++J)
			{
				Mesh.AddQuad(Index(I, J), Index(I + 1, J), Index(I + 1, J + 1), Index(I, J + 1));
			}
		}
	}
}
