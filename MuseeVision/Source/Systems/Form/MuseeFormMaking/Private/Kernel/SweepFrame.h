#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	/**
	 * Where a profile lands at one station of a sweep: the plane its (A, B) drawing is laid in, the basis its normals
	 * turn by, the direction the sweep travels, and the U of its UVs.
	 *
	 * A profile point (a, b) lands at Origin + a AxisA + b AxisB. At a mitre AxisA is stretched by 1 / cos(half the
	 * turn), so the moulding keeps its section on both sides of the cut. Its 2D normal (na, nb) turns into
	 * na NormalA + nb NormalB: the swept surface's own normal, since for a section at right angles to the travel the
	 * surface normal lies in the section's plane.
	 *
	 * U is the distance travelled, in metres, along the sweep's reference line (the profile's origin), and the whole
	 * section shares it. Lines of constant U are therefore the sections themselves and the texture never shears, however
	 * far the sweep turns. The price is a stretch at an offset from a curved path: an offset a towards the centre of a
	 * turn of radius R is shorter by the factor 1 - a / R than its U says (7 percent for a 0.5 m cornice on a 7 m
	 * drum). A shear would grow without bound round a ring; the stretch stays that small.
	 */
	struct FSweepFrame
	{
		FVector Origin = FVector::ZeroVector;
		FVector AxisA = FVector::ForwardVector;
		FVector AxisB = FVector::UpVector;

		/** Unit and perpendicular to Along: the basis the profile's normals are carried by. */
		FVector NormalA = FVector::ForwardVector;
		FVector NormalB = FVector::UpVector;

		/** Unit: the direction of travel. A cap at a run's start faces back along it, one at its end forward. */
		FVector Along = FVector::RightVector;

		/** Metres travelled along the reference line: the U of every point of the section. */
		double U = 0.0;

		FVector At(const FVector2D& Point) const { return Origin + AxisA * Point.X + AxisB * Point.Y; }
		FVector NormalAt(const FVector2D& Normal) const
		{
			return (NormalA * Normal.X + NormalB * Normal.Y).GetSafeNormal();
		}
	};
}
