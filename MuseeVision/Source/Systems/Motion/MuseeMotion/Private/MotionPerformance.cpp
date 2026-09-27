#include "MotionPerformance.h"

#include "Engine/World.h"
#include "MotionConditions.h"

UMotionPerformance::UMotionPerformance()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMotionPerformance::Configure(const FMotionPerformanceSpec& InSpec)
{
	PerformanceSpec = InSpec;
}

void UMotionPerformance::SetMatedPorts(const TArray<FMuseeKey>& Ports)
{
	MatedPorts = Ports;
}

double UMotionPerformance::EndsAt() const
{
	// Ended early, the piece still plays out its fade-out; otherwise it ends at its full length, fade-out included.
	return EndedAt.IsSet() ? FMath::Min(double(PerformanceSpec.Seconds), EndedAt.GetValue() + PerformanceSpec.FadeOut) : PerformanceSpec.Seconds;
}

FMuseeUse UMotionPerformance::Usage() const
{
	const FText End = NSLOCTEXT("Musee", "motion.end", "End the piece");
	if (Clock.IsSet())
	{
		return EndedAt.IsSet() ? FMuseeUse::Unavailable(End, FText::GetEmpty(), NSLOCTEXT("Musee", "motion.ending", "It is ending."))
			: FMuseeUse::Available(End, FText::GetEmpty());
	}
	if (!Musee::Motion::IsMated(MatedPorts, PerformanceSpec.Requires))
	{
		return FMuseeUse();
	}
	const FText Caption = PerformanceSpec.Caption.Key.IsNone() ? FText::GetEmpty() : PerformanceSpec.Caption.ToText();
	if (!Musee::Motion::Holds(GetWorld(), MatedPorts, PerformanceSpec.Requires))
	{
		return FMuseeUse::Unavailable(PerformanceSpec.Verb.ToText(), Caption, NSLOCTEXT("Musee", "motion.not-ready", "It is not ready."));
	}
	return FMuseeUse::Available(PerformanceSpec.Verb.ToText(), Caption);
}

void UMotionPerformance::Use()
{
	if (!Usage().bAvailable)
	{
		return;
	}
	if (Clock.IsSet())
	{
		EndedAt = Clock.GetValue();
		return;
	}
	Clock = 0.0;
	EndedAt.Reset();
	Publish();
}

void UMotionPerformance::Advance(float DeltaSeconds)
{
	if (!Clock.IsSet())
	{
		return;
	}
	Clock = Clock.GetValue() + DeltaSeconds;
	if (Clock.GetValue() >= EndsAt())
	{
		Clock.Reset();
		EndedAt.Reset();
	}
	Publish();
}

void UMotionPerformance::Publish() const
{
	Musee::Motion::PublishChannel(GetWorld(), PerformanceSpec.Publishes, IsRunning() ? 1.0f : 0.0f);
}

void UMotionPerformance::StandAtStart()
{
	Clock.Reset();
	EndedAt.Reset();
	Publish();
}

void UMotionPerformance::BeginPlay()
{
	Super::BeginPlay();
	StandAtStart();
}

void UMotionPerformance::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}
