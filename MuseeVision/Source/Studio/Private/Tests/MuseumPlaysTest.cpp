#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Make/EngineUnits.h"
#include "Make/Make.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/EditorPlay.h"
#include "Tests/ProofPlan.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Studio
{
	namespace
	{
		/** How long play may take to stream the stage in, in seconds of wall time. */
		constexpr double StreamingPatienceSeconds = 60.0;

		/** Where the proof's west column must stand once the museum plays: the stage's translation plus its place in the stage. */
		struct FExpectedColumn
		{
			FString StageName;
			FString ComponentName;
			FVector WorldLocation;
		};

		/**
		 * Waits for play to stream the proof stage in through its Level Instance, then checks that the stage's column
		 * stands at the museum's translation of its place in the stage.
		 */
		class FExpectStageInPlay : public IAutomationLatentCommand
		{
		public:
			FExpectStageInPlay(FAutomationTestBase& InTest, FExpectedColumn InExpected)
				: Test(InTest), Expected(MoveTemp(InExpected)), Deadline(FPlatformTime::Seconds() + StreamingPatienceSeconds) {}

			virtual bool Update() override
			{
				const UStaticMeshComponent* Column = FindColumn();
				if (!Column)
				{
					if (FPlatformTime::Seconds() > Deadline)
					{
						Test.AddError(TEXT("the proof stage never streamed into play"));
						return true;
					}
					return false;
				}
				Test.TestEqual(TEXT("in play, the column stands at the stage's translation plus its place"),
					Column->GetComponentLocation(), Expected.WorldLocation, 0.1f);
				Test.AddInfo(FString::Printf(TEXT("In play the stage streamed in through its Level Instance; %s stands at %s (engine units)."),
					*Expected.ComponentName, *Column->GetComponentLocation().ToString()));
				return true;
			}

		private:
			const UStaticMeshComponent* FindColumn() const
			{
				UWorld* Play = GEditor ? GEditor->PlayWorld.Get() : nullptr;
				ULevelInstanceSubsystem* Instances = Play ? Play->GetSubsystem<ULevelInstanceSubsystem>() : nullptr;
				if (!Instances)
				{
					return nullptr;
				}
				for (TActorIterator<ALevelInstance> It(Play); It; ++It)
				{
					ULevel* Level = Instances->IsLoaded(*It) ? Instances->GetLevelInstanceLevel(*It) : nullptr;
					if (!Level)
					{
						continue;
					}
					for (AActor* Actor : Level->Actors)
					{
						if (!Actor || Actor->GetName() != Expected.StageName) { continue; }
						for (UActorComponent* Component : Actor->GetComponents())
						{
							if (Component->GetName() == Expected.ComponentName) { return Cast<UStaticMeshComponent>(Component); }
						}
					}
				}
				return nullptr;
			}

			FAutomationTestBase& Test;
			FExpectedColumn Expected;
			double Deadline;
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioMuseumPlaysTest, "Musee.Studio.MuseumPlays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStudioMuseumPlaysTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;

	FMakePlan Plan = ProofPlan();
	const FMakeStage& Stage = Plan.Stages[0];
	const FMakePlacement& West = Stage.Placements[0];
	FExpectedColumn Expected{Stage.Name, West.Name, EngineFromMeters(Stage.TranslationMeters + West.LocationMeters)};
	if (!TestTrue(TEXT("the proof is made"), Make(Plan).Succeeded()))
	{
		return false;
	}
	// Opens the museum level in the editor and starts play (UEditorEngine::AutomationLoadMap).
	TestTrue(TEXT("the museum level opens"), AutomationOpenMap(ProofPaths().MuseumLevel()));
	ADD_LATENT_AUTOMATION_COMMAND(FExpectStageInPlay(*this, MoveTemp(Expected)));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FOpenUntitledLevel());
	return true;
}

#endif
