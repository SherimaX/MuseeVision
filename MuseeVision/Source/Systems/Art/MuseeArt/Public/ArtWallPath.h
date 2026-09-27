#pragma once

#include "CoreMinimal.h"

#include "ArtWallPath.generated.h"

/**
 * The path of a curved hanging wall's face on plan, as a hanging plan writes it: an ellipse centred on the stage's
 * origin, x = a cos(theta), y = b sin(theta) in the stage's frame (a circle is [r, r]), theta the parametric angle from
 * +x towards +y (south). The wall starts at theta `from_deg` and runs with theta `increasing` or `decreasing`
 * (`clockwise` true on a north-up plan is increasing).
 */
USTRUCT()
struct MUSEEART_API FArtWallPath
{
	GENERATED_BODY()

	/** The ellipse's semi-axes [a, b], in metres. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Ellipse;

	/** Where the wall starts: the parametric angle, in degrees. Spelt as the hanging plan's key, `from_deg`. */
	UPROPERTY(meta = (MuseeRequired, Units = "Degrees"))
	float From_Deg = 0.0f;

	/** `increasing` or `decreasing`. */
	UPROPERTY()
	FName Direction;

	/** The same as `increasing` when true, `decreasing` when false; `direction` wins where both are written. */
	UPROPERTY()
	bool Clockwise = false;

	/** Whether the path runs with theta increasing. */
	[[nodiscard]] bool IsIncreasing() const { return Direction.IsNone() ? Clockwise : Direction == TEXT("increasing"); }
};
