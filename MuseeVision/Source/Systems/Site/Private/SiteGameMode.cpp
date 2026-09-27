#include "SiteGameMode.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "MuseeCollision.h"
#include "SiteLog.h"
#include "UObject/SoftObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SiteGameMode)

namespace Musee::Site
{
	namespace
	{
		/** Half the height of the body a controller is given, from that body's own capsule; zero when there is none. */
		double BodyHalfHeight(const UClass* BodyClass)
		{
			const APawn* Body = BodyClass ? BodyClass->GetDefaultObject<APawn>() : nullptr;
			return Body ? Body->GetDefaultHalfHeight() : 0.0;
		}
	}
}

ASiteGameMode::ASiteGameMode()
{
	// The body is Visitor's; until InitGame finds it there is none, and never a stand-in. Nor is a waiting visitor a
	// spectator: the engine's would stand at the arrival, in the place the body is about to take.
	DefaultPawnClass = nullptr;
	SpectatorClass = nullptr;
	// The mode ticks only while a visitor waits for the museum to stream in.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
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

bool ASiteGameMode::StagesStreamedIn() const
{
	const UWorld* World = GetWorld();
	const ULevelInstanceSubsystem* Instances = World->GetSubsystem<ULevelInstanceSubsystem>();
	for (TActorIterator<ALevelInstance> It(World); It; ++It)
	{
		const ULevel* Level = Instances && Instances->IsLoaded(*It) ? Instances->GetLevelInstanceLevel(*It) : nullptr;
		if (It->IsWorldAssetValid() && !(Level && Level->bIsVisible))
		{
			return false;
		}
	}
	return !World->IsVisibilityRequestPending();
}

bool ASiteGameMode::IsReadyAt(const AActor& Start) const
{
	if (!StagesStreamedIn())
	{
		return false;
	}
	// A line down through where the body will stand, from a body's height above the floor to a body's height below it.
	const FVector Reach(0.0, 0.0, 2.0 * Musee::Site::BodyHalfHeight(DefaultPawnClass));
	const FVector Floor = Start.GetActorLocation();
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(MuseeSiteArrival), /*bTraceComplex*/ false, &Start);
	return GetWorld()->LineTraceTestByChannel(Floor + Reach, Floor - Reach, Musee::Collision::Body.Channel, Query);
}

void ASiteGameMode::RestartPlayer(AController* NewPlayer)
{
	AActor* Start = NewPlayer && DefaultPawnClass ? FindPlayerStart(NewPlayer) : nullptr;
	if (!Start || IsReadyAt(*Start))
	{
		Super::RestartPlayer(NewPlayer);
		return;
	}
	if (Waiting.IsEmpty())
	{
		WaitingSince = GetWorld()->GetTimeSeconds();
		UE_LOG(LogMuseeSite, Log, TEXT("The visitor waits at %s for the museum's stages to stream in."), *Start->GetName());
	}
	Waiting.AddUnique(NewPlayer);
	SetActorTickEnabled(true);
}

void ASiteGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const TArray<TWeakObjectPtr<AController>> Arriving = Waiting;
	for (const TWeakObjectPtr<AController>& Player : Arriving)
	{
		AController* Controller = Player.Get();
		AActor* Start = Controller ? FindPlayerStart(Controller) : nullptr;
		if (Controller && Start && !IsReadyAt(*Start))
		{
			continue;
		}
		Waiting.Remove(Player);
		if (Controller)
		{
			UE_LOG(LogMuseeSite, Log, TEXT("The museum has streamed in after %.2f s: the visitor arrives."), GetWorld()->GetTimeSeconds() - WaitingSince);
			Super::RestartPlayer(Controller);
		}
	}
	SetActorTickEnabled(!Waiting.IsEmpty());
}

APawn* ASiteGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// A start marks the floor under the feet, and a body is placed by its centre: it stands on the floor when its centre
	// is its own half height above it. As the engine does, the body keeps only the start's yaw.
	const double HalfHeight = Musee::Site::BodyHalfHeight(GetDefaultPawnClassForController(NewPlayer));
	const FTransform Standing(FRotator(0.0, StartSpot->GetActorRotation().Yaw, 0.0), StartSpot->GetActorLocation() + FVector(0.0, 0.0, HalfHeight));
	return SpawnDefaultPawnAtTransform(NewPlayer, Standing);
}
