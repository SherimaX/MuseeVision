#pragma once

#include "CoreMinimal.h"
#include "Hash/Blake3.h"
#include "MatterMasterRecipe.h"

class UMaterial;
class UPackage;

/**
 * Writing Matter's made assets: each is built fresh in its package, given identifiers derived from its inputs, tagged
 * with what it was made from, and saved only when its bytes differ from the file already on disk, so a second make
 * with nothing changed rewrites nothing.
 */
namespace Musee::Matter
{
	/** The package metadata every made asset carries, which the asset registry shows as tags. */
	namespace MadeTag
	{
		/** What the asset is: a master's key, an instance's spec, the Moment's collection. */
		inline constexpr const TCHAR* Made = TEXT("Musee.Matter.Made");
		/** The hash of everything it was made from: its recipe, or the records it read. */
		inline constexpr const TCHAR* Inputs = TEXT("Musee.Matter.Inputs");
		/** The records it read, each with the hash of its content. */
		inline constexpr const TCHAR* Records = TEXT("Musee.Matter.Records");
		/** The version of the generator that made it. */
		inline constexpr const TCHAR* Generator = TEXT("Musee.Matter.Generator");
	}

	/** A package ready for a made asset: created (or found), marked fully loaded so nothing is read from its old file,
	 *  and emptied of the asset an earlier make left in it. */
	UPackage* PrepareMadePackage(const FString& PackageName, FName AssetName);

	/** What saving a made asset did. */
	struct FMadeSave
	{
		FString Filename;
		/** BLAKE3 of the package's bytes. */
		FBlake3Hash Hash;
		int64 Bytes = 0;
		/** Whether the file on disk was written; false when it already held these bytes. */
		bool bWritten = false;
		TArray<FString> Problems;

		bool Succeeded() const { return Problems.IsEmpty(); }
	};

	/** Tags Asset with Tags (package metadata), gives its package PersistentGuid, and saves it if its bytes differ from
	 *  the file on disk. */
	FMadeSave SaveMadeAsset(UObject& Asset, const FGuid& PersistentGuid, const TMap<FString, FString>& Tags);

	/** What a master's make produced. */
	struct FMasterMade
	{
		UMaterial* Material = nullptr;
		FMadeSave Save;
		TArray<FString> Problems;

		bool Succeeded() const { return Material != nullptr && Problems.IsEmpty() && Save.Succeeded(); }
	};

	/** Makes Recipe's master in its package, retiring any material an earlier make left there, and saves it. */
	FMasterMade MakeMasterAsset(const FMasterRecipe& Recipe);
}
