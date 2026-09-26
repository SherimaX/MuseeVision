#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "MuseeUnits.h"

#include "MuseeActuation.generated.h"

/**
 * A mechanism moved: its drive ran at a speed while carrying a load. Motion publishes it while the mechanism travels;
 * Sound gives the drive its voice on the moving body, found by the same key.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeActuation
{
	GENERATED_BODY()

	/** The mechanism that moved. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Mechanism;

	/** The speed of its driven point along its travel; zero when it stops. */
	UPROPERTY(meta = (MuseeRequired, Units = "MetersPerSecond"))
	float SpeedMetersPerSecond = 0.0f;

	/** The mass the drive carries (a lift's car and riders, a cylinder's stone), as a lift's rated load is given. */
	UPROPERTY(meta = (Units = "Kilograms"))
	float LoadKilograms = 0.0f;

	[[nodiscard]] Musee::MetersPerSecond Speed() const { return Musee::MetersPerSecond(SpeedMetersPerSecond); }
	[[nodiscard]] Musee::Kilograms Load() const { return Musee::Kilograms(LoadKilograms); }
};
