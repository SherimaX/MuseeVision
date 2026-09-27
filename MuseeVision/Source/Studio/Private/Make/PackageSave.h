#pragma once

#include "CoreMinimal.h"

class UObject;
class UPackage;

namespace Musee::Studio
{
	/**
	 * The in-memory package a made asset is written into. It owns its whole content, so saving it replaces the file on
	 * disk outright; nothing is read from the old file first.
	 */
	UPackage* CreateMadePackage(const FString& PackageName);

	/**
	 * The asset a make writes at a package: the one already loaded when there is one (so nothing loaded goes stale), the
	 * one on disk when its package is in memory without it, else a fresh one in a made package that replaces the file.
	 * The asset is named after its package.
	 */
	UObject* FindOrCreateMadeAsset(UClass& Class, const FString& PackageName);

	template <typename T>
	T* FindOrCreateMadeAsset(const FString& PackageName)
	{
		return CastChecked<T>(FindOrCreateMadeAsset(*T::StaticClass(), PackageName));
	}

	/**
	 * Saves a made asset's package to the file its name gives, the same way every time: the package's persistent GUID
	 * is derived from its name, and nothing time-dependent is written. Returns false with the reason in OutWhy.
	 */
	bool SaveMadePackage(UObject& Asset, FString& OutWhy);

	/**
	 * Unloads packages from memory (their worlds cleaned up, their objects collected), so that the next load reads
	 * them from disk. The editor's undo history is kept.
	 */
	void ReleasePackages(const TArray<UPackage*>& Packages);
}
