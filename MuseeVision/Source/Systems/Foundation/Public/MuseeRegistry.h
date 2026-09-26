#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "Subsystems/WorldSubsystem.h"

#include "MuseeRegistry.generated.h"

class USceneComponent;

/**
 * The world's register of keyed things. A key resolves to the scene component that carries it: a moving body, a
 * water body, a luminaire. Systems attach to one another's things by key without seeing one another: Sound finds the
 * body a mechanism voice rides on, Nature the basin its water fills.
 *
 * One key names one live component. Registrations are weak, so a destroyed component frees its key.
 */
UCLASS()
class MUSEEFOUNDATION_API UMuseeRegistry final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Announces a component registered under a key, for things that begin before the thing they attach to. */
	using FRegistered = TMulticastDelegate<void(const FMuseeKey& Key, USceneComponent* Component)>;

	/**
	 * Registers a component under its key. Refused (with an error) for the none key, a null component, or a key that
	 * another live component already carries. Registering the same component again under its key does nothing.
	 */
	bool Register(const FMuseeKey& Key, USceneComponent* Component);

	/** Frees a key, if the given component is the one that carries it. */
	void Unregister(const FMuseeKey& Key, const USceneComponent* Component);

	/** The live component that carries a key, or null. */
	[[nodiscard]] USceneComponent* Find(const FMuseeKey& Key) const;

	/** Fires after each successful registration. */
	FRegistered& OnRegistered() { return Registered; }

	virtual void Deinitialize() override;

private:
	TMap<FMuseeKey, TWeakObjectPtr<USceneComponent>> Components;
	FRegistered Registered;
};
