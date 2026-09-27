#pragma once

#include "CoreMinimal.h"

#include "VesselRecord.generated.h"

/**
 * The carved stand a vessel stands on where its plan says `stand` (`Data/Art/Vessels.json`, from the stage's sheet): a
 * drum whose radius is so many times the vessel's foot, low under a small vessel and higher under one of TallMeters or
 * more, in its wood's palette finish.
 */
USTRUCT()
struct FArtVesselStand
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	double FootRadii = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double LowMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double HighMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double TallMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	FName Palette;

	UPROPERTY(meta = (MuseeRequired))
	FName Working;

	UPROPERTY(meta = (MuseeRequired))
	FName History;
};

/**
 * A vessel's turned section, as the stage's sheet traced it from a photograph of the real vessel: [radius, height] in
 * metres from the centre of its foot, out along its underside, up its outside, over its lip and down its inside to the
 * axis again, the solid on the left. Turned about its axis it is the vessel's closed solid.
 */
USTRUCT()
struct FArtVessel
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FString Work;

	UPROPERTY()
	FString Source;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FVector2D> Section;
};

/** Every turned vessel's section and the stand they stand on (`Data/Art/Vessels.json`). */
USTRUCT()
struct FArtVessels
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Vessels.json");

	/** A section turns smoothly through a traced point where it turns less than this, and keeps an arris where it turns more. */
	UPROPERTY(meta = (MuseeRequired, Units = "Degrees"))
	double CreaseDegrees = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	FArtVesselStand Stand;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FArtVessel> Vessels;

	[[nodiscard]] const FArtVessel* Find(const FString& Work) const
	{
		return Vessels.FindByPredicate([&Work](const FArtVessel& V) { return V.Work == Work; });
	}
};
