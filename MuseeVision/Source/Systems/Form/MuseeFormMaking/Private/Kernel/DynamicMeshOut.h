#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::Form
{
	class FMeshBuilder;

	/** What writing a mesh found in it. */
	struct FDynamicMeshWriteReport
	{
		/** Triangles whose corners welded into fewer than three points. They bound nothing and are not written. */
		int32 Collapsed = 0;

		/**
		 * Triangles on an edge two others already bound, or repeating a triangle already written. FDynamicMesh3 is
		 * manifold, so each is written on its own copies of its points, and the shell query then reads it as open.
		 */
		int32 Separated = 0;
	};

	/**
	 * Writes a mesh as the FDynamicMesh3 that Studio's caster converts to a static mesh (MeshConversion's
	 * FDynamicMeshToMeshDescription, with tangents copied and never recomputed). Out is replaced. The contract:
	 *
	 * - Vertices are the mesh's welded points (Welding.h), so surfaces whose stations are shared share vertices and
	 *   the shell query (ShellEdges.h) reads the written mesh as it reads the builder. A position is metres times
	 *   PositionUnitsPerMeter: the caller passes the engine units in one metre (Foundation's Musee::ToEngine).
	 * - Triangles are the builder's, in its order and winding: Unreal's front face, whose normal is cross(c - a, b - a).
	 * - Overlay elements are the builder's vertices. Triangles that share a builder vertex share its elements, so
	 *   normals, tangents, UVs and colours split exactly where the builder split them (creases and UV seams).
	 * - Normal layer 0 is the unit normal. Layer 1 is the unit tangent: TangentX, along increasing U (Tangents.h).
	 *   Layer 2 is the unit bitangent, BinormalSign * cross(Normal, TangentX), along increasing V. The sign is what
	 *   VectorUtil::BitangentSign(Normal, Tangent, Bitangent) recovers: +1 where the UVs run the engine's way round,
	 *   -1 where they are mirrored.
	 * - UV layer k is the builder's channel k: layer 0 in metres, layer 1 Matter's piece layout (zero until Matter
	 *   writes it), and any further channels the drawing set. There are always at least two.
	 * - The primary colour overlay is linear RGBA, white unless the drawing set a colour: Matter's per-vertex facts.
	 * - MaterialID is the triangle's slot: an index into FMeshBuilder::Slots(), with 0 the piece's own material.
	 */
	FDynamicMeshWriteReport WriteDynamicMesh(const FMeshBuilder& Mesh, double PositionUnitsPerMeter,
		UE::Geometry::FDynamicMesh3& Out);
}
