#include "Stages/CastScans.h"

#include "AssetCompilingManager.h"
#include "Engine/StaticMesh.h"
#include "InterchangeGenericAnimationPipeline.h"
#include "InterchangeGenericAssetsPipeline.h"
#include "InterchangeGenericAssetsPipelineSharedSettings.h"
#include "InterchangeGenericMaterialPipeline.h"
#include "InterchangeGenericMeshPipeline.h"
#include "InterchangeGenericTexturePipeline.h"
#include "InterchangeManager.h"
#include "InterchangeSourceData.h"
#include "Make/InputHash.h"
#include "Make/PackageSave.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "Stages/StageMake.h"
#include "UObject/Package.h"

namespace Musee::Studio
{
	namespace
	{
		/** The revision of these rules: raise it whenever a scan would be imported differently from the same file. */
		constexpr int64 CastScanRevision = 1;

		/** The hash of a scan file's bytes, or empty when it can't be read. */
		FString FileHash(const FString& Path)
		{
			TArray<uint8> Bytes;
			return FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent) ? FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString() : FString();
		}

		/**
		 * The import's rules: the scan's one mesh as a static mesh named AssetName, its points and normals as the file
		 * gives them; nothing else it might carry (materials, textures, animation, grooms), no Nanite and no collision.
		 */
		UInterchangeGenericAssetsPipeline* ScanPipeline(const FString& AssetName)
		{
			UInterchangeGenericAssetsPipeline* Pipeline = NewObject<UInterchangeGenericAssetsPipeline>(GetTransientPackage());
			Pipeline->bUseSourceNameForAsset = false;
			Pipeline->AssetName = AssetName;
			if (UInterchangeGenericCommonMeshesProperties* Common = Pipeline->CommonMeshesProperties)
			{
				Common->ForceAllMeshAsType = EInterchangeForceMeshType::IFMT_StaticMesh;
				Common->bImportLods = false;
				Common->bRecomputeNormals = false;
				Common->bRecomputeTangents = false;
				Common->bRemoveDegenerates = false;
			}
			if (UInterchangeGenericMeshPipeline* Meshes = Pipeline->MeshPipeline)
			{
				Meshes->bImportStaticMeshes = true;
				Meshes->CombineStaticMeshesBehavior = EInterchangeCombineStaticMeshesBehavior::All;
				Meshes->bImportSkeletalMeshes = false;
				Meshes->bImportGeometryCaches = false;
				Meshes->bBuildNanite = false;
				Meshes->bCollision = false;
			}
			if (UInterchangeGenericMaterialPipeline* Materials = Pipeline->MaterialPipeline)
			{
				Materials->bImportMaterials = false;
				if (UInterchangeGenericTexturePipeline* Textures = Materials->TexturePipeline)
				{
					Textures->bImportTextures = false;
				}
			}
			if (UInterchangeGenericAnimationPipeline* Animation = Pipeline->AnimationPipeline)
			{
				Animation->bImportAnimations = false;
			}
			return Pipeline;
		}

		/**
		 * The static mesh an import made, moved to Package if the import named it otherwise, or null. The import names a
		 * lone mesh by the pipeline's asset name; moving it is for an engine that names it by the file's node instead.
		 */
		UStaticMesh* ImportedMesh(const TArray<UObject*>& Imported, const FString& Package)
		{
			UObject* const* Found = Imported.FindByPredicate([](const UObject* Object) { return Object && Object->IsA<UStaticMesh>(); });
			UStaticMesh* Mesh = Found ? Cast<UStaticMesh>(*Found) : nullptr;
			if (Mesh && Mesh->GetPackage()->GetName() != Package)
			{
				UPackage* Target = CreateMadePackage(Package);
				const FString AssetName = FPackageName::GetShortName(Package);
				if (UObject* InTheWay = StaticFindObjectFast(nullptr, Target, FName(*AssetName)))
				{
					InTheWay->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
				}
				Mesh->Rename(*AssetName, Target, REN_DontCreateRedirectors | REN_NonTransactional);
			}
			return Mesh;
		}
	}

	void ImportScan(const FArtCastScan& Scan, const FString& Package, FMakeReport& Report)
	{
		const FString Bytes = FileHash(Scan.MeshPath);
		if (Bytes.IsEmpty())
		{
			Report.Packages.Add({Package, EMadeOutcome::Failed, FString::Printf(TEXT("its scan %s can't be read"), *Scan.MeshPath)});
			return;
		}
		FInputHasher Inputs;
		const FString Hash = Inputs.Add(TEXT("cast-scan")).Add(CastScanRevision).Add(Bytes).Finish();
		const FString AssetName = FPackageName::GetShortName(Package);
		if (StoredInputHash(Package, AssetName) == Hash)
		{
			Report.Packages.Add({Package, EMadeOutcome::Unchanged, FString()});
			return;
		}
		UInterchangeManager& Manager = UInterchangeManager::GetInterchangeManager();
		const UInterchangeSourceData* Source = UInterchangeManager::CreateSourceData(Scan.MeshPath);
		if (!Manager.CanTranslateSourceData(Source))
		{
			Report.Packages.Add({Package, EMadeOutcome::Failed, FString::Printf(TEXT("the engine's glTF import can't read %s"), *Scan.MeshPath)});
			return;
		}
		FImportAssetParameters Parameters;
		Parameters.bIsAutomated = true;
		Parameters.bReplaceExisting = true;
		Parameters.DestinationName = AssetName;
		Parameters.OverridePipelines.Add(FSoftObjectPath(ScanPipeline(AssetName)));
		TArray<UObject*> Imported;
		const bool bImported = Manager.ImportAsset(FPackageName::GetLongPackagePath(Package), Source, Parameters, Imported);
		FAssetCompilingManager::Get().FinishAllCompilation();
		UStaticMesh* Mesh = bImported ? ImportedMesh(Imported, Package) : nullptr;
		if (!Mesh)
		{
			Report.Packages.Add({Package, EMadeOutcome::Failed, FString::Printf(TEXT("the engine's glTF import made no static mesh of %s"), *Scan.MeshPath)});
			return;
		}
		Mesh->SetLightingGuid(FGuid::NewDeterministicGuid(Hash));
		FMadePackage& Made = Report.Packages.AddDefaulted_GetRef();
		Made.PackageName = Package;
		StoreInputHash(*Mesh, Hash);
		if (SaveMadePackage(*Mesh, Made.Why))
		{
			Made.Outcome = EMadeOutcome::Written;
			return;
		}
		StoreInputHash(*Mesh, FString());
	}

	void MakeCastScans(const FMuseeStageContext& Stage, const FStageMakeOptions& Options, FMakeReport& Report)
	{
		for (const FArtCastScan& Scan : Musee::ArtMaking::CastScans(Stage))
		{
			ImportScan(Scan, Musee::ArtMaking::ScanPackage(Options.Paths.Root, Scan.Scan), Report);
		}
	}
}
