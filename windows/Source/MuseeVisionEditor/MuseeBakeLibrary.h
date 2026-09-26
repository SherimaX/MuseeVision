#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MuseeBakeLibrary.generated.h"

class AActor;

/**
 * Bakes the museum's native procedural architecture (the C++ rooms, frames and plants, made of
 * UProceduralMeshComponents) into static meshes, for Nanite, mesh distance fields and Lumen's surface
 * cards, and so the rooms don't rebuild when the map loads. Editor only; Scripts/bake.py and the
 * bake step of Scripts/apply_all.py call it.
 *
 * For each procedural component of a bakeable actor, up to three meshes are saved in
 * <Folder>/<ActorLabel>/: <Component> (the opaque sections: Nanite), <Component>_Translucent (glass:
 * no Nanite, no distance field) and <Component>_Collision (sections hidden but colliding, like a
 * frame's hit boxes: never drawn). Each becomes a static mesh component of the same actor (tagged
 * musee.baked), at the same place, with the same materials, shadows and collision; the procedural
 * component is emptied, hidden and loses its collision, and the actor is tagged musee.baked.
 * Rebaking replaces the meshes and components. See Source/MuseeVision/Geometry/MuseeBake.h for the
 * guards the actors need and for what is never baked.
 */
UCLASS()
class MUSEEVISIONEDITOR_API UMuseeBakeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Bake one actor (rebaking it if it was baked). Folder: e.g. /Game/Museum/Baked (empty = that). Returns the static mesh components made. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Bake")
	static int32 BakeActor(AActor* Actor, const FString& Folder);

	/** Bake these actors, building all their meshes in one batch. Returns the static mesh components made. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Bake")
	static int32 BakeActors(const TArray<AActor*>& Actors, const FString& Folder);

	/** Bake every bakeable actor of the editor's world. Returns the static mesh components made. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Bake")
	static int32 BakeAll(const FString& Folder);

	/** Undo a bake: the baked components go, the procedural ones come back and are rebuilt. False if it wasn't baked. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Bake")
	static bool UnbakeActor(AActor* Actor);

	/** Unbake every baked actor of the editor's world. Returns how many. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Bake")
	static int32 UnbakeAll();

	UFUNCTION(BlueprintPure, Category = "Musee|Bake")
	static bool IsBaked(const AActor* Actor);

	/** Why an actor is never baked (it ticks, the visitor uses it, it is tagged musee.nobake…); empty if it can be. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Bake")
	static FString WhyNotBakeable(AActor* Actor);
};
