#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"
#include "Templates/ValueOrError.h"

/**
 * The catalogue of the museum's architectural language: its moulding profiles by key, in Data/Form/Profiles.json,
 * named for what they are (`cove.quarter-ellipse.1450`) and never for a room. A detail used by one stage alone stays in
 * that stage's plan; its second use promotes it here.
 *
 * Profiles are read through Foundation's record reader on every call, so a make that uses a profile depends on the
 * catalogue and remakes when it changes.
 */
namespace Musee::Form
{
	/** The catalogue's file, under Data. */
	inline constexpr const TCHAR* ProfilesRecord = TEXT("Form/Profiles.json");

	/** Every profile in a catalogue file (the museum's by default), in the file's order. */
	[[nodiscard]] MUSEEFORM_API TValueOrError<TArray<FFormProfile>, FString> ReadProfiles(const FString& Path = ProfilesRecord);

	/** The catalogue's profile of this key, or why there is none. */
	[[nodiscard]] MUSEEFORM_API TValueOrError<FFormProfile, FString> FindProfile(const FMuseeKey& Key, const FString& Path = ProfilesRecord);
}
