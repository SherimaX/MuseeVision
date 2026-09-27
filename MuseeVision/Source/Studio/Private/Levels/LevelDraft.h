#pragma once

#include "CoreMinimal.h"
#include "Templates/UniquePtr.h"

class AActor;
class UClass;
class UPackage;
class UWorld;

namespace Musee::Studio
{
	/**
	 * A level being written by code: a fresh world without World Partition, in its own package. Studio spawns the
	 * level's actors into it, saves it, and lets it go when the draft ends; the editor's open level is never touched.
	 */
	class FLevelDraft
	{
	public:
		/**
		 * Starts a draft of the level PackageName. A copy of it already in memory is unloaded first; a copy that is open
		 * in the editor or in play stops the draft, with the reason in OutWhy.
		 */
		static TUniquePtr<FLevelDraft> Start(const FString& PackageName, FString& OutWhy);

		~FLevelDraft();
		FLevelDraft(const FLevelDraft&) = delete;
		FLevelDraft& operator=(const FLevelDraft&) = delete;

		UWorld& World() const { return *DraftWorld; }

		/**
		 * Spawns an actor of Class named Name into the level at a transform in engine units, or returns null when the
		 * name is taken. Its GUID is derived from the level and the name, so a level written twice holds the same actors.
		 */
		AActor* Spawn(UClass* Class, const FString& Name, const FTransform& Transform);

		/** Stores the level's input hash and saves it; false with the reason in OutWhy. */
		bool Save(const FString& InputHash, FString& OutWhy);

	private:
		FLevelDraft(UPackage* InPackage, UWorld* InWorld);

		UPackage* Package = nullptr;
		UWorld* DraftWorld = nullptr;
	};
}
