#include "MotionDerivedChannel.h"

#include "Engine/World.h"
#include "MotionConditions.h"

UMotionDerivedChannel::UMotionDerivedChannel()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UMotionDerivedChannel::Configure(const FMotionChannelSpec& InSpec)
{
	ChannelSpec = InSpec;
}

void UMotionDerivedChannel::Advance()
{
	const float Followed = Musee::Motion::ChannelValue(GetWorld(), ChannelSpec.Follows.Channel);
	Musee::Motion::PublishChannel(GetWorld(), ChannelSpec.Key, static_cast<float>(ChannelSpec.Follows.At(Followed)));
}

void UMotionDerivedChannel::BeginPlay()
{
	Super::BeginPlay();
	Advance();
}

void UMotionDerivedChannel::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance();
}
