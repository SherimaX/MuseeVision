#pragma once

#include "ArtCatalogueEntry.h"
#include "ArtGallery.h"
#include "CoreMinimal.h"

#include "ArtCatalogue.generated.h"

/**
 * One of the museum's catalogues, read as it is written: a title, a description, perhaps its galleries, and its works
 * under whichever name the catalogue uses (`works`, `artworks`, `sculptures`). The catalogues are never unified; a
 * hanging plan names the one that owns each work.
 */
USTRUCT()
struct MUSEEART_API FArtCatalogue
{
	GENERATED_BODY()

	UPROPERTY()
	FString Title;

	UPROPERTY()
	FString Description;

	UPROPERTY()
	TArray<FArtGallery> Galleries;

	UPROPERTY()
	TArray<FArtCatalogueEntry> Works;

	UPROPERTY()
	TArray<FArtCatalogueEntry> Artworks;

	UPROPERTY()
	TArray<FArtCatalogueEntry> Sculptures;

	/** The entry with an id, from whichever list holds it, or null. */
	[[nodiscard]] const FArtCatalogueEntry* Find(const FString& Id) const
	{
		for (const TArray<FArtCatalogueEntry>* List : { &Works, &Artworks, &Sculptures })
		{
			if (const FArtCatalogueEntry* Found = List->FindByPredicate([&Id](const FArtCatalogueEntry& E) { return E.Id == Id; }))
			{
				return Found;
			}
		}
		return nullptr;
	}
};
