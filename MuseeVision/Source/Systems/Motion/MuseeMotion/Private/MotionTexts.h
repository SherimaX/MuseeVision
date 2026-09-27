#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "MuseeText.h"

namespace Musee::Motion
{
	/**
	 * Gives a text written as a plain string the key its place implies: its owner's key and the field
	 * (`atrium.car.lift` and `open-use.verb` make `atrium.car.lift.open-use.verb`). A text with a key keeps it, and one
	 * with no source (an empty caption) stays unkeyed, since it says nothing.
	 */
	inline void KeyText(FMuseeText& Text, const FMuseeKey& Owner, FStringView Field)
	{
		if (!Text.Key.IsNone() || Text.Source.IsEmpty())
		{
			return;
		}
		FMuseeKey Keyed = Owner;
		TArray<FString> Segments;
		FString(Field).ParseIntoArray(Segments, TEXT("."));
		for (const FString& Segment : Segments)
		{
			Keyed = Keyed.Child(Segment);
		}
		Text.Key = Keyed;
	}
}
