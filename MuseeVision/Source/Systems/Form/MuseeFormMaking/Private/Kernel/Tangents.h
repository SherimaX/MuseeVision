#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	class FMeshBuilder;

	/**
	 * A vertex's tangent frame, as a normal map reads it. TangentX points along increasing U. The binormal is
	 * cross(Normal, TangentX) * BinormalSign and points along increasing V: the engine's convention
	 * (FStaticMeshBuilder builds TangentY = (Normal ^ TangentX) * BinormalSign).
	 */
	struct FVertexTangent
	{
		FVector TangentX = FVector::ForwardVector;
		float BinormalSign = 1.0f;
	};

	/**
	 * Tangents derived from the UVs (channel 0), so a normal map lies on the surface the way its texture does.
	 *
	 * Each triangle's dP/dU and dP/dV are solved from its edges and its UV deltas. A vertex sums them over its
	 * triangles, each weighted by the angle of its corner there, and makes TangentX perpendicular to its normal. The
	 * sign is the UVs' handedness: +1 where increasing V lies on the cross(Normal, TangentX) side, -1 where the UVs
	 * are mirrored. A triangle whose UVs have no area gives no direction; a vertex that gets none takes a tangent
	 * perpendicular to its normal and sign +1.
	 */
	TArray<FVertexTangent> DeriveTangents(const FMeshBuilder& Mesh);
}
