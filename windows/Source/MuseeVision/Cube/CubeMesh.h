#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

/**
 * An indexed mesh for the Cube and the journeys: positions in metres (written in centimetres), normals, UV0, UV1,
 * vertex colours (linear) and tangents, with smoothing and the plan's polar helpers (angles from east towards south).
 * A named namespace: the module builds in unity files.
 */
namespace CubeMesh
{
	constexpr double Cm = 100.0;

	inline FVector Polar(double R, double A, double Z) { return FVector(R * FMath::Cos(A), R * FMath::Sin(A), Z); }
	inline FVector Outward(double A) { return FVector(FMath::Cos(A), FMath::Sin(A), 0); }
	inline FVector Along(double A) { return FVector(-FMath::Sin(A), FMath::Cos(A), 0); }

	struct FMesh
	{
		TArray<FVector> Positions;   // cm
		TArray<FVector> Normals;
		TArray<FVector2D> UV0, UV1;
		TArray<FLinearColor> Colours;
		TArray<FProcMeshTangent> Tangents;
		TArray<int32> Indices;

		void Reserve(int32 Vertices)
		{
			Positions.Reserve(Vertices); Normals.Reserve(Vertices); UV0.Reserve(Vertices); UV1.Reserve(Vertices);
			Colours.Reserve(Vertices); Tangents.Reserve(Vertices); Indices.Reserve(Vertices * 6);
		}

		/** A vertex (P in metres). A zero tangent is made up from the normal. */
		int32 V(const FVector& P, const FVector& N, const FVector2D& Uv = FVector2D::ZeroVector, const FLinearColor& C = FLinearColor::White,
				const FVector2D& Uv1 = FVector2D::ZeroVector, const FVector& T = FVector::ZeroVector)
		{
			const FVector Nn = N.GetSafeNormal();
			FVector Tn = T.IsNearlyZero() ? FVector::CrossProduct(FMath::Abs(Nn.Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0), Nn) : T;
			Tn = (Tn - Nn * FVector::DotProduct(Tn, Nn)).GetSafeNormal();
			Normals.Add(Nn);
			UV0.Add(Uv);
			UV1.Add(Uv1);
			Colours.Add(C);
			Tangents.Add(FProcMeshTangent(Tn, false));
			return Positions.Add(P * Cm);
		}

		/** A triangle facing along its vertices' normals (degenerate ones are dropped). */
		void Tri(int32 A, int32 B, int32 C)
		{
			const FVector X = FVector::CrossProduct(Positions[B] - Positions[A], Positions[C] - Positions[A]);
			if (X.SizeSquared() < 1e-12) { return; }
			// Unreal's front faces: cross(b − a, c − a) points away from the side they face.
			if (FVector::DotProduct(X, Normals[A] + Normals[B] + Normals[C]) < 0.0) { Indices.Append({A, B, C}); }
			else { Indices.Append({A, C, B}); }
		}

		void Quad(int32 A, int32 B, int32 C, int32 D)
		{
			Tri(A, B, C);
			Tri(A, C, D);
		}

		/** The faces of an axis-aligned box (metres), facing out. */
		void Box(const FVector& Lo, const FVector& Hi)
		{
			const FVector C[8] = {
				FVector(Lo.X, Lo.Y, Lo.Z), FVector(Hi.X, Lo.Y, Lo.Z), FVector(Hi.X, Hi.Y, Lo.Z), FVector(Lo.X, Hi.Y, Lo.Z),
				FVector(Lo.X, Lo.Y, Hi.Z), FVector(Hi.X, Lo.Y, Hi.Z), FVector(Hi.X, Hi.Y, Hi.Z), FVector(Lo.X, Hi.Y, Hi.Z)};
			const int32 F[6][4] = {{0, 3, 7, 4}, {1, 2, 6, 5}, {0, 1, 5, 4}, {3, 2, 6, 7}, {0, 1, 2, 3}, {4, 5, 6, 7}};
			const FVector N[6] = {FVector(-1, 0, 0), FVector(1, 0, 0), FVector(0, -1, 0), FVector(0, 1, 0), FVector(0, 0, -1), FVector(0, 0, 1)};
			for (int32 f = 0; f < 6; ++f)
			{
				auto Uv = [&](const FVector& P)
				{
					return FMath::Abs(N[f].Z) > 0.5 ? FVector2D(P.X, P.Y) : (FMath::Abs(N[f].X) > 0.5 ? FVector2D(P.Y, -P.Z) : FVector2D(P.X, -P.Z));
				};
				int32 Id[4];
				for (int32 k = 0; k < 4; ++k) { Id[k] = V(C[F[f][k]], N[f], Uv(C[F[f][k]])); }
				Quad(Id[0], Id[1], Id[2], Id[3]);
			}
		}

		void Append(const FMesh& O)
		{
			const int32 Base = Positions.Num();
			Positions.Append(O.Positions); Normals.Append(O.Normals); UV0.Append(O.UV0); UV1.Append(O.UV1);
			Colours.Append(O.Colours); Tangents.Append(O.Tangents);
			Indices.Reserve(Indices.Num() + O.Indices.Num());
			for (const int32 I : O.Indices) { Indices.Add(Base + I); }
		}

		/**
		 * Normals from the triangles (area weighted), kept on the side the given normals faced. bWeldSeams: vertices at
		 * the same position share their normal (a grid's closing seam).
		 */
		void SmoothNormals(bool bWeldSeams)
		{
			TArray<FVector> Acc;
			Acc.SetNumZeroed(Positions.Num());
			for (int32 t = 0; t + 2 < Indices.Num(); t += 3)
			{
				const int32 A = Indices[t], B = Indices[t + 1], C = Indices[t + 2];
				// Front faces: cross(b − a, c − a) points away from the viewer, so the facing normal is its opposite.
				const FVector X = -FVector::CrossProduct(Positions[B] - Positions[A], Positions[C] - Positions[A]);
				Acc[A] += X; Acc[B] += X; Acc[C] += X;
			}
			if (bWeldSeams)
			{
				TMap<FIntVector, FVector> Shared;
				auto Key = [](const FVector& P) { return FIntVector(FMath::RoundToInt32(P.X * 10), FMath::RoundToInt32(P.Y * 10), FMath::RoundToInt32(P.Z * 10)); };
				for (int32 i = 0; i < Positions.Num(); ++i) { Shared.FindOrAdd(Key(Positions[i])) += Acc[i]; }
				for (int32 i = 0; i < Positions.Num(); ++i) { Acc[i] = Shared[Key(Positions[i])]; }
			}
			for (int32 i = 0; i < Positions.Num(); ++i)
			{
				FVector N = Acc[i].GetSafeNormal();
				if (N.IsNearlyZero()) { continue; }
				if (FVector::DotProduct(N, Normals[i]) < 0) { N = -N; }
				Normals[i] = N;
				FVector T = Tangents[i].TangentX;
				T = (T - N * FVector::DotProduct(T, N)).GetSafeNormal();
				if (!T.IsNearlyZero()) { Tangents[i].TangentX = T; }
			}
		}

		void Write(UProceduralMeshComponent* Mesh, int32 Section, bool bCollision) const
		{
			if (!Mesh || Positions.Num() == 0) { return; }
			TArray<FVector2D> Empty;
			Mesh->CreateMeshSection_LinearColor(Section, Positions, Indices, Normals, UV0, UV1, Empty, Empty, Colours, Tangents, bCollision);
		}
	};

	/** A vertical cylinder's face (R, Z0 … Z1, angles A0 … A1), facing in (bInward) or out. */
	inline void Cylinder(FMesh& M, double R, double Z0, double Z1, bool bInward, double A0, double A1, int32 Round = 192)
	{
		const int32 N = FMath::Max(2, FMath::CeilToInt32(Round * FMath::Abs(A1 - A0) / (2 * UE_DOUBLE_PI)));
		for (int32 i = 0; i < N; ++i)
		{
			const double T0 = A0 + (A1 - A0) * i / N, T1 = A0 + (A1 - A0) * (i + 1) / N;
			const double S = bInward ? -1.0 : 1.0;
			M.Quad(M.V(Polar(R, T0, Z0), Outward(T0) * S, FVector2D(T0 * R, -Z0), FLinearColor::White, FVector2D::ZeroVector, Along(T0)),
				   M.V(Polar(R, T1, Z0), Outward(T1) * S, FVector2D(T1 * R, -Z0), FLinearColor::White, FVector2D::ZeroVector, Along(T1)),
				   M.V(Polar(R, T1, Z1), Outward(T1) * S, FVector2D(T1 * R, -Z1), FLinearColor::White, FVector2D::ZeroVector, Along(T1)),
				   M.V(Polar(R, T0, Z1), Outward(T0) * S, FVector2D(T0 * R, -Z1), FLinearColor::White, FVector2D::ZeroVector, Along(T0)));
		}
	}

	/** A flat ring (R0 … R1) at Z, facing up (bUp) or down. */
	inline void Annulus(FMesh& M, double R0, double R1, double Z, bool bUp, int32 Round = 192)
	{
		const FVector N(0, 0, bUp ? 1 : -1);
		for (int32 i = 0; i < Round; ++i)
		{
			const double T0 = 2 * UE_DOUBLE_PI * i / Round, T1 = 2 * UE_DOUBLE_PI * (i + 1) / Round;
			auto Uv = [](const FVector& P) { return FVector2D(P.X, P.Y); };
			const FVector P[4] = {Polar(R0, T0, Z), Polar(R0, T1, Z), Polar(R1, T1, Z), Polar(R1, T0, Z)};
			M.Quad(M.V(P[0], N, Uv(P[0])), M.V(P[1], N, Uv(P[1])), M.V(P[2], N, Uv(P[2])), M.V(P[3], N, Uv(P[3])));
		}
	}

	/** A closed band of rectangular section: R0 … R1, Z0 … Z1, all the way round. */
	inline void Band(FMesh& M, double R0, double R1, double Z0, double Z1, int32 Round = 192)
	{
		Cylinder(M, R0, Z0, Z1, true, 0, 2 * UE_DOUBLE_PI, Round);
		Cylinder(M, R1, Z0, Z1, false, 0, 2 * UE_DOUBLE_PI, Round);
		Annulus(M, R0, R1, Z0, false, Round);
		Annulus(M, R0, R1, Z1, true, Round);
	}

	/** A round tube on the horizontal circle Major at height Zc (a bead). */
	inline void Torus(FMesh& M, double Major, double Minor, double Zc, int32 Round = 192, int32 Around = 12)
	{
		TArray<int32> Ids;
		for (int32 i = 0; i <= Round; ++i)
		{
			const double T = 2 * UE_DOUBLE_PI * i / Round;
			for (int32 j = 0; j <= Around; ++j)
			{
				const double P = 2 * UE_DOUBLE_PI * j / Around;
				const FVector D = Outward(T) * FMath::Cos(P) + FVector(0, 0, FMath::Sin(P));
				Ids.Add(M.V(Polar(Major, T, Zc) + D * Minor, D, FVector2D(T * Major, P * Minor), FLinearColor::White, FVector2D::ZeroVector, Along(T)));
			}
		}
		for (int32 i = 0; i < Round; ++i)
		{
			for (int32 j = 0; j < Around; ++j)
			{
				const int32 A = i * (Around + 1) + j;
				M.Quad(A, A + 1, A + Around + 2, A + Around + 1);
			}
		}
	}

	/** A box: centre C, half-sizes Half along the unit axes X, Y, Z (metres). */
	inline void Block(FMesh& M, const FVector& C, const FVector& X, const FVector& Y, const FVector& Z, const FVector& Half)
	{
		const FVector Ax[3] = {X * Half.X, Y * Half.Y, Z * Half.Z};
		const FVector Dir[3] = {X, Y, Z};
		for (int32 a = 0; a < 3; ++a)
		{
			for (const double S : {-1.0, 1.0})
			{
				const FVector& U = Ax[(a + 1) % 3];
				const FVector& W = Ax[(a + 2) % 3];
				const FVector F = C + Ax[a] * S;
				const FVector N = Dir[a] * S;
				auto Uv = [&](const FVector& P) { return FVector2D(FVector::DotProduct(P, Dir[(a + 1) % 3]), FVector::DotProduct(P, Dir[(a + 2) % 3])); };
				const FVector P0 = F - U - W, P1 = F + U - W, P2 = F + U + W, P3 = F - U + W;
				M.Quad(M.V(P0, N, Uv(P0), FLinearColor::White, FVector2D::ZeroVector, Dir[(a + 1) % 3]),
					   M.V(P1, N, Uv(P1), FLinearColor::White, FVector2D::ZeroVector, Dir[(a + 1) % 3]),
					   M.V(P2, N, Uv(P2), FLinearColor::White, FVector2D::ZeroVector, Dir[(a + 1) % 3]),
					   M.V(P3, N, Uv(P3), FLinearColor::White, FVector2D::ZeroVector, Dir[(a + 1) % 3]));
			}
		}
	}

	/** A round rod from A to B (metres), radius R, capped. */
	inline void Rod(FMesh& M, const FVector& A, const FVector& B, double R, int32 Around = 16, bool bCaps = true)
	{
		const FVector Ax = (B - A).GetSafeNormal();
		const FVector U = FVector::CrossProduct(FMath::Abs(Ax.Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0), Ax).GetSafeNormal();
		const FVector W = FVector::CrossProduct(Ax, U);
		const double L = (B - A).Size();
		for (int32 j = 0; j < Around; ++j)
		{
			const double P0 = 2 * UE_DOUBLE_PI * j / Around, P1 = 2 * UE_DOUBLE_PI * (j + 1) / Around;
			const FVector D0 = U * FMath::Cos(P0) + W * FMath::Sin(P0), D1 = U * FMath::Cos(P1) + W * FMath::Sin(P1);
			M.Quad(M.V(A + D0 * R, D0, FVector2D(P0 * R, 0), FLinearColor::White, FVector2D::ZeroVector, Ax),
				   M.V(A + D1 * R, D1, FVector2D(P1 * R, 0), FLinearColor::White, FVector2D::ZeroVector, Ax),
				   M.V(B + D1 * R, D1, FVector2D(P1 * R, L), FLinearColor::White, FVector2D::ZeroVector, Ax),
				   M.V(B + D0 * R, D0, FVector2D(P0 * R, L), FLinearColor::White, FVector2D::ZeroVector, Ax));
			if (bCaps)
			{
				M.Tri(M.V(A, -Ax), M.V(A + D0 * R, -Ax), M.V(A + D1 * R, -Ax));
				M.Tri(M.V(B, Ax), M.V(B + D0 * R, Ax), M.V(B + D1 * R, Ax));
			}
		}
	}
}
