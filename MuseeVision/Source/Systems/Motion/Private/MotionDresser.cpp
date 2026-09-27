#include "MotionDresser.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "MotionControl.h"
#include "MotionMechanism.h"
#include "MotionProfile.h"
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

		TArray<FString> StepFaults(const TArray<FMotionStep>& Steps, const TCHAR* Sequence)
		{
			TArray<FString> Out;
			for (int32 Index = 0; Index < Steps.Num(); ++Index)
			{
				const FMotionStep& Step = Steps[Index];
				const FString Where = FString::Printf(TEXT("%s step %d"), Sequence, Index);
				if (Step.IsMove() == !Step.Channel.IsNone())
				{
					Out.Add(Where + TEXT(" must either move or ramp a channel"));
				}
				if (Step.IsMove() && Step.Move != UMotionMechanism::Open && Step.Move != UMotionMechanism::Closed)
				{
					Out.Add(FString::Printf(TEXT("%s moves to '%s', which is neither open nor closed"), *Where, *Step.Move.ToString()));
				}
				if (Step.Time < 0.0f || Step.To < 0.0f || Step.To > 1.0f)
				{
					Out.Add(Where + TEXT(" needs a time of zero or more and a value from 0 to 1"));
				}
			}
			return Out;
		}

		void PlaceControl(const FMuseeStageContext& Stage, const FMotionMechanismSpec& Spec, UMotionMechanism& Mechanism, TArray<FMuseeFinding>& OutFindings)
		{
			const FMuseeStageAnchor* Anchor = Stage.FindAnchor(Spec.Control, UseKind);
			if (!Anchor)
			{
				OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Mechanism %s: its control %s is not a Use anchor of the stage."), *Spec.Key.ToString(), *Spec.Control.ToString())));
				return;
			}
			AActor& Actor = *Stage.StageActor;
			const FTransform Where = ToEngine(Anchor->Frame) * Actor.GetActorTransform();
			USceneComponent* Parent = IsOnBody(*Mechanism.Body(), Where.GetLocation()) ? Mechanism.Body() : Actor.GetRootComponent();
			UMotionControl* Control = NewObject<UMotionControl>(&Actor, ObjectName(TEXT("Control"), Spec.Control));
			Control->SetupAttachment(Parent);
			Control->SetRelativeTransform(Where.GetRelativeTransform(Parent->GetComponentTransform()));
			Control->ComponentTags.Add(Spec.Control.ToName());
			Control->Bind(&Mechanism);
			Actor.AddInstanceComponent(Control);
			Control->RegisterComponent();
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
			if (!Spec.Control.IsNone())
			{
				PlaceControl(Stage, Spec, *Mechanism, OutFindings);
			}
		}
	}

	TArray<FString> Faults(const FMotionMechanismSpec& Spec)
	{
		TArray<FString> Out;
		if (Spec.Joint != TEXT("prismatic") && Spec.Joint != TEXT("revolute"))
		{
			Out.Add(FString::Printf(TEXT("its joint '%s' is neither prismatic nor revolute"), *Spec.Joint.ToString()));
		}
		if (Spec.Axis.Num() != 3 || FVector(Spec.Axis[0], Spec.Axis[1], Spec.Axis[2]).IsNearlyZero())
		{
			Out.Add(TEXT("its axis is not a direction of three numbers"));
		}
		if (Spec.Travel <= 0.0f || Spec.Time <= 0.0f)
		{
			Out.Add(TEXT("its travel and its time must both be more than zero"));
		}
		if (!ParseProfile(Spec.Profile).IsSet())
		{
			Out.Add(FString::Printf(TEXT("its profile '%s' is neither linear nor ease-in-out"), *Spec.Profile.ToString()));
		}
		if (!Spec.Guard.IsNone() && Spec.Guard != TEXT("none") && Spec.Guard != TEXT("body-in-travel") && Spec.Guard != TEXT("body-in-travel-reverse"))
		{
			Out.Add(FString::Printf(TEXT("its guard '%s' is not none, body-in-travel or body-in-travel-reverse"), *Spec.Guard.ToString()));
		}
		if (Spec.Clearance < 0.0f)
		{
			Out.Add(TEXT("its clearance is negative"));
		}
		if (Spec.Follows.IsSet() && (!Spec.Control.IsNone() || !Spec.Open.IsEmpty() || !Spec.Close.IsEmpty()))
		{
			Out.Add(TEXT("it follows a channel, so it has no control and no sequences"));
		}
		if (Spec.Follows.IsSet() && (Spec.Follows.In.Num() != 2 || Spec.Follows.Out.Num() != 2 || Spec.Follows.In[0] == Spec.Follows.In[1]))
		{
			Out.Add(TEXT("it follows a channel with an input range of two different values and an output range of two"));
		}
		Out.Append(StepFaults(Spec.Open, TEXT("open")));
		Out.Append(StepFaults(Spec.Close, TEXT("close")));
		return Out;
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
