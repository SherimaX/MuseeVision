#pragma once

#include "CoreMinimal.h"

/**
 * The repository's own files (data/, assets/) read at runtime. In the editor and development
 * builds they come straight from the repository beside windows/; a packaged app reads the copy
 * staged in Content/MuseeData (Scripts/stage_data.py), same relative paths.
 */
namespace MuseeFiles
{
	/** Absolute path of a repository-relative file (e.g. "data/artworks.json"), or empty if missing. */
	FString Resolve(const FString& RepoRelative);

	bool Load(const FString& RepoRelative, TArray<uint8>& OutBytes);
	bool Load(const FString& RepoRelative, FString& OutText);
}
