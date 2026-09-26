#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"

/**
 * The bake of the native architecture into Nanite static meshes (Source/MuseeVisionEditor,
 * UMuseeBakeLibrary; Scripts/bake.py). A baked actor keeps its procedural components, empty, hidden
 * and without collision, and carries static mesh components (tagged musee.baked) with the same geometry.
 *
 * An actor that builds procedural geometry must not build it again once baked, or it would draw over
 * the baked meshes (a component made at load or at BeginPlay is visible) or at least waste the time
 * and memory. The first line of its build (whatever OnConstruction, BeginPlay or PostRegisterAllComponents
 * call) is:
 *
 *     if (MuseeBake::IsBaked(this)) { return; }
 *
 * A component the bake must leave procedural (it moves, it is shown and hidden, its material is made
 * at run time) is tagged in the constructor:
 *
 *     MuseeBake::NoBake(Iris);
 *
 * and a whole actor with the actor tag musee.nobake. Actors that tick or that the visitor uses
 * (IMuseeInteractable, IMuseePromptProvider) are left procedural, unless they carry the actor tag
 * musee.bakeable: their author vouches that nothing of theirs moves but what is tagged musee.nobake
 * (e.g. a room that ticks only to dim its lamps).
 */
namespace MuseeBake
{
	/** On a baked actor and on each static mesh component the bake made. */
	inline FName BakedTag() { return FName(TEXT("musee.baked")); }

	/** On an actor or a procedural component the bake must leave as it is. */
	inline FName NoBakeTag() { return FName(TEXT("musee.nobake")); }

	/** On an actor that ticks or that the visitor uses, whose procedural components may still be baked (all but those tagged musee.nobake). */
	inline FName BakeableTag() { return FName(TEXT("musee.bakeable")); }

	/** True once the actor's procedural geometry has been baked (by its tag, or by its baked components if a script reset its tags). */
	inline bool IsBaked(const AActor* Actor)
	{
		if (!Actor) { return false; }
		if (Actor->Tags.Contains(BakedTag())) { return true; }
		for (const UActorComponent* Component : Actor->GetComponents())
		{
			if (Component && Component->ComponentTags.Contains(BakedTag())) { return true; }
		}
		return false;
	}

	/** Keep this component procedural (call in the constructor). */
	inline void NoBake(UActorComponent* Component)
	{
		if (Component) { Component->ComponentTags.AddUnique(NoBakeTag()); }
	}
}
