#pragma once

#include "CoreMinimal.h"
#include "MuseeActuation.h"
#include "MuseeChannel.h"
#include "MuseeContact.h"
#include "MuseeKey.h"
#include "Subsystems/WorldSubsystem.h"

#include "MuseeSignals.generated.h"

/**
 * The world's signals and control points: how news travels between systems that never see each other. Contacts and
 * actuations are events, heard by whoever listens. Channels are points with a current value: publishing a new value
 * notifies the point's subscribers, and anyone may read the value at any time.
 *
 * Signals are published and heard on the game thread.
 */
UCLASS()
class MUSEEFOUNDATION_API UMuseeSignals final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	using FContactEvent = TMulticastDelegate<void(const FMuseeContact&)>;
	using FActuationEvent = TMulticastDelegate<void(const FMuseeActuation&)>;
	using FChannelEvent = TMulticastDelegate<void(const FMuseeChannel&)>;
	using FChannelDelegate = FChannelEvent::FDelegate;

	/** Announces a contact to every listener. */
	void Publish(const FMuseeContact& Contact);

	/** Announces an actuation to every listener. */
	void Publish(const FMuseeActuation& Actuation);

	/**
	 * Sets a channel's value. The point's subscribers hear it when the value changes, as a building-automation point
	 * reports a change of value; publishing the value it already holds is silent. Refused for the none key.
	 */
	void Publish(const FMuseeChannel& Channel);

	FContactEvent& OnContact() { return ContactEvent; }
	FActuationEvent& OnActuation() { return ActuationEvent; }

	/** A channel's current value, or nothing if nobody has published on it. */
	[[nodiscard]] TOptional<float> Read(const FMuseeKey& Channel) const;

	/** Hears every change of a channel's value from now on. Read the channel for its value before this. */
	FDelegateHandle Subscribe(const FMuseeKey& Channel, FChannelDelegate Delegate);

	/** Stops hearing a channel. */
	void Unsubscribe(const FMuseeKey& Channel, FDelegateHandle Handle);

	virtual void Deinitialize() override;

private:
	FContactEvent ContactEvent;
	FActuationEvent ActuationEvent;
	TMap<FMuseeKey, float> ChannelValues;
	/** Shared, so a point's subscribers may subscribe and unsubscribe while they hear it. */
	TMap<FMuseeKey, TSharedRef<FChannelEvent>> ChannelEvents;
};
