#pragma once

#include "CoreMinimal.h"
#include "FormMesh.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::NatureMaking
{
	/**
	 * The surface of a grown organ, as the growers draw it: positions in metres in the plant's frame (x east, y south,
	 * z up), a unit normal per vertex facing the side the surface is seen from, UVs in metres along the organ, and the
	 * organ's own linear tint per vertex (its alpha the occlusion deep inside a clump). It is drawn on Form's mesh
	 * (FFormMesh), so a grown piece welds, winds and takes its tangents as every made piece does.
	 */
	class FNatureSurface
	{
	public:
		int32 Vertex(const FVector& PositionMeters, const FVector& Normal, const FVector2D& UVMeters, const FLinearColor& Tint, double Occlusion = 1.0);

		/** Adds a triangle facing along its vertices' normals; one too small to carry a surface is dropped. */
		void Triangle(int32 A, int32 B, int32 C);

		/** Adds the quad A, B, C, D (in order round its edge) as two triangles meeting on A-C, as it was drawn. */
		void Quad(int32 A, int32 B, int32 C, int32 D);

		void Append(const FNatureSurface& Other);

		/** Moves every vertex by an offset (metres). */
		void Translate(const FVector& OffsetMeters);

		[[nodiscard]] bool IsEmpty() const { return Mesh.IsEmpty(); }
		[[nodiscard]] int32 NumVertices() const { return Mesh.NumVertices(); }
		[[nodiscard]] int32 NumTriangles() const { return Mesh.NumTriangles(); }
		[[nodiscard]] const FVector& Position(int32 Index) const { return Mesh.Position(Index); }
		[[nodiscard]] TConstArrayView<FVector> AllPositions() const { return Mesh.AllPositions(); }

		/** Writes the surface as a made piece's mesh (FFormMesh::Write): the tints are its colours, every triangle slot 0. */
		void Write(UE::Geometry::FDynamicMesh3& Out) const;

	private:
		FFormMesh Mesh;
	};
}
