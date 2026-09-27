#include "MotionChannel.h"

#include "Engine/World.h"
#include "MotionConditions.h"

UMotionChannel::UMotionChannel()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMotionChannel::Configure(const FMotionChannelSpec& InSpec)
{
	ChannelSpec = InSpec;
}

void UMotionChannel::StandAtStart()
{
	if (ChannelSpec.Follows.IsSet())
	{
		Advance();
		return;
	}
	Musee::Motion::PublishChannel(GetWorld(), ChannelSpec.Key, ChannelSpec.Rest);
}

void UMotionChannel::Advance()
{
	if (!ChannelSpec.Follows.IsSet())
	{
		return;
	}
	const float Followed = Musee::Motion::ChannelValue(GetWorld(), ChannelSpec.Follows.Channel);
	Musee::Motion::PublishChannel(GetWorld(), ChannelSpec.Key, static_cast<float>(ChannelSpec.Follows.At(Followed)));
}

void UMotionChannel::BeginPlay()
{
	Super::BeginPlay();
	StandAtStart();
}

void UMotionChannel::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance();
}
