#pragma once

#include "ArtWallPath.h"
#include "CoreMinimal.h"
#include "MuseeFrame.h"

namespace Musee::ArtMaking
{
	/**
	 * A hanging wall's face on plan, walked by distance: a straight wall along its Wall anchor's x axis, or a curved one
	 * along its ellipse (FArtWallPath), where distance is arc length from the anchor. At each distance it gives the
	 * point of the face (in the stage's frame, at the anchor's floor level), the direction the wall runs, and the
	 * direction out of the wall into the room.
	 */
	class FWallPath
	{
	public:
		/** A straight wall: the anchor's own frame. */
		explicit FWallPath(const FMuseeFrame& Anchor);

		/**
		 * A curved wall starting at the anchor. The room lies on the side of the face the anchor's y axis points to;
		 * Faults says if the anchor doesn't stand on the path.
		 */
		FWallPath(const FMuseeFrame& Anchor, const FArtWallPath& Path);

		struct FStation
		{
			FVector Point = FVector::ZeroVector;
			FVector Along = FVector::ForwardVector;
			FVector Out = FVector::RightVector;
		};

		/** The face at a distance from the anchor. */
		[[nodiscard]] FStation At(double DistanceMeters) const;

		/** The spacing of stations that keeps chords within the museum's chord tolerance of the face (the whole span when straight). */
		[[nodiscard]] double StationSpacing(double SpanMeters) const;

		/** The length of the face from the anchor to where the path closes (unbounded for a straight wall). */
		[[nodiscard]] double Length() const;

		/** What is wrong with the path, or empty. */
		[[nodiscard]] const FString& Faults() const { return Fault; }

	private:
		FMuseeFrame Anchor;
		bool bCurved = false;
		FVector2D Axes = FVector2D::ZeroVector;
		double Direction = 1.0;
		double RoomSide = 1.0;
		/** Arc length at evenly spaced parameter steps from the start, one full turn. */
		TArray<double> ArcAt;
		double StartTheta = 0.0;
		double ThetaStep = 0.0;
		FString Fault;

		[[nodiscard]] double ThetaAt(double DistanceMeters) const;
	};
}
