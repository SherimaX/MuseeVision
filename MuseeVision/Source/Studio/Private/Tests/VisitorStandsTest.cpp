#include "Components/PrimitiveComponent.h"
#include "Editor.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "MuseeCollision.h"
#include "MuseeHumanMeasure.h"
#include "MuseeUnits.h"
#include "SiteGameMode.h"
#include "Stages/StageMake.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/EditorPlay.h"
#include "VisitorBody.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * The visitor stands where they arrive: the made museum level played in a real game world (PIE) under the site's game
 * mode. Play in the editor streams the stages in before it starts the player, so the test then starts the visitor a
 * second time as a packaged game does: with the stages still streaming in through their Level Instances.
 */
namespace Musee::Studio
{
	namespace
	{
		/** How long the visitor stands before being measured, in seconds of the play world's own time. */
		constexpr double StandSeconds = 5.0;

		/** How near the feet stand to the floor they arrived on, and a floor answers the body to its own top. */
		constexpr Musee::Meters StandTolerance{0.02};

		/** How long each step of play may take, in seconds of wall time. */
		constexpr double PlayPatienceSeconds = 180.0;

		/** A second stage, and a floor of it, whose collision the body must meet. */
		const TCHAR* const OtherStage = TEXT("Salon");
		const TCHAR* const OtherFloorTag = TEXT("musee.piece:floor.bay1");

		UWorld* PlayWorld() { return GEditor ? GEditor->PlayWorld.Get() : nullptr; }

		double Metres(double EngineLength) { return FromEngine<Musee::Meters>(EngineLength).Get(); }

		/** The first thing the body's channel meets on a line down from one point to another. */
		bool FirstBelow(const UWorld& World, const FVector& From, const FVector& To, const AActor* Ignored, FHitResult& OutHit)
		{
			const FCollisionQueryParams Query(SCENE_QUERY_STAT(MuseeVisitorStands), /*bTraceComplex*/ false, Ignored);
			return World.LineTraceSingleByChannel(OutHit, From, To, Musee::Collision::Body.Channel, Query);
		}

		const APlayerStart* FindArrival(UWorld& Play)
		{
			for (TActorIterator<APlayerStart> It(&Play); It; ++It)
			{
				if (It->PlayerStartTag == FName(Musee::Site::ArrivalTag)) { return *It; }
			}
			return nullptr;
		}

		/** The piece of a stage in play with a tag, or null while the stage has not streamed in. */
		const UPrimitiveComponent* FindStagePiece(UWorld& Play, const TCHAR* Stage, const TCHAR* Tag)
		{
			const ULevelInstanceSubsystem* Instances = Play.GetSubsystem<ULevelInstanceSubsystem>();
			for (TActorIterator<ALevelInstance> It(&Play); Instances && It; ++It)
			{
				const ULevel* Level = It->GetName() == Stage && Instances->IsLoaded(*It) ? Instances->GetLevelInstanceLevel(*It) : nullptr;
				if (!Level)
				{
					continue;
				}
				for (const AActor* Actor : Level->Actors)
				{
					TInlineComponentArray<UPrimitiveComponent*> Pieces(Actor);
					for (const UPrimitiveComponent* Piece : Pieces)
					{
						if (Piece->ComponentHasTag(FName(Tag))) { return Piece; }
					}
				}
			}
			return nullptr;
		}

		/**
		 * Watches one arrival: finds the visitor when it arrives, checks that the floor under the arrival answers the body
		 * then, lets the world run five seconds, and measures where the feet stand; then, if asked, meets a floor of
		 * another stage.
		 */
		class FWatchVisitorStand : public IAutomationLatentCommand
		{
		public:
			FWatchVisitorStand(FAutomationTestBase& InTest, const TCHAR* InWhen, bool bInMeetOtherStage)
				: Test(InTest), When(InWhen), bMeetOtherStage(bInMeetOtherStage), Deadline(FPlatformTime::Seconds() + PlayPatienceSeconds) {}

			virtual bool Update() override
			{
				UWorld* Play = PlayWorld();
				const APlayerController* Player = Play ? Play->GetFirstPlayerController() : nullptr;
				const AVisitorBody* Body = Player ? Cast<AVisitorBody>(Player->GetPawn()) : nullptr;
				const APlayerStart* Arrival = Play ? FindArrival(*Play) : nullptr;
				if (!Body || !Arrival)
				{
					return GiveUpAfterDeadline(TEXT("the visitor never arrived"));
				}
				if (ArrivedAt < 0.0)
				{
					ArrivedAt = Play->GetTimeSeconds();
					CheckFloorAtArrival(*Play, *Body, *Arrival);
				}
				if (Play->GetTimeSeconds() - ArrivedAt < StandSeconds)
				{
					return GiveUpAfterDeadline(TEXT("play never ran five seconds"));
				}
				CheckStanding(*Body, *Arrival);
				if (bMeetOtherStage)
				{
					CheckOtherFloor(*Play, *Body);
				}
				return true;
			}

		private:
			bool GiveUpAfterDeadline(const TCHAR* Why) const
			{
				if (FPlatformTime::Seconds() < Deadline) { return false; }
				Test.AddError(FString::Printf(TEXT("%s: %s"), When, Why));
				return true;
			}

			/** When the visitor arrives, the floor under the arrival has streamed in and answers the body's channel. */
			void CheckFloorAtArrival(const UWorld& Play, const AVisitorBody& Body, const APlayerStart& Arrival) const
			{
				const FVector Floor = Arrival.GetActorLocation();
				const FVector Reach(0.0, 0.0, ToEngine(FMuseeHumanMeasure::Stature));
				FHitResult Hit;
				const bool bFound = FirstBelow(Play, Floor + Reach, Floor - Reach, &Body, Hit);
				Test.TestTrue(FString::Printf(TEXT("%s, the arrival's floor answers the body as the visitor arrives (found %s)"), When,
					bFound ? *FString::Printf(TEXT("%s at z %.4f m"), *GetNameSafe(Hit.GetComponent()), Metres(Hit.ImpactPoint.Z)) : TEXT("nothing")),
					bFound && FMath::Abs(Hit.ImpactPoint.Z - Floor.Z) <= ToEngine(StandTolerance));
			}

			/** After five seconds the feet stand on the arrival's floor. */
			void CheckStanding(const AVisitorBody& Body, const APlayerStart& Arrival) const
			{
				const double Feet = Body.FeetLocation().Z;
				const double Floor = Arrival.GetActorLocation().Z;
				Test.AddInfo(FString::Printf(TEXT("%s, after %.1f s the visitor's feet are at z %.4f m; the arrival's floor is at z %.4f m."),
					When, StandSeconds, Metres(Feet), Metres(Floor)));
				Test.TestTrue(FString::Printf(TEXT("%s, the feet stand within %.0f cm of the arrival's floor (%.4f m away)"),
					When, StandTolerance.Get() * 100.0, Metres(Feet - Floor)), FMath::Abs(Feet - Floor) <= ToEngine(StandTolerance));
			}

			/** Another stage's floor answers the body's channel at its own top. */
			void CheckOtherFloor(UWorld& Play, const AVisitorBody& Body) const
			{
				const UPrimitiveComponent* Piece = FindStagePiece(Play, OtherStage, OtherFloorTag);
				if (!Test.TestNotNull(FString::Printf(TEXT("the %s's floor (%s) has streamed into play"), OtherStage, OtherFloorTag), Piece))
				{
					return;
				}
				const FBox Bounds = Piece->Bounds.GetBox();
				const double Step = ToEngine(FMuseeHumanMeasure::MaxStepHeight);
				const FVector Centre = Bounds.GetCenter();
				FHitResult Hit;
				const bool bFound = FirstBelow(Play, FVector(Centre.X, Centre.Y, Bounds.Max.Z + Step), FVector(Centre.X, Centre.Y, Bounds.Min.Z - Step), &Body, Hit);
				Test.AddInfo(FString::Printf(TEXT("A trace down the body's channel in the %s meets %s at z %.4f m; the floor's top is at z %.4f m."),
					OtherStage, bFound ? *GetNameSafe(Hit.GetComponent()) : TEXT("nothing"), Metres(Hit.ImpactPoint.Z), Metres(Bounds.Max.Z)));
				Test.TestTrue(FString::Printf(TEXT("the %s's floor answers the body at its top"), OtherStage),
					bFound && FMath::Abs(Hit.ImpactPoint.Z - Bounds.Max.Z) <= ToEngine(StandTolerance));
			}

			FAutomationTestBase& Test;
			const TCHAR* When;
			bool bMeetOtherStage;
			double Deadline;
			double ArrivedAt = -1.0;
		};

		/**
		 * Starts the visitor again as a packaged game starts it: the visitor leaves, every stage streams out, and the game
		 * mode restarts the visitor in the frame the stages are asked to stream back in.
		 */
		class FArriveWhileStreaming : public IAutomationLatentCommand
		{
		public:
			explicit FArriveWhileStreaming(FAutomationTestBase& InTest) : Test(InTest), Deadline(FPlatformTime::Seconds() + PlayPatienceSeconds) {}

			virtual bool Update() override
			{
				UWorld* Play = PlayWorld();
				APlayerController* Player = Play ? Play->GetFirstPlayerController() : nullptr;
				AGameModeBase* Mode = Play ? Play->GetAuthGameMode() : nullptr;
				const ULevelInstanceSubsystem* Instances = Play ? Play->GetSubsystem<ULevelInstanceSubsystem>() : nullptr;
				if (!Player || !Mode || !Instances)
				{
					Test.AddError(TEXT("play has no visitor, game mode or Level Instances to start again"));
					return true;
				}
				if (!bStreamedOut)
				{
					if (APawn* Body = Player->GetPawn())
					{
						Player->UnPossess();
						Body->Destroy();
					}
					for (TActorIterator<ALevelInstance> It(Play); It; ++It) { It->UnloadLevelInstance(); }
					bStreamedOut = true;
					return false;
				}
				for (TActorIterator<ALevelInstance> It(Play); It; ++It)
				{
					if (Instances->IsLoaded(*It) || Instances->IsLoading(*It))
					{
						if (FPlatformTime::Seconds() < Deadline) { return false; }
						Test.AddError(TEXT("the stages never streamed out"));
						return true;
					}
				}
				for (TActorIterator<ALevelInstance> It(Play); It; ++It) { It->LoadLevelInstance(); }
				Mode->RestartPlayer(Player);
				return true;
			}

		private:
			FAutomationTestBase& Test;
			double Deadline;
			bool bStreamedOut = false;
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSiteVisitorStandsTest, "Musee.Site.VisitorStands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSiteVisitorStandsTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;

	const FString Museum = FStageMakeOptions().Paths.MuseumLevel();
	if (!TestTrue(FString::Printf(TEXT("the museum level %s is made (make.ps1 rotunda, salon, then site)"), *Museum), FPackageName::DoesPackageExist(Museum)))
	{
		return false;
	}
	// Opens the museum level in the editor and plays it (UEditorEngine::AutomationLoadMap), under its own game mode.
	TestTrue(TEXT("the museum level opens"), AutomationOpenMap(Museum));
	ADD_LATENT_AUTOMATION_COMMAND(FWatchVisitorStand(*this, TEXT("When play starts"), false));
	ADD_LATENT_AUTOMATION_COMMAND(FArriveWhileStreaming(*this));
	ADD_LATENT_AUTOMATION_COMMAND(FWatchVisitorStand(*this, TEXT("Arriving while the stages stream in"), true));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FOpenUntitledLevel());
	return true;
}

#endif
