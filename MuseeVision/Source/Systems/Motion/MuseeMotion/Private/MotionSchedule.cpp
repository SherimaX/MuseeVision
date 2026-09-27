#include "MotionSchedule.h"

#include "MotionTexts.h"
#include "MuseeRecords.h"

TValueOrError<FMotionSchedule, FString> FMotionSchedule::Read(const FString& Path)
{
	TValueOrError<FMotionSchedule, FString> Read = Musee::ReadRecord<FMotionSchedule>(Path);
	if (Read.HasError())
	{
		return Read;
	}
	FMotionSchedule Schedule = Read.StealValue();
	for (FMotionMechanismSpec& Mechanism : Schedule.Mechanisms)
	{
		Mechanism.KeyTexts();
	}
	for (FMotionPerformanceSpec& Performance : Schedule.Performances)
	{
		Musee::Motion::KeyText(Performance.Verb, Performance.Key, TEXTVIEW("verb"));
		Musee::Motion::KeyText(Performance.Caption, Performance.Key, TEXTVIEW("caption"));
	}
	return MakeValue(MoveTemp(Schedule));
}

const FMotionMechanismSpec* FMotionSchedule::FindMechanism(const FMuseeKey& Key) const
{
	return Mechanisms.FindByPredicate([&Key](const FMotionMechanismSpec& Spec) { return Spec.Key == Key; });
}
