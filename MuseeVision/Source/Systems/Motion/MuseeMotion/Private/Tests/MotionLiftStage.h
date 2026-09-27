#pragma once

#include "CoreMinimal.h"
#include "MotionChannel.h"
#include "MotionDresser.h"
#include "MotionExtent.h"
#include "MotionMechanism.h"
#include "MotionPerformance.h"
#include "MotionTestStage.h"
#include "MuseeSignals.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Motion::Tests
{
	/** A frame, as the tests run the machines: a sixtieth of a second. */
	inline constexpr float LiftFrame = 1.0f / 60.0f;

	inline FMuseeKey LiftKey(const TCHAR* Text)
	{
		return FMuseeKey(Text);
	}

	/** The Atrium's collar iris pins (Docs/Stages/atrium.md, the anchors), on each blade's top face. */
	inline const FVector BladePins[7] = {
		FVector(3.745, 0.000, 7.695), FVector(2.335, 2.928, 7.691), FVector(-0.833, 3.651, 7.687), FVector(-3.374, 1.625, 7.683),
		FVector(-3.374, -1.625, 7.679), FVector(-0.833, -3.651, 7.675), FVector(2.335, -2.928, 7.671),
	};

	/**
	 * The Atrium's lift as its sheet draws it (section 2, the anchors; section 3.6, the car), dressed from the Atrium's own
	 * schedule (Data/Stages/atrium): the car drawn at the Atrium stop carrying its two leaves and the Cube mast's head
	 * (with one turn of the mast's column, as Motion draws it); the landing's two leaves; the collar iris's seven blades
	 * at their pins; the call on the waiting mark and the panel in the car. Each body carries a block where its part is.
	 * The context carries the ports the site plan mates, none unless given.
	 */
	inline void DressAtrium(FTestStage& Stage, TArray<FMuseeFinding>& OutFindings, TArray<FMuseeKey> MatedPorts = {})
	{
		TArray<FMuseeStageAnchor> Anchors = {
			Anchor(TEXT("atrium.car"), TEXT("Body"), FVector::ZeroVector),
			Anchor(TEXT("atrium.car.mast.head"), TEXT("Body"), FVector(0.0, 0.0, 2.78)),
			Anchor(TEXT("atrium.car.leaf.north"), TEXT("Body"), FVector::ZeroVector),
			Anchor(TEXT("atrium.car.leaf.south"), TEXT("Body"), FVector::ZeroVector),
			Anchor(TEXT("atrium.landing.leaf.north"), TEXT("Body"), FVector::ZeroVector),
			Anchor(TEXT("atrium.landing.leaf.south"), TEXT("Body"), FVector::ZeroVector),
			Anchor(TEXT("atrium.call"), TEXT("Use"), FVector(-3.31, 0.0, 0.0)),
			Anchor(TEXT("atrium.car.panel"), TEXT("Use"), FVector::ZeroVector),
		};
		USceneComponent* Car = Stage.AddBody(LiftKey(TEXT("atrium.car")), FVector::ZeroVector, FVector(0.0, 0.0, 1.3), FVector(2.2, 2.2, 1.3));
		USceneComponent* Head = Stage.AddBody(LiftKey(TEXT("atrium.car.mast.head")), FVector(0.0, 0.0, 2.78), FVector(0.0, 0.0, 0.08), FVector(0.26, 0.26, 0.08), Car);
		Stage.AddPiece(TEXT("atrium.cube-mast.column.turn"), Head);
		Stage.AddBody(LiftKey(TEXT("atrium.car.leaf.north")), FVector::ZeroVector, FVector(-2.09, -0.46, 1.28), FVector(0.05, 0.45, 1.25), Car);
		Stage.AddBody(LiftKey(TEXT("atrium.car.leaf.south")), FVector::ZeroVector, FVector(-2.09, 0.46, 1.28), FVector(0.05, 0.45, 1.25), Car);
		Stage.AddBody(LiftKey(TEXT("atrium.landing.leaf.north")), FVector::ZeroVector, FVector(-2.33, -0.51, 1.47), FVector(0.05, 0.5, 1.46));
		Stage.AddBody(LiftKey(TEXT("atrium.landing.leaf.south")), FVector::ZeroVector, FVector(-2.33, 0.51, 1.47), FVector(0.05, 0.5, 1.46));
		for (int32 Blade = 0; Blade < 7; ++Blade)
		{
			const FString BladeName = FString::Printf(TEXT("atrium.up.iris.blade.%d"), Blade + 1);
			Anchors.Add(Anchor(*BladeName, TEXT("Body"), BladePins[Blade]));
			const FVector Inward = -FVector(BladePins[Blade].X, BladePins[Blade].Y, 0.0).GetSafeNormal() * 0.65;
			Stage.AddBody(LiftKey(*BladeName), BladePins[Blade], Inward - FVector(0.0, 0.0, 0.001), FVector(0.6, 0.3, 0.001));
		}
		FMuseeStageContext Context = Stage.ContextAt(LiftKey(TEXT("atrium")), TEXT("Stages/atrium"), MoveTemp(Anchors));
		Context.MatedPorts = MoveTemp(MatedPorts);
		DressMechanisms(Context, OutFindings);
	}

	/** Every machine on the stage, in the order the dresser placed them. */
	inline TArray<UMotionMechanism*> Machines(const FTestStage& Stage)
	{
		TArray<UMotionMechanism*> Mechanisms;
		Stage.Actor->GetComponents(Mechanisms);
		return Mechanisms;
	}

	/** The stage's machine with the key. */
	inline UMotionMechanism* Machine(const FTestStage& Stage, const TCHAR* MechanismKey)
	{
		for (UMotionMechanism* Mechanism : Machines(Stage))
		{
			if (Mechanism->Spec().Key == LiftKey(MechanismKey))
			{
				return Mechanism;
			}
		}
		return nullptr;
	}

	/** Where a body stands above the stage's origin (or its parent body's), in metres. */
	inline double LevelMeters(const UMotionMechanism& Mechanism)
	{
		return FromEngine<Meters>(Mechanism.Body()->GetRelativeLocation().Z).Get();
	}

	/** Sets every machine and every point the stage keeps where the museum opens them. */
	inline void StandAll(const FTestStage& Stage)
	{
		for (UMotionMechanism* Mechanism : Machines(Stage))
		{
			Mechanism->StandAtStart();
		}
		TArray<UMotionChannel*> Channels;
		Stage.Actor->GetComponents(Channels);
		for (UMotionChannel* Channel : Channels)
		{
			Channel->StandAtStart();
		}
		TArray<UMotionPerformance*> Performances;
		Stage.Actor->GetComponents(Performances);
		for (UMotionPerformance* Performance : Performances)
		{
			Performance->StandAtStart();
		}
	}

	/** What a run saw: the interlocks that held any machine, by key, and for how many frames. */
	struct FRunLog
	{
		TMap<FMuseeKey, int32> Held;
	};

	/**
	 * Runs everything Motion placed on the stage frame by frame, as the world ticks it: the machines in the order they
	 * were placed, then the timed pieces, the derived channels and the pieces of variable length.
	 */
	inline void RunAll(const FTestStage& Stage, double Seconds, double& Clock, FRunLog* Log = nullptr)
	{
		const TArray<UMotionMechanism*> Mechanisms = Machines(Stage);
		TArray<UMotionPerformance*> Performances;
		TArray<UMotionChannel*> Channels;
		TArray<UMotionExtent*> Extents;
		Stage.Actor->GetComponents(Performances);
		Stage.Actor->GetComponents(Channels);
		Stage.Actor->GetComponents(Extents);
		for (double Done = 0.0; Done < Seconds; Done += LiftFrame)
		{
			for (UMotionMechanism* Mechanism : Mechanisms)
			{
				Mechanism->Advance(LiftFrame);
				if (Log && Mechanism->WaitsFor() == UMotionMechanism::EWait::Interlock)
				{
					++Log->Held.FindOrAdd(Mechanism->HeldBy());
				}
			}
			for (UMotionPerformance* Performance : Performances)
			{
				Performance->Advance(LiftFrame);
			}
			for (UMotionChannel* Channel : Channels)
			{
				Channel->Advance();
			}
			for (UMotionExtent* Extent : Extents)
			{
				Extent->Advance();
			}
			Clock += LiftFrame;
		}
	}

	/** A body's turn about the stage's z axis from where it is drawn, in degrees (positive from +x towards +y). */
	inline double TurnDegrees(const UMotionMechanism& Mechanism)
	{
		return Mechanism.Body()->GetRelativeRotation().Yaw;
	}

	/** A channel's value, or -1 where nobody has published on it. */
	inline float ChannelValue(const FTestStage& Stage, const TCHAR* Point)
	{
		return Stage.World->GetSubsystem<UMuseeSignals>()->Read(LiftKey(Point)).Get(-1.0f);
	}

	inline void PublishValue(const FTestStage& Stage, const TCHAR* Point, float Value)
	{
		FMuseeChannel Published;
		Published.Key = LiftKey(Point);
		Published.Value = Value;
		Stage.World->GetSubsystem<UMuseeSignals>()->Publish(Published);
	}
}

#endif
