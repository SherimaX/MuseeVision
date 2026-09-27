#include "Stages/PaintLayers.h"

#include "ArtPaintLayer.h"
#include "ArtWorkImage.h"
#include "AssetCompilingManager.h"
#include "Engine/Texture2D.h"
#include "MatterNames.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Make/InputHash.h"
#include "Make/PackageSave.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "Stages/MakeFinding.h"
#include "Stages/StageMake.h"

namespace Musee::Studio
{
	namespace
	{
		/** The revision of these rules: raise it whenever a paint layer would be made differently from the same inputs. */
		constexpr int64 PaintLayerRevision = 1;

		/**
		 * The paint master's parameters for a work's own (Matter's paint master, MatterMasterParameters.h): the image, and
		 * the image's width and height in metres, against which the face's UV0 (metres from the image's top-left corner)
		 * finds its place in the image.
		 */
		const FName ImageParameter(TEXT("Image"));
		const FName ImageSizeParameter(TEXT("ImageSizeMeters"));

		/** The name of UMaterialInterface's lighting GUID, which a made instance derives from its inputs. */
		const FName LightingGuidProperty(TEXT("LightingGuid"));

		void StampLightingGuid(UMaterialInterface& Material, const FGuid& Guid)
		{
			const FStructProperty* Lighting = FindFProperty<FStructProperty>(UMaterialInterface::StaticClass(), LightingGuidProperty);
			checkf(Lighting && Lighting->Struct == TBaseStructure<FGuid>::Get(), TEXT("UMaterialInterface no longer reflects its LightingGuid as an FGuid."));
			*Lighting->ContainerPtrToValuePtr<FGuid>(&Material) = Guid;
		}

		/** The hash of an image file's bytes, or empty when it can't be read. */
		FString ImageHash(const FString& Path)
		{
			TArray<uint8> Bytes;
			return FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent) ? FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString() : FString();
		}

		/** Saves a made asset with its input hash, recording what became of its package. */
		void Save(UObject& Asset, const FString& Hash, FMakeReport& Report)
		{
			FMadePackage& Made = Report.Packages.AddDefaulted_GetRef();
			Made.PackageName = Asset.GetPackage()->GetName();
			StoreInputHash(Asset, Hash);
			if (SaveMadePackage(Asset, Made.Why))
			{
				Made.Outcome = EMadeOutcome::Written;
				return;
			}
			StoreInputHash(Asset, FString());
		}

		/** The work's image as a texture, made when its image changed; null when it can't be. */
		UTexture2D* MakeImage(const FArtPaintLayer& Layer, const FString& Package, const FString& Hash, FMakeReport& Report)
		{
			const FString Object = Musee::Matter::ObjectPathOf(Package);
			if (StoredInputHash(Package, FPackageName::GetShortName(Package)) == Hash)
			{
				Report.Packages.Add({Package, EMadeOutcome::Unchanged, FString()});
				return LoadObject<UTexture2D>(nullptr, *Object);
			}
			UTexture2D* Texture = FindOrCreateMadeAsset<UTexture2D>(Package);
			FString Why;
			if (!Musee::ArtMaking::ComposeWorkImage(Layer.ImagePath, *Texture, Why))
			{
				Report.Packages.Add({Package, EMadeOutcome::Failed, Why});
				return nullptr;
			}
			Texture->SetLightingGuid(FGuid::NewDeterministicGuid(Hash));
			Texture->PostEditChange();
			FAssetCompilingManager::Get().FinishAllCompilation();
			Save(*Texture, Hash, Report);
			return Texture;
		}

		/** The work's paint instance over its parent, carrying the texture and the image's size. */
		void MakeInstance(const FArtPaintLayer& Layer, const FString& Package, UMaterialInterface& Parent, UTexture2D& Texture, const FString& Hash,
			FMakeReport& Report)
		{
			if (StoredInputHash(Package, FPackageName::GetShortName(Package)) == Hash)
			{
				Report.Packages.Add({Package, EMadeOutcome::Unchanged, FString()});
				return;
			}
			UMaterialInstanceConstant* Instance = FindOrCreateMadeAsset<UMaterialInstanceConstant>(Package);
			Instance->SetParentEditorOnly(&Parent);
			Instance->ClearParameterValuesEditorOnly();
			Instance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(ImageParameter), &Texture);
			Instance->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(ImageSizeParameter),
				FLinearColor(static_cast<float>(Layer.SizeMeters.X), static_cast<float>(Layer.SizeMeters.Y), 0.0f, 0.0f));
			Instance->PostEditChange();
			Instance->ParameterStateId = FGuid::NewDeterministicGuid(Hash + TEXT(".parameters"));
			StampLightingGuid(*Instance, FGuid::NewDeterministicGuid(Hash + TEXT(".lighting")));
			Save(*Instance, Hash, Report);
		}
	}

	void MakePaintLayers(const FMuseeStageContext& Stage, const FStageMakeOptions& Options, FMakeReport& Report)
	{
		const FMaterialResolver Resolve = Options.Resolve ? Options.Resolve : MatterResolver();
		for (const FArtPaintLayer& Layer : Musee::ArtMaking::PaintLayers(Stage))
		{
			UMaterialInterface* Parent = Resolve(Layer.Parent, Layer.Working, Layer.History);
			const FString Image = ImageHash(Layer.ImagePath);
			if (!Parent || Image.IsEmpty())
			{
				Report.Findings.Add(MakeFinding(TEXT("Matter.Resolved"), FName(*Stage.Stage.ToString()), FString::Printf(
					TEXT("the paint of work %s can't be made: %s"), *Layer.Work, Parent ? *FString::Printf(TEXT("its image %s can't be read"),
						*Layer.ImagePath) : *FString::Printf(TEXT("its parent %s (%s, %s) resolves to no material"), *Layer.Parent.ToString(),
						*Layer.Working.ToString(), *Layer.History.ToString())), Layer.Palette.ToString()));
				continue;
			}
			const FString Texture = Options.Paths.WorkImage(Musee::Matter::PascalCase(Layer.Work));
			FInputHasher ImageInputs;
			ImageInputs.Add(TEXT("work-image")).Add(PaintLayerRevision).Add(Image);
			UTexture2D* Made = MakeImage(Layer, Texture, ImageInputs.Finish(), Report);
			if (!Made)
			{
				continue;
			}
			FInputHasher PaintInputs;
			PaintInputs.Add(TEXT("paint-layer")).Add(PaintLayerRevision).Add(Image).Add(Texture).Add(Parent->GetPathName())
				.Add(Layer.SizeMeters.X).Add(Layer.SizeMeters.Y);
			const FString Instance = Options.Paths.MatterInstance(Musee::Matter::InstanceName(Layer.Palette, Layer.Working, Layer.History));
			MakeInstance(Layer, Instance, *Parent, *Made, PaintInputs.Finish(), Report);
		}
	}
}
