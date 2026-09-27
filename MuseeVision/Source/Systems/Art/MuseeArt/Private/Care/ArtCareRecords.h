#pragma once

#include "CoreMinimal.h"

#include "ArtCareRecords.generated.h"

/** One row of evidence behind a care value: the fields it supports, its source's id, whether it was read, the row. */
USTRUCT()
struct FArtCareEvidence
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> Fields;

	UPROPERTY()
	FString Source;

	UPROPERTY()
	bool Confirmed = false;

	UPROPERTY()
	FString Row;
};

/** One care class: the media it covers and its limits for light (lux, annual lux-hours, UV) and colour temperature. */
USTRUCT()
struct FArtCareClass
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FName Key;

	UPROPERTY()
	FString Responsivity;

	UPROPERTY()
	TArray<FString> Media;

	UPROPERTY(meta = (Units = "Lux"))
	float IlluminanceLuxMax = 0.0f;

	UPROPERTY(meta = (Units = "Lux"))
	float IlluminanceLuxRecommended = 0.0f;

	UPROPERTY()
	double AnnualExposureLuxHoursMax = 0.0;

	UPROPERTY()
	float UvMicrowattsPerLumenMax = 0.0f;

	UPROPERTY()
	TArray<float> CctKelvinRange;

	UPROPERTY()
	FString CctBasis;

	UPROPERTY()
	TArray<FArtCareEvidence> Evidence;

	UPROPERTY()
	FString Note;
};

/** How the annual dose is counted: the reference hours of light a year and a day. */
USTRUCT()
struct FArtCareDoseAccounting
{
	GENERATED_BODY()

	UPROPERTY()
	FString Note;

	UPROPERTY()
	float ReferenceHoursPerYear = 0.0f;

	UPROPERTY()
	float ReferenceHoursPerDay = 0.0f;

	UPROPERTY()
	TArray<FArtCareEvidence> Evidence;
};

/** What the museum's light sources emit outside the visible, which the UV limit bears on. */
USTRUCT()
struct FArtCareSourceSpectrum
{
	GENERATED_BODY()

	UPROPERTY()
	FString Note;

	UPROPERTY()
	TArray<FArtCareEvidence> Evidence;
};

/** A source the care values cite. */
USTRUCT()
struct FArtCareSource
{
	GENERATED_BODY()

	UPROPERTY()
	FString Id;

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Url;

	UPROPERTY()
	FString ReadOn;
};

/** Data/Art/Care.json, as it is written. */
USTRUCT()
struct FArtCare
{
	GENERATED_BODY()

	UPROPERTY()
	FString About;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FArtCareClass> Classes;

	UPROPERTY()
	FArtCareDoseAccounting DoseAccounting;

	UPROPERTY()
	FArtCareSourceSpectrum SourceSpectrum;

	UPROPERTY()
	TArray<FArtCareSource> Sources;

	UPROPERTY()
	TArray<FString> Unverified;
};
