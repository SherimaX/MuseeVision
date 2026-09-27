#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"
#include "MuseeFrame.h"

/** The kind of curved surface a drawing is set out on. */
enum class EFormGroundKind : uint8
{
	/** The drawing frame's own flat space: nothing is carried. */
	Flat,
	/** A surface turned about the drawing frame's z axis: a dome, a drum, a sphere's skin, a barrel vault. */
	Revolution,
	/** A sphere seen from its centre, developed on a plane tangent to it. */
	Sphere,
};

/**
 * The curved surface a drawing is set out on, when that isn't the flat space of its frame: the coffers sunk in a dome,
 * the seams of a sphere's skin, the crossed bars and hot-bent panes of a barrel vault, the panes of a glazed spherical
 * zone. The drawing is drawn as if the ground were flat, in the ground's development, where U and V span the ground and W
 * stands out of it along its normal (a drawing's x, y and z); every point is then carried onto the ground. A line drawn
 * straight in the development follows the ground, a face drawn flat lies on it (divided as finely as the chord tolerance
 * needs), and whatever stood square out of the development stands square out of the ground.
 *
 * - **Revolution:** the meridian, (A the radius from the drawing frame's z axis, never negative; B the height along it),
 *   turned about the axis. U is metres round the parallel of ReferenceRadiusMeters (the angle from the frame's x axis
 *   towards its y, times that radius), V is metres along the meridian from its start, and W stands out along the side
 *   the meridian faces (its right, as a section's does: a meridian drawn down a dome's inner face, from the crown
 *   towards the springing, faces the room). Lines of constant U are meridians and lines of constant V parallels, so a
 *   coffer whose sides lie in meridian planes and on parallels, as a dome's coffers are set out, is a rectangle in the
 *   development. Away from the reference parallel U stretches with the radius: its lines stay meridians.
 * - **Sphere:** the sphere on which Tangent's origin lies, RadiusMeters behind it along its z axis (so z points out of
 *   the sphere there), developed on the plane of Tangent's x and y: each point (U, V) of the plane is carried along its
 *   ray from the centre onto the sphere, and W is out along the radius. A straight line in the plane is a great circle,
 *   so a pane cut on straight lines has edges square to its glass, as a glazier cuts a spherical pane on planes through
 *   the centre. A length in the plane is true at the origin and shortens away from it, by the square of the cosine of
 *   its angle from the origin along a ray (0.3 % at 0.8 m on a sphere of 14 m).
 */
struct FFormGround
{
	EFormGroundKind Kind = EFormGroundKind::Flat;

	/** Revolution: the meridian, (radius, height) in metres, in the drawing's frame. */
	FFormProfile Meridian;

	/** Revolution: the parallel along which U is true metres. */
	double ReferenceRadiusMeters = 0.0;

	/** Sphere: the tangent plane of the development, its z out of the sphere, in the drawing's frame. */
	FMuseeFrame Tangent;

	/** Sphere: the sphere's radius, metres. */
	double RadiusMeters = 0.0;

	/** A ground of revolution. */
	[[nodiscard]] static FFormGround Revolved(FFormProfile InMeridian, double InReferenceRadiusMeters)
	{
		FFormGround Ground;
		Ground.Kind = EFormGroundKind::Revolution;
		Ground.Meridian = MoveTemp(InMeridian);
		Ground.ReferenceRadiusMeters = InReferenceRadiusMeters;
		return Ground;
	}

	/** A sphere developed on its tangent plane at a point. */
	[[nodiscard]] static FFormGround OnSphere(const FMuseeFrame& InTangent, double InRadiusMeters)
	{
		FFormGround Ground;
		Ground.Kind = EFormGroundKind::Sphere;
		Ground.Tangent = InTangent;
		Ground.RadiusMeters = InRadiusMeters;
		return Ground;
	}

	/** Whether the drawing stands in its frame's flat space. */
	[[nodiscard]] bool IsFlat() const { return Kind == EFormGroundKind::Flat; }
};
