#pragma once

#include "CoreMinimal.h"
#include "Vessel/VesselSection.h"

namespace UE::Geometry
{
	class FDynamicMesh3;
}

namespace Musee::ArtMaking
{
	/**
	 * How a vessel's section is turned into its surface: smooth through a point where the section turns less than the
	 * crease angle and an arris where it turns more; divided round the axis so no chord strays from the true curve by
	 * more than the tolerance; and for an oval vessel, the turned section scaled across (x) and front to back (y) to its
	 * mouth's axes, as a potter's oval is thrown round and pressed to its plan.
	 */
	struct FVesselTurning
	{
		double CreaseDegrees = 20.0;
		double ChordToleranceMeters = 0.001;
		FVector2D Oval = FVector2D(1.0, 1.0);
	};

	/**
	 * Builds a vessel in its own frame (the foot's centre at the origin, x its right, y its front, z up), to the Form
	 * kernel's mesh contract: its closed section turned about z, a point (r, z) at the angle phi from the front towards
	 * the right landing at (r sin phi, r cos phi, z); UV0 its layout's (U the widest parallel's metres round from the
	 * front, V the section's metres along), so a decoration composed in that layout lies where it was photographed. A
	 * full turn keeps its UV seam at the front, its points shared.
	 */
	void BuildVessel(TConstArrayView<FVector2D> Section, const FVesselLayout& Layout, const FVesselTurning& Turning, UE::Geometry::FDynamicMesh3& Out);
}
