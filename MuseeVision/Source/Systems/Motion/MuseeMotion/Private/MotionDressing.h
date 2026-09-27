#pragma once

#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "MuseeKey.h"
#include "MuseeStageDresser.h"

class AActor;
class USceneComponent;
struct FMotionSchedule;

/** What Motion's dresser shares between its steps: its findings, names, and the stage's parts it looks up. */
namespace Musee::Motion::Dressing
{
	/** A finding owned by Motion on the stage. */
	[[nodiscard]] FMuseeFinding Finding(const FMuseeStageContext& Stage, const FString& Summary);

	/** An engine object name for a key: its dots, which object paths reserve, become underscores. */
	[[nodiscard]] FName ObjectName(const TCHAR* Prefix, const FMuseeKey& Key);

	/** The scene component tagged with a name (a body's key, or `musee.piece:<name>`), or null. */
	[[nodiscard]] USceneComponent* FindTagged(const AActor& Actor, FName Tag);

	/** The tag Studio gives a made piece's component: `musee.piece:<name>` (the stage contract). */
	[[nodiscard]] FName PieceTag(FName Piece);

	/** Adds a component the dresser made to the stage actor, attached to a parent, and registers it. */
	void Adopt(AActor& Actor, USceneComponent& Component, USceneComponent& Parent);

	/**
	 * The value a channel has where the stage is drawn: the drawn position of the stage's mechanism that publishes it,
	 * or else the value at which a mechanism of the stage that follows it stands where it is drawn, or else its rest, 0.
	 */
	[[nodiscard]] double DrawnValue(const FMotionSchedule& Schedule, const FMuseeKey& Channel);

	/** Places the stage's timed pieces and their controls, and its derived channels. */
	void DressPerformancesAndChannels(const FMuseeStageContext& Stage, const FMotionSchedule& Schedule, TArray<FMuseeFinding>& OutFindings);

	/** Makes the stage's pieces of variable length work: a telescope's carriages, a column's turns. */
	void DressExtents(const FMuseeStageContext& Stage, const FMotionSchedule& Schedule, TArray<FMuseeFinding>& OutFindings);
}
