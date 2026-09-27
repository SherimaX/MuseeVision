#pragma once

#include "CoreMinimal.h"
#include "MatterMasterRecipe.h"

class UMaterial;

/**
 * Making a master as an asset: the recipe's material, built fresh in its package under /Game/Generated/Matter and
 * saved to disk. The package's own persistent guid is derived from the recipe's identity like every other identifier,
 * so a make writes the same bytes each time the recipe is the same, in any session.
 */
namespace Musee::Matter
{
	/** What a make produced. */
	struct FMasterMade
	{
		UMaterial* Material = nullptr;
		/** The package file on disk. */
		FString Filename;
		TArray<FString> Problems;

		bool Succeeded() const { return Material != nullptr && Problems.IsEmpty(); }
	};

	/** Makes Recipe's master in its package, retiring any material an earlier make left there, and saves it. */
	FMasterMade MakeMasterAsset(const FMasterRecipe& Recipe);
}
