#pragma once

#include "CoreMinimal.h"
#include "MuseeUnits.h"

#include "MuseeFrame.generated.h"

/**
 * A local frame in metres: where a stage, a body, a luminaire or a seat stands inside its parent, and how it is
 * turned. A stage's own frame has x pointing east, y south and z up, and plan north is true north, so a stage laid
 * out on its plan is already oriented to the real sun. These are the engine's axes too (Unreal is left-handed with
 * x forward, y right and z up), so passing a frame to the engine changes only the unit.
 *
 * A frame is rigid: it has no scale. Compose frames child into parent; the engine sees a frame only through ToEngine.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeFrame
{
	GENERATED_BODY()

	/** Where the frame's origin lies in its parent, in metres. */
	UPROPERTY(meta = (Units = "Meters"))
	FVector OriginMeters = FVector::ZeroVector;

	/** How the frame's axes are turned in its parent; the identity keeps x east, y south and z up. Always unit length. */
	UPROPERTY()
	FQuat Rotation = FQuat::Identity;

	/** The parent's own frame: no offset, no turn. */
	FMuseeFrame() = default;

	/** A frame at a point of its parent, turned by a rotation (normalised here). */
	FMuseeFrame(const FVector& InOriginMeters, const FQuat& InRotation);

	/** A frame at a point of its parent, with the parent's axes. */
	explicit FMuseeFrame(const FVector& InOriginMeters);

	/** Plan north, the direction of the true north pole along the ground: -y. */
	[[nodiscard]] static FVector North() { return FVector(0.0, -1.0, 0.0); }
	/** East: +x. */
	[[nodiscard]] static FVector East() { return FVector(1.0, 0.0, 0.0); }
	/** South: +y. */
	[[nodiscard]] static FVector South() { return FVector(0.0, 1.0, 0.0); }
	/** West: -x. */
	[[nodiscard]] static FVector West() { return FVector(-1.0, 0.0, 0.0); }
	/** Up, against gravity: +z. */
	[[nodiscard]] static FVector Up() { return FVector(0.0, 0.0, 1.0); }

	/** A point given in this frame (metres), in the parent. */
	[[nodiscard]] FVector PointToParent(const FVector& PointMeters) const;

	/** A point given in the parent (metres), in this frame. */
	[[nodiscard]] FVector PointFromParent(const FVector& PointMeters) const;

	/** A direction given in this frame, in the parent: turned, never moved. */
	[[nodiscard]] FVector DirectionToParent(const FVector& Direction) const;

	/** A direction given in the parent, in this frame. */
	[[nodiscard]] FVector DirectionFromParent(const FVector& Direction) const;

	/** A child frame given in this frame, in the parent: a body's frame in its stage, placed into the museum. */
	[[nodiscard]] FMuseeFrame FrameToParent(const FMuseeFrame& Child) const;

	/** A frame given in the parent, in this frame. */
	[[nodiscard]] FMuseeFrame FrameFromParent(const FMuseeFrame& Other) const;

	/** The parent's frame seen from this one. */
	[[nodiscard]] FMuseeFrame Inverse() const;

	/** Whether two frames coincide within a distance between origins and an angle between rotations. */
	[[nodiscard]] bool Equals(const FMuseeFrame& Other, Musee::Meters Tolerance, Musee::Radians AngleTolerance) const;
};

namespace Musee
{
	/** A frame as an engine transform: the origin in engine units, the same rotation, unit scale. */
	[[nodiscard]] MUSEEFOUNDATION_API FTransform ToEngine(const FMuseeFrame& Frame);

	/**
	 * An engine transform as a frame. A frame has no scale, and a transform with scale is not a frame of the museum: it
	 * is reported by an ensure and its scale is left behind.
	 */
	[[nodiscard]] MUSEEFOUNDATION_API FMuseeFrame FromEngine(const FTransform& Transform);
}
