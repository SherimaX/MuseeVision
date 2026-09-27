#pragma once

#include "CoreMinimal.h"

#include "SiteFootprint.generated.h"

/** What a stage's footprint is, which sets how it may meet the others. */
UENUM()
enum class ESiteFootprintKind : uint8
{
	/** A building: it never overlaps another building. */
	Building,
	/** The ground (one stage only, the grounds): it contains every building. */
	Ground,
};

/** The ground a stage stands on and the height it rises through, in its own frame: the volume it may build in. */
USTRUCT()
struct MUSEESITE_API FSiteFootprint
{
	GENERATED_BODY()

	UPROPERTY()
	ESiteFootprintKind Kind = ESiteFootprintKind::Building;

	/** The footprint's outline in plan, (x east, y south), metres, closed. */
	UPROPERTY()
	TArray<FVector2D> OutlineMeters;

	/** The lowest and highest points the stage builds to, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	double BottomMeters = 0.0;

	UPROPERTY(meta = (Units = "Meters"))
	double TopMeters = 0.0;
};
