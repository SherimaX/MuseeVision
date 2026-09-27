#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "SiteDescription.h"
#include "UObject/Object.h"

#include "SiteRoom.generated.h"

/**
 * The contract every room fulfils: a stage that describes itself. A room is a description, not a program. Its class
 * derives from this one, lives in its own stage module, and says which stage it is and what it is made of, in its own
 * frame (x east, y south, z up, true north), knowing nothing of other stages or of where it stands. Studio finds rooms
 * by class registration and asks each to describe itself; Site places them by the site plan.
 *
 * Describe is a plain, deterministic function of the room's plan and the records it reads: the same description on
 * every call, every machine.
 */
UCLASS(Abstract)
class MUSEESITE_API USiteRoom : public UObject
{
	GENERATED_BODY()

public:
	/** The stage's key: nympheas. */
	[[nodiscard]] virtual FMuseeKey StageKey() const PURE_VIRTUAL(USiteRoom::StageKey, return FMuseeKey(););

	/** Describes the stage: its footprint, enclosures, openings, ports, anchors and pieces. */
	virtual void Describe(FSiteDescription& Out) const PURE_VIRTUAL(USiteRoom::Describe, );

	/** Every room whose module is loaded (one default object per class), in the order of their keys. */
	[[nodiscard]] static TArray<const USiteRoom*> AllRooms();

	/** The room of a stage, or null when no loaded room has that key. */
	[[nodiscard]] static const USiteRoom* Find(const FMuseeKey& Stage);
};
