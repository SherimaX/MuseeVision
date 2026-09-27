#pragma once

#include "CoreMinimal.h"
#include "MatterFinishSchedule.h"
#include "MatterHistory.h"
#include "MatterPalette.h"
#include "MatterSubstance.h"
#include "MatterWorking.h"
#include "MuseeKey.h"
#include "Templates/ValueOrError.h"

/**
 * Reading Matter's records through Foundation's record reader (Musee::ReadRecord), so every file read joins the
 * dependency sets in scope and every required field and key is checked. Paths are relative to the project's Data
 * folder:
 * - the palette: Matter/Palette.json;
 * - typed records: Matter/Shading/Substances/<key>.json, Matter/Shading/Workings/<key>.json and
 *   Matter/Shading/Histories/<master>.<regime>.json;
 * - a stage's finish schedule: <schedule folder>/Finishes.json.
 * Each also checks that the record's own key is the key it was read by.
 */
namespace Musee::Matter
{
	MUSEEMATTER_API TValueOrError<FMatterPalette, FString> ReadPalette();

	MUSEEMATTER_API TValueOrError<FMatterSubstance, FString> ReadSubstance(const FMuseeKey& Key);

	MUSEEMATTER_API TValueOrError<FMatterWorking, FString> ReadWorking(const FMuseeKey& Key);

	/** The history regime for a master (or a layer working) and a regime word: stone and worn read stone.worn. */
	MUSEEMATTER_API TValueOrError<FMatterHistory, FString> ReadHistory(const FMuseeKey& Master, const FMuseeKey& Regime);

	/** A stage's finish schedule, from the folder its stage context names (relative to Data, or absolute). */
	MUSEEMATTER_API TValueOrError<FMatterFinishSchedule, FString> ReadFinishSchedule(const FString& ScheduleFolder);
}
