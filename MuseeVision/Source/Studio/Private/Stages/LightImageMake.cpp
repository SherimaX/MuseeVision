#include "Stages/LightImageMake.h"

#include "ArtWork.h"
#include "AssetCompilingManager.h"
#include "Engine/Texture2D.h"
#include "LightImage.h"
#include "Make/InputHash.h"
#include "Make/PackageSave.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraVectorWS.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSubstrate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "MatterMasterCompile.h"
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
		/** The revision of the image's making: raise it whenever the same inputs would make a different star field or material. */
		constexpr int64 LightImageRevision = 1;

		/** Whose finding an unmade image is: Light's, whose image it is. */
		const TCHAR* const ImageCheck = TEXT("Light.LuminairesFromCatalogue");

		namespace Image = Musee::Light::Image;

		/** Records one made package's outcome: written, or failed with its reason. */
		FMadePackage& Made(FMakeReport& Report, const FString& Package)
		{
			FMadePackage& Entry = Report.Packages.AddDefaulted_GetRef();
			Entry.PackageName = Package;
			return Entry;
		}

		/** Saves a made asset under its input hash; false, with the reason on the package, when it can't be. */
		bool Save(UObject& Asset, const FString& Hash, FMadePackage& Entry)
		{
			StoreInputHash(Asset, Hash);
			if (SaveMadePackage(Asset, Entry.Why))
			{
				Entry.Outcome = EMadeOutcome::Written;
				return true;
			}
			StoreInputHash(Asset, FString());
			Entry.Outcome = EMadeOutcome::Failed;
			return false;
		}

		template <typename TExpression>
		TExpression* AddExpression(UMaterial& Material)
		{
			TExpression* Expression = NewObject<TExpression>(&Material, NAME_None, RF_Transactional);
			Material.GetExpressionCollection().AddExpression(Expression);
			Expression->Material = &Material;
			return Expression;
		}

		UMaterialExpressionVectorParameter* Vector(UMaterial& Material, const TCHAR* Name, const FLinearColor& Default)
		{
			UMaterialExpressionVectorParameter* Parameter = AddExpression<UMaterialExpressionVectorParameter>(Material);
			Parameter->ParameterName = Name;
			Parameter->DefaultValue = Default;
			return Parameter;
		}

		UMaterialExpressionScalarParameter* Scalar(UMaterial& Material, const TCHAR* Name, float Default)
		{
			UMaterialExpressionScalarParameter* Parameter = AddExpression<UMaterialExpressionScalarParameter>(Material);
			Parameter->ParameterName = Name;
			Parameter->DefaultValue = Default;
			return Parameter;
		}
	}

	bool FillStarField(const TArray<uint8>& CatalogueBytes, UTexture2D& Texture, FString& OutWhy)
	{
		const TValueOrError<Image::FStarField, FString> Field = Image::DrawStarField(CatalogueBytes, Image::StarFieldWidth, Image::StarFieldHeight);
		if (Field.HasError())
		{
			OutWhy = Field.GetError();
			return false;
		}
		TArray<FFloat16Color> Texels;
		Texels.Reserve(Field.GetValue().Luminance.Num());
		for (const FLinearColor& Luminance : Field.GetValue().Luminance)
		{
			Texels.Add(FFloat16Color(FLinearColor(Luminance.R, Luminance.G, Luminance.B, 1.0f)));
		}
		Texture.Source.Init(Field.GetValue().Width, Field.GetValue().Height, 1, 1, TSF_RGBA16F, reinterpret_cast<const uint8*>(Texels.GetData()));
		Texture.CompressionSettings = TC_HDR;
		Texture.SRGB = false;
		Texture.MipGenSettings = TMGS_NoMipmaps;
		Texture.LODGroup = TEXTUREGROUP_World;
		Texture.Filter = TF_Bilinear;
		Texture.AddressX = TA_Wrap;
		Texture.AddressY = TA_Clamp;
		return true;
	}

	void BuildImageMaterial(UMaterial& Material, UTexture2D& StarField)
	{
		for (UMaterialExpression* Earlier : TArray<TObjectPtr<UMaterialExpression>>(Material.GetExpressionCollection().Expressions))
		{
			if (Earlier)
			{
				Earlier->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}
		Material.GetExpressionCollection().Empty();
		Material.BlendMode = BLEND_Opaque;
		Material.TwoSided = true;

		UMaterialExpressionCustom* Emission = AddExpression<UMaterialExpressionCustom>(Material);
		Emission->Description = TEXT("The sky, as Light's image shows it");
		Emission->Code = Image::EmissionHlsl();
		Emission->OutputType = CMOT_Float3;
		Emission->Inputs.Empty();
		const auto Feed = [Emission](const TCHAR* Name, UMaterialExpression* From)
		{
			FCustomInput& Input = Emission->Inputs.AddDefaulted_GetRef();
			Input.InputName = Name;
			Input.Input.Connect(0, From);
		};
		Feed(TEXT("ToEye"), AddExpression<UMaterialExpressionCameraVectorWS>(Material));
		UMaterialExpressionTextureObjectParameter* Stars = AddExpression<UMaterialExpressionTextureObjectParameter>(Material);
		Stars->ParameterName = Image::Parameter::StarField;
		Stars->Texture = &StarField;
		Stars->SamplerType = SAMPLERTYPE_LinearColor;
		Feed(Image::Parameter::StarField, Stars);
		// At rest the sky is unturned, the Moon hidden (its disc of no size) and the panel dark until Light runs it.
		Feed(Image::Parameter::SkyRowX, Vector(Material, Image::Parameter::SkyRowX, FLinearColor(1.0f, 0.0f, 0.0f, 0.0f)));
		Feed(Image::Parameter::SkyRowY, Vector(Material, Image::Parameter::SkyRowY, FLinearColor(0.0f, 1.0f, 0.0f, 0.0f)));
		Feed(Image::Parameter::SkyRowZ, Vector(Material, Image::Parameter::SkyRowZ, FLinearColor(0.0f, 0.0f, 1.0f, 0.0f)));
		Feed(Image::Parameter::MoonDirection, Vector(Material, Image::Parameter::MoonDirection, FLinearColor(0.0f, 0.0f, 1.0f, 0.0f)));
		Feed(Image::Parameter::MoonSunward, Vector(Material, Image::Parameter::MoonSunward, FLinearColor(0.0f, 0.0f, -1.0f, 0.0f)));
		Feed(Image::Parameter::MoonCosRadius, Scalar(Material, Image::Parameter::MoonCosRadius, 1.0f));
		Feed(Image::Parameter::MoonLuminance, Scalar(Material, Image::Parameter::MoonLuminance, 0.0f));
		Feed(Image::Parameter::PeakLuminance, Scalar(Material, Image::Parameter::PeakLuminance, 0.0f));
		Feed(Image::Parameter::Level, Scalar(Material, Image::Parameter::Level, 0.0f));

		UMaterialExpressionSubstrateUnlitBSDF* Unlit = AddExpression<UMaterialExpressionSubstrateUnlitBSDF>(Material);
		Unlit->EmissiveColor.Connect(0, Emission);
		UMaterialEditingLibrary::ConnectMaterialProperty(Unlit, FString(), MP_FrontMaterial);
		UMaterialEditingLibrary::LayoutMaterialExpressions(&Material);
		Material.PostEditChange();
	}

	FString MakeLightImage(const FStageMakeOptions& Options, FMakeReport& Report)
	{
		FInputHasher Fingerprint;
		Fingerprint.Add(TEXT("light-image"));
		const FString Catalogue = Musee::Art::SourcePath(Image::CatalogueSource);
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *Catalogue, FILEREAD_Silent))
		{
			Report.Findings.Add(MakeFinding(ImageCheck, TEXT("Light"), FString::Printf(TEXT("the star catalogue %s of Light's image can't be read"), *Catalogue)));
			return Fingerprint.Finish();
		}

		// The star field, from the catalogue.
		FInputHasher FieldInputs;
		FieldInputs.Add(TEXT("light-star-field")).Add(LightImageRevision).Add(FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString())
			.Add(int64(Image::StarFieldWidth)).Add(int64(Image::StarFieldHeight)).Add(Image::FaintestMagnitude);
		const FString FieldHash = FieldInputs.Finish();
		const FString FieldPackage = Options.Paths.LightImage(Image::StarFieldName);
		FMadePackage& FieldMade = Made(Report, FieldPackage);
		// The stored hash is read before the asset is found or made: a fresh one made in its package would hide the one on
		// disk. An unchanged field is loaded as it is, for the material that samples it.
		const bool bFieldUnchanged = StoredInputHash(FieldPackage, Image::StarFieldName) == FieldHash;
		UTexture2D* StarField = bFieldUnchanged ? LoadObject<UTexture2D>(nullptr, *(FieldPackage + TEXT(".") + Image::StarFieldName))
			: FindOrCreateMadeAsset<UTexture2D>(FieldPackage);
		if (!StarField)
		{
			FieldMade.Outcome = EMadeOutcome::Failed;
			FieldMade.Why = TEXT("its made star field doesn't load");
			return Fingerprint.Finish();
		}
		if (bFieldUnchanged)
		{
			FieldMade.Outcome = EMadeOutcome::Unchanged;
		}
		else if (!FillStarField(Bytes, *StarField, FieldMade.Why))
		{
			FieldMade.Outcome = EMadeOutcome::Failed;
			return Fingerprint.Finish();
		}
		else
		{
			StarField->PostEditChange();
			FAssetCompilingManager::Get().FinishAllCompilation();
			StarField->SetLightingGuid(FGuid::NewDeterministicGuid(FieldHash));
			if (!Save(*StarField, FieldHash, FieldMade))
			{
				return Fingerprint.Finish();
			}
		}
		Fingerprint.Add(FieldPackage).Add(FieldHash);

		// The material over it, whose emission is Light's.
		FInputHasher MaterialInputs;
		MaterialInputs.Add(TEXT("light-image-material")).Add(LightImageRevision).Add(Image::EmissionHlsl()).Add(FieldPackage);
		const FString MaterialHash = MaterialInputs.Finish();
		const FString MaterialPackage = Options.Paths.LightImage(Image::MaterialName);
		FMadePackage& MaterialMade = Made(Report, MaterialPackage);
		if (StoredInputHash(MaterialPackage, Image::MaterialName) == MaterialHash)
		{
			MaterialMade.Outcome = EMadeOutcome::Unchanged;
		}
		else
		{
			UMaterial* Material = FindOrCreateMadeAsset<UMaterial>(MaterialPackage);
			BuildImageMaterial(*Material, *StarField);
			const Musee::Matter::FMasterCompile Compiled = Musee::Matter::CompileMasterShaders(*Material, Musee::Matter::MuseumShaderPlatform, Musee::Matter::MuseumFeatureLevel);
			if (!Compiled.bCompiled)
			{
				MaterialMade.Outcome = EMadeOutcome::Failed;
				MaterialMade.Why = FString::Join(Compiled.Errors, TEXT(" "));
				Report.Findings.Add(MakeFinding(ImageCheck, TEXT("Light"), FString::Printf(TEXT("Light's image material doesn't compile: %s"), *MaterialMade.Why)));
				return Fingerprint.Finish();
			}
			if (!Save(*Material, MaterialHash, MaterialMade))
			{
				return Fingerprint.Finish();
			}
		}
		return Fingerprint.Add(MaterialPackage).Add(MaterialHash).Finish();
	}
}
