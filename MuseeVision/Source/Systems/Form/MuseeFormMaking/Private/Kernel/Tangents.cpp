#include "Kernel/Tangents.h"

#include "Kernel/MeshBuilder.h"

namespace Musee::Form
{
	namespace
	{
		/** A UV triangle whose edges meet at a sine below this has no area to take directions from. */
		constexpr double FlatUVSine = 1.0e-10;

		/**
		 * A vertex with no UV direction crosses its normal with the vertical, unless the normal lies within about
		 * 25 degrees of it (|z| above 0.9), where it crosses with x instead so the result stays well conditioned.
		 */
		constexpr double NearVerticalZ = 0.9;

		/** The angle of a triangle's corner at P between the edges to Q and R, in radians. */
		double CornerAngle(const FVector& P, const FVector& Q, const FVector& R)
		{
			const FVector A = Q - P;
			const FVector B = R - P;
			return FMath::Atan2(FVector::CrossProduct(A, B).Size(), FVector::DotProduct(A, B));
		}

		/** The directions of increasing U and V over one triangle, or false when its UVs have no area. */
		bool TriangleDirections(const FMeshBuilder& Mesh, const FIntVector& T, FVector& OutAlongU, FVector& OutAlongV)
		{
			const FVector E1 = Mesh.Position(T.Y) - Mesh.Position(T.X);
			const FVector E2 = Mesh.Position(T.Z) - Mesh.Position(T.X);
			const FVector2D D1 = Mesh.UV(T.Y) - Mesh.UV(T.X);
			const FVector2D D2 = Mesh.UV(T.Z) - Mesh.UV(T.X);
			const double Det = D1.X * D2.Y - D2.X * D1.Y;
			if (Det == 0.0 || FMath::Abs(Det) <= FlatUVSine * D1.Size() * D2.Size())
			{
				return false;
			}
			// P = P0 + a E1 + b E2 and UV = UV0 + a D1 + b D2, solved for dP/dU and dP/dV.
			OutAlongU = ((E1 * D2.Y - E2 * D1.Y) / Det).GetSafeNormal();
			OutAlongV = ((E2 * D1.X - E1 * D2.X) / Det).GetSafeNormal();
			return true;
		}

		/** Completes one vertex's frame from its normal and its summed U and V directions. */
		FVertexTangent Complete(const FVector& Normal, const FVector& SumU, const FVector& SumV)
		{
			FVertexTangent Frame;
			FVector AlongU = SumU - Normal * FVector::DotProduct(Normal, SumU);
			if (AlongU.Normalize(UE_DOUBLE_SMALL_NUMBER))
			{
				Frame.TangentX = AlongU;
				const bool bMirrored = FVector::DotProduct(FVector::CrossProduct(Normal, AlongU), SumV) < 0.0;
				Frame.BinormalSign = bMirrored ? -1.0f : 1.0f;
				return Frame;
			}
			FVector AlongV = SumV - Normal * FVector::DotProduct(Normal, SumV);
			if (AlongV.Normalize(UE_DOUBLE_SMALL_NUMBER))
			{
				// cross(Normal, cross(AlongV, Normal)) = AlongV, so the sign stays +1.
				Frame.TangentX = FVector::CrossProduct(AlongV, Normal);
				return Frame;
			}
			const FVector Across = FMath::Abs(Normal.Z) < NearVerticalZ ? FVector::UpVector : FVector::ForwardVector;
			Frame.TangentX = FVector::CrossProduct(Normal, Across).GetSafeNormal();
			return Frame;
		}
	}

	TArray<FVertexTangent> DeriveTangents(const FMeshBuilder& Mesh)
	{
		const int32 Count = Mesh.NumVertices();
		TArray<FVector> SumU;
		TArray<FVector> SumV;
		SumU.Init(FVector::ZeroVector, Count);
		SumV.Init(FVector::ZeroVector, Count);
		for (const FIntVector& T : Mesh.AllTriangles())
		{
			FVector AlongU;
			FVector AlongV;
			if (!TriangleDirections(Mesh, T, AlongU, AlongV))
			{
				continue;
			}
			const int32 Corners[3] = {T.X, T.Y, T.Z};
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 Here = Corners[Corner];
				const double Angle = CornerAngle(Mesh.Position(Here), Mesh.Position(Corners[(Corner + 1) % 3]),
					Mesh.Position(Corners[(Corner + 2) % 3]));
				SumU[Here] += AlongU * Angle;
				SumV[Here] += AlongV * Angle;
			}
		}
		TArray<FVertexTangent> Result;
		Result.Reserve(Count);
		for (int32 Vertex = 0; Vertex < Count; ++Vertex)
		{
			Result.Add(Complete(Mesh.Normal(Vertex), SumU[Vertex], SumV[Vertex]));
		}
		return Result;
	}
}
