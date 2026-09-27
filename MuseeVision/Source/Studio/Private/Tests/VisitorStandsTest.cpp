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
#include "UObject/UnrealType.h"
#include "VisitorBody.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * The visitor stands where they arrive: the made museum level played in a real game world (PIE) under the site's game
 * mode. Play in the editor streams the stages in before it starts the player, so the test then starts the visitor a
 * second time as a packaged game does: with the stages still streaming in through their Level Instances, and with two
 * stages that never will. Last, the arrival's own stage is broken, and the visit must fail loudly rather than wait.
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

		/** The stage the arrival stands in. */
		const TCHAR* const ArrivalStage = TEXT("Rotunda");

		/** A level that is not in the build, and a package that is but holds no level (as a broken package would). */
		const TCHAR* const MissingLevel = TEXT("/Game/Generated/Nowhere/L_Nowhere.L_Nowhere");
		const TCHAR* const NotALevel = TEXT("/Engine/BasicShapes/Cube.Cube");

		/** The stages this test adds, broken on purpose, by name; and the tag it takes them away again by. */
		const TCHAR* const MissingStage = TEXT("BrokenStage_Missing");
		const TCHAR* const NotALevelStage = TEXT("BrokenStage_NotALevel");
		const FName BrokenStageTag(TEXT("musee.test.broken-stage"));

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

		/** Points a stage at a level. A Level Instance refuses a level that is not there, so the test sets it as a level file does. */
		void PointStageAt(ALevelInstance& Stage, const TCHAR* Level)
		{
			const FSoftObjectProperty* WorldAsset = FindFProperty<FSoftObjectProperty>(ALevelInstance::StaticClass(), TEXT("WorldAsset"));
			check(WorldAsset);
			WorldAsset->SetPropertyValue_InContainer(&Stage, FSoftObjectPtr(FSoftObjectPath(Level)));
		}

		/** Adds two stages that can never stream in: one whose level is missing, one whose package holds no level. */
		void AddBrokenStages(UWorld& Play)
		{
			const TPair<const TCHAR*, const TCHAR*> Broken[] = {{MissingStage, MissingLevel}, {NotALevelStage, NotALevel}};
			for (const TPair<const TCHAR*, const TCHAR*>& Each : Broken)
			{
				FActorSpawnParameters Spawn;
				Spawn.Name = FName(Each.Key);
				ALevelInstance* Stage = Play.SpawnActor<ALevelInstance>(ALevelInstance::StaticClass(), FTransform::Identity, Spawn);
				Stage->Tags.Add(BrokenStageTag);
				Stage->SetDesiredRuntimeBehavior(ELevelInstanceRuntimeBehavior::LevelStreaming);
				PointStageAt(*Stage, Each.Value);
			}
		}

		/** Breaks the arrival's own stage: its level is no longer there. */
		void BreakArrivalStage(UWorld& Play)
		{
			for (TActorIterator<ALevelInstance> It(&Play); It; ++It)
			{
				if (It->GetName() == ArrivalStage)
				{
					PointStageAt(**It, MissingLevel);
				}
			}
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
		 * Starts the visitor again as a packaged game starts it: the visitor leaves, every stage streams out (and the
		 * stages this test broke on purpose are taken away), the museum is changed as the step asks, and the game mode
		 * restarts the visitor in the frame the stages are asked to stream back in.
		 */
		class FArriveWhileStreaming : public IAutomationLatentCommand
		{
		public:
			FArriveWhileStreaming(FAutomationTestBase& InTest, TFunction<void(UWorld&)> InChange)
				: Test(InTest), Change(MoveTemp(InChange)), Deadline(FPlatformTime::Seconds() + PlayPatienceSeconds) {}

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
					for (TActorIterator<ALevelInstance> It(Play); It; ++It)
					{
						if (It->Tags.Contains(BrokenStageTag)) { It->Destroy(); }
						else { It->UnloadLevelInstance(); }
					}
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
				Change(*Play);
				for (TActorIterator<ALevelInstance> It(Play); It; ++It) { It->LoadLevelInstance(); }
				Mode->RestartPlayer(Player);
				return true;
			}

		private:
			FAutomationTestBase& Test;
			TFunction<void(UWorld&)> Change;
			double Deadline;
			bool bStreamedOut = false;
		};

		/** Watches an arrival that has no floor: the visit must fail, loudly, and neither start the visitor nor wait. */
		class FWatchArrivalFails : public IAutomationLatentCommand
		{
		public:
			explicit FWatchArrivalFails(FAutomationTestBase& InTest) : Test(InTest), Deadline(FPlatformTime::Seconds() + PlayPatienceSeconds) {}

			virtual bool Update() override
			{
				UWorld* Play = PlayWorld();
				const ASiteGameMode* Mode = Play ? Cast<ASiteGameMode>(Play->GetAuthGameMode()) : nullptr;
				const APlayerController* Player = Play ? Play->GetFirstPlayerController() : nullptr;
				if (Player && Player->GetPawn())
				{
					Test.AddError(TEXT("With the arrival's stage broken, the visitor was started with no floor under the arrival"));
					return true;
				}
				if (!Mode)
				{
					Test.AddInfo(TEXT("With the arrival's stage broken, the visit ended."));
					return true;
				}
				if (Mode->HasArrivalFailed())
				{
					Test.AddInfo(TEXT("With the arrival's stage broken, the game mode failed the arrival."));
					return true;
				}
				if (FPlatformTime::Seconds() < Deadline) { return false; }
				Test.AddError(TEXT("With the arrival's stage broken, the visit waited and never failed"));
				return true;
			}

		private:
			FAutomationTestBase& Test;
			double Deadline;
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
	// The broken stages are named once each by the game mode, and the broken arrival fails the visit.
	constexpr EAutomationExpectedErrorFlags::MatchType Contains = EAutomationExpectedErrorFlags::Contains;
	AddExpectedError(FString::Printf(TEXT("The stage %s did not stream in"), MissingStage), Contains, 1, false);
	AddExpectedError(FString::Printf(TEXT("The stage %s did not stream in"), NotALevelStage), Contains, 1, false);
	AddExpectedError(FString::Printf(TEXT("The stage %s did not stream in"), ArrivalStage), Contains, 1, false);
	AddExpectedError(TEXT("The museum cannot open"), Contains, 1, false);
	// What the engine says of them: the editor refuses a missing level, and a package with no level fails to stream.
	AddExpectedError(TEXT("because it refers to an invalid package"), Contains, 0, false);
	AddExpectedMessage(TEXT("Failed to find a world in package"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 0, false);

	// Opens the museum level in the editor and plays it (UEditorEngine::AutomationLoadMap), under its own game mode.
	TestTrue(TEXT("the museum level opens"), AutomationOpenMap(Museum));
	ADD_LATENT_AUTOMATION_COMMAND(FWatchVisitorStand(*this, TEXT("When play starts"), false));
	ADD_LATENT_AUTOMATION_COMMAND(FArriveWhileStreaming(*this, &AddBrokenStages));
	ADD_LATENT_AUTOMATION_COMMAND(FWatchVisitorStand(*this, TEXT("Arriving while the stages stream in, two of them broken"), true));
	ADD_LATENT_AUTOMATION_COMMAND(FArriveWhileStreaming(*this, &BreakArrivalStage));
	ADD_LATENT_AUTOMATION_COMMAND(FWatchArrivalFails(*this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FOpenUntitledLevel());
	return true;
}

#endif
