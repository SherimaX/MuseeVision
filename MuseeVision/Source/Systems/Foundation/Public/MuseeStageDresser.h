#pragma once

#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "MuseeFrame.h"
#include "MuseeKey.h"

class AActor;

/**
 * A named place a stage declares for the systems that furnish it: a wall a painting hangs on, a mount a luminaire sits
 * on, a bed a plant grows in, a moving body a mechanism drives. Its frame is in the stage's own frame.
 */
struct FMuseeStageAnchor
{
	/** The anchor's key, dot-scoped under its stage: nympheas.wall.north, nympheas.basin. */
	FMuseeKey Key;
	/** What kind of place it is, in the words of the system that uses it: Wall, Mount, Bed, Body, Seat. */
	FName Kind;
	/** Where it is, in the stage's frame. */
	FMuseeFrame Frame;
};

/**
 * What a stage offers the systems that dress it while it is being made: which stage it is, the folder holding its
 * schedules (Data/Stages/<stage>: Finishes, Lighting, Hang, Planting), its anchors, and the actor in its level that
 * owns what they place. Studio builds one per stage and hands it to every registered dresser in turn.
 */
struct FMuseeStageContext
{
	FMuseeKey Stage;
	FString ScheduleDir;
	TArray<FMuseeStageAnchor> Anchors;
	AActor* StageActor = nullptr;

	/** The anchor with this key, or nullptr. */
	const FMuseeStageAnchor* FindAnchor(const FMuseeKey& Key) const
	{
		return Anchors.FindByPredicate([&Key](const FMuseeStageAnchor& A) { return A.Key == Key; });
	}
};

/**
 * One system's step in furnishing a stage at making time: it reads its own schedule from the context's folder and
 * places its components on the stage actor (Light its luminaires, Art its works, Motion its mechanisms, Nature its
 * water and plants). Anything wrong (a missing anchor, an unknown key) is a finding, never a silent default.
 */
using FMuseeStageDresser = TFunction<void(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings)>;

namespace Musee
{
	/**
	 * Registers a system's dresser, usually from its module's startup. Dressers run in ascending Order, so a system
	 * that places anchors others attach to (Motion's moving bodies) runs before them.
	 */
	MUSEEFOUNDATION_API void RegisterStageDresser(FName System, int32 Order, FMuseeStageDresser Dresser);

	/** Removes a system's dresser (its module's shutdown). */
	MUSEEFOUNDATION_API void UnregisterStageDresser(FName System);

	/** Runs every registered dresser on a stage, in order. Studio calls this once per stage it makes. */
	MUSEEFOUNDATION_API void DressStage(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings);

	/** The systems currently registered, in the order they run. */
	MUSEEFOUNDATION_API TArray<FName> StageDressers();
}
