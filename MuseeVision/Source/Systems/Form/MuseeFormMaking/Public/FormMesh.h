#pragma once

#include "CoreMinimal.h"
#include "Templates/UniquePtr.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::Form
{
	class FMeshBuilder;
}

/**
 * The surface of a made piece as a making module draws it by hand (a grown leaf, a canvas, a body of water): the Form
 * kernel's one mesh builder, offered to the systems that add pieces, so every made mesh welds, winds and takes its
 * tangents the one way (Private/Kernel/DynamicMeshOut.h).
 *
 * Positions are metres in the piece's frame (x east, y south, z up). Every vertex has a unit normal facing the side the
 * surface is seen from, UV0 in metres (so a material's texel density is physical), UV1 for Matter's piece layout (left
 * at zero: Matter writes it) and a linear colour (white unless set: the drawing's own per-vertex facts, such as a
 * leaf's tint with its occlusion in alpha). Triangles face along their vertices' normals, in the material slot in use
 * when they were added; one too small to carry a surface is dropped. Vertices closer than a micrometre are welded
 * into one point when the mesh is written, while each keeps its own normal and UVs (a crease, a seam).
 */
class MUSEEFORMMAKING_API FFormMesh
{
public:
	FFormMesh();
	~FFormMesh();
	FFormMesh(const FFormMesh& Other);
	FFormMesh(FFormMesh&& Other);
	FFormMesh& operator=(const FFormMesh& Other);
	FFormMesh& operator=(FFormMesh&& Other);

	/**
	 * Makes Surface the material slot for the triangles added from now on, and returns its index. A mesh starts in slot
	 * 0, named NAME_None: the piece's own surface. The written mesh's MaterialID is this index.
	 */
	int32 UseSlot(FName Surface);

	/** Adds a vertex and returns its index. Its normal is normalised; a zero normal is refused by the kernel. */
	int32 AddVertex(const FVector& PositionMeters, const FVector& Normal, const FVector2D& UVMeters);

	/** Sets a vertex's linear colour. */
	void SetColor(int32 Vertex, const FLinearColor& Color);

	/** Adds a triangle facing along its vertices' normals; false, adding nothing, when it is too small to carry a surface. */
	bool AddTriangle(int32 A, int32 B, int32 C);

	/** Adds the quad A, B, C, D (in order round its edge) as two triangles split along its shorter diagonal. */
	void AddQuad(int32 A, int32 B, int32 C, int32 D);

	/** Adds every vertex and triangle of Other, keeping its slots by name. */
	void Append(const FFormMesh& Other);

	/** Moves every vertex by an offset, metres. */
	void Translate(const FVector& OffsetMeters);

	[[nodiscard]] int32 NumVertices() const;
	[[nodiscard]] int32 NumTriangles() const;
	[[nodiscard]] bool IsEmpty() const { return NumTriangles() == 0; }
	[[nodiscard]] const FVector& Position(int32 Vertex) const;
	[[nodiscard]] TConstArrayView<FVector> AllPositions() const;

	/** The slots' surfaces by index, slot 0 first (NAME_None: the piece's own). */
	[[nodiscard]] TConstArrayView<FName> Slots() const;

	/**
	 * Writes the mesh as a made piece's FDynamicMesh3, to the kernel's contract (FormMadePiece.h): welded points in
	 * engine units, normals, UV-derived tangents and bitangents in normal layers 0 to 2, UV layers 0 and 1, the colours,
	 * and MaterialID the slot. Out is replaced.
	 */
	void Write(UE::Geometry::FDynamicMesh3& Out) const;

private:
	TUniquePtr<Musee::Form::FMeshBuilder> Builder;
};
