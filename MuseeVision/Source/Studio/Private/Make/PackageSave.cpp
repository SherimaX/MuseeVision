#include "Make/PackageSave.h"

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
