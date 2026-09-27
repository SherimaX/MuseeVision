#include "Cast/Caster.h"

#include "AssetCompilingManager.h"
#include "Cast/CastParts.h"
#include "Cast/CastRules.h"
#include "DistanceFieldAtlas.h"
#include "Engine/StaticMesh.h"
#include "Make/InputHash.h"
#include "Make/MakeMeasures.h"
#include "Make/PackageSave.h"
#include "Materials/MaterialInterface.h"
#include "MeshCardBuild.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"
#include "StudioLog.h"
#include "UObject/Package.h"

namespace Musee::Studio
{
	namespace
	{
		/**
		 * How far a stored unit vector's length may stray from 1. Three floats renormalised keep a unit vector within
		 * about 1e-6 of unit length; 1e-3 only flags a normal or tangent that was never drawn.
		 */
		constexpr float UnitLengthTolerance = 1.0e-3f;

		/** A part on its way to becoming a mesh asset. */
		struct FPendingPart
		{
			FString PackageName;
			FString AssetName;
			FCastPart Part;
			TArray<UMaterialInterface*> Materials;
			bool bCollides = true;
			FString InputHash;
			int64 Triangles = 0;
			UStaticMesh* Mesh = nullptr;
		};

		/** Why a description's vertex frames are not the drawing's unit normals and tangents; empty when they are. */
		FString WhyNotUnitFrames(const FMeshDescription& Description)
		{
			const FStaticMeshConstAttributes Attributes(Description);
			const TVertexInstanceAttributesConstRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
			const TVertexInstanceAttributesConstRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
			const TVertexInstanceAttributesConstRef<float> Signs = Attributes.GetVertexInstanceBinormalSigns();
			if (!Normals.IsValid() || !Tangents.IsValid() || !Signs.IsValid() || Attributes.GetVertexInstanceUVs().GetNumChannels() < 1)
			{
				return TEXT("it has no normals, tangents, binormal signs or UVs");
			}
			for (const FVertexInstanceID Instance : Description.VertexInstances().GetElementIDs())
			{
				const bool bUnitNormal = FMath::IsNearlyEqual(Normals[Instance].Size(), 1.0f, UnitLengthTolerance);
				const bool bUnitTangent = FMath::IsNearlyEqual(Tangents[Instance].Size(), 1.0f, UnitLengthTolerance);
				if (!bUnitNormal || !bUnitTangent || FMath::Abs(Signs[Instance]) != 1.0f)
				{
					return FString::Printf(TEXT("vertex instance %d has no unit normal and tangent (normal %s, tangent %s)"),
						Instance.GetValue(), *Normals[Instance].ToString(), *Tangents[Instance].ToString());
				}
			}
			return FString();
		}

		/** Why an input cannot be cast; empty when it can. */
		FString WhyNotCastable(const FCastInput& Input)
		{
			FText Reason;
			if (!FPackageName::IsValidLongPackageName(Input.PackageName, /*bIncludeReadOnlyRoots*/ false, &Reason))
			{
				return Reason.ToString();
			}
			const FMeshDescription& Description = Input.Description;
			if (Input.Slots.IsEmpty() || Description.PolygonGroups().Num() != Input.Slots.Num())
			{
				return FString::Printf(TEXT("it has %d polygon groups for %d slots"), Description.PolygonGroups().Num(), Input.Slots.Num());
			}
			for (int32 Slot = 0; Slot < Input.Slots.Num(); ++Slot)
			{
				const FPolygonGroupID Group(Slot);
				if (!Description.IsPolygonGroupValid(Group) || Description.GetPolygonGroupTriangles(Group).IsEmpty())
				{
					return FString::Printf(TEXT("slot %d (%s) has no triangles"), Slot, *Input.Slots[Slot].Name.ToString());
				}
			}
			return WhyNotUnitFrames(Description);
		}

		/** Loads each slot's material and reads its kind; false with the reason when a material is missing. */
		bool ResolveMaterials(const FCastInput& Input, TArray<UMaterialInterface*>& OutMaterials, TArray<ECastKind>& OutKinds, FString& OutWhy)
		{
			for (const FCastSlot& Slot : Input.Slots)
			{
				UMaterialInterface* Material = Cast<UMaterialInterface>(Slot.Material.TryLoad());
				if (!Material)
				{
					OutWhy = FString::Printf(TEXT("slot %s has no material at %s"), *Slot.Name.ToString(), *Slot.Material.ToString());
					return false;
				}
				OutMaterials.Add(Material);
				OutKinds.Add(KindOf(*Material));
			}
			return true;
		}

		FString HashOf(FPendingPart& Pending)
		{
			FInputHasher Hasher;
			Hasher.Add(TEXT("cast")).Add(CastRulesRevision).Add(static_cast<int64>(Pending.Part.Kind)).Add(static_cast<int64>(Pending.bCollides));
			for (const FCastSlot& Slot : Pending.Part.Slots)
			{
				Hasher.Add(Slot.Name.ToString()).Add(Slot.Material.ToString());
			}
			return Hasher.Add(Pending.Part.Description).Finish();
		}

		/** Splits each castable input into its parts, with their names, materials and input hashes. */
		void Prepare(TArray<FCastInput>& Inputs, TArray<FPendingPart>& OutParts, TArray<FCastResult>& OutResults, TArray<FMadePackage>& OutMade)
		{
			TSet<FString> Claimed;
			for (FCastInput& Input : Inputs)
			{
				FCastResult& Result = OutResults.AddDefaulted_GetRef();
				TArray<UMaterialInterface*> Materials;
				TArray<ECastKind> Kinds;
				bool bClaimed = false;
				Claimed.Add(Input.PackageName, &bClaimed);
				FString Why = bClaimed ? TEXT("another cast in this make writes the same package") : WhyNotCastable(Input);
				if (!Why.IsEmpty() || !ResolveMaterials(Input, Materials, Kinds, Why))
				{
					OutMade.Add({Input.PackageName, EMadeOutcome::Failed, Why});
					continue;
				}
				for (FCastPart& Part : SplitByKind(Input.Description, Input.Slots, Kinds))
				{
					FPendingPart& Pending = OutParts.AddDefaulted_GetRef();
					Pending.PackageName = Input.PackageName + PartSuffix(Part.Kind);
					Pending.AssetName = FPackageName::GetShortName(Pending.PackageName);
					for (int32 Slot = 0; Slot < Kinds.Num(); ++Slot)
					{
						if (Kinds[Slot] == Part.Kind) { Pending.Materials.Add(Materials[Slot]); }
					}
					Pending.bCollides = Input.bCollides;
					Pending.Triangles = Part.Description.Triangles().Num();
					Pending.Part = MoveTemp(Part);
					Pending.InputHash = HashOf(Pending);
					Result.Meshes.Add({FSoftObjectPath(Pending.PackageName + TEXT(".") + Pending.AssetName), Pending.Part.Kind});
				}
			}
		}

		void SetMaterials(UStaticMesh& Mesh, const FPendingPart& Pending)
		{
			TArray<FStaticMaterial> Materials;
			FMeshSectionInfoMap& Sections = Mesh.GetSectionInfoMap();
			Sections.Clear();
			for (int32 Slot = 0; Slot < Pending.Part.Slots.Num(); ++Slot)
			{
				const FName Name = Pending.Part.Slots[Slot].Name;
				Materials.Emplace(Pending.Materials[Slot], Name, Name);
				FMeshSectionInfo Section(Slot);
				Section.bEnableCollision = Pending.bCollides;
				Section.bCastShadow = true;
				Sections.Set(0, Slot, Section);
			}
			Mesh.SetStaticMaterials(Materials);
			Mesh.GetOriginalSectionInfoMap().CopyFrom(Sections);
		}

		/** Collision is the mesh itself (complex as simple), read from the fallback mesh, which keeps every triangle. */
		void SetCollision(UStaticMesh& Mesh)
		{
			Mesh.CreateBodySetup();
			UBodySetup* Body = Mesh.GetBodySetup();
			Body->RemoveSimpleCollision();
			Body->CollisionTraceFlag = CTF_UseComplexAsSimple;
			// Placements rotate and translate; nothing is mirrored.
			Body->bGenerateMirroredCollision = false;
		}

		/** Commits a part's drawing and settings to its mesh asset, ready to build. */
		void Commit(FPendingPart& Pending)
		{
			UStaticMesh* Mesh = FindOrCreateMadeAsset<UStaticMesh>(Pending.PackageName);
			Pending.Mesh = Mesh;
			const ECastKind Kind = Pending.Part.Kind;
			Mesh->SetNumSourceModels(1);
			FStaticMeshSourceModel& Source = Mesh->GetSourceModel(0);
			Source.BuildSettings = BuildSettingsFor(Kind, LargestExtentMeters(Pending.Part.Description));
			Source.ReductionSettings = FMeshReductionSettings();
			Mesh->CreateMeshDescription(0, MoveTemp(Pending.Part.Description));
			// The description's GUID is its content hash, so the DDC keys of every build follow the drawing alone.
			UStaticMesh::FCommitMeshDescriptionParams ContentKeyed;
			ContentKeyed.bUseHashAsGuid = true;
			Mesh->CommitMeshDescription(0, ContentKeyed);
			SetMaterials(*Mesh, Pending);
			Mesh->SetNaniteSettings(NaniteSettingsFor(Kind));
			Mesh->SetLightingGuid(FGuid::NewDeterministicGuid(Mesh->GetPathName()));
			Mesh->SetImportVersion(EImportStaticMeshVersion::LastVersion);
			SetCollision(*Mesh);
		}

		/** Builds the meshes a batch at a time (render data, Nanite, distance fields, cards and collision), and waits for each. */
		void Build(TConstArrayView<FPendingPart*> Changed, FCastStats& Stats)
		{
			UStaticMesh::FBuildParameters Parameters;
			Parameters.bInSilent = true;
			Parameters.bInRebuildUVChannelData = true;
			for (int32 First = 0; First < Changed.Num(); First += MeshesPerBatch)
			{
				TArray<UStaticMesh*> Batch;
				for (int32 Index = First; Index < FMath::Min(First + MeshesPerBatch, Changed.Num()); ++Index)
				{
					Batch.Add(Changed[Index]->Mesh);
					Stats.TrianglesBuilt += Changed[Index]->Triangles;
				}
				UStaticMesh::BatchBuild(Batch, Parameters);
				FAssetCompilingManager::Get().FinishAllCompilation();
				if (GDistanceFieldAsyncQueue) { GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete(); }
				if (GCardRepresentationAsyncQueue) { GCardRepresentationAsyncQueue->BlockUntilAllBuildsComplete(); }
				Stats.PartsBuilt += Batch.Num();
				++Stats.Batches;
				UE_LOG(LogMuseeStudio, Display, TEXT("Cast: batch %d built %d meshes; peak physical memory %.0f MB."),
					Stats.Batches, Batch.Num(), MeasurePeakPhysMemoryMB());
			}
			for (const FPendingPart* Pending : Changed)
			{
				// The build cooks collision under a fresh random GUID (UBodySetup::InvalidatePhysicsData). Keying it by the
				// part's input hash instead means the same drawing finds the same cooked collision in the DDC everywhere.
				Pending->Mesh->GetBodySetup()->BodySetupGuid = FGuid::NewDeterministicGuid(Pending->InputHash);
			}
			Stats.PeakPhysMemoryMB = MeasurePeakPhysMemoryMB();
		}

		/** Stores each built mesh's input hash and saves it; a mesh that failed keeps no hash, so the next make retries it. */
		void Save(TConstArrayView<FPendingPart*> Changed, TArray<FMadePackage>& OutMade)
		{
			for (const FPendingPart* Pending : Changed)
			{
				FMadePackage& Made = OutMade.AddDefaulted_GetRef();
				Made.PackageName = Pending->PackageName;
				if (!Pending->Mesh->HasValidRenderData())
				{
					Made.Why = TEXT("its build produced no render data");
					continue;
				}
				StoreInputHash(*Pending->Mesh, Pending->InputHash);
				if (SaveMadePackage(*Pending->Mesh, Made.Why))
				{
					Made.Outcome = EMadeOutcome::Written;
				}
				else
				{
					StoreInputHash(*Pending->Mesh, FString());
				}
			}
		}
	}

	TArray<FCastResult> CastMeshes(TArray<FCastInput>& Inputs, TArray<FMadePackage>& OutMade, FCastStats& OutStats)
	{
		TArray<FPendingPart> Parts;
		TArray<FCastResult> Results;
		Prepare(Inputs, Parts, Results, OutMade);

		TArray<FPendingPart*> Changed;
		for (FPendingPart& Pending : Parts)
		{
			if (StoredInputHash(Pending.PackageName, Pending.AssetName) == Pending.InputHash)
			{
				OutMade.Add({Pending.PackageName, EMadeOutcome::Unchanged, FString()});
				continue;
			}
			Changed.Add(&Pending);
		}
		for (FPendingPart* Pending : Changed)
		{
			Commit(*Pending);
		}
		Build(Changed, OutStats);
		Save(Changed, OutMade);
		return Results;
	}
}
