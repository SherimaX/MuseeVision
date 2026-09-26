#pragma once

#include "CoreMinimal.h"
#include "Kernel/Tolerances.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::Form
{
	class FMeshBuilder;
	struct FWeld;

	/**
	 * A mesh read as a shell, edge by edge over its points. A closed shell (a solid's whole skin, with nothing for
	 * light or water to pass through) has every edge shared by exactly two triangles that run it in opposite
	 * directions. Form.Sealed reads this over each enclosure; the boundary edges are where it is open.
	 */
	struct FShellEdges
	{
		/** Points used by at least one triangle. */
		int32 Points = 0;

		/** Distinct edges between points. */
		int32 Edges = 0;

		/** Triangles, not counting any whose corners stand on fewer than three points. */
		int32 Faces = 0;

		/** Triangles whose corners stand on fewer than three points: they bound nothing. */
		int32 Collapsed = 0;

		/** Edges used by one triangle only: the shell is open along them. */
		int32 Boundary = 0;

		/** Edges used by more than two triangles. */
		int32 NonManifold = 0;

		/** Edges used by two triangles running them the same way: one of the two faces is turned inside out. */
		int32 Misoriented = 0;

		/** The boundary edges, as point pairs in the direction their one triangle runs them. */
		TArray<FIntPoint> BoundaryEdges;

		/** True when the mesh is one or more closed, consistently oriented shells. */
		bool IsClosed() const { return Faces > 0 && Boundary == 0 && NonManifold == 0 && Misoriented == 0; }

		/** Points - Edges + Faces: 2 for each closed shell of genus zero (a block), 0 for a ring (a torus). */
		int32 EulerCharacteristic() const { return Points - Edges + Faces; }
	};

	/** Reads triangles given as the indices of the points they stand on. */
	FShellEdges MeasureShellEdges(TConstArrayView<FIntVector> PointTriangles);

	/** Reads a mesh's edges over a weld of it. */
	FShellEdges MeasureShellEdges(const FMeshBuilder& Mesh, const FWeld& Weld);

	/** Welds a mesh at the tolerance and reads its edges. */
	FShellEdges MeasureShellEdges(const FMeshBuilder& Mesh, double WeldMeters = WeldToleranceMeters);

	/** Reads a written mesh (DynamicMeshOut.h), whose vertices are already the welded points. */
	FShellEdges MeasureShellEdges(const UE::Geometry::FDynamicMesh3& Mesh);
}
