#pragma once

#include "CoreMinimal.h"
#include "Hash/Blake3.h"
#include "Serialization/MemoryHasher.h"

class UObject;
struct FMeshDescription;

namespace Musee::Studio
{
	/**
	 * The fingerprint of everything a made asset was made from: the revision of the rules that made it and every
	 * input those rules read, in a fixed order. Two makes from the same inputs give the same hash in any process, on
	 * any checkout, so a make compares the hash it would store with the one the asset already carries and leaves the
	 * package alone when they agree.
	 */
	class FInputHasher
	{
	public:
		FInputHasher();

		/** Adds text as UTF-8 with its length, so "ab"+"c" and "a"+"bc" hash apart. */
		FInputHasher& Add(FStringView Text);
		FInputHasher& Add(int64 Value);
		FInputHasher& Add(double Value);
		FInputHasher& Add(const FVector& Value);
		FInputHasher& Add(const FQuat& Value);

		/** Adds a whole mesh description: every element, every attribute, names as text. It is read, not changed. */
		FInputHasher& Add(FMeshDescription& Description);

		/** The hash as 64 lowercase hex digits. */
		FString Finish();

	private:
		TMemoryHasher<FBlake3, FBlake3Hash> Archive;
	};

	/** The name a made asset's input hash goes by, in its package metadata and in the asset registry. */
	FName InputHashKey();

	/** The input hash stored on a made asset, read without loading it; empty when the asset was never made. */
	FString StoredInputHash(const FString& PackageName, const FString& AssetName);

	/** Stores a made asset's input hash in its package's metadata, which is saved with it and stripped when cooking. */
	void StoreInputHash(UObject& Asset, const FString& Hash);

	/** The input hash a loaded asset carries in its package's metadata, or null for an asset Studio did not make. */
	const FString* FindInputHash(const UObject& Asset);
}
