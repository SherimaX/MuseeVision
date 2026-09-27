#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	/**
	 * The one indexed mesh builder: the surface of made pieces as the kernel draws it, before Studio casts it.
	 *
	 * Positions are metres in the drawing's frame (x east, y south, z up). Every vertex carries a unit normal that
	 * faces the side the surface is seen from, UV channel 0 in metres (so a material's texel density is physical),
	 * UV channel 1 for Matter's piece layout, any further UV channels the caller sets, and an optional linear colour.
	 * Each triangle belongs to a material slot and is wound so that it faces along its vertices' normals under
	 * Unreal's convention: a front face's normal is cross(c - a, b - a)
	 * (FStaticMeshOperations::ComputeTriangleTangentsAndNormals). Triangles too small to carry a surface are dropped.
	 *
	 * A vertex is a corner with its attributes: surfaces meeting at a crease, or a UV seam, keep separate vertices at
	 * one position. Welding (Welding.h) finds the points they share.
	 */
	class FMeshBuilder
	{
	public:
		/** The most UV channels a vertex may carry: the engine's MAX_MESH_TEXTURE_COORDS_MD. */
		static constexpr int32 MaxUVChannels = 8;

		/**
		 * The channel that carries Matter's piece layout: each piece's PieceId, which Matter defines and writes. The
		 * kernel draws surfaces, not pieces, so it leaves this channel at zero.
		 */
		static constexpr int32 PieceLayoutChannel = 1;

		/** Every Form mesh carries UV0 in metres and the piece layout channel. */
		static constexpr int32 MinUVChannels = 2;

		/** A builder whose vertices carry UVChannelCount UV channels (MinUVChannels to MaxUVChannels). */
		explicit FMeshBuilder(int32 UVChannelCount = MinUVChannels);

		/**
		 * Makes SlotName the material slot for the triangles added from now on, and returns its index. A builder starts
		 * in one slot named NAME_None: the piece's own material.
		 */
		int32 UseSlot(FName SlotName);

		/** Adds a vertex and returns its index. The normal is normalised; its other UV channels start at zero. */
		int32 AddVertex(const FVector& PositionMeters, const FVector& Normal, const FVector2D& UVMeters);

		/** Sets one UV channel (1 or above) of a vertex. Channel 1 is the piece layout's (PieceLayoutChannel). */
		void SetUV(int32 Vertex, int32 Channel, const FVector2D& UV);

		/** Sets a vertex's linear colour. Once any vertex has a colour, every vertex has one (white unless set). */
		void SetColor(int32 Vertex, const FLinearColor& Color);

		/**
		 * Adds the triangle (A, B, C) in the current slot, wound to face along the sum of its vertices' normals. Returns
		 * false, adding nothing, when it is degenerate: an edge shorter than two weld tolerances (its corners would weld
		 * into one) or a height below one (it has no area).
		 */
		bool AddTriangle(int32 A, int32 B, int32 C);

		/** Adds the quad A, B, C, D (in order round its edge) as two triangles split along its shorter diagonal. */
		void AddQuad(int32 A, int32 B, int32 C, int32 D);

		/** Adds every vertex and triangle of Other, keeping its slots by name. */
		void Append(const FMeshBuilder& Other);

		/** Moves every vertex by an offset, metres. */
		void Translate(const FVector& OffsetMeters);

		int32 NumVertices() const { return Positions.Num(); }
		int32 NumTriangles() const { return Triangles.Num(); }
		int32 NumUVChannels() const { return UVs.Num(); }
		int32 NumDroppedTriangles() const { return DroppedTriangles; }
		/** Vertices added with no normal: a drawing fault the maker reports, never a mesh to cast. */
		int32 NumUnfacedVertices() const { return UnfacedVertices; }
		bool HasColors() const { return Colors.Num() > 0; }

		const FVector& Position(int32 Vertex) const { return Positions[Vertex]; }
		const FVector& Normal(int32 Vertex) const { return Normals[Vertex]; }
		const FVector2D& UV(int32 Vertex, int32 Channel = 0) const { return UVs[Channel][Vertex]; }
		const FLinearColor& Color(int32 Vertex) const { return Colors[Vertex]; }
		const FIntVector& Triangle(int32 Index) const { return Triangles[Index]; }
		int32 TriangleSlot(int32 Index) const { return TriangleSlots[Index]; }
		TConstArrayView<FName> Slots() const { return SlotNames; }
		TConstArrayView<FVector> AllPositions() const { return Positions; }
		TConstArrayView<FVector> AllNormals() const { return Normals; }
		TConstArrayView<FVector2D> AllUVs(int32 Channel = 0) const { return UVs[Channel]; }
		TConstArrayView<FIntVector> AllTriangles() const { return Triangles; }

	private:
		TArray<FVector> Positions;
		TArray<FVector> Normals;
		TArray<TArray<FVector2D>> UVs;
		TArray<FLinearColor> Colors;
		TArray<FIntVector> Triangles;
		TArray<int32> TriangleSlots;
		TArray<FName> SlotNames;
		int32 Slot = 0;
		int32 DroppedTriangles = 0;
		int32 UnfacedVertices = 0;
	};
}
