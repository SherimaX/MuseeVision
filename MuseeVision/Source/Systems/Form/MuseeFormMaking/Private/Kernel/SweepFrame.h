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
	 * A section that doesn't stand at right angles to the travel (a plumb section on a rising path: a string's board up
	 * a helix) has its surface normal out of its plane. At a point a along AxisA the level travel is shortened by the
	 * factor 1 - a Bend (Bend the plan curvature of the reference line, positive when it turns towards AxisA) while the
	 * rise over it is not, so the normal there is na NormalA + nb (NormalB + Rake / (1 - a Bend)), where Rake is minus the
	 * grade times the level direction of travel. For a section square to the travel Rake is zero and Bend is unused.
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

		/** A plumb section's lean of its B normal against the rake: minus the grade times the level travel. Zero otherwise. */
		FVector Rake = FVector::ZeroVector;

		/** The plan curvature of the reference line, per metre, positive when it turns towards AxisA. */
		double Bend = 0.0;

		/** Unit: the direction of travel. A cap at a run's start faces back along it, one at its end forward. */
		FVector Along = FVector::RightVector;

		/** Metres travelled along the reference line: the U of every point of the section. */
		double U = 0.0;

		FVector At(const FVector2D& Point) const { return Origin + AxisA * Point.X + AxisB * Point.Y; }

		/** The surface normal of a section drawn square to the travel, from its 2D normal. */
		FVector NormalAt(const FVector2D& Normal) const
		{
			return (NormalA * Normal.X + NormalB * Normal.Y).GetSafeNormal();
		}

		/** The surface normal at a point of the section, from its 2D normal: exact for a raked plumb section too. */
		FVector NormalAt(const FVector2D& Point, const FVector2D& Normal) const
		{
			if (Rake.IsZero())
			{
				return NormalAt(Normal);
			}
			// The travel shortens to nothing only at the turn's centre, which no plumb section of a buildable path reaches.
			const double Travel = FMath::Max(1.0 - Point.X * Bend, UE_DOUBLE_KINDA_SMALL_NUMBER);
			return (NormalA * Normal.X + (NormalB + Rake / Travel) * Normal.Y).GetSafeNormal();
		}
	};
}
