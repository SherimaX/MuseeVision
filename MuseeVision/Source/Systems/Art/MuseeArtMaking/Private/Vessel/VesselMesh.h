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
	 * more than the tolerance; and for an oval vessel, the turned section pressed front to back (y) by its pressing's
	 * ratio at each height (FVesselPress), as a potter's oval is thrown round and pressed to its plan.
	 */
	struct FVesselTurning
	{
		double CreaseDegrees = 20.0;
		double ChordToleranceMeters = 0.001;
		FVesselPress Press;
	};

	/**
	 * How a vessel is turned at the size its plan shows it (SizedSection): the record's crease angle, the chord tolerance,
	 * and its pressing at that size (PressOf). Empty if so; else why it can't be pressed.
	 */
	[[nodiscard]] FString TurningOf(const FArtVessel& Vessel, const FVector2D& PlanMeters, double CreaseDegrees, double ChordToleranceMeters,
		FVesselTurning& Out);

	/**
	 * Builds a vessel in its own frame (the foot's centre at the origin, x its right, y its front, z up), to the Form
	 * kernel's mesh contract: its closed section turned about z, a point (r, z) at the angle phi from the front towards
	 * the right landing at (r s sin phi, k(z) r s cos phi, z), k its pressing's ratio (one for a round vessel) and s the
	 * way out along that bearing to its plan's superellipse (one for an ellipse: FVesselPress::Pressed); UV0 its
	 * layout's (U the widest parallel's metres round from the front, V the section's metres along), so a decoration
	 * composed in that layout lies where it was photographed. A full turn keeps its UV seam at the front, its points
	 * shared.
	 */
	void BuildVessel(TConstArrayView<FVector2D> Section, const FVesselLayout& Layout, const FVesselTurning& Turning, UE::Geometry::FDynamicMesh3& Out);
}
