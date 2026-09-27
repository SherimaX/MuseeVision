#include "SiteGameMode.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "SiteLog.h"
#include "UObject/SoftObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SiteGameMode)

ASiteGameMode::ASiteGameMode()
{
	// The body is Visitor's; until InitGame finds it there is none, and never a stand-in.
	DefaultPawnClass = nullptr;
}

void ASiteGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	const TSoftClassPtr<APawn> Body{FSoftObjectPath(Musee::Site::VisitorBodyClassPath)};
	DefaultPawnClass = Body.LoadSynchronous();
	if (!DefaultPawnClass)
	{
		UE_LOG(LogMuseeSite, Error, TEXT("The visitor's body %s is not in this build: the visit has no body."), Musee::Site::VisitorBodyClassPath);
	}
}

AActor* ASiteGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const FName Arrival(Musee::Site::ArrivalTag);
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (It->PlayerStartTag == Arrival)
		{
			return *It;
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}
