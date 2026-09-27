#pragma once

#include "CoreMinimal.h"

#include "NatureOutline.generated.h"

/**
 * The plan outline of a body of water, in its Water anchor's frame (the datum at the water's centre): an ellipse of
 * two semi-axes, a along the anchor's x and b along its y, or a polygon of plan points, closed from its last point to
 * its first, either way round. A planting plan writes one or the other:
 * `{ "ellipse": [a, b] }` or `{ "polygon": [[x, y], ...] }`.
 */
USTRUCT()
struct MUSEENATURE_API FNatureOutline
{
	GENERATED_BODY()

	/** The ellipse's semi-axes [a, b], in metres. */
	UPROPERTY()
	TArray<double> Ellipse;

	/** The polygon's corners, in metres. */
	UPROPERTY()
	TArray<FVector2D> Polygon;

	/** Whether the outline is an ellipse (rather than a polygon). */
	[[nodiscard]] bool IsEllipse() const { return !Ellipse.IsEmpty(); }

	/**
	 * Whether the outline is one proper shape: an ellipse of two semi-axes both more than zero, or a polygon of three
	 * corners or more that encloses an area, but not both.
	 */
	[[nodiscard]] bool IsValid() const;

	/** The ellipse's semi-axes, as a plan vector; zero for a polygon. */
	[[nodiscard]] FVector2D SemiAxes() const;

	/** Whether a plan point (metres, in the anchor's frame) lies inside the outline drawn in by a margin. */
	[[nodiscard]] bool Contains(const FVector2D& Point, double MarginMeters) const;

	/** The area of the water's surface, in square metres. */
	[[nodiscard]] double Area() const;
};
