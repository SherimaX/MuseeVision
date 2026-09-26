#include "Catalog/MuseeFiles.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FString MuseeFiles::Resolve(const FString& RepoRelative)
{
	const FString Candidates[] = {
		FPaths::Combine(FPaths::ProjectContentDir(), TEXT("MuseeData"), RepoRelative),
		FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), RepoRelative),
	};
	for (const FString& Candidate : Candidates)
	{
		FString Full = FPaths::ConvertRelativePathToFull(Candidate);
		if (FPaths::FileExists(Full)) { return Full; }
	}
	return FString();
}

bool MuseeFiles::Load(const FString& RepoRelative, TArray<uint8>& OutBytes)
{
	const FString Path = Resolve(RepoRelative);
	return !Path.IsEmpty() && FFileHelper::LoadFileToArray(OutBytes, *Path);
}

bool MuseeFiles::Load(const FString& RepoRelative, FString& OutText)
{
	const FString Path = Resolve(RepoRelative);
	return !Path.IsEmpty() && FFileHelper::LoadFileToString(OutText, *Path);
}
