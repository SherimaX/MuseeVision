#include "Make/MakeMeasures.h"

#include "HAL/PlatformMemory.h"
#include "ProfilingDebugging/CookStats.h"

namespace Musee::Studio
{
	namespace
	{
		/** Bytes in a megabyte, as the engine's memory reports count them. */
		constexpr double BytesPerMegabyte = 1024.0 * 1024.0;

		/** The engine's names for the three mesh resources (StaticMesh.cpp, DistanceFieldAtlas.cpp, MeshCardRepresentation.cpp). */
		const TCHAR* const MeshStat = TEXT("StaticMesh.Usage");
		const TCHAR* const FieldStat = TEXT("DistanceField.Usage");
		const TCHAR* const CardStat = TEXT("CardRepresentation.Usage");

		const FString* FindAttribute(const TArray<FCookStatsManager::StringKeyValue>& Attributes, const TCHAR* Key)
		{
			const FCookStatsManager::StringKeyValue* Found = Attributes.FindByPredicate(
				[Key](const FCookStatsManager::StringKeyValue& Pair) { return Pair.Key == Key; });
			return Found ? &Found->Value : nullptr;
		}

		/** Adds one engine stat line to the tally: its synchronous-work count, as a fetch (hit) or a build (miss). */
		void Count(const FString& StatName, const TArray<FCookStatsManager::StringKeyValue>& Attributes, FDerivedDataTally& Tally)
		{
			const FString* Call = FindAttribute(Attributes, TEXT("Call"));
			const FString* HitOrMiss = FindAttribute(Attributes, TEXT("HitOrMiss"));
			const FString* CountText = FindAttribute(Attributes, TEXT("Count"));
			if (!Call || *Call != TEXT("SyncWork") || !HitOrMiss || !CountText)
			{
				return;
			}
			const bool bHit = *HitOrMiss == TEXT("Hit");
			const int64 Count = FCString::Atoi64(**CountText);
			if (StatName == MeshStat) { (bHit ? Tally.MeshHits : Tally.MeshMisses) += Count; }
			else if (StatName == FieldStat) { (bHit ? Tally.FieldHits : Tally.FieldMisses) += Count; }
			else if (StatName == CardStat) { (bHit ? Tally.CardHits : Tally.CardMisses) += Count; }
		}
	}

	FDerivedDataTally FDerivedDataTally::operator-(const FDerivedDataTally& Earlier) const
	{
		FDerivedDataTally Difference;
		Difference.MeshHits = MeshHits - Earlier.MeshHits;
		Difference.MeshMisses = MeshMisses - Earlier.MeshMisses;
		Difference.FieldHits = FieldHits - Earlier.FieldHits;
		Difference.FieldMisses = FieldMisses - Earlier.FieldMisses;
		Difference.CardHits = CardHits - Earlier.CardHits;
		Difference.CardMisses = CardMisses - Earlier.CardMisses;
		return Difference;
	}

	FDerivedDataTally MeasureDerivedData()
	{
		FDerivedDataTally Tally;
		FCookStatsManager::LogCookStats([&Tally](const FString& StatName, const TArray<FCookStatsManager::StringKeyValue>& Attributes)
		{
			Count(StatName, Attributes, Tally);
		});
		return Tally;
	}

	double MeasurePeakPhysMemoryMB()
	{
		return static_cast<double>(FPlatformMemory::GetStats().PeakUsedPhysical) / BytesPerMegabyte;
	}
}
