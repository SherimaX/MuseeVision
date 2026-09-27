#include "SiteGameMode.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
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

		/** Whether a start is the museum's arrival, the player start Studio writes at the floor under the visitor's feet. */
		bool IsArrival(const AActor& Start)
		{
			const APlayerStart* Player = Cast<APlayerStart>(&Start);
			return Player && Player->PlayerStartTag == FName(ArrivalTag);
		}

		/**
		 * Whether a stage's level can never stream in: its package is not in this build (an editor or -game run refuses
		 * such a stage before it streams, so it is never loading), or its streaming has failed (a packaged game streams
		 * a missing or broken package and ends there).
		 */
		bool StageFailed(const UWorld& World, const ALevelInstance& Stage)
		{
			if (!FPackageName::DoesPackageExist(Stage.GetWorldAssetPackage()))
			{
				return true;
			}
			const ILevelInstanceInterface* Instance = &Stage;
			for (const ULevelStreaming* Streaming : World.GetStreamingLevels())
			{
				const ULevelStreamingLevelInstance* Streams = Cast<ULevelStreamingLevelInstance>(Streaming);
				if (Streams && Streams->GetLevelInstance() == Instance && Streams->GetLevelStreamingState() == ELevelStreamingState::FailedToLoad)
				{
					return true;
				}
			}
			return false;
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
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		if (Musee::Site::IsArrival(**It))
		{
			return *It;
		}
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

bool ASiteGameMode::StagesSettled()
{
	const UWorld* World = GetWorld();
	const ULevelInstanceSubsystem* Instances = World->GetSubsystem<ULevelInstanceSubsystem>();
	bool bSettled = true;
	for (TActorIterator<ALevelInstance> It(World); It; ++It)
	{
		const ULevel* Level = Instances && Instances->IsLoaded(*It) ? Instances->GetLevelInstanceLevel(*It) : nullptr;
		if (!It->IsWorldAssetValid() || (Level && Level->bIsVisible) || FailedStages.Contains(*It))
		{
			continue;
		}
		if (!Musee::Site::StageFailed(*World, **It))
		{
			bSettled = false;
			continue;
		}
		FailedStages.Add(*It);
		UE_LOG(LogMuseeSite, Error, TEXT("The stage %s did not stream in from %s: the visit goes on without it."), *It->GetName(), *It->GetWorldAssetPackage());
	}
	return bSettled && !World->IsVisibilityRequestPending();
}

bool ASiteGameMode::FloorAnswersAt(const AActor& Start) const
{
	// A line down through where the body will stand, from a body's height above the floor to a body's height below it.
	const FVector Reach(0.0, 0.0, 2.0 * Musee::Site::BodyHalfHeight(DefaultPawnClass));
	const FVector Floor = Start.GetActorLocation();
	const FCollisionQueryParams Query(SCENE_QUERY_STAT(MuseeSiteArrival), /*bTraceComplex*/ false, &Start);
	return GetWorld()->LineTraceTestByChannel(Floor + Reach, Floor - Reach, Musee::Collision::Body.Channel, Query);
}

bool ASiteGameMode::TryArrive(AController& Player, const AActor& Start)
{
	if (!StagesSettled())
	{
		return false;
	}
	// Every stage is now in, or never will be; a stage's collision is in the physics scene once its level is visible.
	// So a floor that does not answer now never will. Only the museum's arrival promises a floor: any other start (the
	// editor's play-from-here camera, or a level with no arrival) is the engine's, and the body starts there as it would.
	if (!Musee::Site::IsArrival(Start) || FloorAnswersAt(Start))
	{
		if (Waiting.Contains(&Player))
		{
			UE_LOG(LogMuseeSite, Log, TEXT("The museum has streamed in after %.2f s: the visitor arrives."), GetWorld()->GetTimeSeconds() - WaitingSince);
		}
		Super::RestartPlayer(&Player);
	}
	else
	{
		FailArrival(Player, Start);
	}
	return true;
}

void ASiteGameMode::FailArrival(AController& Player, const AActor& Start)
{
	bArrivalFailed = true;
	TArray<FString> Failed;
	for (const TWeakObjectPtr<const ALevelInstance>& Stage : FailedStages)
	{
		if (const ALevelInstance* Level = Stage.Get())
		{
			Failed.Add(FString::Printf(TEXT("%s (%s)"), *Level->GetName(), *Level->GetWorldAssetPackage()));
		}
	}
	const FString Why = FString::Printf(TEXT("The museum cannot open: every stage has streamed in or failed, and nothing under the arrival %s holds the visitor up. Stages that did not stream in: %s."),
		*Start.GetName(), Failed.IsEmpty() ? TEXT("none") : *FString::Join(Failed, TEXT(", ")));
	// The log is for the editor and development builds; the dialog is what a packaged visit shows, and the visit then ends.
	UE_LOG(LogMuseeSite, Error, TEXT("%s"), *Why);
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Why), FText::FromString(TEXT("Musée Vision")));
	UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(&Player), EQuitPreference::Quit, /*bIgnorePlatformRestrictions*/ false);
}

void ASiteGameMode::RestartPlayer(AController* NewPlayer)
{
	AActor* Start = NewPlayer && DefaultPawnClass ? FindPlayerStart(NewPlayer) : nullptr;
	if (!Start)
	{
		Super::RestartPlayer(NewPlayer);
		return;
	}
	if (TryArrive(*NewPlayer, *Start))
	{
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
		if (Controller && Start && !TryArrive(*Controller, *Start))
		{
			continue;
		}
		Waiting.Remove(Player);
		if (Controller && !Start)
		{
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
