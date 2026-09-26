#include "MatterMasterAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "MatterMasterGraph.h"
#include "Materials/Material.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace Musee::Matter
{
namespace
{
	/** Moves a material an earlier make left in the package into the transient package, where it is collected, so
	 *  the new master is built from a fresh object and carries nothing over from the old one. */
	void RetireEarlierMake(UPackage& Package, FName AssetName)
	{
		UObject* Earlier = StaticFindObjectFast(nullptr, &Package, AssetName);
		if (!Earlier)
		{
			return;
		}
		Earlier->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		Earlier->ClearFlags(RF_Public | RF_Standalone);
		Earlier->MarkAsGarbage();
	}
}

FMasterMade MakeMasterAsset(const FMasterRecipe& Recipe)
{
	FMasterMade Made;
	const FString PackageName = PackageNameOf(Recipe);
	UPackage* Package = CreatePackage(*PackageName);
	// A package whose file is on disk but not loaded counts as partly loaded, and the engine refuses to save over it.
	// The make replaces its whole content, so nothing on disk is kept.
	Package->MarkAsFullyLoaded();
	RetireEarlierMake(*Package, Recipe.AssetName);

	Made.Material = BuildMaster(Recipe, Package, Recipe.AssetName, RF_Public | RF_Standalone | RF_Transactional, Made.Problems);
	if (!Made.Material)
	{
		return Made;
	}

	// CreatePackage gives every new package a random persistent guid, which the package summary saves.
	Package->SetPersistentGuid(IdentityOf(Recipe).Derive(TEXT("Package.PersistentGuid")));
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Made.Material);

	Made.Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs Save;
	Save.TopLevelFlags = RF_Public | RF_Standalone;
	Save.SaveFlags = SAVE_NoError;
	Save.bSlowTask = false;
	const FSavePackageResultStruct Saved = UPackage::Save(Package, Made.Material, *Made.Filename, Save);
	if (Saved.Result != ESavePackageResult::Success)
	{
		Made.Problems.Add(FString::Printf(TEXT("%s could not be saved to %s (ESavePackageResult %d)."),
			*PackageName, *Made.Filename, static_cast<int32>(Saved.Result)));
	}
	return Made;
}
}
