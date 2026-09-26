#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MuseeLawnLibrary.generated.h"

/**
 * The lawn's patches of grass (MuseeLawn::BuildPatch) and the hedges' leaf modules (MuseeHedge::BuildModule) as
 * Nanite static meshes. The lawn's in /Game/Museum/Nature/Lawn, for
 * AMuseeLawn's instances; editor only (Scripts/lawn.py). No collision, no distance field, no Lumen cards (the blades
 * are kept out of the ray-traced and Lumen scenes); the blade material is marked for Nanite and instancing, which a
 * game cannot do for itself.
 */
UCLASS()
class MUSEEVISIONEDITOR_API UMuseeLawnLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Build and save every patch mesh. Returns how many were saved. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Lawn")
	static int32 BuildLawnMeshes();

	/**
	 * The clipped hedges' modules (MuseeHedge::BuildModule: box and yew leaves as geometry) as Nanite static meshes in
	 * /Game/Museum/Nature/Hedge, for AMuseeLandscape's instances. They cast shadows and are ray traced (their own
	 * self-shadowing is what makes the leaf shell read as deep); no collision, distance field or Lumen cards (the
	 * landscape's dark core carries those). Returns how many packages were saved.
	 */
	UFUNCTION(BlueprintCallable, Category = "Musee|Lawn")
	static int32 BuildHedgeMeshes();
};
