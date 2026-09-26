#include "MuseeLawnLibrary.h"

#include "Nature/MuseeHedge.h"
#include "Nature/MuseeLawn.h"

#include "AssetCompilingManager.h"
#include "Async/ParallelFor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogMuseeLawn, Log, All);

namespace MuseeLawnBuild
{
	FMeshDescription Describe(const FMuseeLawnPatch& Patch, const FName Slot)
	{
		FMeshDescription Description;
		FStaticMeshAttributes Attributes(Description);
		Attributes.Register();
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> Signs = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector4f> Colors = Attributes.GetVertexInstanceColors();
		TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
		TPolygonGroupAttributesRef<FName> SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
		UVs.SetNumChannels(1);
		const int32 Count = Patch.Positions.Num();
		Description.ReserveNewVertices(Count);
		Description.ReserveNewVertexInstances(Count);
		Description.ReserveNewTriangles(Patch.NumTriangles());
		Description.ReserveNewPolygons(Patch.NumTriangles());
		Description.ReserveNewEdges(Patch.Indices.Num());
		const FPolygonGroupID Group = Description.CreatePolygonGroup();
		SlotNames[Group] = Slot;
		TArray<FVertexInstanceID> Instances;
		Instances.SetNumUninitialized(Count);
		for (int32 v = 0; v < Count; ++v)
		{
			const FVertexID Vertex = Description.CreateVertex();
			Positions[Vertex] = Patch.Positions[v];
			const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
			Instances[v] = Instance;
			Normals[Instance] = Patch.Normals[v];
			Tangents[Instance] = Patch.Tangents[v];
			Signs[Instance] = 1.f;
			// Linear here, stored as sRGB bytes when built: the material decodes them (the Nature materials' convention).
			Colors[Instance] = FVector4f(Patch.Colours[v]);
			UVs.Set(Instance, 0, Patch.UVs[v]);
		}
		for (int32 t = 0; t + 2 < Patch.Indices.Num(); t += 3)
		{
			const FVertexInstanceID Corners[3] = {Instances[Patch.Indices[t]], Instances[Patch.Indices[t + 1]], Instances[Patch.Indices[t + 2]]};
			Description.CreateTriangle(Group, Corners);
		}
		return Description;
	}

	UStaticMesh* FindOrMakeMesh(const FString& PackageName, const FString& AssetName)
	{
		const FString ObjectPath = PackageName + TEXT(".") + AssetName;
		UStaticMesh* Mesh = FindObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!Mesh && FPackageName::DoesPackageExist(PackageName))
		{
			Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		}
		if (Mesh)
		{
			Mesh->Modify();
			return Mesh;
		}
		UPackage* Package = CreatePackage(*PackageName);
		Package->FullyLoad();
		Mesh = NewObject<UStaticMesh>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
		Mesh->SetLightingGuid();
		FAssetRegistryModule::AssetCreated(Mesh);
		return Mesh;
	}

	bool Save(UPackage* Package)
	{
		UObject* Asset = Package->FindAssetInPackage();
		Package->FullyLoad();
		ResetLoaders(Package);
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		const bool bSaved = UPackage::SavePackage(Package, Asset, *File, Args);
		if (!bSaved) { UE_LOG(LogMuseeLawn, Error, TEXT("Lawn: could not save %s."), *Package->GetName()); }
		return bSaved;
	}

	/** Nanite, and instanced: usages a cooked game can't add for itself. */
	void MarkUsages(UMaterialInterface* Material, TSet<UPackage*>& ToSave)
	{
		UMaterial* Base = Material ? Material->GetMaterial() : nullptr;
		if (!Base) { return; }
		bool bChanged = false;
		for (const EMaterialUsage Usage : {MATUSAGE_Nanite, MATUSAGE_InstancedStaticMeshes})
		{
			if (!Base->GetUsageByFlag(Usage)) { bChanged |= Base->SetMaterialUsage(Usage); }
		}
		if (bChanged)
		{
			Base->MarkPackageDirty();
			ToSave.Add(Base->GetPackage());
		}
	}

	/**
	 * A Nanite static mesh of the patch (not yet built): area kept as it simplifies with distance (foliage), positions to
	 * 1/64 cm (blades 2–4 mm wide, leaves 1–2 cm), a small fallback; no collision, distance field or Lumen cards.
	 */
	UStaticMesh* MakeMesh(const FMuseeLawnPatch& Patch, const FString& Folder, const FString& Name, UMaterialInterface* Material, const FName Slot,
						  bool bCastShadow)
	{
		UStaticMesh* Mesh = FindOrMakeMesh(Folder / Name, Name);
		Mesh->SetNumSourceModels(1);
		FStaticMeshSourceModel& Source = Mesh->GetSourceModel(0);
		Source.BuildSettings = FMeshBuildSettings();
		FMeshBuildSettings& Build = Source.BuildSettings;
		Build.bRecomputeNormals = false;
		Build.bRecomputeTangents = false;
		Build.bRemoveDegenerates = false;
		Build.bUseMikkTSpace = true;
		Build.bUseHighPrecisionTangentBasis = false;
		Build.bUseFullPrecisionUVs = false;
		Build.bGenerateLightmapUVs = false;
		Build.DistanceFieldResolutionScale = 0.f;
		Build.MaxLumenMeshCards = 0;
		Mesh->CreateMeshDescription(0, Describe(Patch, Slot));
		Mesh->CommitMeshDescription(0);

		TArray<FStaticMaterial>& Materials = Mesh->GetStaticMaterials();
		Materials.Reset();
		Materials.Add(FStaticMaterial(Material, Slot, Slot));
		Mesh->GetSectionInfoMap().Clear();
		FMeshSectionInfo Info(0);
		Info.bEnableCollision = false;
		Info.bCastShadow = bCastShadow;
		Mesh->GetSectionInfoMap().Set(0, 0, Info);
		Mesh->GetOriginalSectionInfoMap().CopyFrom(Mesh->GetSectionInfoMap());

		FMeshNaniteSettings Nanite;
		Nanite.bEnabled = true;
		Nanite.bExplicitTangents = false;
		Nanite.ShapePreservation = ENaniteShapePreservation::PreserveArea;
		Nanite.PositionPrecision = 6;
		Nanite.GenerateFallback = ENaniteGenerateFallback::Enabled;
		Nanite.FallbackTarget = ENaniteFallbackTarget::PercentTriangles;
		Nanite.FallbackPercentTriangles = 0.05f;
		Mesh->SetNaniteSettings(Nanite);
		Mesh->SetImportVersion(EImportStaticMeshVersion::LastVersion);
		Mesh->MarkAsNotHavingNavigationData();
		Mesh->MarkPackageDirty();
		return Mesh;
	}

	int32 BuildAndSave(TArray<UStaticMesh*>& Meshes, TSet<UPackage*>& ToSave)
	{
		UStaticMesh::FBuildParameters Params;
		Params.bInSilent = true;
		UStaticMesh::BatchBuild(Meshes, Params);
		FAssetCompilingManager::Get().FinishAllCompilation();
		for (UStaticMesh* Mesh : Meshes) { ToSave.Add(Mesh->GetPackage()); }
		int32 Saved = 0;
		for (UPackage* Package : ToSave) { Saved += Save(Package) ? 1 : 0; }
		return Saved;
	}}

int32 UMuseeLawnLibrary::BuildLawnMeshes()
{
	using namespace MuseeLawnBuild;
	const double Start = FPlatformTime::Seconds();
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, MuseeLawn::MaterialPath(), nullptr, LOAD_NoWarn);
	if (!Material) { UE_LOG(LogMuseeLawn, Warning, TEXT("Lawn: %s is missing (Scripts/lawn.py makes it first)."), MuseeLawn::MaterialPath()); }
	TSet<UPackage*> ToSave;
	MarkUsages(Material, ToSave);

	TArray<UStaticMesh*> Meshes;
	int64 Triangles = 0;
	for (int32 m = 0; m < MuseeLawn::NumMeshes; ++m)
	{
		int32 Level, Sparse, Variant;
		MuseeLawn::DecodeMeshIndex(m, Level, Sparse, Variant);
		FMuseeLawnPatch Patch;
		MuseeLawn::BuildPatch(Level, Sparse, Variant, Patch);
		const FString Name = MuseeLawn::MeshName(m);
		Meshes.Add(MakeMesh(Patch, MuseeLawn::Folder(), Name, Material, FName(TEXT("Blades")), true));
		Triangles += Patch.NumTriangles();
		UE_LOG(LogMuseeLawn, Log, TEXT("Lawn: %s: %d blades, %d triangles."), *Name, Patch.Blades, Patch.NumTriangles());
	}
	const int32 Saved = BuildAndSave(Meshes, ToSave);
	UE_LOG(LogMuseeLawn, Log, TEXT("Lawn: %d patch meshes (%lld triangles in all) built, %d packages saved, in %.0f s."), Meshes.Num(), Triangles, Saved,
		   FPlatformTime::Seconds() - Start);
	return Saved;
}

int32 UMuseeLawnLibrary::BuildHedgeMeshes()
{
	using namespace MuseeLawnBuild;
	const double Start = FPlatformTime::Seconds();
	UMaterialInterface* Box = LoadObject<UMaterialInterface>(nullptr, MuseeHedge::BoxMaterialPath(), nullptr, LOAD_NoWarn);
	UMaterialInterface* Yew = LoadObject<UMaterialInterface>(nullptr, MuseeHedge::YewMaterialPath(), nullptr, LOAD_NoWarn);
	if (!Box || !Yew) { UE_LOG(LogMuseeLawn, Warning, TEXT("Hedges: MI_Hedge_Box or MI_Hedge_Yew is missing (Scripts/lawn.py hedges makes them first).")); }
	TSet<UPackage*> ToSave;
	MarkUsages(Box, ToSave);
	MarkUsages(Yew, ToSave);

	const int32 Num = MuseeHedge::NumMeshes();
	TArray<FMuseeLawnPatch> Patches;
	Patches.SetNum(Num);
	// The modules are independent: grow them side by side.
	ParallelFor(Num, [&Patches](int32 m) { MuseeHedge::BuildModule(m, Patches[m]); });
	TArray<UStaticMesh*> Meshes;
	int64 Triangles = 0;
	for (int32 m = 0; m < Num; ++m)
	{
		MuseeHedge::EKind Kind;
		int32 Variant;
		MuseeHedge::DecodeMeshIndex(m, Kind, Variant);
		const bool bYew = MuseeHedge::Spec(Kind).bYew;
		const FString Name = MuseeHedge::MeshName(m);
		Meshes.Add(MakeMesh(Patches[m], MuseeHedge::Folder(), Name, bYew ? Yew : Box, FName(bYew ? TEXT("Needles") : TEXT("Leaves")), true));
		Triangles += Patches[m].NumTriangles();
		UE_LOG(LogMuseeLawn, Log, TEXT("Hedges: %s: %d leaves, %d triangles."), *Name, Patches[m].Blades, Patches[m].NumTriangles());
	}
	Patches.Empty();
	const int32 Saved = BuildAndSave(Meshes, ToSave);
	UE_LOG(LogMuseeLawn, Log, TEXT("Hedges: %d module meshes (%lld triangles in all) built, %d packages saved, in %.0f s."), Meshes.Num(), Triangles, Saved,
		   FPlatformTime::Seconds() - Start);
	return Saved;
}