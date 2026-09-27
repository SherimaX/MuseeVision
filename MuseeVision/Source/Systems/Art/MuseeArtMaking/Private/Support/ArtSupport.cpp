#include "Support/ArtSupport.h"

#include "MuseeRecords.h"

namespace Musee::ArtMaking
{
	const TValueOrError<FArtSupports, FString>& ReadSupports()
	{
		static const TValueOrError<FArtSupports, FString> Read = ReadRecord<FArtSupports>(FArtSupports::Record);
		return Read;
	}

	const FArtSupport* FindSupport(FName Support)
	{
		return FindSupport(Support, NAME_None);
	}

	const FArtSupport* FindSupport(FName Support, FName Kind)
	{
		const TValueOrError<FArtSupports, FString>& Read = ReadSupports();
		if (Read.HasError())
		{
			return nullptr;
		}
		const TArray<FArtSupport>& All = Read.GetValue().Supports;
		const FArtSupport* Made = Kind.IsNone() ? nullptr : All.FindByPredicate([Support, Kind](const FArtSupport& S) { return S.Support == Support && S.Kind == Kind; });
		return Made ? Made : All.FindByPredicate([Support](const FArtSupport& S) { return S.Support == Support && S.Kind.IsNone(); });
	}
}
