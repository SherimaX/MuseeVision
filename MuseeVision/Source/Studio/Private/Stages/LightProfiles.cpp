#include "Stages/LightProfiles.h"

#include "AssetCompilingManager.h"
#include "Engine/TextureLightProfile.h"
#include "IESConverter.h"
#include "LightLuminaire.h"
#include "Make/InputHash.h"
#include "Make/PackageSave.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "Stages/MakeFinding.h"
#include "Stages/StageMake.h"
#include "UObject/Package.h"

namespace Musee::Studio
{
	namespace
	{
		/** The revision of the import: raise it whenever a profile would be made differently from the same file. */
		constexpr int64 LightProfileRevision = 1;

		/** Whose finding an unmade profile is: Light's, whose catalogue names the file. */
		const TCHAR* const ProfileCheck = TEXT("Light.LuminairesFromCatalogue");

		/** One photometric file the catalogue lights a type by, and the profile it is made into. */
		struct FProfileToMake
		{
			FString File;
			FString Name;
			FMuseeKey Type;
		};

		/** Every photometric file the catalogue names, once each, in name order. */
		TArray<FProfileToMake> ProfilesToMake()
		{
			TArray<FProfileToMake> Profiles;
			for (const FMuseeKey& Key : Musee::Light::LuminaireKeys())
			{
				const FLightLuminaire* Type = Musee::Light::FindLuminaire(Key);
				const FString Name = Type ? Musee::Light::ProfileName(*Type) : FString();
				if (!Name.IsEmpty() && !Profiles.ContainsByPredicate([&Name](const FProfileToMake& Made) { return Made.Name == Name; }))
				{
					Profiles.Add({Type->PhotometricFile, Name, Key});
				}
			}
			Profiles.Sort([](const FProfileToMake& A, const FProfileToMake& B) { return A.Name < B.Name; });
			return Profiles;
		}

		/** Makes one profile from its file's bytes, or leaves it when its input hash is unchanged. */
		void MakeProfile(const FString& Package, const TArray<uint8>& Bytes, const FString& Hash, FMakeReport& Report)
		{
			FMadePackage& Made = Report.Packages.AddDefaulted_GetRef();
			Made.PackageName = Package;
			if (StoredInputHash(Package, FPackageName::GetShortName(Package)) == Hash)
			{
				Made.Outcome = EMadeOutcome::Unchanged;
				return;
			}
			UTextureLightProfile* Profile = FindOrCreateMadeAsset<UTextureLightProfile>(Package);
			if (!ImportLightProfile(Bytes, *Profile, Made.Why))
			{
				Made.Outcome = EMadeOutcome::Failed;
				return;
			}
			Profile->SetLightingGuid(FGuid::NewDeterministicGuid(Hash));
			Profile->PostEditChange();
			FAssetCompilingManager::Get().FinishAllCompilation();
			StoreInputHash(*Profile, Hash);
			if (SaveMadePackage(*Profile, Made.Why))
			{
				Made.Outcome = EMadeOutcome::Written;
				return;
			}
			StoreInputHash(*Profile, FString());
			Made.Outcome = EMadeOutcome::Failed;
		}
	}

	bool ImportLightProfile(const TArray<uint8>& FileBytes, UTextureLightProfile& Profile, FString& OutWhy)
	{
		const FIESConverter Converter(FileBytes.GetData(), static_cast<uint32>(FileBytes.Num()));
		if (!Converter.IsValid())
		{
			OutWhy = FString::Printf(TEXT("the engine's IES import refuses it: %s"), Converter.GetError());
			return false;
		}
		Profile.Source.Init(Converter.GetWidth(), Converter.GetHeight(), 1, 1, TSF_RGBA16F, Converter.GetRawData().GetData());
		Profile.AddressX = TA_Clamp;
		Profile.AddressY = TA_Clamp;
		Profile.CompressionSettings = TC_HDR;
		Profile.SRGB = false;
		Profile.MipGenSettings = TMGS_NoMipmaps;
		Profile.LODGroup = TEXTUREGROUP_IESLightProfile;
		Profile.Brightness = Converter.GetBrightness();
		Profile.TextureMultiplier = Converter.GetMultiplier();
		return true;
	}

	FString MakeLightProfiles(const FStageMakeOptions& Options, FMakeReport& Report)
	{
		FInputHasher Made;
		Made.Add(TEXT("light-profiles"));
		for (const FProfileToMake& Profile : ProfilesToMake())
		{
			TArray<uint8> Bytes;
			if (!FFileHelper::LoadFileToArray(Bytes, *Profile.File, FILEREAD_Silent))
			{
				Report.Findings.Add(MakeFinding(ProfileCheck, TEXT("Light"),
					FString::Printf(TEXT("the photometric file %s of %s can't be read"), *Profile.File, *Profile.Type.ToString()), Profile.Type.ToString()));
				continue;
			}
			FInputHasher Inputs;
			Inputs.Add(TEXT("light-profile")).Add(LightProfileRevision).Add(FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString());
			const FString Hash = Inputs.Finish();
			const FString Package = Options.Paths.LightProfile(Profile.Name);
			MakeProfile(Package, Bytes, Hash, Report);
			Made.Add(Package).Add(Hash);
		}
		return Made.Finish();
	}
}
