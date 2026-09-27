#include "MotionConditions.h"

#include "Engine/World.h"
#include "MuseeSignals.h"

namespace Musee::Motion
{
	namespace
	{
		/** A channel point nobody has published on stands at its rest. */
		constexpr float UnpublishedValue = 0.0f;
	}

	float ChannelValue(const UWorld* World, const FMuseeKey& Channel)
	{
		const UMuseeSignals* Signals = World ? World->GetSubsystem<UMuseeSignals>() : nullptr;
		return Signals ? Signals->Read(Channel).Get(UnpublishedValue) : UnpublishedValue;
	}

	void PublishChannel(UWorld* World, const FMuseeKey& Channel, float Value)
	{
		if (UMuseeSignals* Signals = World ? World->GetSubsystem<UMuseeSignals>() : nullptr)
		{
			FMuseeChannel Point;
			Point.Key = Channel;
			Point.Value = Value;
			Signals->Publish(Point);
		}
	}

	bool IsMated(const TArray<FMuseeKey>& MatedPorts, const FMotionRequirement& Requirement)
	{
		for (const FMuseeKey& Port : Requirement.Mated)
		{
			if (!MatedPorts.Contains(Port))
			{
				return false;
			}
		}
		return true;
	}

	bool Holds(const UWorld* World, const TArray<FMuseeKey>& MatedPorts, const FMotionRequirement& Requirement)
	{
		if (!IsMated(MatedPorts, Requirement))
		{
			return false;
		}
		for (const FMotionChannelCondition& Condition : Requirement.Channels)
		{
			if (!Condition.Holds(ChannelValue(World, Condition.Channel)))
			{
				return false;
			}
		}
		return true;
	}
}
