#pragma once

#include "CoreMinimal.h"

#include "NatureOutline.generated.h"

/**
 * The plan outline of a body of water, in its Water anchor's frame (the datum at the water's centre): an ellipse of
 * two semi-axes, a along the anchor's x and b along its y.
 */
USTRUCT()
struct MUSEENATURE_API FNatureOutline
{
	GENERATED_BODY()

	/** The ellipse's semi-axes [a, b], in metres. */
	UPROPERTY()
	TArray<double> Ellipse;

	/** Whether the outline is a proper ellipse (two semi-axes, both more than zero). */
	[[nodiscard]] bool IsValid() const { return Ellipse.Num() == 2 && Ellipse[0] > 0.0 && Ellipse[1] > 0.0; }

	/** The semi-axes, as a plan vector. */
	[[nodiscard]] FVector2D SemiAxes() const { return IsValid() ? FVector2D(Ellipse[0], Ellipse[1]) : FVector2D::ZeroVector; }

	/** Whether a plan point (metres, in the anchor's frame) lies inside the outline drawn in by a margin. */
	[[nodiscard]] bool Contains(const FVector2D& Point, double MarginMeters) const
	{
		const FVector2D Axes = SemiAxes() - FVector2D(MarginMeters);
		return Axes.X > 0.0 && Axes.Y > 0.0 && FMath::Square(Point.X / Axes.X) + FMath::Square(Point.Y / Axes.Y) <= 1.0;
	}

	/** The area of the water's surface, in square metres. */
	[[nodiscard]] double Area() const { return UE_DOUBLE_PI * SemiAxes().X * SemiAxes().Y; }
};
