#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::NatureMaking
{
	/**
	 * The surface of a grown organ, as the growers draw it: positions in metres in the plant's frame (x east, y south,
	 * z up), a unit normal per vertex facing the side the surface is seen from, UVs in metres along the organ, and the
	 * organ's own linear tint per vertex (its alpha the occlusion deep inside a clump). Triangles are wound to face
	 * along their vertices' normals, the engine's front face.
	 */
	class FNatureSurface
	{
	public:
		int32 Vertex(const FVector& PositionMeters, const FVector& Normal, const FVector2D& UVMeters, const FLinearColor& Tint, double Occlusion = 1.0);

		/** Adds a triangle facing along its vertices' normals; a triangle with no area is dropped. */
		void Triangle(int32 A, int32 B, int32 C);

		/** Adds the quad A, B, C, D (in order round its edge) as two triangles. */
		void Quad(int32 A, int32 B, int32 C, int32 D);

		void Append(const FNatureSurface& Other);

		/** Moves every vertex by an offset (metres). */
		void Translate(const FVector& OffsetMeters);

		[[nodiscard]] bool IsEmpty() const { return Triangles.IsEmpty(); }
		[[nodiscard]] int32 NumVertices() const { return Positions.Num(); }
		[[nodiscard]] int32 NumTriangles() const { return Triangles.Num(); }
		[[nodiscard]] const FVector& Position(int32 Index) const { return Positions[Index]; }
		[[nodiscard]] TConstArrayView<FVector> AllPositions() const { return Positions; }

		/**
		 * Writes the surface as a made piece's mesh, to the Form kernel's contract (FormMadePiece.h): positions in
		 * engine units, normal layer 0 the drawn normals, layers 1 and 2 the tangent and bitangent derived from UV0
		 * (the engine's FMeshTangentsd), UV0 in metres, UV1 (Matter's piece layout) zero, the primary colours the tints,
		 * and every triangle in slot 0. Out is replaced.
		 */
		void Write(UE::Geometry::FDynamicMesh3& Out) const;

	private:
		TArray<FVector> Positions;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Tints;
		TArray<FIntVector> Triangles;
	};
}
