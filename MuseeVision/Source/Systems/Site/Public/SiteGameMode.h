#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "SiteGameMode.generated.h"

/**
 * The museum's game mode: the visit begins with the visitor's body standing at the site plan's arrival point. Studio
 * writes the arrival into the museum level as a player start tagged Musee::Site::ArrivalTag; the body is Visitor's
 * pawn, found by its class path so Site never needs Visitor's headers.
 */
UCLASS()
class MUSEESITE_API ASiteGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASiteGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};

namespace Musee::Site
{
	/** The tag of the player start that marks the arrival. */
	inline constexpr const TCHAR* ArrivalTag = TEXT("musee.arrival");

	/** The visitor's body: Visitor's pawn class. */
	inline constexpr const TCHAR* VisitorBodyClassPath = TEXT("/Script/MuseeVisitor.VisitorBody");
}
