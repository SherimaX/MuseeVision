#pragma once

#include "CoreMinimal.h"

#include "NatureArc.generated.h"

/**
 * An arc of a circle in the stage's plan that a hedge is planted along (the grounds' yew exedra): its centre [x, y] and
 * radius in metres, and the angles it runs from and to, in degrees from the stage's +x towards +y (south), spelt as the
 * planting plan writes them, `from_deg` and `to_deg`. The hedge is set out along it as chords, one to each plant.
 */
USTRUCT()
struct MUSEENATURE_API FNatureArc
{
	GENERATED_BODY()

	/** The centre [x, y], in metres. */
	UPROPERTY()
	TArray<double> Centre;

	UPROPERTY(meta = (Units = "Meters"))
	float R = 0.0f;

	UPROPERTY(meta = (Units = "Degrees"))
	float From_Deg = 0.0f;

	UPROPERTY(meta = (Units = "Degrees"))
	float To_Deg = 0.0f;

	/** Whether the plan gives an arc at all. */
	[[nodiscard]] bool IsSet() const { return !Centre.IsEmpty() || R != 0.0f; }

	/** Whether it is one proper arc: a centre of two numbers, a radius of more than zero, and some sweep. */
	[[nodiscard]] bool IsValid() const { return Centre.Num() == 2 && R > 0.0f && From_Deg != To_Deg; }

	/** Its point at a share of its sweep (0 at its start, 1 at its end), in the stage's plan. */
	[[nodiscard]] FVector2D At(double Share) const
	{
		const double Angle = FMath::DegreesToRadians(FMath::Lerp(double(From_Deg), double(To_Deg), Share));
		return FVector2D(Centre[0] + R * FMath::Cos(Angle), Centre[1] + R * FMath::Sin(Angle));
	}
};
