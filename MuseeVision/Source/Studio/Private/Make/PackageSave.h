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
