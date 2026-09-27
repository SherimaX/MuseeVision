#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "Templates/ValueOrError.h"

#include "FinishSchedule.generated.h"

/** One line of a stage's finish schedule: a named surface and the material it is finished in. */
USTRUCT()
struct FStudioFinish
{
	GENERATED_BODY()

	/** The surface, as the stage's drawings name it: coping, wall, canvas.<work>. */
	UPROPERTY(meta = (MuseeRequired))
	FName Surface;

	/** The palette key (Data/Matter/Palette.json), its working and its history. */
	UPROPERTY(meta = (MuseeRequired))
	FName Material;

	UPROPERTY()
	FName Working;

	UPROPERTY()
	FName History;

	UPROPERTY()
	FString Note;
};

/**
 * A stage's finish schedule, Data/Stages/<stage>/Finishes.json, named after the document real architects use: every
 * surface the stage draws, and what it is finished in. Changing a material is a one-line edit here.
 */
USTRUCT()
struct FStudioFinishSchedule
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Stage;

	UPROPERTY()
	TArray<FStudioFinish> Finishes;

	/** The finish of a surface, or null. */
	[[nodiscard]] const FStudioFinish* Find(FName Surface) const
	{
		return Finishes.FindByPredicate([Surface](const FStudioFinish& Finish) { return Finish.Surface == Surface; });
	}
};

namespace Musee::Studio
{
	/** The schedule's file within a stage's schedule folder. */
	inline constexpr const TCHAR* FinishesFile = TEXT("Finishes.json");

	/** Reads the finish schedule in a stage's schedule folder. */
	[[nodiscard]] TValueOrError<FStudioFinishSchedule, FString> ReadFinishSchedule(const FString& ScheduleDir);
}
