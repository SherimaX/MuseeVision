#include "ArtCare.h"

#include "Care/ArtCareRecords.h"
#include "MuseeRecords.h"

namespace Musee::Art
{
	TValueOrError<TArray<FName>, FString> CareClasses()
	{
		const TValueOrError<FArtCare, FString> Care = ReadRecord<FArtCare>(TEXT("Art/Care.json"));
		if (Care.HasError())
		{
			return MakeError(Care.GetError());
		}
		TArray<FName> Keys;
		for (const FArtCareClass& Class : Care.GetValue().Classes)
		{
			Keys.Add(Class.Key);
		}
		return MakeValue(MoveTemp(Keys));
	}
}
