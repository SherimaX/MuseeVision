#include "MotionDresser.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "MotionControl.h"
#include "MotionMechanism.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"
#include "MuseeUnits.h"

namespace Musee::Motion
{
	namespace
	{
		const FName BodyKind(TEXT("Body"));
		const FName UseKind(TEXT("Use"));
		const FName MotionSystem(TEXT("Motion"));

		FMuseeFinding Finding(const FMuseeStageContext& Stage, const FString& Summary)
		{
			FMuseeFinding Found = FMuseeFinding::Make(Stage.Stage.ToName(), Summary, FMuseeMeasurement());
			Found.Owner = MotionSystem;
			return Found;
		}

		/** An engine object name for a key: its dots, which object paths reserve, become underscores. */
		FName ObjectName(const TCHAR* Prefix, const FMuseeKey& Key)
		{
			return FName(FString::Printf(TEXT("%s_%s"), Prefix, *Key.ToString().Replace(TEXT("."), TEXT("_"))));
		}

		FString SchedulePath(const FMuseeStageContext& Stage)
		{
			const FString Folder = FPaths::IsRelative(Stage.ScheduleDir) ? Records::DataRoot() / Stage.ScheduleDir : Stage.ScheduleDir;
			return Folder / FMotionSchedule::FileName;
		}

		USceneComponent* FindBody(const AActor& Actor, const FMuseeKey& Key)
		{
			TArray<USceneComponent*> Components;
			Actor.GetComponents(Components);
			USceneComponent* const* Found = Components.FindByPredicate([&Key](const USceneComponent* C) { return C->ComponentHasTag(Key.ToName()); });
			return Found ? *Found : nullptr;
		}

		/** Whether a point (world) lies on the body: within its solid parts' bounds, grown by a control's reach. */
		bool IsOnBody(const USceneComponent& Body, const FVector& Point)
		{
			TArray<USceneComponent*> Parts;
			Body.GetChildrenComponents(true, Parts);
			const double Reach = ToEngine(Meters(UMotionControl::RadiusMeters));
			for (const USceneComponent* Part : Parts)
			{
				const UPrimitiveComponent* Solid = Cast<UPrimitiveComponent>(Part);
				if (Solid && !Solid->IsA<UMotionControl>() && Solid->CalcBounds(Solid->GetComponentTransform()).GetBox().ExpandBy(Reach).IsInside(Point))
				{
					return true;
				}
			}
			return false;
		}

		/** Whether a key names a place on another stage: its outermost scope is not this stage's (`sphere.call` on the Atrium). */
		bool IsAcrossSeam(const FMuseeStageContext& Stage, const FMuseeKey& Key)
		{
			const TArray<FString> Scopes = Key.Segments();
			const TArray<FString> StageScopes = Stage.Stage.Segments();
			return !Scopes.IsEmpty() && !StageScopes.IsEmpty() && Scopes[0] != StageScopes[0];
		}

		/**
		 * Places the control at a Use anchor: on the body when the anchor lies on it, otherwise on the stage. A ride
		 * offered at a Use anchor of another stage (a landing call across a seam) is Site's to bind, so it is no finding.
		 */
		void PlaceControl(const FMuseeStageContext& Stage, const FMuseeKey& UseKey, UMotionMechanism& Mechanism, TArray<FMuseeFinding>& OutFindings)
		{
			const FMuseeStageAnchor* Anchor = Stage.FindAnchor(UseKey, UseKind);
			if (!Anchor)
			{
				if (!IsAcrossSeam(Stage, UseKey))
				{
					OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Mechanism %s: its control %s is not a Use anchor of the stage."), *Mechanism.Spec().Key.ToString(), *UseKey.ToString())));
				}
				return;
			}
			AActor& Actor = *Stage.StageActor;
			const FTransform Where = ToEngine(Anchor->Frame) * Actor.GetActorTransform();
			USceneComponent* Parent = IsOnBody(*Mechanism.Body(), Where.GetLocation()) ? Mechanism.Body() : Actor.GetRootComponent();
			UMotionControl* Control = NewObject<UMotionControl>(&Actor, ObjectName(TEXT("Control"), UseKey));
			Control->SetupAttachment(Parent);
			Control->SetRelativeTransform(Where.GetRelativeTransform(Parent->GetComponentTransform()));
			Control->ComponentTags.Add(UseKey.ToName());
			Control->Bind(&Mechanism, UseKey);
			Actor.AddInstanceComponent(Control);
			Control->RegisterComponent();
		}

		/** One control at each Use anchor the mechanism is worked from: its own control's and its rides'. */
		void PlaceControls(const FMuseeStageContext& Stage, const FMotionMechanismSpec& Spec, UMotionMechanism& Mechanism, TArray<FMuseeFinding>& OutFindings)
		{
			TArray<FMuseeKey> Uses;
			if (!Spec.Control.IsNone())
			{
				Uses.Add(Spec.Control);
			}
			for (const FMotionRide& Ride : Spec.Rides)
			{
				Uses.AddUnique(Ride.Use);
			}
			for (const FMuseeKey& UseKey : Uses)
			{
				PlaceControl(Stage, UseKey, Mechanism, OutFindings);
			}
		}

		void PlaceMechanism(const FMuseeStageContext& Stage, const FMotionMechanismSpec& Spec, TArray<FMuseeFinding>& OutFindings)
		{
			const FMuseeStageAnchor* Anchor = Stage.FindAnchor(Spec.Body, BodyKind);
			USceneComponent* Body = FindBody(*Stage.StageActor, Spec.Body);
			if (!Anchor || !Body)
			{
				OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Mechanism %s: its body %s is not a Body anchor with a tagged scene component."), *Spec.Key.ToString(), *Spec.Body.ToString())));
				return;
			}
			AActor& Actor = *Stage.StageActor;
			UMotionMechanism* Mechanism = NewObject<UMotionMechanism>(&Actor, ObjectName(TEXT("Mechanism"), Spec.Key));
			Mechanism->ComponentTags.Add(Spec.Key.ToName());
			Actor.AddInstanceComponent(Mechanism);
			Mechanism->RegisterComponent();
			Mechanism->Configure(Spec, Body);
			PlaceControls(Stage, Spec, *Mechanism, OutFindings);
		}
	}

	void DressMechanisms(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings)
	{
		const FString Path = SchedulePath(Stage);
		if (!Stage.StageActor || !IFileManager::Get().FileExists(*Path))
		{
			return;
		}
		const TValueOrError<FMotionSchedule, FString> Schedule = ReadRecord<FMotionSchedule>(Path);
		if (Schedule.HasError())
		{
			OutFindings.Add(Finding(Stage, Schedule.GetError()));
			return;
		}
		if (!(Schedule.GetValue().Stage == Stage.Stage))
		{
			OutFindings.Add(Finding(Stage, FString::Printf(TEXT("%s names the stage %s."), *Path, *Schedule.GetValue().Stage.ToString())));
			return;
		}
		for (const FMotionMechanismSpec& Spec : Schedule.GetValue().Mechanisms)
		{
			const TArray<FString> SpecFaults = Faults(Spec);
			if (!SpecFaults.IsEmpty())
			{
				OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Mechanism %s: %s."), *Spec.Key.ToString(), *FString::Join(SpecFaults, TEXT("; ")))));
				continue;
			}
			PlaceMechanism(Stage, Spec, OutFindings);
		}
	}
}
