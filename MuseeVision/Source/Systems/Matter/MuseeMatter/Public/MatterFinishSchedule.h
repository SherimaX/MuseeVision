#pragma once

#include "CoreMinimal.h"
#include "MatterFinish.h"
#include "MuseeKey.h"

#include "MatterFinishSchedule.generated.h"

/** A stage's finish schedule, the document real architects keep: every named surface and what it is made of. */
USTRUCT()
struct MUSEEMATTER_API FMatterFinishSchedule
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Stage;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FMatterFinish> Finishes;

	/** The finish for a surface, or nullptr. */
	const FMatterFinish* Find(FName Surface) const
	{
		return Finishes.FindByPredicate([Surface](const FMatterFinish& Finish) { return Finish.Surface.ToName() == Surface; });
	}
};
