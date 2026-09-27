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

TValueOrError<TArray<FFormTypeface>, FString> Musee::Form::ReadTypefaces(const FString& Path)
{
	TValueOrError<TArray<FFormTypeface>, FString> Read = ReadRecords<FFormTypeface>(Path);
	if (Read.HasError())
	{
		return Read;
	}
	for (const FFormTypeface& Typeface : Read.GetValue())
	{
		if (Typeface.Key.IsNone() || Typeface.Glyphs.IsEmpty())
		{
			return MakeError(FString::Printf(TEXT("%s: a typeface has a key and glyphs"), *Path));
		}
		for (const FFormGlyph& Glyph : Typeface.Glyphs)
		{
			FString Why;
			if (!Glyph.IsWellFormed(&Why))
			{
				return MakeError(FString::Printf(TEXT("%s: typeface %s: %s"), *Path, *Typeface.Key.ToString(), *Why));
			}
		}
	}
	return Read;
}

TValueOrError<FFormTypeface, FString> Musee::Form::FindTypeface(const FMuseeKey& Key, const FString& Path)
{
	TValueOrError<TArray<FFormTypeface>, FString> Read = ReadTypefaces(Path);
	if (Read.HasError())
	{
		return MakeError(Read.StealError());
	}
	for (FFormTypeface& Typeface : Read.GetValue())
	{
		if (Typeface.Key == Key)
		{
			return MakeValue(MoveTemp(Typeface));
		}
	}
	return MakeError(FString::Printf(TEXT("%s holds no typeface %s"), *Path, *Key.ToString()));
}
