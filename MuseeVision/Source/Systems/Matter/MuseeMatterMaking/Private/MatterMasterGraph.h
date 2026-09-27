#pragma once

#include "CoreMinimal.h"
#include "MatterMasterRecipe.h"

class UMaterial;

/**
 * The master graph builder: it turns a master recipe into a Substrate material, in C++ (Decision D-007).
 *
 * Each parameter becomes a parameter expression, each call a Custom expression that includes Matter's shading
 * library, each transform a Transform expression, and the graph ends in one Substrate Slab on the material's front
 * material, with haze resolved onto the Slab's second specular lobe. The builder names every expression after its
 * recipe node and gives the material and each expression identifiers derived from the recipe's hash, so the material
 * it makes depends on nothing but the recipe: not on the session, the order of earlier makes, or the clock.
 */
namespace Musee::Matter
{
	/** Builds Recipe's graph into a new material called Name inside Outer, with Flags. The material comes back
	 *  complete: connected, laid out, its cached data updated, its StateId and every guid set from the recipe's
	 *  identity. Returns null, with OutProblems filled, when the recipe does not validate. */
	UMaterial* BuildMaster(const FMasterRecipe& Recipe, UObject* Outer, FName Name, EObjectFlags Flags, TArray<FString>& OutProblems);

	/** Sets the identifiers the engine gives a material at random (its StateId and its lighting guid) from Identity.
	 *  It must run after the material's last PostEditChange: UMaterialInterface::PostEditChangeProperty renews the
	 *  lighting guid, and UMaterial::CacheResourceShadersForRendering renews the StateId when asked to regenerate it. */
	void StampMaterialIdentity(UMaterial& Material, const FMasterIdentity& Identity);
}
