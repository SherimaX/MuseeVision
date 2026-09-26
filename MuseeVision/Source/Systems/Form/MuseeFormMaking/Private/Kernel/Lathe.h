#pragma once

#include "CoreMinimal.h"
#include "Kernel/SweepFrame.h"
#include "Stations/Stations.h"

namespace Musee::Form
{
	class FMeshBuilder;
	struct FProfileStations;

	/**
	 * The axis a lathe turns about: a dome's, a drum's, a column's, a baluster's. Angles turn from Zero towards
	 * Up x Zero: with Up the vertical, x east and y south, from east towards south.
	 */
	struct FLatheAxis
	{
		/** A point on the axis, metres: where the profile's (0, 0) turns. */
		FVector Origin = FVector::ZeroVector;

		/** Unit: the axis, and the profile's B. */
		FVector Up = FVector::UpVector;

		/** Unit and perpendicular to Up: the profile's A at angle zero. */
		FVector Zero = FVector::ForwardVector;

		FVector Radial(double AngleRadians) const;
		FVector Tangent(double AngleRadians) const;
	};

	/** The profile's largest distance from the axis, in metres: its widest parallel. */
	double LargestRadius(const FProfileStations& Profile);

	/**
	 * The angles a lathe turns its profile through: stations on the arc at the profile's largest radius, so every
	 * parallel keeps within the tolerance, with every required angle among them (the edges of an opening, the joints
	 * of another surface that meets this one). A surface that meets the lathe takes the same list.
	 */
	TArray<double> LatheStations(const FProfileStations& Profile, double FromRadians, double ToRadians,
		double ToleranceMeters = DefaultChordToleranceMeters, TConstArrayView<double> RequiredRadians = {});

	/**
	 * The frames of a lathe at the given angles. A profile point (r, h) lands at Origin + r Radial + h Up. Its U is
	 * the arc length along the reference parallel, ReferenceRadiusMeters times its angle: metres round the circle,
	 * never a 0 to 1 stretched round it. The whole meridian shares its U, so the texture never shears (FSweepFrame);
	 * a parallel of radius r carries it at the scale r / reference. Surfaces that meet round a parallel (a drum and its
	 * dome) take the same reference, so their UVs run on across the joint. A full turn keeps a UV seam where it returns
	 * to its start, with its points shared.
	 */
	TArray<FSweepFrame> LatheFrames(const FLatheAxis& Axis, TConstArrayView<double> AnglesRadians,
		double ReferenceRadiusMeters);

	/**
	 * Turns a profile (A the radius from the axis, never negative; B the height along it) through the angles, with its
	 * largest radius as the UV reference: metres exact along its widest parallel.
	 */
	void Lathe(FMeshBuilder& Mesh, const FProfileStations& Profile, const FLatheAxis& Axis,
		TConstArrayView<double> AnglesRadians);
}
