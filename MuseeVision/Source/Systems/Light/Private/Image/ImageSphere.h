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
	 */
	void BuildImageSphere(double RadiusCentimeters, double OpeningCentimeters, UE::Geometry::FDynamicMesh3& Out);
}
