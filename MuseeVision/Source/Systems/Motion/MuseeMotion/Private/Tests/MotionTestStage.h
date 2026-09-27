#pragma once

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "MuseeCollision.h"
#include "MuseeStageDresser.h"
#include "MuseeUnits.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Motion::Tests
{
	/** Motion's tests run in every application context and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** A fixture folder under Private/Tests/Data, as a full path. */
	inline FString Fixture(const FString& Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir() / TEXT("Systems/Motion/MuseeMotion/Private/Tests/Data") / Name);
	}

	/**
	 * A small stage in a game world of its own, destroyed with the scope: a stage actor whose root stands at the
	 * stage's origin, and moving bodies on it as Studio makes them (a scene component tagged with the body's key,
	 * carrying a solid block).
	 */
	class FTestStage
	{
	public:
		FTestStage()
			: World(UWorld::CreateWorld(EWorldType::Game, false))
		{
			Actor = World->SpawnActor<AActor>();
			USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
			Actor->SetRootComponent(Root);
			Actor->AddInstanceComponent(Root);
			Root->RegisterComponent();
		}

		~FTestStage()
		{
			World->DestroyWorld(false);
		}

		/**
		 * A body at a point (metres) of the stage, or of the body it rides on, carrying a solid block (its centre and half
		 * size in the body's frame).
		 */
		USceneComponent* AddBody(const FMuseeKey& Key, const FVector& AtMeters, const FVector& BlockCentreMeters = FVector(0.0, 0.0, 0.5),
			const FVector& BlockHalfMeters = FVector(0.5), USceneComponent* RidesOn = nullptr)
		{
			USceneComponent* Body = NewObject<USceneComponent>(Actor, *FString::Printf(TEXT("Body_%d"), ++Count));
			Body->SetupAttachment(RidesOn ? RidesOn : Actor->GetRootComponent());
			Body->SetRelativeLocation(ToEngine(AtMeters));
			Body->ComponentTags.Add(Key.ToName());
			Actor->AddInstanceComponent(Body);
			Body->RegisterComponent();
			UBoxComponent* Block = NewObject<UBoxComponent>(Actor, *FString::Printf(TEXT("Block_%d"), Count));
			Block->SetupAttachment(Body);
			Block->SetBoxExtent(ToEngine(BlockHalfMeters));
			Block->SetRelativeLocation(ToEngine(BlockCentreMeters));
			Block->SetCollisionProfileName(Collision::Solid.Profile());
			Actor->AddInstanceComponent(Block);
			Block->RegisterComponent();
			return Body;
		}

		/** The visitor's body: a capsule of the human measure's size, standing with its feet at a point (metres). */
		UCapsuleComponent* AddVisitor(const FVector& FeetMeters)
		{
			AActor* Visitor = World->SpawnActor<AActor>();
			UCapsuleComponent* Capsule = NewObject<UCapsuleComponent>(Visitor, TEXT("Capsule"));
			Capsule->InitCapsuleSize(ToEngine(Meters(0.3)), ToEngine(Meters(0.9)));
			Capsule->SetCollisionProfileName(Collision::Body.Profile());
			Visitor->SetRootComponent(Capsule);
			Capsule->RegisterComponent();
			Capsule->SetWorldLocation(ToEngine(FeetMeters + FVector(0.0, 0.0, 0.9)));
			return Capsule;
		}

		/** The stage's context for its dressers, with its schedules in a fixture folder. */
		FMuseeStageContext Context(const FMuseeKey& Stage, const FString& FixtureFolder, TArray<FMuseeStageAnchor> Anchors) const
		{
			FMuseeStageContext Context;
			Context.Stage = Stage;
			Context.ScheduleDir = Fixture(FixtureFolder);
			Context.Anchors = MoveTemp(Anchors);
			Context.StageActor = Actor;
			return Context;
		}

		UWorld* World;
		AActor* Actor = nullptr;

	private:
		int32 Count = 0;
	};

	inline FMuseeStageAnchor Anchor(const TCHAR* Key, const TCHAR* Kind, const FVector& AtMeters)
	{
		FMuseeStageAnchor Out;
		Out.Key = FMuseeKey(Key);
		Out.Kind = Kind;
		Out.Frame = FMuseeFrame(AtMeters);
		return Out;
	}
}

#endif
