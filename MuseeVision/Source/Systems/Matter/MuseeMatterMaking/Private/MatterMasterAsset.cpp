#include "MatterMasterAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "MatterMasterGraph.h"
#include "Materials/Material.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace Musee::Matter
{
namespace
{
	/** Where a make saves before comparing with the file on disk: the project's Saved folder, never the content. */
	constexpr const TCHAR* StagingFolder = TEXT("MuseeMatter");

	/** Moves an object an earlier make left in the package into the transient package, where it is collected, so the
	 *  new asset is built from a fresh object and carries nothing over from the old one. */
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

	TArray<uint8> ReadFile(const FString& Filename)
	{
		TArray<uint8> Bytes;
		FFileHelper::LoadFileToArray(Bytes, *Filename, FILEREAD_Silent);
		return Bytes;
	}
}

UPackage* PrepareMadePackage(const FString& PackageName, FName AssetName)
{
	UPackage* Package = CreatePackage(*PackageName);
	// A package whose file is on disk but not loaded counts as partly loaded, and the engine refuses to save over it.
	// The make replaces its whole content, so nothing on disk is kept.
	Package->MarkAsFullyLoaded();
	RetireEarlierMake(*Package, AssetName);
	return Package;
}

FMadeSave SaveMadeAsset(UObject& Asset, const FGuid& PersistentGuid, const TMap<FString, FString>& Tags)
{
	FMadeSave Save;
	UPackage* Package = Asset.GetPackage();
	FMetaData& MetaData = Package->GetMetaData();
	for (const TPair<FString, FString>& Tag : Tags)
	{
		MetaData.SetValue(&Asset, *Tag.Key, *Tag.Value);
	}
	// CreatePackage gives every new package a random persistent guid, which the package summary saves.
	Package->SetPersistentGuid(PersistentGuid);
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(&Asset);

	const FString PackageName = Package->GetName();
	Save.Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	const FString Staged = FPaths::ProjectSavedDir() / StagingFolder / FPaths::GetCleanFilename(Save.Filename);
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	Args.bSlowTask = false;
	const FSavePackageResultStruct Saved = UPackage::Save(Package, &Asset, *Staged, Args);
	if (Saved.Result != ESavePackageResult::Success)
	{
		Save.Problems.Add(FString::Printf(TEXT("%s could not be saved (ESavePackageResult %d)."), *PackageName, static_cast<int32>(Saved.Result)));
		return Save;
	}
	const TArray<uint8> Bytes = ReadFile(Staged);
	Save.Bytes = Bytes.Num();
	Save.Hash = FBlake3::HashBuffer(Bytes.GetData(), Bytes.Num());
	IFileManager& Files = IFileManager::Get();
	if (ReadFile(Save.Filename) == Bytes)
	{
		Files.Delete(*Staged);
		return Save;
	}
	Save.bWritten = Files.Move(*Save.Filename, *Staged, true, true);
	if (!Save.bWritten)
	{
		Save.Problems.Add(FString::Printf(TEXT("%s could not be moved into place at %s."), *PackageName, *Save.Filename));
	}
	return Save;
}

FMasterMade MakeMasterAsset(const FMasterRecipe& Recipe)
{
	FMasterMade Made;
	UPackage* Package = PrepareMadePackage(PackageNameOf(Recipe), Recipe.AssetName);
	Made.Material = BuildMaster(Recipe, Package, Recipe.AssetName, RF_Public | RF_Standalone | RF_Transactional, Made.Problems);
	if (!Made.Material)
	{
		return Made;
	}
	const FMasterIdentity Identity = IdentityOf(Recipe);
	const TMap<FString, FString> Tags = {
		{ MadeTag::Made, Recipe.AssetName.ToString() },
		{ MadeTag::Inputs, LexToString(Identity.InputHash) },
		{ MadeTag::Generator, FString::Printf(TEXT("musee.matter.master builder=%d"), MasterBuilderVersion) },
	};
	Made.Save = SaveMadeAsset(*Made.Material, Identity.Derive(TEXT("Package.PersistentGuid")), Tags);
	return Made;
}
}
