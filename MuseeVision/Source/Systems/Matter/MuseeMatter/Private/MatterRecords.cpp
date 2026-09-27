#include "MatterRecords.h"

#include "Misc/Paths.h"
#include "MuseeRecords.h"

namespace Musee::Matter
{
namespace
{
	constexpr const TCHAR* PalettePath = TEXT("Matter/Palette.json");
	constexpr const TCHAR* SubstancesFolder = TEXT("Matter/Shading/Substances");
	constexpr const TCHAR* WorkingsFolder = TEXT("Matter/Shading/Workings");
	constexpr const TCHAR* HistoriesFolder = TEXT("Matter/Shading/Histories");
	constexpr const TCHAR* FinishesFile = TEXT("Finishes.json");
	constexpr const TCHAR* RecordExtension = TEXT(".json");

	/** A typed record, read by its key, whose own key must be that key. */
	template <typename T>
	TValueOrError<T, FString> ReadKeyed(const TCHAR* Folder, const FMuseeKey& Key)
	{
		if (Key.IsNone())
		{
			return MakeError(FString::Printf(TEXT("%s: no key was given"), Folder));
		}
		const FString Path = FString(Folder) / Key.ToString() + RecordExtension;
		TValueOrError<T, FString> Read = ReadRecord<T>(Path);
		if (Read.HasValue() && !(Read.GetValue().Key == Key))
		{
			return MakeError(FString::Printf(TEXT("%s: the record's key is '%s', not '%s'"), *Path,
				*Read.GetValue().Key.ToString(), *Key.ToString()));
		}
		return Read;
	}
}

TValueOrError<FMatterPalette, FString> ReadPalette()
{
	return ReadRecord<FMatterPalette>(PalettePath);
}

TValueOrError<FMatterSubstance, FString> ReadSubstance(const FMuseeKey& Key)
{
	return ReadKeyed<FMatterSubstance>(SubstancesFolder, Key);
}

TValueOrError<FMatterWorking, FString> ReadWorking(const FMuseeKey& Key)
{
	return ReadKeyed<FMatterWorking>(WorkingsFolder, Key);
}

TValueOrError<FMatterHistory, FString> ReadHistory(const FMuseeKey& Master, const FMuseeKey& Regime)
{
	return ReadKeyed<FMatterHistory>(HistoriesFolder, Master.Child(Regime.ToString()));
}

TValueOrError<FMatterFinishSchedule, FString> ReadFinishSchedule(const FString& ScheduleFolder)
{
	return ReadRecord<FMatterFinishSchedule>(ScheduleFolder / FinishesFile);
}
}
