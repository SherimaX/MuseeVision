#include "Live/MuseeLiveBake.h"
#include "MuseeVision.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "DistanceFieldAtlas.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

namespace MuseeLiveBake
{
	namespace
	{
		const FName LiveBakedTag(TEXT("musee.live.baked"));      // the static mesh components a live bake made
		const FName LiveHiddenTag(TEXT("musee.live.hidden"));    // the procedural components it hid

#if WITH_EDITOR
		/** The component's sections as one mesh description, a polygon group per section (as MuseeBakeLibrary). */
		FMeshDescription Describe(UProceduralMeshComponent* Proc, TArray<int32>& OutSections, FBox& OutBounds)
		{
			FMeshDescription Description;
			FStaticMeshAttributes Attributes(Description);
			Attributes.Register();
			TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
			TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
			TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
			TVertexInstanceAttributesRef<float> Signs = Attributes.GetVertexInstanceBinormalSigns();
			TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
			TPolygonGroupAttributesRef<FName> SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
			UVs.SetNumChannels(1);
			OutBounds = FBox(ForceInit);
			for (int32 s = 0; s < Proc->GetNumSections(); ++s)
			{
				const FProcMeshSection* Section = Proc->GetProcMeshSection(s);
				if (!Section || !Section->bSectionVisible || Section->ProcIndexBuffer.Num() < 3) { continue; }
				OutSections.Add(s);
				const FPolygonGroupID Group = Description.CreatePolygonGroup();
				SlotNames[Group] = FName(*FString::Printf(TEXT("Section%d"), s));
				const int32 Count = Section->ProcVertexBuffer.Num();
				TArray<FVertexInstanceID> Instances;
				TArray<FVertexID> VertexOf;
				Instances.SetNumUninitialized(Count);
				VertexOf.SetNumUninitialized(Count);
				TMap<FVector3f, FVertexID> Welded;
				for (int32 v = 0; v < Count; ++v)
				{
					const FProcMeshVertex& V = Section->ProcVertexBuffer[v];
					const FVector3f P(V.Position);
					FVertexID Vertex;
					if (const FVertexID* Existing = Welded.Find(P)) { Vertex = *Existing; }
					else
					{
						Vertex = Description.CreateVertex();
						Positions[Vertex] = P;
						Welded.Add(P, Vertex);
						OutBounds += V.Position;
					}
					VertexOf[v] = Vertex;
					const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertex);
					Instances[v] = Instance;
					Normals[Instance] = FVector3f(V.Normal);
					Tangents[Instance] = FVector3f(V.Tangent.TangentX);
					Signs[Instance] = V.Tangent.bFlipTangentY ? -1.f : 1.f;
					UVs.Set(Instance, 0, FVector2f(V.UV0));
				}
				const TArray<uint32>& Index = Section->ProcIndexBuffer;
				for (int32 t = 0; t + 2 < Index.Num(); t += 3)
				{
					const uint32 A = Index[t], B = Index[t + 1], C = Index[t + 2];
					if (A >= uint32(Count) || B >= uint32(Count) || C >= uint32(Count)) { continue; }
					if (VertexOf[A] == VertexOf[B] || VertexOf[B] == VertexOf[C] || VertexOf[A] == VertexOf[C]) { continue; }
					const FVertexInstanceID Corners[3] = {Instances[A], Instances[B], Instances[C]};
					Description.CreateTriangle(Group, Corners);
				}
			}
			return Description;
		}
#endif
	}

	int32 Bake(AActor* Actor)
	{
#if WITH_EDITOR
		if (!Actor) { return 0; }
		Unbake(Actor);
		TArray<UProceduralMeshComponent*> Procs;
		Actor->GetComponents(Procs);
		struct FMade { UProceduralMeshComponent* Proc; UStaticMesh* Mesh; TArray<int32> Sections; };
		TArray<FMade> Made;
		for (UProceduralMeshComponent* Proc : Procs)
		{
			if (!Proc->IsVisible() || Proc->GetNumSections() == 0) { continue; }
			TArray<int32> Sections;
			FBox Bounds;
			FMeshDescription Description = Describe(Proc, Sections, Bounds);
			if (Sections.IsEmpty()) { continue; }
			bool bTranslucent = false;
			for (const int32 s : Sections)
			{
				const UMaterialInterface* M = Proc->GetMaterial(s);
				bTranslucent |= M && IsTranslucentBlendMode(M->GetBlendMode());
			}
			UStaticMesh* Mesh = NewObject<UStaticMesh>(GetTransientPackage(), NAME_None, RF_Transient);
			Mesh->SetNumSourceModels(1);
			FMeshBuildSettings& Build = Mesh->GetSourceModel(0).BuildSettings;
			Build.bRecomputeNormals = false;
			Build.bRecomputeTangents = false;
			Build.bRemoveDegenerates = false;
			Build.bUseMikkTSpace = true;
			Build.bUseHighPrecisionTangentBasis = true;
			Build.bUseFullPrecisionUVs = true;
			Build.bGenerateLightmapUVs = false;
			Build.DistanceFieldResolutionScale = bTranslucent ? 0.f : 1.f;
			Build.MaxLumenMeshCards = bTranslucent ? 0 : (Bounds.IsValid && Bounds.GetSize().GetMax() > 1000.0 ? 32 : 12);
			Mesh->CreateMeshDescription(0, MoveTemp(Description));
			Mesh->CommitMeshDescription(0);
			TArray<FStaticMaterial>& Materials = Mesh->GetStaticMaterials();
			for (int32 k = 0; k < Sections.Num(); ++k)
			{
				const FName Slot(*FString::Printf(TEXT("Section%d"), Sections[k]));
				Materials.Add(FStaticMaterial(Proc->GetMaterial(Sections[k]), Slot, Slot));
				FMeshSectionInfo Info(k);
				Info.bEnableCollision = false;
				Info.bCastShadow = true;
				Mesh->GetSectionInfoMap().Set(0, k, Info);
			}
			Mesh->Build(true);
			Made.Add({Proc, Mesh, Sections});
		}
		// every mesh's derived data (render data, distance fields, cards) before the components draw them
		FAssetCompilingManager::Get().FinishAllCompilation();
		if (GDistanceFieldAsyncQueue) { GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete(); }
		for (const FMade& M : Made)
		{
			UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Actor, NAME_None, RF_Transient);
			C->ComponentTags.Add(LiveBakedTag);
			C->SetMobility(EComponentMobility::Movable);
			C->SetupAttachment(M.Proc->GetAttachParent());
			C->SetRelativeTransform(M.Proc->GetRelativeTransform());
			C->SetStaticMesh(M.Mesh);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(M.Proc->CastShadow);
			for (int32 k = 0; k < M.Sections.Num(); ++k) { C->SetMaterial(k, M.Proc->GetMaterial(M.Sections[k])); }
			C->RegisterComponent();
			Actor->AddInstanceComponent(C);
			// the procedural component stays for the walk's collision, not drawn
			M.Proc->SetVisibility(false);
			M.Proc->ComponentTags.AddUnique(LiveHiddenTag);
		}
		return Made.Num();
#else
		return 0;
#endif
	}

	void Unbake(AActor* Actor)
	{
		if (!Actor) { return; }
		TArray<UActorComponent*> Components = Actor->GetComponents().Array();
		for (UActorComponent* C : Components)
		{
			if (!C) { continue; }
			if (C->ComponentTags.Contains(LiveBakedTag))
			{
				Actor->RemoveInstanceComponent(C);
				C->DestroyComponent();
			}
			else if (C->ComponentTags.Contains(LiveHiddenTag))
			{
				C->ComponentTags.Remove(LiveHiddenTag);
				if (USceneComponent* S = Cast<USceneComponent>(C)) { S->SetVisibility(true); }
			}
		}
	}
}
