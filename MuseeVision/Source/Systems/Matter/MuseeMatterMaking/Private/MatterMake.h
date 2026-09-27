#pragma once

#include "CoreMinimal.h"
#include "MatterInstances.h"
#include "MatterMasters.h"

class UMaterial;
class UMaterialParameterCollection;

/**
 * Matter's make step: everything Matter defines, made as assets, deterministically.
 *
 * 1. The museum Moment's parameter collection, /Game/Generated/Time/MPC_Moment.
 * 2. Every master (Musee::Matter::AllMasters) in /Game/Generated/Matter, each compiled for the museum's shader
 *    platform (PCD3D_SM6) without rendering.
 * 3. One instance for every finish every palette entry offers, /Game/Generated/Matter/MI_<Palette>_<Working>_<History>.
 *
 * Each asset is saved only when its bytes differ from the file on disk, so a second make with nothing changed rewrites
 * nothing. Every problem is reported; nothing is made in place of what failed.
 */
namespace Musee::Matter
{
	/** What a make did. */
	struct FMatterMakeReport
	{
		int32 Written = 0;
		int32 Unchanged = 0;
		/** One line per asset: what it is, whether it was written, its size and hash, and for a master its shaders. */
		TArray<FString> Lines;
		TArray<FString> Problems;

		bool Succeeded() const { return Problems.IsEmpty(); }
	};

	/** A made master, by its kind. */
	struct FMadeMaster
	{
		FMasterKind Kind;
		UMaterial* Material = nullptr;
	};

	/** Makes MPC_Moment. */
	UMaterialParameterCollection* MakeMomentCollection(FMatterMakeReport& Report);

	/** Makes every master, reading Moment, and compiles each when bCompile. */
	TArray<FMadeMaster> MakeMasters(UMaterialParameterCollection& Moment, bool bCompile, FMatterMakeReport& Report);

	/** Every finish the palette offers, as specs, in the palette's order. */
	TValueOrError<TArray<FMatterSpec>, FString> PaletteSpecs();

	/** Makes the instance of every palette finish over the made masters. */
	void MakeInstances(const TArray<FMadeMaster>& Masters, FMatterMakeReport& Report);

	/** The whole make: the Moment, the masters, the instances. */
	FMatterMakeReport MakeMatter(bool bCompile);
}
