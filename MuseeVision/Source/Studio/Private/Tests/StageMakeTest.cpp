#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "Make/PackageSave.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "MuseeCollision.h"
#include "MuseeStageDresser.h"
#include "SiteGameMode.h"
#include "SitePlan.h"
#include "SiteRoom.h"
#include "SiteVolume.h"
#include "Stages/StageFabric.h"
#include "Stages/StageMake.h"
#include "UObject/Package.h"

/**
 * The stage contract end to end on Studio's specimen room: described, made, its door closed by Site's blind, cast,
 * written as a level with its bodies (one riding another), its tagged pieces (identical ones instanced) and its barrier,
 * and left alone by a second make; placed in a museum level with the arrival and the site's game mode; and joined to the
 * specimen annex, with Site's joint cast into the museum. The Atrium and the Sphere, stacked on one axis, stand apart.
 */
namespace Musee::Studio::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

		/** The specimens are made under their own root, so they never touch the museum's assets or Studio's proof. */
		const TCHAR* const SpecimenRoot = TEXT("/Game/Generated/Specimens");

		/** The engine's stock opaque surface: the test makes no claim about any substance. */
		const TCHAR* const StockMaterial = TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial");

		FString TestData(const FString& Relative)
		{
			return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Source/Studio/Private/Tests/Data") / Relative);
		}

		/** Options that make the specimen from its test schedules, every palette key but Refused dressed in stock material. */
		FStageMakeOptions SpecimenOptions(const FString& Root, FName Refused = NAME_None)
		{
			FStageMakeOptions Options;
			Options.Paths = FMakePaths{Root};
			Options.ScheduleDir = TestData(TEXT("Stages/specimen-room"));
			Options.PlanRecord = TestData(TEXT("Site/SitePlan.json"));
			Options.Resolve = [Refused](FName Palette, FName, FName) -> UMaterialInterface*
			{
				return Palette == Refused ? nullptr : LoadObject<UMaterialInterface>(nullptr, StockMaterial);
			};
			return Options;
		}

		const USiteRoom* Specimen(FAutomationTestBase& Test)
		{
			const USiteRoom* Room = USiteRoom::Find(FMuseeKey(TEXT("specimen-room")));
			Test.TestNotNull(TEXT("the specimen room is found by class registration"), Room);
			return Room;
		}

		void LogFindings(FAutomationTestBase& Test, const FMakeReport& Report)
		{
			for (const FMuseeFinding& Finding : Report.Findings) { Test.AddInfo(Finding.ToString()); }
			for (const FMadePackage& Made : Report.Packages)
			{
				if (Made.Outcome == EMadeOutcome::Failed) { Test.AddInfo(FString::Printf(TEXT("%s failed: %s"), *Made.PackageName, *Made.Why)); }
			}
		}

		/** A made level loaded from disk, and let go of afterwards. */
		class FLoadedLevel
		{
		public:
			explicit FLoadedLevel(const FString& PackageName)
				: Package(LoadPackage(nullptr, *PackageName, LOAD_None))
				, World(Package ? UWorld::FindWorldInPackage(Package) : nullptr)
			{
			}
			~FLoadedLevel() { ReleasePackages({Package}); }

			template <typename TActor>
			TActor* Find(TFunctionRef<bool(const TActor&)> Predicate) const
			{
				for (AActor* Actor : World ? World->PersistentLevel->Actors : TArray<TObjectPtr<AActor>>())
				{
					if (TActor* Typed = Cast<TActor>(Actor); Typed && Predicate(*Typed)) { return Typed; }
				}
				return nullptr;
			}

			UPackage* Package;
			UWorld* World;
		};

		USceneComponent* Tagged(AActor& Actor, const FString& Tag)
		{
			for (UActorComponent* Component : Actor.GetInstanceComponents())
			{
				if (Component && Component->ComponentHasTag(FName(*Tag))) { return Cast<USceneComponent>(Component); }
			}
			return nullptr;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioStageMakeTest, "Musee.Studio.StageMake", Flags)
	bool FStudioStageMakeTest::RunTest(const FString& Parameters)
	{
		const USiteRoom* Room = Specimen(*this);
		if (!Room)
		{
			return false;
		}
		const FStageMakeOptions Options = SpecimenOptions(SpecimenRoot);
		const FMakeReport Report = MakeStage(*Room, Options);
		LogFindings(*this, Report);
		TestTrue(TEXT("the specimen is made with nothing found wrong"), Report.Succeeded());

		const FLoadedLevel Level(Options.Paths.StageLevel(TEXT("SpecimenRoom")));
		AActor* Stage = Level.Find<AActor>([](const AActor& A) { return A.GetName() == TEXT("SpecimenRoom"); });
		if (!TestNotNull(TEXT("the level holds the stage's actor"), Stage))
		{
			return false;
		}
		USceneComponent* Plinth = Tagged(*Stage, TEXT("specimen-room.plinth"));
		TestTrue(TEXT("the moving body is a movable scene component tagged with its key"), Plinth && Plinth->Mobility == EComponentMobility::Movable);
		for (const TCHAR* Piece : {TEXT("wall.west"), TEXT("wall.north"), TEXT("floor"), TEXT("post"), TEXT("blind.west.left"), TEXT("blind.west.right")})
		{
			TestNotNull(*FString::Printf(TEXT("piece %s is a tagged mesh component"), Piece),
				Cast<UStaticMeshComponent>(Tagged(*Stage, FString(TEXT("musee.piece:")) + Piece)));
		}
		USceneComponent* Post = Tagged(*Stage, TEXT("musee.piece:post"));
		TestTrue(TEXT("the post rides on its body"), Post && Plinth && Post->GetAttachParent() == Plinth);
		USceneComponent* Floor = Tagged(*Stage, TEXT("musee.piece:floor"));
		TestTrue(TEXT("the floor stands still, on the stage's frame"), Floor && Floor->Mobility == EComponentMobility::Static
			&& Floor->GetAttachParent() == Stage->GetRootComponent());
		const UPrimitiveComponent* Walked = Cast<UPrimitiveComponent>(Floor);
		TestEqual(TEXT("the floor is walked on"), Walked ? Walked->GetCollisionProfileName() : FName(), FName(TEXT("Walkable")));

		const UBoxComponent* Barrier = Cast<UBoxComponent>(Tagged(*Stage, TEXT("specimen-room.plinth.barrier")));
		TestTrue(TEXT("the barrier is an unseen box that stops only the body"), Barrier && Barrier->bHiddenInGame
			&& Barrier->GetCollisionProfileName() == Musee::Collision::Barrier.Profile() && Barrier->GetUnscaledBoxExtent().Equals(FVector(40.0, 40.0, 60.0), 1e-6));
		USceneComponent* Cap = Tagged(*Stage, TEXT("specimen-room.plinth.cap"));
		TestTrue(TEXT("the cap's body rides the plinth's"), Cap && Plinth && Cap->GetAttachParent() == Plinth);
		TestTrue(TEXT("hung where it is drawn, on the post's top"), Cap && Cap->GetRelativeLocation().Equals(FVector(0.0, 0.0, 100.0), 1e-6));
		const UInstancedStaticMeshComponent* Studs = Cast<UInstancedStaticMeshComponent>(Tagged(*Stage, TEXT("musee.piece:stud.a")));
		TestTrue(TEXT("the two identical studs are one instanced component"), Studs && Studs == Tagged(*Stage, TEXT("musee.piece:stud.b")));
		if (Studs)
		{
			TestEqual(TEXT("with an instance for each"), Studs->GetInstanceCount(), 2);
			FTransform Second;
			Studs->GetInstanceTransform(1, Second, /*bWorldSpace*/ false);
			TestTrue(TEXT("the second at its own frame"), Second.GetLocation().Equals(FVector(600.0, 200.0, 0.0), 1e-6));
			TestEqual(TEXT("the studs stop only the body"), Studs->GetCollisionProfileName(), Musee::Collision::Barrier.Profile());
			TestTrue(TEXT("cast once, as the first"), Studs->GetStaticMesh() && Studs->GetStaticMesh()->GetName() == TEXT("SM_SpecimenRoom_stud_a"));
		}

		const FMakeReport Again = MakeStage(*Room, Options);
		TestEqual(TEXT("a second make writes nothing"), Again.Count(EMadeOutcome::Written), 0);
		TestTrue(TEXT("and finds nothing wrong"), Again.Succeeded());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioStageMakeFindingsTest, "Musee.Studio.StageMakeFindings", Flags)
	bool FStudioStageMakeFindingsTest::RunTest(const FString& Parameters)
	{
		const USiteRoom* Room = Specimen(*this);
		if (!Room)
		{
			return false;
		}
		// Matter resolves no lime plaster: the walls' faces have no material, so the walls are left out, and said so.
		const FStageMakeOptions Options = SpecimenOptions(FString(SpecimenRoot) + TEXT("/Refused"), TEXT("white.lime-plaster"));
		const FMakeReport Report = MakeStage(*Room, Options);
		TestFalse(TEXT("a make with an unresolved material doesn't succeed"), Report.Succeeded());
		int32 Unresolved = 0;
		for (const FMuseeFinding& Finding : Report.Findings)
		{
			Unresolved += Finding.Check == FName(TEXT("Matter.Resolved")) ? 1 : 0;
		}
		TestEqual(TEXT("each wall's face is a finding"), Unresolved, 2);
		const FLoadedLevel Level(Options.Paths.StageLevel(TEXT("SpecimenRoom")));
		AActor* Stage = Level.Find<AActor>([](const AActor& A) { return A.GetName() == TEXT("SpecimenRoom"); });
		TestTrue(TEXT("the level is still written, without the walls"), Stage && !Tagged(*Stage, TEXT("musee.piece:wall.west"))
			&& Tagged(*Stage, TEXT("musee.piece:floor")));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioMuseumMakeTest, "Musee.Studio.MuseumMake", Flags)
	bool FStudioMuseumMakeTest::RunTest(const FString& Parameters)
	{
		const USiteRoom* Room = Specimen(*this);
		if (!Room)
		{
			return false;
		}
		const FStageMakeOptions Options = SpecimenOptions(SpecimenRoot);
		TestTrue(TEXT("the specimen's level is made"), MakeStage(*Room, Options).Succeeded());
		const FMakeReport Report = MakeMuseum(Options);
		LogFindings(*this, Report);
		TestTrue(TEXT("the museum level is made with nothing found wrong"), Report.Succeeded());

		const FLoadedLevel Museum(Options.Paths.MuseumLevel());
		TestNotNull(TEXT("the specimen stands in the museum as a Level Instance"),
			Museum.Find<ALevelInstance>([](const ALevelInstance& A) { return A.GetName() == TEXT("SpecimenRoom"); }));
		APlayerStart* Arrival = Museum.Find<APlayerStart>([](const APlayerStart& A) { return A.PlayerStartTag == FName(Musee::Site::ArrivalTag); });
		TestTrue(TEXT("the arrival stands where the plan puts it, facing west"), Arrival
			&& Arrival->GetActorLocation().Equals(FVector(400.0, 0.0, 0.0), 0.01) && FMath::IsNearlyEqual(FMath::Abs(Arrival->GetActorRotation().Yaw), 180.0, 0.01));
		TestTrue(TEXT("the museum plays under the site's game mode"), Museum.World
			&& Museum.World->GetWorldSettings()->DefaultGameMode == ASiteGameMode::StaticClass());
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioJoinedMakeTest, "Musee.Studio.JoinedMake", Flags)
	bool FStudioJoinedMakeTest::RunTest(const FString& Parameters)
	{
		const USiteRoom* Room = Specimen(*this);
		const USiteRoom* Annex = USiteRoom::Find(FMuseeKey(TEXT("specimen-annex")));
		if (!Room || !TestNotNull(TEXT("the specimen annex is found by class registration"), Annex))
		{
			return false;
		}
		FStageMakeOptions Options = SpecimenOptions(FString(SpecimenRoot) + TEXT("/Joined"));
		Options.PlanRecord = TestData(TEXT("Site/SitePlanJoined.json"));
		// A dresser that notes the mated ports each stage's context carries to the systems.
		const FName Listener(TEXT("StudioJoinedTest"));
		TMap<FMuseeKey, TArray<FMuseeKey>> Mated;
		Musee::RegisterStageDresser(Listener, TNumericLimits<int32>::Max(), [&Mated](const FMuseeStageContext& Stage, TArray<FMuseeFinding>&)
		{
			Mated.Add(Stage.Stage, Stage.MatedPorts);
		});
		ON_SCOPE_EXIT { Musee::UnregisterStageDresser(Listener); };

		// Made first with its door unmated (a blind), the room's level is written again once the plan joins it, so its
		// dressers run with the joined plan's context.
		FStageMakeOptions Alone = Options;
		Alone.PlanRecord = TestData(TEXT("Site/SitePlan.json"));
		TestTrue(TEXT("the room is made alone"), MakeStage(*Room, Alone).Succeeded());
		Mated.Reset();
		for (const USiteRoom* Made : {Room, Annex})
		{
			const FMakeReport Report = MakeStage(*Made, Options);
			LogFindings(*this, Report);
			TestTrue(*FString::Printf(TEXT("%s is made with nothing found wrong"), *Made->StageKey().ToString()), Report.Succeeded());
		}
		const TArray<FMuseeKey>* RoomMated = Mated.Find(FMuseeKey(TEXT("specimen-room")));
		TestTrue(TEXT("the room's dressers see its west port mated"), RoomMated && *RoomMated == TArray<FMuseeKey>({FMuseeKey(TEXT("specimen-room.west"))}));
		{
			const FLoadedLevel Level(Options.Paths.StageLevel(TEXT("SpecimenRoom")));
			AActor* Stage = Level.Find<AActor>([](const AActor& A) { return A.GetName() == TEXT("SpecimenRoom"); });
			TestTrue(TEXT("a mated door gets no blind"), Stage && !Tagged(*Stage, TEXT("musee.piece:blind.west.left")));
		}

		const FMakeReport Report = MakeMuseum(Options);
		LogFindings(*this, Report);
		TestTrue(TEXT("the museum is made with nothing found wrong"), Report.Succeeded());
		{
			const FLoadedLevel Joints(Options.Paths.JointsLevel());
			AActor* Actor = Joints.Find<AActor>([](const AActor& A) { return A.GetName() == TEXT("Joints"); });
			if (TestNotNull(TEXT("the joints' level holds the joints' actor"), Actor))
			{
				for (const TCHAR* Part : {TEXT("strip.0"), TEXT("surround.a"), TEXT("surround.b")})
				{
					USceneComponent* Placed = Tagged(*Actor, FString(TEXT("musee.piece:joint.specimen-room.west.")) + Part);
					TestNotNull(*FString::Printf(TEXT("the joint's %s is cast"), Part), Cast<UStaticMeshComponent>(Placed));
				}
				// The joint is drawn from the room's port frame on the mating plane, x -0.6, the strip laid its depth below the floor.
				const USceneComponent* Strip = Tagged(*Actor, TEXT("musee.piece:joint.specimen-room.west.strip.0"));
				TestTrue(TEXT("the strip lies on the mating plane"), Strip && Strip->GetRelativeLocation().Equals(FVector(-60.0, 0.0, -3.0), 1e-6));
			}
			const FLoadedLevel Museum(Options.Paths.MuseumLevel());
			TestNotNull(TEXT("the museum instances the joints' level"),
				Museum.Find<ALevelInstance>([](const ALevelInstance& A) { return A.GetName() == TEXT("Joints"); }));
			TestNotNull(TEXT("and the annex, west of the room"), Museum.Find<ALevelInstance>([](const ALevelInstance& A)
			{
				return A.GetName() == TEXT("SpecimenAnnex") && A.GetActorLocation().Equals(FVector(-60.0, 0.0, 0.0), 1e-6);
			}));
		}
		const FMakeReport Again = MakeMuseum(Options);
		TestEqual(TEXT("a second site make writes nothing"), Again.Count(EMadeOutcome::Written), 0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioStackedStagesTest, "Musee.Studio.StackedStages", Flags)
	bool FStudioStackedStagesTest::RunTest(const FString& Parameters)
	{
		// The Atrium and the Sphere, stacked on one axis by the museum's plan, meet only on their seams (Site.NoOverlap).
		const TValueOrError<FSitePlan, FString> Plan = Musee::Site::ReadPlan();
		if (!TestTrue(TEXT("the site plan reads"), Plan.HasValue()) || !USiteRoom::Find(FMuseeKey(TEXT("atrium"))) || !USiteRoom::Find(FMuseeKey(TEXT("sphere"))))
		{
			return false;
		}
		FMuseeDependencySet Dependencies;
		const FSiteRooms Rooms = DescribeRooms(Dependencies);
		TArray<FMuseeFinding> PlanFindings;
		const FSiteLayout Layout = Musee::Site::Solve(Plan.GetValue(), Rooms.Ports, PlanFindings);
		TMap<FMuseeKey, FSiteFootprint> Stacked;
		for (const TCHAR* Stage : {TEXT("atrium"), TEXT("sphere")})
		{
			Stacked.Add(FMuseeKey(Stage), Rooms.Footprints[FMuseeKey(Stage)]);
			TestTrue(*FString::Printf(TEXT("%s is placed"), Stage), Layout.TranslationsMeters.Contains(FMuseeKey(Stage)));
		}
		const TArray<FMuseeFinding> Found = Musee::Site::CheckOverlaps(Stacked, Layout);
		for (const FMuseeFinding& Finding : Found) { AddInfo(Finding.Summary); }
		TestEqual(TEXT("the Sphere's neck and bowl stand in the Atrium's canopy without overlap"), Found.Num(), 0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioStageNamesTest, "Musee.Studio.StageNames", Flags)
	bool FStudioStageNamesTest::RunTest(const FString& Parameters)
	{
		TestEqual(TEXT("a stage's folder is its key in PascalCase"), StageFolder(FMuseeKey(TEXT("nympheas"))), FString(TEXT("Nympheas")));
		TestEqual(TEXT("hyphens join words"), StageFolder(FMuseeKey(TEXT("classical-hall"))), FString(TEXT("ClassicalHall")));
		TestEqual(TEXT("a piece's asset token has no dots"), AssetToken(TEXT("blind.west.left")), FString(TEXT("blind_west_left")));
		FMakeReport Report;
		FString Why;
		TestFalse(TEXT("a name that is no stage is refused"), MakeByName(TEXT("no-such-stage"), Report, Why));
		return true;
	}
}

#endif
