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
		const TValueOrError<FArtSupports, FString>& Read = ReadSupports();
		return Read.HasValue() ? Read.GetValue().Supports.FindByPredicate([Support](const FArtSupport& S) { return S.Support == Support; }) : nullptr;
	}
}
