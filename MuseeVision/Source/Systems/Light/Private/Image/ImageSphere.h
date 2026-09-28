#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::Light
{
	/**
	 * The surface of an image that encloses its luminaire: the inside of the sphere of a radius about its seat, less the cap
	 * a round opening of a radius cuts at its foot, facing its centre. Rings of latitude from its crown down to the
	 * opening's edge, so the edge is drawn exactly; points in the engine's centimetres, in the luminaire's frame.
	 *
	 * The sphere is the nearest the surface comes to its centre: a flat face between points of a sphere cuts inside it, so
	 * the points stand out from it, at the radius over the cosine of the angle from a face's axis to its corners (half the
	 * angle of its longest chord, for these faces), taken for the face that cuts deepest; no point of any face comes nearer
	 * the centre than the radius.
	 */
	void BuildImageSphere(double RadiusCentimeters, double OpeningCentimeters, UE::Geometry::FDynamicMesh3& Out);

	/** How near its centre a point of a triangle comes: the distance from the origin to the nearest point of the triangle. */
	double NearestToCentre(const FVector3d& A, const FVector3d& B, const FVector3d& C);
}
