#include "FormCatalogue.h"

#include "MuseeRecords.h"

TValueOrError<TArray<FFormProfile>, FString> Musee::Form::ReadProfiles(const FString& Path)
{
	TValueOrError<TArray<FFormProfile>, FString> Read = ReadRecords<FFormProfile>(Path);
	if (Read.HasError())
	{
		return Read;
	}
	for (const FFormProfile& Profile : Read.GetValue())
	{
		FString Why;
		if (Profile.Key.IsNone())
		{
			return MakeError(FString::Printf(TEXT("%s: a catalogue profile has no key"), *Path));
		}
		if (!Profile.IsWellFormed(&Why))
		{
			return MakeError(FString::Printf(TEXT("%s: profile %s: %s"), *Path, *Profile.Key.ToString(), *Why));
		}
	}
	return Read;
}

TValueOrError<FFormProfile, FString> Musee::Form::FindProfile(const FMuseeKey& Key, const FString& Path)
{
	TValueOrError<TArray<FFormProfile>, FString> Read = ReadProfiles(Path);
	if (Read.HasError())
	{
		return MakeError(Read.StealError());
	}
	for (FFormProfile& Profile : Read.GetValue())
	{
		if (Profile.Key == Key)
		{
			return MakeValue(MoveTemp(Profile));
		}
	}
	return MakeError(FString::Printf(TEXT("%s holds no profile %s"), *Path, *Key.ToString()));
}
