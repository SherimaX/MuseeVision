#pragma once

#include "CoreMinimal.h"
#include "Kernel/Tolerances.h"

namespace Musee::Form
{
	class FMeshBuilder;

	/**
	 * The points a mesh stands on. Where surfaces meet, their vertices keep their own normals and UVs but stand on one
	 * point: vertices closer than the weld tolerance are one point. The cast mesh's vertices are these points, and
	 * its vertex instances are the builder's vertices.
	 */
	struct FWeld
	{
		/** For each builder vertex, the index of the point it stands on. */
		TArray<int32> PointOfVertex;

		/** The points, in metres: each is the position of the first vertex that stood on it. */
		TArray<FVector> Points;
	};

	/**
	 * Welds a mesh's vertices into points. Each vertex joins the lowest-numbered earlier point within the tolerance,
	 * or starts a point of its own, so the result depends only on the mesh.
	 */
	FWeld WeldVertices(const FMeshBuilder& Mesh, double ToleranceMeters = WeldToleranceMeters);
}
