#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	class FMeshBuilder;

	/**
	 * A plane in space with two metre axes: where a flat face (a floor, a slab's soffit, a wall's face) is drawn in
	 * two dimensions. A point (u, v) of the drawing lies at Origin + u AxisU + v AxisV, and its UVs are (u, v) metres.
	 */
	struct FPlaneFrame
	{
		FVector Origin = FVector::ZeroVector;

		/** Unit: the drawing's first axis. */
		FVector AxisU = FVector::ForwardVector;

		/** Unit and perpendicular to AxisU: the drawing's second axis. */
		FVector AxisV = FVector::RightVector;

		/** Unit and perpendicular to both axes: the side the face is seen from. */
		FVector Normal = FVector::UpVector;

		FVector At(const FVector2D& Point) const { return Origin + AxisU * Point.X + AxisV * Point.Y; }
	};

	/**
	 * Adds a flat face with holes, drawn in the frame's (u, v) metres and facing its normal, with UVs of (u, v) metres.
	 * Returns false, adding nothing, when the outline cannot be triangulated (see TriangulatePolygon).
	 */
	bool AddPlanarFace(FMeshBuilder& Mesh, const FPlaneFrame& Plane, TConstArrayView<FVector2D> Outer,
		TConstArrayView<TArray<FVector2D>> Holes = {});

	/**
	 * Adds a flat polygon with holes drawn in 2D and carried into space by Place, every vertex facing Normal, with
	 * UVs from UVOf(drawn point). The general form of AddPlanarFace, for faces whose plane carries a sweep's frame.
	 */
	bool AddFlatPolygon(FMeshBuilder& Mesh, TConstArrayView<FVector2D> Outer, TConstArrayView<TArray<FVector2D>> Holes,
		TFunctionRef<FVector(const FVector2D&)> Place, const FVector& Normal,
		TFunctionRef<FVector2D(const FVector2D&)> UVOf);
}
