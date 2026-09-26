#pragma once

#include "CoreMinimal.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"

/**
 * Builds one procedural mesh section, like the Swift MeshBuilder (Shared/Core/MeshKit.swift):
 * positions in metres in the actor's frame (X east, Y south, Z up), written in centimetres.
 *
 * Winding follows the normals given: Unreal's front faces have cross(b − a, c − a) pointing away
 * from the side they face, so Tri() orders each triangle to face along its normals. Give normals
 * that point towards the viewer of that face.
 */
struct FMuseeMesh
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FProcMeshTangent> Tangents;

	void Tri(const FVector& A, const FVector& B, const FVector& C, const FVector& NA, const FVector& NB, const FVector& NC,
			 const FVector2D& UA = FVector2D::ZeroVector, const FVector2D& UB = FVector2D::ZeroVector, const FVector2D& UC = FVector2D::ZeroVector)
	{
		const int32 Base = Vertices.Num();
		Vertices.Append({A * MuseePlan::Cm, B * MuseePlan::Cm, C * MuseePlan::Cm});
		Normals.Append({NA.GetSafeNormal(), NB.GetSafeNormal(), NC.GetSafeNormal()});
		UVs.Append({UA, UB, UC});
		const FVector Edge = (B - A).GetSafeNormal();
		for (int32 i = 0; i < 3; ++i) { Tangents.Add(FProcMeshTangent(Edge, false)); }
		// Unreal's front faces: cross(b − a, c − a) points away from the side they face.
		if (FVector::DotProduct(FVector::CrossProduct(B - A, C - A), NA + NB + NC) < 0) { Triangles.Append({Base, Base + 1, Base + 2}); }
		else { Triangles.Append({Base, Base + 2, Base + 1}); }
	}

	void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D,
			  const FVector& NA, const FVector& NB, const FVector& NC, const FVector& ND,
			  const FVector2D& UA = FVector2D::ZeroVector, const FVector2D& UB = FVector2D::ZeroVector,
			  const FVector2D& UC = FVector2D::ZeroVector, const FVector2D& UD = FVector2D::ZeroVector)
	{
		Tri(A, B, C, NA, NB, NC, UA, UB, UC);
		Tri(A, C, D, NA, NC, ND, UA, UC, UD);
	}

	void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& N)
	{
		Quad(A, B, C, D, N, N, N, N);
	}

	/** A profile (radius, height) turned about the vertical axis (MeshBuilder.lathe). */
	void Lathe(const TArray<FVector2D>& Profile, int32 Segments, bool bBothSides, FVector2D UVScale = FVector2D(1, 1))
	{
		for (int32 k = 0; k + 1 < Profile.Num(); ++k)
		{
			const FVector2D A = Profile[k], B = Profile[k + 1];
			const FVector2D D = B - A;
			const FVector2D N2 = FVector2D(D.Y, -D.X).GetSafeNormal();   // outward in (r, h)
			for (int32 i = 0; i < Segments; ++i)
			{
				const double T0 = 2 * UE_DOUBLE_PI * i / Segments, T1 = 2 * UE_DOUBLE_PI * (i + 1) / Segments;
				auto P = [](const FVector2D& Q, double T) { return FVector(Q.X * FMath::Cos(T), Q.X * FMath::Sin(T), Q.Y); };
				auto N = [&N2](double T) { return FVector(N2.X * FMath::Cos(T), N2.X * FMath::Sin(T), N2.Y); };
				const double V0 = double(k) / (Profile.Num() - 1), V1 = double(k + 1) / (Profile.Num() - 1);
				const double U0 = double(i) / Segments, U1 = double(i + 1) / Segments;
				const FVector2D UV00 = FVector2D(U0, V0) * UVScale, UV10 = FVector2D(U1, V0) * UVScale;
				const FVector2D UV11 = FVector2D(U1, V1) * UVScale, UV01 = FVector2D(U0, V1) * UVScale;
				Quad(P(A, T0), P(A, T1), P(B, T1), P(B, T0), N(T0), N(T1), N(T1), N(T0), UV00, UV10, UV11, UV01);
				if (bBothSides)
				{
					Quad(P(A, T0), P(A, T1), P(B, T1), P(B, T0), -N(T0), -N(T1), -N(T1), -N(T0), UV00, UV10, UV11, UV01);
				}
			}
		}
	}

	void Write(UProceduralMeshComponent* Mesh, int32 Index) const
	{
		Mesh->CreateMeshSection(Index, Vertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, false);
	}
};
