#include "Stages/PaintLayers.h"

#include "ArtPaintLayer.h"
#include "ArtWorkImage.h"
#include "AssetCompilingManager.h"
#include "Engine/Texture2D.h"
#include "MatterNames.h"
#include "MatterPaintLayer.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Make/InputHash.h"
#include "Make/PackageSave.h"
#include "Misc/PackageName.h"
#include "Stages/MakeFinding.h"
#include "Stages/StageMake.h"
#include "UObject/Package.h"

namespace Musee::Studio
{
	namespace
	{
		/** The revisions of these rules: raise one whenever its asset would be made differently from the same inputs. */
		constexpr int64 WorkImageRevision = 1;
		constexpr int64 PaintLayerRevision = 2;

		/** The paint master a stage's paint layers are composed over, or null when it hasn't been made. */
		UMaterialInterface* FindPaintMaster(const FString& Package)
		{
			const FString Object = Musee::Matter::ObjectPathOf(Package);
			UMaterialInterface* Found = FindObject<UMaterialInterface>(nullptr, *Object);
			return Found || !FPackageName::DoesPackageExist(Package) ? Found : LoadObject<UMaterialInterface>(nullptr, *Object);
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
			if (!Musee::ArtMaking::ComposeLayerImage(Layer, *Texture, Why))
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

		/** The palette finish Art's layer derives from, in Matter's words, before it has an image. */
		Musee::Matter::FMatterPaintLayer MatterFinish(const FArtPaintLayer& Layer)
		{
			Musee::Matter::FMatterPaintLayer Finish;
			Finish.Work = Layer.Palette;
			Finish.Palette = Layer.Parent;
			Finish.Working = Layer.Working;
			Finish.History = Layer.History;
			return Finish;
		}

		/** Art's paint layer in Matter's words: the work's own key, the paint's palette finish, the texture and the size. */
		Musee::Matter::FMatterPaintLayer MatterLayer(const FArtPaintLayer& Layer, UTexture2D& Texture)
		{
			Musee::Matter::FMatterPaintLayer Paint;
			Paint.Work = Layer.Palette;
			Paint.Palette = Layer.Parent;
			Paint.Working = Layer.Working;
			Paint.History = Layer.History;
			Paint.Image = &Texture;
			Paint.WidthMeters = Layer.SizeMeters.X;
			Paint.HeightMeters = Layer.SizeMeters.Y;
			return Paint;
		}

		/**
		 * The work's paint instance, composed by Matter. It is composed aside first: the instance's identity (its parameter
		 * state, derived from every record and value written) is its input, so one whose composition hasn't changed is left
		 * as it is. The problems Matter finds, or none.
		 */
		TArray<FString> MakeInstance(const Musee::Matter::FMatterPaintLayer& Paint, const FString& Package, UMaterialInterface& Master, FMakeReport& Report)
		{
			UMaterialInstanceConstant* Aside = NewObject<UMaterialInstanceConstant>(GetTransientPackage());
			TArray<FString> Problems = Musee::Matter::ComposePaintLayer(Paint, Master, *Aside);
			FInputHasher Inputs;
			Inputs.Add(TEXT("paint-layer")).Add(PaintLayerRevision).Add(Master.GetPathName()).Add(Aside->ParameterStateId.ToString());
			Aside->MarkAsGarbage();
			if (!Problems.IsEmpty())
			{
				return Problems;
			}
			const FString Hash = Inputs.Finish();
			if (StoredInputHash(Package, FPackageName::GetShortName(Package)) == Hash)
			{
				Report.Packages.Add({Package, EMadeOutcome::Unchanged, FString()});
				return {};
			}
			UMaterialInstanceConstant* Instance = FindOrCreateMadeAsset<UMaterialInstanceConstant>(Package);
			Instance->ClearParameterValuesEditorOnly();
			Problems = Musee::Matter::ComposePaintLayer(Paint, Master, *Instance);
			if (Problems.IsEmpty())
			{
				Save(*Instance, Hash, Report);
			}
			return Problems;
		}

		void Refuse(const FMuseeStageContext& Stage, const FArtPaintLayer& Layer, const FString& Why, FMakeReport& Report)
		{
			Report.Findings.Add(MakeFinding(TEXT("Matter.Resolved"), FName(*Stage.Stage.ToString()),
				FString::Printf(TEXT("the paint of work %s can't be made: %s"), *Layer.Work, *Why), Layer.Palette.ToString()));
		}
	}

	void MakePaintLayers(const FMuseeStageContext& Stage, const FStageMakeOptions& Options, FMakeReport& Report)
	{
		const TArray<FArtPaintLayer> Layers = Musee::ArtMaking::PaintLayers(Stage);
		for (const FArtPaintLayer& Layer : Layers)
		{
			// Matter names the master a layer is composed over by its finish: the paint master (which the options may
			// place elsewhere) for a canvas, the stained-glass master for a panel of stained glass.
			const TValueOrError<FString, FString> Package = Musee::Matter::LayerMasterPackage(MatterFinish(Layer));
			if (Package.HasError())
			{
				Refuse(Stage, Layer, Package.GetError(), Report);
				continue;
			}
			const FString MasterPackage = Package.GetValue() == Musee::Matter::PaintMasterPackage() ? Options.PaintMaster : Package.GetValue();
			UMaterialInterface* Master = FindPaintMaster(MasterPackage);
			// What the image is made from: the photograph's bytes, or a painted vessel's views and record (Art says which).
			const FString Image = Musee::ArtMaking::LayerImageIdentity(Layer);
			if (!Master || Image.IsEmpty())
			{
				const FString Source = Layer.ImagePath.IsEmpty() ? FString::Printf(TEXT("the views of %s"), *Layer.Work) : Layer.ImagePath;
				Refuse(Stage, Layer, Master ? FString::Printf(TEXT("its image (%s) can't be read"), *Source)
					: FString::Printf(TEXT("its master %s has not been made (MuseeMatterMake)"), *MasterPackage), Report);
				continue;
			}
			const FString Texture = Options.Paths.WorkImage(Musee::Matter::PascalCase(Layer.Work));
			FInputHasher ImageInputs;
			ImageInputs.Add(TEXT("work-image")).Add(WorkImageRevision).Add(Image);
			UTexture2D* Made = MakeImage(Layer, Texture, ImageInputs.Finish(), Report);
			if (!Made)
			{
				continue;
			}
			const FString Instance = Options.Paths.MatterInstance(Musee::Matter::InstanceName(Layer.Palette, Layer.Working, Layer.History));
			const TArray<FString> Problems = MakeInstance(MatterLayer(Layer, *Made), Instance, *Master, Report);
			if (!Problems.IsEmpty())
			{
				Refuse(Stage, Layer, FString::Join(Problems, TEXT(" ")), Report);
			}
		}
	}
}
