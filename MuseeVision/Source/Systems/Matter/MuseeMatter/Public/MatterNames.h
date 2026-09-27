#pragma once

#include "CoreMinimal.h"

/**
 * Where Matter's generated assets live and what they are called: the one place these paths are spelled.
 *
 * - a master:   /Game/Generated/Matter/M_Matter_<Master>, and a master carrying a layer working
 *               /Game/Generated/Matter/M_Matter_<Master><Layer> (M_Matter_MetalGilt);
 * - an instance: /Game/Generated/Matter/MI_<Palette>_<Working>_<History> (MI_StoneTravertineRomano_Honed_Worn);
 * - the museum Moment's parameter collection: /Game/Generated/Time/MPC_Moment.
 *
 * Keys become names by PascalCase: every dot or hyphen ends a word, and each word is capitalised.
 */
namespace Musee::Matter
{
	/** The folder Matter's assets are made in. */
	inline constexpr const TCHAR* GeneratedFolder = TEXT("/Game/Generated/Matter");

	/** The long package name of the museum Moment's parameter collection, which Matter's make step creates. */
	inline constexpr const TCHAR* MomentCollectionPackage = TEXT("/Game/Generated/Time/MPC_Moment");

	/** A key as a name part: `stone.travertine-romano` becomes `StoneTravertineRomano`. */
	MUSEEMATTER_API FString PascalCase(FStringView Key);

	/** The asset name of a master, with the layer working it carries (none for a plain master). */
	MUSEEMATTER_API FString MasterName(FName Master, FName Layer = NAME_None);

	/** The long package name of a master. */
	MUSEEMATTER_API FString MasterPackage(FName Master, FName Layer = NAME_None);

	/** The asset name of the instance for a palette key, a working and a history. */
	MUSEEMATTER_API FString InstanceName(FName Palette, FName Working, FName History);

	/** The long package name of that instance. */
	MUSEEMATTER_API FString InstancePackage(FName Palette, FName Working, FName History);

	/** The object path of an asset in a package whose asset has the package's own name: /Game/X/A.A. */
	MUSEEMATTER_API FString ObjectPathOf(const FString& PackageName);
}
