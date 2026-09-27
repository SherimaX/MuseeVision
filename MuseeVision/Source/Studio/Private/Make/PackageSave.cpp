#include "Make/PackageSave.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/World.h"
#include "Misc/OutputDeviceError.h"
#include "Misc/PackageName.h"
#include "Misc/StringOutputDevice.h"
#include "PackageTools.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace Musee::Studio
{
	UPackage* CreateMadePackage(const FString& PackageName)
	{
		UPackage* Package = CreatePackage(*PackageName);
		// A package whose file exists counts as partially loaded until marked; this one replaces that file's content.
		Package->MarkAsFullyLoaded();
		return Package;
	}

	UObject* FindOrCreateMadeAsset(UClass& Class, const FString& PackageName)
	{
		const FString AssetName = FPackageName::GetShortName(PackageName);
		const FString ObjectPath = PackageName + TEXT(".") + AssetName;
		if (UObject* Loaded = StaticFindObject(&Class, nullptr, *ObjectPath); IsValid(Loaded))
		{
			return Loaded;
		}
		if (FindPackage(nullptr, *PackageName))
		{
			if (UObject* OnDisk = StaticLoadObject(&Class, nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				return OnDisk;
			}
		}
		UPackage* Package = CreateMadePackage(PackageName);
		if (UObject* InTheWay = StaticFindObjectFast(nullptr, Package, FName(*AssetName)))
		{
			InTheWay->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
		}
		UObject* Asset = NewObject<UObject>(Package, &Class, FName(*AssetName), RF_Public | RF_Standalone);
		IAssetRegistry::GetChecked().AssetCreated(Asset);
		return Asset;
	}

	bool SaveMadePackage(UObject& Asset, FString& OutWhy)
	{
		UPackage* Package = Asset.GetPackage();
		const FString PackageName = Package->GetName();
		const bool bLevel = Asset.IsA<UWorld>();
		const FString Filename = FPackageName::LongPackageNameToFilename(PackageName,
			bLevel ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension());

		// A new package gets a random persistent GUID; one derived from the name makes every checkout write the same.
		Package->SetPersistentGuid(FGuid::NewDeterministicGuid(PackageName));

		FStringOutputDevice Errors;
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		Args.bSlowTask = false;
		Args.Error = &Errors;
		if (!UPackage::SavePackage(Package, &Asset, *Filename, Args))
		{
			OutWhy = FString::Printf(TEXT("could not save %s: %s"), *Filename, Errors.IsEmpty() ? TEXT("no reason given") : *Errors);
			return false;
		}
		return true;
	}

	void ReleasePackages(const TArray<UPackage*>& Packages)
	{
		if (Packages.IsEmpty())
		{
			return;
		}
		UPackageTools::FUnloadPackageParams Params(Packages);
		Params.bUnloadDirtyPackages = true;
		Params.bResetTransBuffer = false;
		UPackageTools::UnloadPackages(Params);
	}
}
