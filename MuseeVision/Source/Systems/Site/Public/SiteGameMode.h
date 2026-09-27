#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

class ALevelInstance;

#include "SiteGameMode.generated.h"

/**
 * The museum's game mode: the visit begins with the visitor's body standing at the site plan's arrival point. Studio
 * writes the arrival into the museum level as a player start tagged Musee::Site::ArrivalTag, at the floor under the
 * visitor's feet; the body is Visitor's pawn, found by its class path so Site never needs Visitor's headers.
 *
 * The stages stream in through their Level Instances, which in a game load asynchronously after the museum level
 * itself. So a visitor is never started before the museum is there: every stage has streamed in and been made visible,
 * and the floor under the start answers the body's collision channel. Until then the visitor has no body, and no
 * spectator stands in for one. The body is then placed standing on that floor, its centre raised by its own capsule's
 * half height.
 *
 * Nothing waits forever. A stage that cannot stream in (its package is missing, or its streaming failed) is logged once
 * as an error, and the visit goes on without it. Once every stage has streamed in or failed, the floor under the arrival
 * either answers the body or never will; if it never will, the visit fails loudly: an error in the log, a message
 * dialog a packaged game shows, and the game quits. A start that is not the arrival (the editor's play-from-here camera,
 * or a level with no arrival) promises no floor: the body starts there once the stages have settled.
 */
UCLASS()
class MUSEESITE_API ASiteGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASiteGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Whether the visit could not begin: every stage had streamed in or failed, and nothing under the arrival held the body. */
	[[nodiscard]] bool HasArrivalFailed() const { return bArrivalFailed; }

private:
	/**
	 * Whether every stage the level places has settled: streamed in and been made visible, or failed and never will.
	 * Logs each failed stage once.
	 */
	[[nodiscard]] bool StagesSettled();

	/** Whether the floor under a start answers the body's collision channel. */
	[[nodiscard]] bool FloorAnswersAt(const AActor& Start) const;

	/**
	 * Starts a visitor at a start once the stages have settled, or fails the visit if nothing there holds the body.
	 * Returns false while the stages are still streaming in.
	 */
	bool TryArrive(AController& Player, const AActor& Start);

	/** Ends a visit whose arrival has no floor, loudly. */
	void FailArrival(AController& Player, const AActor& Start);

	/** The visitors waiting for the museum to stream in before they start. */
	TArray<TWeakObjectPtr<AController>> Waiting;

	/** The stages that failed to stream in, each logged once. */
	TArray<TWeakObjectPtr<const ALevelInstance>> FailedStages;

	/** The world time when the first visitor began to wait, for the arrival's diagnostic. */
	double WaitingSince = 0.0;

	/** Whether the visit failed at its arrival. */
	bool bArrivalFailed = false;
};

namespace Musee::Site
{
	/** The tag of the player start that marks the arrival. */
	inline constexpr const TCHAR* ArrivalTag = TEXT("musee.arrival");

	/** The visitor's body: Visitor's pawn class. */
	inline constexpr const TCHAR* VisitorBodyClassPath = TEXT("/Script/MuseeVisitor.VisitorBody");
}
