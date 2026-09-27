#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

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

private:
	/** Whether every stage the level places has streamed in and been made visible. */
	[[nodiscard]] bool StagesStreamedIn() const;

	/** Whether the museum is there for a visitor at this start: the stages streamed in, and a floor under the start. */
	[[nodiscard]] bool IsReadyAt(const AActor& Start) const;

	/** The visitors waiting for the museum to stream in before they start. */
	TArray<TWeakObjectPtr<AController>> Waiting;

	/** The world time when the first visitor began to wait, for the arrival's diagnostic. */
	double WaitingSince = 0.0;
};

namespace Musee::Site
{
	/** The tag of the player start that marks the arrival. */
	inline constexpr const TCHAR* ArrivalTag = TEXT("musee.arrival");

	/** The visitor's body: Visitor's pawn class. */
	inline constexpr const TCHAR* VisitorBodyClassPath = TEXT("/Script/MuseeVisitor.VisitorBody");
}
