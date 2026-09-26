#include "MuseeConfiguration.h"

const FMuseePose* FMuseeConfiguration::Find(const FMuseeKey& Body) const
{
	return Poses.FindByPredicate([&Body](const FMuseePose& Pose) { return Pose.Key == Body; });
}

void FMuseeConfiguration::Set(const FMuseePose& Pose)
{
	if (FMuseePose* Existing = Poses.FindByPredicate([&Pose](const FMuseePose& Other) { return Other.Key == Pose.Key; }))
	{
		*Existing = Pose;
		return;
	}
	Poses.Add(Pose);
}
