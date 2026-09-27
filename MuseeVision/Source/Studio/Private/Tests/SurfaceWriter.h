#pragma once

#include "CoreMinimal.h"
#include "MeshDescription.h"

namespace Musee::Studio
{
	/**
	 * Writes a surface into a mesh description the way a drawing hands one to the caster: positions in metres, every
	 * corner with its unit normal, unit tangent and a UV in metres, and corners at one position sharing one vertex.
	 * Studio's proof pieces are drawn with it.
	 */
	class FSurfaceWriter
	{
	public:
		/** One corner of a face: where it is, which way the surface faces, and how its texture runs. */
		struct FCorner
		{
			FVector PositionMeters = FVector::ZeroVector;
			FVector Normal = FVector::UpVector;
			/** The direction in which U grows. */
			FVector Tangent = FVector::ForwardVector;
			/** The direction in which V grows; it decides the binormal sign. */
			FVector Bitangent = FVector::RightVector;
			FVector2D UVMeters = FVector2D::ZeroVector;
		};

		explicit FSurfaceWriter(FMeshDescription& InDescription);

		/** Adds a polygon group, filled by the slot of the same index. */
		FPolygonGroupID AddGroup(FName SlotName);

		/** A triangle, wound so that it faces the way its corners' normals do. */
		void Triangle(FPolygonGroupID Group, const FCorner& A, const FCorner& B, const FCorner& C);

		/** A quad A-B-C-D around its edge, as two triangles. */
		void Quad(FPolygonGroupID Group, const FCorner& A, const FCorner& B, const FCorner& C, const FCorner& D);

		/**
		 * A flat face cut into cells: from Origin, along AxisU through the offsets UCuts and along AxisV through VCuts
		 * (both in metres, first 0). Cells for which Skip returns true are left open. UVs are the planar projection onto
		 * the two axes, so neighbouring faces continue each other's texture.
		 */
		void Face(FPolygonGroupID Group, const FVector& OriginMeters, const FVector& AxisU, TConstArrayView<double> UCuts,
			const FVector& AxisV, TConstArrayView<double> VCuts, const FVector& Normal, TFunctionRef<bool(int32, int32)> Skip);

	private:
		FVertexInstanceID Instance(const FCorner& Corner);

		FMeshDescription& Description;
		TMap<FVector3f, FVertexID> Welded;
	};
}
