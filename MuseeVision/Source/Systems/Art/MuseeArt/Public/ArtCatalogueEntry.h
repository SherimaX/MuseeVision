#pragma once

#include "ArtScan.h"
#include "CoreMinimal.h"

#include "ArtCatalogueEntry.generated.h"

/**
 * One work as the museum's catalogues write it (`assets/collection.json`, `data/*.json`), read as it is: its id, its
 * artist and titles, its date as the catalogue gives it (`1914–1926`), its medium, the collection that holds it and
 * its city, and the notes a label draws on. The sculptures catalogue adds a height, rights and a scan.
 */
USTRUCT()
struct MUSEEART_API FArtCatalogueEntry
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FString Id;

	UPROPERTY()
	FString Artist;

	UPROPERTY(meta = (MuseeRequired))
	FString Title;

	UPROPERTY()
	FString OriginalTitle;

	UPROPERTY()
	FString Year;

	UPROPERTY()
	FString Medium;

	UPROPERTY()
	FString Collection;

	UPROPERTY()
	FString City;

	UPROPERTY()
	FString Notes;

	UPROPERTY()
	FString Gallery;

	UPROPERTY()
	bool Core = false;

	UPROPERTY(meta = (Units = "Centimeters"))
	float HeightCm = 0.0f;

	UPROPERTY()
	FString Rights;

	/**
	 * The credit the photographs a work is made from ask for, as their licences word it (a painted vessel's decoration,
	 * composed from its museum's and others' photographs); shown on its label.
	 */
	UPROPERTY()
	FString Credit;

	UPROPERTY()
	FArtScan Scan;
};
