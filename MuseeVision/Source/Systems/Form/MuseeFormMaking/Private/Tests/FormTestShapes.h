#pragma once

#include "CoreMinimal.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Profile.h"

/**
 * Shapes and measures the Form kernel's tests share: a triangle's face normal under the engine's convention, and
 * the sections the tests sweep and turn.
 */
namespace Musee::Form::Tests
{
	/** The unit normal a triangle faces in Unreal: cross(c - a, b - a). */
	inline FVector FaceNormal(const FMeshBuilder& Mesh, int32 Triangle)
	{
		const FIntVector& T = Mesh.Triangle(Triangle);
		const FVector A = Mesh.Position(T.X);
		return FVector::CrossProduct(Mesh.Position(T.Z) - A, Mesh.Position(T.Y) - A).GetSafeNormal();
	}

	/** The angle between two directions, in radians. */
	inline double AngleBetween(const FVector& A, const FVector& B)
	{
		return FMath::Atan2(FVector::CrossProduct(A, B).Size(), FVector::DotProduct(A, B));
	}

	/**
	 * An open cornice section, A out of the wall and B up, drawn with its solid on the left: a fillet out along the
	 * soffit, a quarter-round ovolo meeting it and the face above tangent (smooth by the drawing), then a crisp arris
	 * at the top and back to the wall.
	 */
	inline FProfile OvoloCornice()
	{
		FProfile Profile(FVector2D(0.0, 0.0));
		Profile.LineTo(FVector2D(0.10, 0.0))
			.Arc(FVector2D(0.10, 0.10), 0.10, -UE_DOUBLE_HALF_PI, 0.0)
			.LineTo(FVector2D(0.20, 0.20))
			.LineTo(FVector2D(0.0, 0.20));
		return Profile;
	}

	/**
	 * A closed, concave section of a solid moulding: a block 0.3 m by 0.3 m with a cove (a quarter circle of 0.2 m
	 * radius) cut from its upper outer corner, drawn counter-clockwise so its solid is on the left.
	 */
	inline FProfile CoveBlock()
	{
		FProfile Profile(FVector2D(0.0, 0.0));
		Profile.LineTo(FVector2D(0.30, 0.0))
			.LineTo(FVector2D(0.30, 0.10))
			.Arc(FVector2D(0.30, 0.30), 0.20, -UE_DOUBLE_HALF_PI, -UE_DOUBLE_PI)
			.LineTo(FVector2D(0.0, 0.30))
			.Close();
		return Profile;
	}
}
