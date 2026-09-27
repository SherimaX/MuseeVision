#include "MuseeSignals.h"

#include "MuseeFoundationLog.h"

void UMuseeSignals::Publish(const FMuseeContact& Contact)
{
	ContactEvent.Broadcast(Contact);
}

void UMuseeSignals::Publish(const FMuseeActuation& Actuation)
{
	ActuationEvent.Broadcast(Actuation);
}

void UMuseeSignals::Publish(const FMuseeChannel& Channel)
{
	if (Channel.Key.IsNone())
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("A channel value was published without a key."));
		return;
	}
	float* Current = ChannelValues.Find(Channel.Key);
	if (Current && *Current == Channel.Value)
	{
		return;
	}
	ChannelValues.Add(Channel.Key, Channel.Value);
	if (const TSharedRef<FChannelEvent>* Found = ChannelEvents.Find(Channel.Key))
	{
		const TSharedRef<FChannelEvent> Event = *Found;
		Event->Broadcast(Channel);
	}
}

TOptional<float> UMuseeSignals::Read(const FMuseeKey& Channel) const
{
	const float* Value = ChannelValues.Find(Channel);
	return Value ? TOptional<float>(*Value) : TOptional<float>();
}

FDelegateHandle UMuseeSignals::Subscribe(const FMuseeKey& Channel, FChannelDelegate Delegate)
{
	if (Channel.IsNone())
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("A channel subscription needs the channel's key."));
		return FDelegateHandle();
	}
	TSharedRef<FChannelEvent>* Event = ChannelEvents.Find(Channel);
	if (!Event)
	{
		Event = &ChannelEvents.Add(Channel, MakeShared<FChannelEvent>());
	}
	return (*Event)->Add(MoveTemp(Delegate));
}

void UMuseeSignals::Unsubscribe(const FMuseeKey& Channel, FDelegateHandle Handle)
{
	if (const TSharedRef<FChannelEvent>* Found = ChannelEvents.Find(Channel))
	{
		const TSharedRef<FChannelEvent> Event = *Found;
		Event->Remove(Handle);
		if (!Event->IsBound())
		{
			ChannelEvents.Remove(Channel);
		}
	}
}

void UMuseeSignals::Deinitialize()
{
	ContactEvent.Clear();
	ActuationEvent.Clear();
	ChannelValues.Empty();
	ChannelEvents.Empty();
	Super::Deinitialize();
}
