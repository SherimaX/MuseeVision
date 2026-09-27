#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "FormExtrusion.h"
#include "MuseeFrame.h"

namespace Musee::ArtMaking
{
	/**
	 * The face a scroll is laid on, as the directions and points it is drawn in: a wall a hanging scroll hangs in front of,
	 * or a case's bed a handscroll lies open on (a Wall anchor either way): along it, out of it, up it.
	 */
	struct FScrollWall
	{
		const FMuseeFrame& Wall;
		FVector Along;
		FVector Out;
		FVector Up;

		explicit FScrollWall(const FMuseeFrame& InWall)
			: Wall(InWall), Along(InWall.DirectionToParent(FVector::ForwardVector)), Out(InWall.DirectionToParent(FVector::RightVector)),
			Up(InWall.DirectionToParent(FVector::UpVector))
		{
		}

		/** A point so far along the face, out of it and up it. */
		FVector At(double AlongMeters, double OutMeters, double UpMeters) const { return Wall.PointToParent(FVector(AlongMeters, OutMeters, UpMeters)); }
	};

	/** A drawing of Art's in its surface, colliding as a solid. */
	[[nodiscard]] FFormDrawing ArtDrawing(FName Name, FName Surface, FFormExtrusion&& Shape, const FMuseeFrame& Frame);

	/**
	 * A round rod from Start along the unit Axis for Length: a circle of its radius pushed along the axis. Across is any
	 * direction square to the axis, the circle's x.
	 */
	[[nodiscard]] FFormDrawing Rod(FName Name, FName Surface, const FVector& Start, const FVector& Axis, const FVector& Across, double Length, double Radius);

	/**
	 * A scroll's mount: a sheet of its silk the mount's size, the painting's window cut through it, its face on Face and its
	 * back the mount's thickness behind, drawn from its lower-left corner along the face and up it and pushed into it.
	 */
	[[nodiscard]] FFormDrawing MountSheet(FName Name, FName Surface, const FScrollWall& Wall, const FVector2D& Corner, const FVector2D& Size,
		const FVector2D& Window, const FVector2D& Painting, double Face, double Thickness);
}
