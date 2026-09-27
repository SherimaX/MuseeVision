#include "MuseeRegistry.h"

#include "Components/SceneComponent.h"
#include "MuseeFoundationLog.h"

bool UMuseeRegistry::Register(const FMuseeKey& Key, USceneComponent* Component)
{
	if (Key.IsNone() || !Component)
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("The registry takes a key and the component that carries it."));
		return false;
	}
	TWeakObjectPtr<USceneComponent>& Holder = Components.FindOrAdd(Key);
	if (Holder.Get() == Component)
	{
		return true;
	}
	if (Holder.IsValid())
	{
		UE_LOG(LogMuseeFoundation, Error, TEXT("Key '%s' is already carried by %s; %s can't carry it too."),
			*Key.ToString(), *Holder->GetPathName(), *Component->GetPathName());
		return false;
	}
	Holder = Component;
	Registered.Broadcast(Key, Component);
	return true;
}

void UMuseeRegistry::Unregister(const FMuseeKey& Key, const USceneComponent* Component)
{
	const TWeakObjectPtr<USceneComponent>* Holder = Components.Find(Key);
	if (Holder && (!Holder->IsValid() || Holder->Get() == Component))
	{
		Components.Remove(Key);
	}
}

USceneComponent* UMuseeRegistry::Find(const FMuseeKey& Key) const
{
	const TWeakObjectPtr<USceneComponent>* Holder = Components.Find(Key);
	return Holder ? Holder->Get() : nullptr;
}

void UMuseeRegistry::Deinitialize()
{
	Components.Empty();
	Registered.Clear();
	Super::Deinitialize();
}
