#include "MuseeJourneyLibrary.h"

#include "Cube/JourneyGen.h"

#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Async/ParallelFor.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogMuseeJourney, Log, All);

namespace MuseeJourneyBuild
{
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
		if (!bSaved) { UE_LOG(LogMuseeJourney, Error, TEXT("Journeys: could not save %s."), *Package->GetName()); }
		return bSaved;
	}

	void MarkUsages(UMaterialInterface* Material, TSet<UPackage*>& ToSave, bool bInstanced, bool bNanite = true)
	{
		UMaterial* Base = Material ? Material->GetMaterial() : nullptr;
		if (!Base) { return; }
		bool bChanged = false;
		for (const EMaterialUsage Usage : {MATUSAGE_Nanite, MATUSAGE_InstancedStaticMeshes})
		{
			if (Usage == MATUSAGE_InstancedStaticMeshes && !bInstanced) { continue; }
			if (Usage == MATUSAGE_Nanite && !bNanite) { continue; }   // (Élan Cube: only for the Nanite things)
			if (!Base->GetUsageByFlag(Usage)) { bChanged |= Base->SetMaterialUsage(Usage); }
		}
		if (bChanged)
		{
			Base->MarkPackageDirty();
			ToSave.Add(Base->GetPackage());
		}
	}

	/** A mesh description of one or more sections (one polygon group each). */
	FMeshDescription Describe(const TArray<const CubeMesh::FMesh*>& Sections, const TArray<FName>& Slots)
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
		UVs.SetNumChannels(2);
		for (int32 s = 0; s < Sections.Num(); ++s)
		{
			const CubeMesh::FMesh& M = *Sections[s];
			const FPolygonGroupID Group = Description.CreatePolygonGroup();
			SlotNames[Group] = Slots[s];
			TArray<FVertexInstanceID> Instances;
			Instances.SetNumUninitialized(M.Positions.Num());
			for (int32 v = 0; v < M.Positions.Num(); ++v)
			{
				const FVertexID Vertex = Description.CreateVertex();
				Positions[Vertex] = FVector3f(M.Positions[v]);
				const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
				Instances[v] = Instance;
				Normals[Instance] = FVector3f(M.Normals[v]);
				Tangents[Instance] = FVector3f(M.Tangents[v].TangentX);
				Signs[Instance] = M.Tangents[v].bFlipTangentY ? -1.f : 1.f;
				Colors[Instance] = FVector4f(M.Colours[v]);
				UVs.Set(Instance, 0, FVector2f(M.UV0[v]));
				UVs.Set(Instance, 1, FVector2f(M.UV1[v]));
			}
			for (int32 t = 0; t + 2 < M.Indices.Num(); t += 3)
			{
				const FVertexInstanceID Corners[3] = {Instances[M.Indices[t]], Instances[M.Indices[t + 1]], Instances[M.Indices[t + 2]]};
				Description.CreateTriangle(Group, Corners);
			}
		}
		return Description;
	}

	UStaticMesh* MakeMesh(const FString& Folder, const FString& Name, const TArray<const CubeMesh::FMesh*>& Sections, const TArray<UMaterialInterface*>& Materials,
						  bool bNanite, bool bCastShadow, bool bCollision, bool bPreserveArea, float FallbackPercent)
	{
		UStaticMesh* Mesh = FindOrMakeMesh(Folder / Name, Name);
		Mesh->SetNumSourceModels(1);
		FStaticMeshSourceModel& Source = Mesh->GetSourceModel(0);
		Source.BuildSettings = FMeshBuildSettings();
		FMeshBuildSettings& Build = Source.BuildSettings;
		Build.bRecomputeNormals = false;
		Build.bRecomputeTangents = false;
		Build.bRemoveDegenerates = true;
		Build.bUseMikkTSpace = true;
		Build.bUseHighPrecisionTangentBasis = true;
		Build.bUseFullPrecisionUVs = true;
		Build.bGenerateLightmapUVs = false;
		Build.DistanceFieldResolutionScale = 0.f;
		TArray<FName> Slots;
		for (int32 s = 0; s < Sections.Num(); ++s) { Slots.Add(FName(*FString::Printf(TEXT("Section%d"), s))); }
		Mesh->CreateMeshDescription(0, Describe(Sections, Slots));
		Mesh->CommitMeshDescription(0);
		TArray<FStaticMaterial>& Static = Mesh->GetStaticMaterials();
		Static.Reset();
		Mesh->GetSectionInfoMap().Clear();
		for (int32 s = 0; s < Sections.Num(); ++s)
		{
			Static.Add(FStaticMaterial(Materials.IsValidIndex(s) ? Materials[s] : nullptr, Slots[s], Slots[s]));
			FMeshSectionInfo Info(s);
			Info.bEnableCollision = bCollision;
			Info.bCastShadow = bCastShadow;
			Mesh->GetSectionInfoMap().Set(0, s, Info);
		}
		Mesh->GetOriginalSectionInfoMap().CopyFrom(Mesh->GetSectionInfoMap());
		FMeshNaniteSettings Nanite;
		Nanite.bEnabled = bNanite;
		Nanite.bExplicitTangents = true;
		Nanite.ShapePreservation = bPreserveArea ? ENaniteShapePreservation::PreserveArea : ENaniteShapePreservation::None;
		Nanite.GenerateFallback = ENaniteGenerateFallback::Enabled;
		Nanite.FallbackTarget = ENaniteFallbackTarget::PercentTriangles;
		Nanite.FallbackPercentTriangles = FallbackPercent;
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
		ToSave.Reset();
		return Saved;
	}

	/** One tile of terrain as a mesh: the grid from its file, the hole cut out (far tiles). */
	bool TileMesh(const FString& File, const TSharedPtr<FJsonObject>& Tile, const TArray<double>& Block, CubeMesh::FMesh& M)
	{
		TArray<uint8> Data;
		if (!FFileHelper::LoadFileToArray(Data, *File) || Data.Num() < 32 || FMemory::Memcmp(Data.GetData(), "MHT1", 4) != 0) { return false; }
		int32 NX = 0, NY = 0;
		double West = 0, North = 0;
		float S = 0;
		FMemory::Memcpy(&NX, Data.GetData() + 4, 4);
		FMemory::Memcpy(&NY, Data.GetData() + 8, 4);
		FMemory::Memcpy(&West, Data.GetData() + 12, 8);
		FMemory::Memcpy(&North, Data.GetData() + 20, 8);
		FMemory::Memcpy(&S, Data.GetData() + 28, 4);
		const float* H = reinterpret_cast<const float*>(Data.GetData() + 32);
		if (Data.Num() < 32 + NX * NY * 4) { return false; }
		// The mesh's origin: the tile's north-west corner (the game places it from its name, SM_<tile>__<west>_<north>).
		const double CE = West, CN = North;
		// The tile's place in its orthophoto block (UV1), and the hole (far tiles).
		FVector2D Offset(0, 0);
		if (Block.Num() == 3) { Offset = FVector2D((West - Block[0]) / Block[2], (Block[1] - North) / Block[2]); }
		const TArray<TSharedPtr<FJsonValue>>* HoleArr = nullptr;
		double HE0 = 1e12, HN0 = 1e12, HE1 = -1e12, HN1 = -1e12;
		if (Tile->TryGetArrayField(TEXT("hole"), HoleArr) && HoleArr->Num() == 4)
		{
			HE0 = (*HoleArr)[0]->AsNumber(); HN0 = (*HoleArr)[1]->AsNumber(); HE1 = (*HoleArr)[2]->AsNumber(); HN1 = (*HoleArr)[3]->AsNumber();
		}
		auto Z = [&](int32 i, int32 j) { return double(H[FMath::Clamp(j, 0, NY - 1) * NX + FMath::Clamp(i, 0, NX - 1)]); };
		M.Reserve(NX * NY);
		M.Positions.SetNumUninitialized(NX * NY);
		M.Normals.SetNumUninitialized(NX * NY);
		M.Tangents.SetNumUninitialized(NX * NY);
		M.UV0.SetNumUninitialized(NX * NY);
		M.UV1.SetNumUninitialized(NX * NY);
		M.Colours.SetNumUninitialized(NX * NY);
		ParallelFor(NY, [&](int32 j)
		{
			for (int32 i = 0; i < NX; ++i)
			{
				const int32 k = j * NX + i;
				const double E = West + S * i, N = North - S * j;
				M.Positions[k] = FVector((E - CE) * 100.0, -(N - CN) * 100.0, Z(i, j) * 100.0);
				// Heights' slopes (one-sided at the edges): dz/dE, and dz/dN (rows run south).
				const double dE = (Z(i + 1, j) - Z(i - 1, j)) / (S * ((i > 0 && i < NX - 1) ? 2.0 : 1.0));
				const double dN = (Z(i, j - 1) - Z(i, j + 1)) / (S * ((j > 0 && j < NY - 1) ? 2.0 : 1.0));
				// Unreal's Y runs south: the normal is (−dz/dE, +dz/dN, 1).
				M.Normals[k] = FVector(-dE, dN, 1.0).GetSafeNormal();
				M.Tangents[k] = FProcMeshTangent(FVector(1.0, 0.0, dE).GetSafeNormal(), false);
				M.UV0[k] = FVector2D(E - West, North - N);
				M.UV1[k] = Offset;
				M.Colours[k] = FLinearColor::White;
			}
		});
		M.Indices.Reserve((NX - 1) * (NY - 1) * 6);
		for (int32 j = 0; j + 1 < NY; ++j)
		{
			for (int32 i = 0; i + 1 < NX; ++i)
			{
				const double E_ = West + S * (i + 0.5), N_ = North - S * (j + 0.5);
				if (E_ > HE0 && E_ < HE1 && N_ > HN0 && N_ < HN1) { continue; }
				const int32 A = j * NX + i, B = A + 1, C = A + NX, D = C + 1;
				// Split along the shorter diagonal of the heights (less sliver shading on ridges).
				const double D1 = FMath::Abs(Z(i, j) - Z(i + 1, j + 1)), D2 = FMath::Abs(Z(i + 1, j) - Z(i, j + 1));
				// Front faces (seen from above): in Unreal's frame (y south) a–b–d runs clockwise seen from +z.
				if (D1 < D2) { M.Indices.Append({A, D, B, A, C, D}); }
				else { M.Indices.Append({A, C, B, B, C, D}); }
			}
		}
		return true;
	}
}

int32 UMuseeJourneyLibrary::BuildTerrain(const FString& TerrainJson, const FString& Folder, const FString& MaterialFolder, const FString& Only)
{
	using namespace MuseeJourneyBuild;
	const double Start = FPlatformTime::Seconds();
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *TerrainJson)) { UE_LOG(LogMuseeJourney, Error, TEXT("Terrain: %s not found."), *TerrainJson); return 0; }
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root) { return 0; }
	const FString Dir = FPaths::GetPath(TerrainJson) / TEXT("tiles");
	TSet<FString> OnlySet;
	if (!Only.IsEmpty()) { TArray<FString> L; Only.ParseIntoArray(L, TEXT(",")); OnlySet.Append(L); }

	TMap<FString, TArray<double>> Blocks;
	for (const TSharedPtr<FJsonValue>& V : Root->GetArrayField(TEXT("blocks")))
	{
		const TSharedPtr<FJsonObject> B = V->AsObject();
		Blocks.Add(B->GetStringField(TEXT("name")), {B->GetNumberField(TEXT("west")), B->GetNumberField(TEXT("north")), B->GetNumberField(TEXT("size"))});
	}
	TSet<FString> Hires;
	for (const TSharedPtr<FJsonValue>& V : Root->GetArrayField(TEXT("hires"))) { Hires.Add(V->AsObject()->GetStringField(TEXT("name"))); }

	TSet<UPackage*> ToSave;
	TArray<UStaticMesh*> Batch;
	TArray<TUniquePtr<CubeMesh::FMesh>> Keep;
	int32 Saved = 0, Count = 0;
	int64 Triangles = 0;
	for (const TSharedPtr<FJsonValue>& V : Root->GetArrayField(TEXT("tiles")))
	{
		const TSharedPtr<FJsonObject> Tile = V->AsObject();
		const FString Name = Tile->GetStringField(TEXT("name"));
		if (OnlySet.Num() && !OnlySet.Contains(Name)) { continue; }
		const FString Kind = Tile->GetStringField(TEXT("kind"));
		FString Block;
		Tile->TryGetStringField(TEXT("block"), Block);
		// The material: the tile's high-resolution image if it has one, its block's, a patch's own, or the far land's.
		FString Mat = TEXT("MI_MH_Far");
		if (Kind == TEXT("patch")) { Mat = TEXT("MI_MH_") + Name; }
		else if (Kind == TEXT("near"))
		{
			TArray<FString> P;
			Name.ParseIntoArray(P, TEXT("_"));   // near_E_N
			const FString H = P.Num() == 3 ? FString::Printf(TEXT("T_MH_Hires_%s_%s"), *P[1], *P[2]) : FString();
			Mat = Hires.Contains(H) ? TEXT("MI_MH_Hires_") + P[1] + TEXT("_") + P[2] : Block.Replace(TEXT("T_MH_"), TEXT("MI_MH_"));
		}
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *(MaterialFolder / Mat + TEXT(".") + Mat), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Material) { UE_LOG(LogMuseeJourney, Warning, TEXT("Terrain: %s: material %s missing."), *Name, *Mat); }
		MarkUsages(Material, ToSave, false);
		TUniquePtr<CubeMesh::FMesh> M = MakeUnique<CubeMesh::FMesh>();
		const TArray<double>* B = Blocks.Find(Block);
		if (!TileMesh(Dir / Name + TEXT(".bin"), Tile, B ? *B : TArray<double>(), *M))
		{
			UE_LOG(LogMuseeJourney, Warning, TEXT("Terrain: %s unreadable."), *Name);
			continue;
		}
		Triangles += M->Indices.Num() / 3;
		const FString Asset = FString::Printf(TEXT("SM_%s__%lld_%lld"), *Name, int64(FMath::RoundToDouble(Tile->GetNumberField(TEXT("west")))),
											   int64(FMath::RoundToDouble(Tile->GetNumberField(TEXT("north")))));
		Batch.Add(MakeMesh(Folder, Asset, {M.Get()}, {Material}, true, true, false, false, 2.f));
		Keep.Add(MoveTemp(M));
		++Count;
		if (Batch.Num() >= 16)
		{
			Saved += BuildAndSave(Batch, ToSave);
			Batch.Reset();
			Keep.Reset();
			UE_LOG(LogMuseeJourney, Log, TEXT("Terrain: %d tiles built (%.0f s)."), Count, FPlatformTime::Seconds() - Start);
		}
	}
	if (Batch.Num()) { Saved += BuildAndSave(Batch, ToSave); }
	UE_LOG(LogMuseeJourney, Log, TEXT("Terrain: %d tiles (%lld triangles) built, %d packages saved, in %.0f s."), Count, Triangles, Saved, FPlatformTime::Seconds() - Start);
	return Saved;
}

int32 UMuseeJourneyLibrary::BuildThings(const FString& Set, const FString& Folder, const FString& MaterialFolder)
{
	using namespace MuseeJourneyBuild;
	const double Start = FPlatformTime::Seconds();
	TArray<FJourneyThing> Things;
	JourneyGen::Build(Set, Things);
	TSet<UPackage*> ToSave;
	TArray<UStaticMesh*> Batch;
	int32 Saved = 0;
	for (int32 t = 0; t < Things.Num(); ++t)
	{
		const FJourneyThing& T = Things[t];
		TArray<const CubeMesh::FMesh*> Sections;
		TArray<UMaterialInterface*> Materials;
		auto Load = [&](const FString& Name) -> UMaterialInterface*
		{
			UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, *(MaterialFolder / Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!M) { UE_LOG(LogMuseeJourney, Warning, TEXT("Things: %s: material %s missing."), *T.Name, *Name); }
			MarkUsages(M, ToSave, true, T.bNanite);
			return M;
		};
		if (T.Sections.Num())
		{
			for (int32 s = 0; s < T.Sections.Num(); ++s)
			{
				Sections.Add(&T.Sections[s]);
				Materials.Add(Load(T.Materials.IsValidIndex(s) ? T.Materials[s] : T.Material));
			}
		}
		else
		{
			Sections.Add(&T.Mesh);
			Materials.Add(Load(T.Material));
		}
		Batch.Add(MakeMesh(Folder, T.Name, Sections, Materials, T.bNanite, T.bCastShadow, T.bCollision, false, 5.f));
		if (Batch.Num() >= 16 || t + 1 == Things.Num())
		{
			Saved += BuildAndSave(Batch, ToSave);
			Batch.Reset();
		}
	}
	UE_LOG(LogMuseeJourney, Log, TEXT("Things %s: %d meshes, %d packages saved, in %.0f s."), *Set, Things.Num(), Saved, FPlatformTime::Seconds() - Start);
	return Saved;
}
