#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "MuseeUnits.h"

#include "MuseeContact.generated.h"

/**
 * A body struck a surface: a footfall on travertine, a door meeting its stop, a pebble dropped in the pond. Whoever
 * moves the body publishes it; Sound voices it by substance and speed, and Matter may mark the surface.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeContact
{
	GENERATED_BODY()

	/** The body that struck (`visitor.body`, or a mechanism's moving body). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Body;

	/** The substance of the surface struck (`travertine`, `oak`). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Substance;

	/** Where the contact happened, in the museum's frame. */
	UPROPERTY(meta = (Units = "Meters"))
	FVector LocationMeters = FVector::ZeroVector;

	/** The speed at which the body met the surface, along the surface's normal. */
	UPROPERTY(meta = (MuseeRequired, Units = "MetersPerSecond"))
	float SpeedMetersPerSecond = 0.0f;

	[[nodiscard]] Musee::MetersPerSecond Speed() const { return Musee::MetersPerSecond(SpeedMetersPerSecond); }
};
