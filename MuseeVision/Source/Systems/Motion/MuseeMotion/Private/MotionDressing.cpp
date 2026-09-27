#include "MotionDressing.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "MotionControl.h"
#include "MotionChannel.h"
#include "MotionPerformance.h"
#include "MotionSchedule.h"
#include "MuseeUnits.h"

namespace Musee::Motion::Dressing
{
	namespace
	{
		const FName MotionSystem(TEXT("Motion"));
		const FName UseKind(TEXT("Use"));
		const TCHAR* const PieceTagPrefix = TEXT("musee.piece:");

		/** The value a following mechanism's channel has where the mechanism is drawn. */
		TOptional<double> FollowedAt(const FMotionMechanismSpec& Spec)
		{
			const FMotionFollowing& Map = Spec.Follows;
			if (!Map.IsSound() || Map.Out[0] == Map.Out[1])
			{
				return {};
			}
			const double Travel = Spec.Travel * Spec.DrawnPosition();
			const double Share = FMath::Clamp((Travel - Map.Out[0]) / (Map.Out[1] - Map.Out[0]), 0.0, 1.0);
			return FMath::Lerp(Map.In[0], Map.In[1], Share);
		}

		void PlacePerformance(const FMuseeStageContext& Stage, const FMotionPerformanceSpec& Spec, TArray<FMuseeFinding>& OutFindings)
		{
			AActor& Actor = *Stage.StageActor;
			UMotionPerformance* Performance = NewObject<UMotionPerformance>(&Actor, ObjectName(TEXT("Performance"), Spec.Key));
			Performance->ComponentTags.Add(Spec.Key.ToName());
			Actor.AddInstanceComponent(Performance);
			Performance->RegisterComponent();
			Performance->Configure(Spec);
			Performance->SetMatedPorts(Stage.MatedPorts);
			const FMuseeStageAnchor* Anchor = Stage.FindAnchor(Spec.Use, UseKind);
			if (!Anchor)
			{
				OutFindings.Add(Finding(Stage, FString::Printf(TEXT("Performance %s: its control %s is not a Use anchor of the stage."), *Spec.Key.ToString(), *Spec.Use.ToString())));
				return;
			}
			UMotionControl* Control = NewObject<UMotionControl>(&Actor, ObjectName(TEXT("Control"), Spec.Use));
			Control->SetRelativeTransform(ToEngine(Anchor->Frame));
			Control->ComponentTags.Add(Spec.Use.ToName());
			Control->Bind(Performance, Spec.Use);
			Adopt(Actor, *Control, *Actor.GetRootComponent());
		}
	}

	FMuseeFinding Finding(const FMuseeStageContext& Stage, const FString& Summary)
	{
		FMuseeFinding Found = FMuseeFinding::Make(Stage.Stage.ToName(), Summary, FMuseeMeasurement());
		Found.Owner = MotionSystem;
		return Found;
	}

	FName ObjectName(const TCHAR* Prefix, const FMuseeKey& Key)
	{
		return FName(FString::Printf(TEXT("%s_%s"), Prefix, *Key.ToString().Replace(TEXT("."), TEXT("_"))));
	}

	USceneComponent* FindTagged(const AActor& Actor, FName Tag)
	{
		TArray<USceneComponent*> Components;
		Actor.GetComponents(Components);
		USceneComponent* const* Found = Components.FindByPredicate([Tag](const USceneComponent* C) { return C->ComponentHasTag(Tag); });
		return Found ? *Found : nullptr;
	}

	FName PieceTag(FName Piece)
	{
		return FName(FString(PieceTagPrefix) + Piece.ToString());
	}

	void Adopt(AActor& Actor, USceneComponent& Component, USceneComponent& Parent)
	{
		Component.SetupAttachment(&Parent);
		Actor.AddInstanceComponent(&Component);
		Component.RegisterComponent();
	}

	double DrawnValue(const FMotionSchedule& Schedule, const FMuseeKey& Channel)
	{
		for (const FMotionMechanismSpec& Spec : Schedule.Mechanisms)
		{
			if (Spec.Publishes == Channel)
			{
				return Spec.DrawnPosition();
			}
		}
		for (const FMotionMechanismSpec& Spec : Schedule.Mechanisms)
		{
			if (Spec.Follows.Channel == Channel)
			{
				if (const TOptional<double> Value = FollowedAt(Spec))
				{
					return Value.GetValue();
				}
			}
		}
		return 0.0;
	}

	void DressPerformancesAndChannels(const FMuseeStageContext& Stage, const FMotionSchedule& Schedule, TArray<FMuseeFinding>& OutFindings)
	{
		for (const FMotionPerformanceSpec& Spec : Schedule.Performances)
		{
			PlacePerformance(Stage, Spec, OutFindings);
		}
		AActor& Actor = *Stage.StageActor;
		for (const FMotionChannelSpec& Spec : Schedule.Channels)
		{
			UMotionChannel* Channel = NewObject<UMotionChannel>(&Actor, ObjectName(TEXT("Channel"), Spec.Key));
			Channel->ComponentTags.Add(Spec.Key.ToName());
			Actor.AddInstanceComponent(Channel);
			Channel->RegisterComponent();
			Channel->Configure(Spec);
		}
	}
}
