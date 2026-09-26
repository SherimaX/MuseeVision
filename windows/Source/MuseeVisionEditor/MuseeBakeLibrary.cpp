#include "MuseeBakeLibrary.h"

#include "Geometry/MuseeBake.h"

#include "AssetCompilingManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "DistanceFieldAtlas.h"
#include "Editor.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshCardBuild.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProceduralMeshComponent.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogMuseeBake, Log, All);

namespace MuseeBakeImpl
{
	const TCHAR* DefaultFolder = TEXT("/Game/Museum/Baked");
	/** On a baked procedural component: how it was (visible, collision), to undo the bake. */
	const TCHAR* WasPrefix = TEXT("musee.bake.was:");
	/** On a baked static mesh component: the procedural component it replaces. */
	const TCHAR* FromPrefix = TEXT("musee.bakedfrom:");

	enum class EPart : uint8 { Opaque, Translucent, Collision };

	const TCHAR* PartSuffix(EPart Part)
	{
		switch (Part)
		{
		case EPart::Translucent: return TEXT("_Translucent");
		case EPart::Collision: return TEXT("_Collision");
		default: return TEXT("");
		}
	}

	struct FSectionRef
	{
		int32 Index = 0;
		UMaterialInterface* Material = nullptr;
		bool bCollision = false;
	};

	/** One static mesh to make: some sections of one procedural component. */
	struct FJob
	{
		AActor* Actor = nullptr;
		UProceduralMeshComponent* Proc = nullptr;
		EPart Part = EPart::Opaque;
		TArray<FSectionRef> Sections;
		UStaticMesh* Mesh = nullptr;
		int32 Triangles = 0;
		bool bNanite = false;
	};

	struct FActorPlan
	{
		AActor* Actor = nullptr;
		TArray<UProceduralMeshComponent*> Procs;
		TArray<FString> Skipped;
	};

	UClass* ScriptClass(const TCHAR* Name)
	{
		return FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/MuseeVision.%s"), Name));
	}

	bool IsPlant(const AActor* Actor)
	{
		static UClass* Nature = ScriptClass(TEXT("MuseeNatureActor"));
		return Nature && Actor->IsA(Nature);
	}

	UWorld* EditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}

	FString CleanFolder(const FString& Folder)
	{
		FString Out = Folder.IsEmpty() ? FString(DefaultFolder) : Folder;
		Out.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (Out.EndsWith(TEXT("/"))) { Out.LeftChopInline(1); }
		if (!Out.StartsWith(TEXT("/"))) { Out = TEXT("/") + Out; }
		return Out;
	}

	/** A package-safe name: ASCII letters, digits, '_' and '-'. */
	FString SafeName(const FString& In)
	{
		FString Out;
		for (const TCHAR C : In)
		{
			const bool bKeep = (C < 128) && (FChar::IsAlnum(C) || C == TEXT('_') || C == TEXT('-'));
			if (bKeep) { Out.AppendChar(C); }
			else if (!Out.EndsWith(TEXT("_"))) { Out.AppendChar(TEXT('_')); }
		}
		while (Out.StartsWith(TEXT("_"))) { Out.RightChopInline(1); }
		while (Out.EndsWith(TEXT("_"))) { Out.LeftChopInline(1); }
		return Out.IsEmpty() ? FString(TEXT("Actor")) : Out;
	}

	/** A material made at run time or kept in the map (a dynamic instance): a static mesh asset can't hold it. */
	bool IsRuntimeMaterial(const UMaterialInterface* Material)
	{
		return Material && (Material->IsA<UMaterialInstanceDynamic>() || !Material->IsAsset());
	}

	/** Not for Nanite: translucent materials, and water (Nanite has no Single Layer Water): the non-Nanite part. */
	bool IsTranslucent(const UMaterialInterface* Material)
	{
		return Material && (IsTranslucentBlendMode(*Material) ||
							Material->GetShadingModels().HasShadingModel(MSM_SingleLayerWater));
	}

	int32 SectionTriangles(UProceduralMeshComponent* Proc, int32 Index)
	{
		const FProcMeshSection* Section = Proc->GetProcMeshSection(Index);
		return Section ? Section->ProcIndexBuffer.Num() / 3 : 0;
	}

	FString WhyNotActor(AActor* Actor)
	{
		if (!IsValid(Actor)) { return TEXT("no actor"); }
		if (Actor->IsTemplate()) { return TEXT("a template"); }
		if (Actor->ActorHasTag(MuseeBake::NoBakeTag())) { return TEXT("tagged musee.nobake"); }
		// Vouched for by the instance or by its class (a class that tags itself in its constructor: the map's scripts
		// rewrite the instance's tags, and the Chinese Wing stayed procedural, outside Lumen's surface cache).
		const AActor* Defaults = Actor->GetClass()->GetDefaultObject<AActor>();
		const bool bVouched = Actor->ActorHasTag(MuseeBake::BakeableTag()) || (Defaults && Defaults->ActorHasTag(MuseeBake::BakeableTag()));
		if (Actor->PrimaryActorTick.bCanEverTick && !bVouched) { return TEXT("it ticks (it may move or rebuild its parts; tag it musee.bakeable if not)"); }
		for (const TCHAR* Interface : {TEXT("MuseeInteractable"), TEXT("MuseePromptProvider")})
		{
			if (UClass* Class = ScriptClass(Interface))
			{
				if (!bVouched && Actor->GetClass()->ImplementsInterface(Class)) { return FString::Printf(TEXT("the visitor uses it (%s)"), Interface); }
			}
		}
		if (const FBoolProperty* Follow = FindFProperty<FBoolProperty>(Actor->GetClass(), TEXT("bFollowToday")))
		{
			if (Follow->GetPropertyValue_InContainer(Actor)) { return TEXT("it follows today's season (bFollowToday)"); }
		}
		return FString();
	}

	FString WhyNotComponent(AActor* Actor, UProceduralMeshComponent* Proc)
	{
		if (!IsValid(Proc) || Proc->GetOwner() != Actor) { return TEXT("gone"); }
		if (Proc->GetClass() != UProceduralMeshComponent::StaticClass()) { return FString::Printf(TEXT("a %s"), *Proc->GetClass()->GetName()); }
		if (Proc->ComponentHasTag(MuseeBake::NoBakeTag())) { return TEXT("tagged musee.nobake"); }
		if (!Proc->IsRegistered()) { return TEXT("not registered"); }
		if (Proc->IsUsingAbsoluteLocation() || Proc->IsUsingAbsoluteRotation() || Proc->IsUsingAbsoluteScale())
		{
			return TEXT("placed in absolute coordinates (it follows something)");
		}
		// Up to the actor's root (the actor itself may hang from another, e.g. a frame from its painting,
		// and moves with it: the baked components do too).
		const USceneComponent* Root = Actor->GetRootComponent();
		for (const USceneComponent* Parent = Proc->GetAttachParent(); Parent && Parent != Root; Parent = Parent->GetAttachParent())
		{
			if (Parent->GetOwner() != Actor) { return TEXT("attached to another actor's component"); }
			const bool bPlain = Parent->GetClass() == USceneComponent::StaticClass() || Parent->IsA<UProceduralMeshComponent>()
				|| Parent == Actor->GetRootComponent();
			if (!bPlain) { return FString::Printf(TEXT("under a %s, which may move it"), *Parent->GetClass()->GetName()); }
		}
		int32 Triangles = 0;
		for (int32 i = 0; i < Proc->GetNumSections(); ++i)
		{
			if (SectionTriangles(Proc, i) == 0) { continue; }
			Triangles += SectionTriangles(Proc, i);
			if (IsRuntimeMaterial(Proc->GetMaterial(i)))
			{
				return FString::Printf(TEXT("section %d's material is made at run time (%s)"), i, *Proc->GetMaterial(i)->GetName());
			}
		}
		if (Triangles == 0) { return TEXT("empty"); }
		return FString();
	}

	/** Build the procedural geometry afresh: the construction, then Regrow (plants) or Rebuild (frames) if the class has them. */
	void Refresh(AActor* Actor)
	{
		Actor->RerunConstructionScripts();
		for (const TCHAR* Name : {TEXT("Regrow"), TEXT("Rebuild")})
		{
			UFunction* Function = Actor->FindFunction(FName(Name));
			if (Function && Function->NumParms == 0) { Actor->ProcessEvent(Function, nullptr); }
		}
	}

	bool HasProcs(const AActor* Actor)
	{
		TInlineComponentArray<UProceduralMeshComponent*> Procs(Actor);
		return Procs.Num() > 0;
	}

	bool CanMakeProcs(const AActor* Actor)
	{
		return HasProcs(Actor) || Actor->FindFunction(TEXT("Regrow")) || Actor->FindFunction(TEXT("Rebuild"));
	}

	bool Unbake(AActor* Actor, bool bRefresh)
	{
		if (!IsValid(Actor)) { return false; }
		bool bWasBaked = Actor->Tags.Contains(MuseeBake::BakedTag());
		Actor->Modify();
		TArray<UActorComponent*> Components = Actor->GetComponents().Array();
		for (UActorComponent* Component : Components)
		{
			if (!Component) { continue; }
			if (Component->ComponentTags.Contains(MuseeBake::BakedTag()))
			{
				bWasBaked = true;
				Component->Modify();
				Actor->RemoveInstanceComponent(Component);
				Component->DestroyComponent();
				// Out of the actor's way, so the next bake can use the name again.
				Component->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
				continue;
			}
			if (UProceduralMeshComponent* Proc = Cast<UProceduralMeshComponent>(Component))
			{
				const int32 Found = Proc->ComponentTags.IndexOfByPredicate([](const FName& Tag) { return Tag.ToString().StartsWith(WasPrefix); });
				if (Found == INDEX_NONE) { continue; }
				bWasBaked = true;
				FString State = Proc->ComponentTags[Found].ToString().RightChop(FCString::Strlen(WasPrefix));
				FString Visible, Collision;
				State.Split(TEXT(","), &Visible, &Collision);
				Proc->Modify();
				Proc->ComponentTags.RemoveAt(Found);
				Proc->SetVisibility(Visible != TEXT("0"));
				Proc->SetCollisionEnabled(static_cast<ECollisionEnabled::Type>(FCString::Atoi(*Collision)));
			}
		}
		Actor->Tags.Remove(MuseeBake::BakedTag());
		if (bWasBaked && bRefresh) { Refresh(Actor); }
		return bWasBaked;
	}

	/** The component the baked one hangs from: the procedural component's parent (the first one saved with the map). */
	USceneComponent* PersistentParent(UProceduralMeshComponent* Proc)
	{
		USceneComponent* Parent = Proc->GetAttachParent();
		while (Parent && Parent->HasAnyFlags(RF_Transient) && Parent->GetAttachParent()) { Parent = Parent->GetAttachParent(); }
		return Parent ? Parent : Proc->GetOwner()->GetRootComponent();
	}

	/** The sections as a mesh description: one polygon group (material slot) per section, positions welded. */
	FMeshDescription Describe(UProceduralMeshComponent* Proc, const TArray<FSectionRef>& Sections, FBox& OutBounds, int32& OutTriangles)
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

		// The UV channels in use (plants use up to four; the rooms one).
		int32 Channels = 1;
		int32 Vertices = 0, Indices = 0;
		for (const FSectionRef& Ref : Sections)
		{
			const FProcMeshSection* Section = Proc->GetProcMeshSection(Ref.Index);
			Vertices += Section->ProcVertexBuffer.Num();
			Indices += Section->ProcIndexBuffer.Num();
			for (const FProcMeshVertex& V : Section->ProcVertexBuffer)
			{
				if (!V.UV3.IsNearlyZero()) { Channels = FMath::Max(Channels, 4); }
				else if (!V.UV2.IsNearlyZero()) { Channels = FMath::Max(Channels, 3); }
				else if (!V.UV1.IsNearlyZero()) { Channels = FMath::Max(Channels, 2); }
			}
		}
		UVs.SetNumChannels(Channels);
		Description.ReserveNewVertices(Vertices);
		Description.ReserveNewVertexInstances(Vertices);
		Description.ReserveNewTriangles(Indices / 3);
		Description.ReserveNewPolygons(Indices / 3);
		Description.ReserveNewEdges(Indices);

		OutBounds = FBox(ForceInit);
		OutTriangles = 0;
		TMap<FVector3f, FVertexID> Welded;
		Welded.Reserve(Vertices);
		for (const FSectionRef& Ref : Sections)
		{
			const FProcMeshSection* Section = Proc->GetProcMeshSection(Ref.Index);
			const FPolygonGroupID Group = Description.CreatePolygonGroup();
			SlotNames[Group] = FName(*FString::Printf(TEXT("Section%d"), Ref.Index));

			const int32 Count = Section->ProcVertexBuffer.Num();
			TArray<FVertexInstanceID> Instances;
			TArray<FVertexID> VertexOf;
			Instances.SetNumUninitialized(Count);
			VertexOf.SetNumUninitialized(Count);
			for (int32 v = 0; v < Count; ++v)
			{
				const FProcMeshVertex& V = Section->ProcVertexBuffer[v];
				const FVector3f P(V.Position);
				FVertexID* Existing = Welded.Find(P);
				FVertexID Vertex;
				if (Existing) { Vertex = *Existing; }
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
				Colors[Instance] = FVector4f(FLinearColor(V.Color));   // back to the same FColor when built (sRGB both ways)
				UVs.Set(Instance, 0, FVector2f(V.UV0));
				if (Channels > 1) { UVs.Set(Instance, 1, FVector2f(V.UV1)); }
				if (Channels > 2) { UVs.Set(Instance, 2, FVector2f(V.UV2)); }
				if (Channels > 3) { UVs.Set(Instance, 3, FVector2f(V.UV3)); }
			}
			const TArray<uint32>& Index = Section->ProcIndexBuffer;
			for (int32 t = 0; t + 2 < Index.Num(); t += 3)
			{
				const uint32 A = Index[t], B = Index[t + 1], C = Index[t + 2];
				if (A >= uint32(Count) || B >= uint32(Count) || C >= uint32(Count)) { continue; }
				// A triangle whose corners weld together has no area: skipped.
				if (VertexOf[A] == VertexOf[B] || VertexOf[B] == VertexOf[C] || VertexOf[A] == VertexOf[C]) { continue; }
				const FVertexInstanceID Corners[3] = {Instances[A], Instances[B], Instances[C]};
				Description.CreateTriangle(Group, Corners);
				++OutTriangles;
			}
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
		if (UObject* Other = StaticFindObjectFast(nullptr, Package, FName(*AssetName)))
		{
			Other->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		}
		Mesh = NewObject<UStaticMesh>(Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional);
		Mesh->SetLightingGuid();
		FAssetRegistryModule::AssetCreated(Mesh);
		return Mesh;
	}

	/** Fill the mesh asset from the job's sections (built later, with the others). */
	void Prepare(FJob& Job, const FString& PackageName, const FString& AssetName)
	{
		FBox Bounds;
		FMeshDescription Description = Describe(Job.Proc, Job.Sections, Bounds, Job.Triangles);
		const bool bPlant = IsPlant(Job.Actor);
		bool bTwoSided = false;
		for (const FSectionRef& Ref : Job.Sections) { bTwoSided |= Ref.Material && Ref.Material->IsTwoSided(); }

		UStaticMesh* Mesh = FindOrMakeMesh(PackageName, AssetName);
		Job.Mesh = Mesh;
		Mesh->SetNumSourceModels(1);
		FStaticMeshSourceModel& Source = Mesh->GetSourceModel(0);
		Source.BuildSettings = FMeshBuildSettings();
		FMeshBuildSettings& Build = Source.BuildSettings;
		// The components' own normals and tangents; full-precision UVs (the rooms' UVs run to tens of metres).
		Build.bRecomputeNormals = false;
		Build.bRecomputeTangents = false;
		Build.bRemoveDegenerates = false;
		Build.bUseMikkTSpace = true;
		Build.bUseHighPrecisionTangentBasis = !bPlant;
		Build.bUseFullPrecisionUVs = true;
		Build.bGenerateLightmapUVs = false;   // Lumen: no baked lighting
		Build.SrcLightmapIndex = 0;
		Build.DstLightmapIndex = 0;
		const bool bDrawn = Job.Part != EPart::Collision;
		const bool bSurface = Job.Part == EPart::Opaque;
		Build.DistanceFieldResolutionScale = bSurface ? 1.f : 0.f;
		Build.bGenerateDistanceFieldAsIfTwoSided = bTwoSided;
		// Lumen's surface cards: a whole room in one mesh needs more than the default 12 to cover it.
		Build.MaxLumenMeshCards = !bSurface ? 0 : (Bounds.IsValid && Bounds.GetSize().GetMax() > 1000.0 ? 32 : 12);

		Mesh->CreateMeshDescription(0, MoveTemp(Description));
		Mesh->CommitMeshDescription(0);

		TArray<FStaticMaterial>& Materials = Mesh->GetStaticMaterials();
		Materials.Reset();
		Mesh->GetSectionInfoMap().Clear();
		for (int32 k = 0; k < Job.Sections.Num(); ++k)
		{
			const FName Slot(*FString::Printf(TEXT("Section%d"), Job.Sections[k].Index));
			Materials.Add(FStaticMaterial(Job.Sections[k].Material, Slot, Slot));
			FMeshSectionInfo Info(k);
			Info.bEnableCollision = Job.Sections[k].bCollision;
			Info.bCastShadow = bDrawn;
			Mesh->GetSectionInfoMap().Set(0, k, Info);
		}
		Mesh->GetOriginalSectionInfoMap().CopyFrom(Mesh->GetSectionInfoMap());

		// Nanite for the opaque sections. The fallback mesh (ray tracing, collision) keeps every triangle
		// of the rooms, so the walk and the reflections see the geometry as drawn; plants get the engine's.
		FMeshNaniteSettings Nanite;
		Job.bNanite = bSurface;
		Nanite.bEnabled = Job.bNanite;
		Nanite.bExplicitTangents = true;
		Nanite.GenerateFallback = ENaniteGenerateFallback::Enabled;
		if (bPlant)
		{
			Nanite.FallbackTarget = ENaniteFallbackTarget::Auto;
			Nanite.ShapePreservation = ENaniteShapePreservation::PreserveArea;
		}
		else
		{
			Nanite.FallbackTarget = ENaniteFallbackTarget::PercentTriangles;
			Nanite.FallbackPercentTriangles = 1.f;
		}
		Mesh->SetNaniteSettings(Nanite);
		Mesh->SetImportVersion(EImportStaticMeshVersion::LastVersion);

		// Collision: the mesh itself (complex as simple), as the procedural components; or their convex hulls.
		Mesh->CreateBodySetup();
		UBodySetup* Body = Mesh->GetBodySetup();
		Body->Modify();
		Body->RemoveSimpleCollision();
		Body->bDoubleSidedGeometry = true;
		Body->bGenerateMirroredCollision = false;
		UBodySetup* ProcBody = Job.Proc->GetBodySetup();
		if (!Job.Proc->bUseComplexAsSimpleCollision && ProcBody && ProcBody->AggGeom.ConvexElems.Num() > 0 && Job.Part == EPart::Opaque)
		{
			Body->AggGeom.ConvexElems = ProcBody->AggGeom.ConvexElems;
			Body->CollisionTraceFlag = CTF_UseDefault;
		}
		else
		{
			Body->CollisionTraceFlag = CTF_UseComplexAsSimple;
		}
		Mesh->MarkPackageDirty();
	}

	/** The render settings the static mesh component takes from the procedural one (by name, so any that exist). */
	void CopyRenderSettings(const UPrimitiveComponent* From, UPrimitiveComponent* To)
	{
		static const TCHAR* const Names[] = {
			TEXT("CastShadow"), TEXT("bCastDynamicShadow"), TEXT("bCastStaticShadow"), TEXT("bCastHiddenShadow"), TEXT("bCastFarShadow"),
			TEXT("bCastContactShadow"), TEXT("bSelfShadowOnly"), TEXT("bCastVolumetricTranslucentShadow"), TEXT("bCastInsetShadow"),
			TEXT("bAffectDynamicIndirectLighting"), TEXT("bAffectDistanceFieldLighting"), TEXT("bAffectIndirectLightingWhileHidden"),
			TEXT("bVisibleInReflectionCaptures"), TEXT("bVisibleInRealTimeSkyCaptures"), TEXT("bVisibleInRayTracing"),
			TEXT("bRenderInMainPass"), TEXT("bRenderInDepthPass"), TEXT("bReceivesDecals"), TEXT("bRenderCustomDepth"),
			TEXT("CustomDepthStencilValue"), TEXT("CustomDepthStencilWriteMask"), TEXT("TranslucencySortPriority"),
			TEXT("TranslucencySortDistanceOffset"), TEXT("LightingChannels"), TEXT("BoundsScale"), TEXT("bHiddenInGame"),
			TEXT("bUseAsOccluder"), TEXT("bHoldout"), TEXT("ShadowCacheInvalidationBehavior"), TEXT("bVisibleInSceneCaptureOnly"),
			TEXT("bHiddenInSceneCapture"), TEXT("bEmissiveLightSource"), TEXT("CanCharacterStepUpOn"), TEXT("bCanEverAffectNavigation"),
		};
		for (const TCHAR* Name : Names)
		{
			const FProperty* Property = FindFProperty<FProperty>(UPrimitiveComponent::StaticClass(), FName(Name));
			if (!Property) { Property = FindFProperty<FProperty>(USceneComponent::StaticClass(), FName(Name)); }
			if (!Property) { Property = FindFProperty<FProperty>(UActorComponent::StaticClass(), FName(Name)); }
			if (Property) { Property->CopyCompleteValue_InContainer(To, From); }
		}
	}

	void SetBool(UObject* Target, const TCHAR* Name, bool bValue)
	{
		if (const FBoolProperty* Property = FindFProperty<FBoolProperty>(Target->GetClass(), FName(Name)))
		{
			Property->SetPropertyValue_InContainer(Target, bValue);
		}
	}

	UStaticMeshComponent* MakeComponent(const FJob& Job)
	{
		AActor* Actor = Job.Actor;
		UProceduralMeshComponent* Proc = Job.Proc;
		FName Name(*(Proc->GetName() + TEXT("_Baked") + PartSuffix(Job.Part)));
		if (StaticFindObjectFast(nullptr, Actor, Name)) { Name = MakeUniqueObjectName(Actor, UStaticMeshComponent::StaticClass(), Name); }
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor, Name, RF_Transactional);
		Mesh->CreationMethod = EComponentCreationMethod::Instance;
		Mesh->SetMobility(Proc->GetMobility());
		USceneComponent* Parent = PersistentParent(Proc);
		Mesh->SetupAttachment(Parent);
		Mesh->SetRelativeTransform(Parent ? Proc->GetComponentTransform().GetRelativeTransform(Parent->GetComponentTransform())
										  : Proc->GetComponentTransform());
		Mesh->SetStaticMesh(Job.Mesh);
		CopyRenderSettings(Proc, Mesh);

		// Collision: the procedural component's, on the sections that had it (the mesh's own section flags).
		const bool bCollides = Job.Sections.ContainsByPredicate([](const FSectionRef& S) { return S.bCollision; });
		Mesh->SetCollisionObjectType(Proc->GetCollisionObjectType());
		Mesh->SetCollisionResponseToChannels(Proc->GetCollisionResponseToChannels());
		Mesh->SetCollisionEnabled(bCollides ? Proc->GetCollisionEnabled() : ECollisionEnabled::NoCollision);
		Mesh->SetGenerateOverlapEvents(Proc->GetGenerateOverlapEvents());
		if (!bCollides) { Mesh->SetCanEverAffectNavigation(false); }

		switch (Job.Part)
		{
		case EPart::Translucent:
			SetBool(Mesh, TEXT("bAffectDistanceFieldLighting"), false);
			break;
		case EPart::Collision:
			// Hit boxes and thresholds: collision only, never drawn.
			Mesh->SetVisibility(false);
			Mesh->SetHiddenInGame(true);
			Mesh->SetCastShadow(false);
			SetBool(Mesh, TEXT("bAffectDistanceFieldLighting"), false);
			SetBool(Mesh, TEXT("bVisibleInRayTracing"), false);
			break;
		default:
			break;
		}
		Mesh->ComponentTags.Add(MuseeBake::BakedTag());
		Mesh->ComponentTags.Add(FName(*(FString(FromPrefix) + Proc->GetName())));
		Actor->AddInstanceComponent(Mesh);
		Mesh->RegisterComponent();
		return Mesh;
	}

	/** Empty, hide and turn off the procedural component, noting how it was. */
	void Retire(UProceduralMeshComponent* Proc)
	{
		Proc->Modify();
		Proc->ComponentTags.RemoveAll([](const FName& Tag) { return Tag.ToString().StartsWith(WasPrefix); });
		Proc->ComponentTags.Add(FName(*FString::Printf(TEXT("%s%d,%d"), WasPrefix, Proc->GetVisibleFlag() ? 1 : 0, int32(Proc->GetCollisionEnabled()))));
		Proc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Proc->SetVisibility(false);
		Proc->ClearAllMeshSections();
	}

	/** Nanite draws a material only if it is marked for Nanite (the editor marks it itself, but a game can't). */
	void MarkForNanite(const TArray<FJob>& Jobs, TSet<UPackage*>& ToSave)
	{
		TSet<UMaterial*> Seen;
		for (const FJob& Job : Jobs)
		{
			if (!Job.bNanite) { continue; }
			for (const FSectionRef& Ref : Job.Sections)
			{
				UMaterial* Base = Ref.Material ? Ref.Material->GetMaterial() : nullptr;
				if (!Base || Seen.Contains(Base)) { continue; }
				Seen.Add(Base);
				if (Base->GetUsageByFlag(MATUSAGE_Nanite)) { continue; }
				if (Base->SetMaterialUsage(MATUSAGE_Nanite))
				{
					Base->MarkPackageDirty();
					if (Base->GetPackage()->GetName().StartsWith(TEXT("/Game/")))
					{
						ToSave.Add(Base->GetPackage());
						UE_LOG(LogMuseeBake, Log, TEXT("Bake: %s marked for Nanite (saved)."), *Base->GetPathName());
					}
				}
			}
		}
	}

	bool Save(UPackage* Package)
	{
		UObject* Asset = Package->FindAssetInPackage();
		// Let go of the file it was loaded from (a loader can hold it open), so it can be replaced.
		Package->FullyLoad();
		ResetLoaders(Package);
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		const bool bSaved = UPackage::SavePackage(Package, Asset, *File, Args);
		if (!bSaved) { UE_LOG(LogMuseeBake, Error, TEXT("Bake: could not save %s."), *Package->GetName()); }
		return bSaved;
	}

	int32 BakeActors(const TArray<AActor*>& Actors, const FString& InFolder)
	{
		const FString Folder = CleanFolder(InFolder);
		const double Start = FPlatformTime::Seconds();
		TArray<FJob> Jobs;
		TArray<FActorPlan> Plans;
		TSet<FString> UsedFolders;
		int32 SkippedActors = 0;

		// 1. Fresh procedural geometry (a baked actor is unbaked first), and the meshes to make.
		for (AActor* Actor : Actors)
		{
			if (!IsValid(Actor)) { continue; }
			const FString Why = WhyNotActor(Actor);
			if (!Why.IsEmpty())
			{
				if (HasProcs(Actor)) { UE_LOG(LogMuseeBake, Log, TEXT("Bake: %s left procedural: %s."), *Actor->GetActorNameOrLabel(), *Why); }
				++SkippedActors;
				continue;
			}
			if (!Unbake(Actor, true)) { Refresh(Actor); }

			FActorPlan& Plan = Plans.AddDefaulted_GetRef();
			Plan.Actor = Actor;
			FString Sub = SafeName(Actor->GetActorLabel());
			if (UsedFolders.Contains(Sub)) { Sub += TEXT("_") + SafeName(Actor->GetName()); }
			UsedFolders.Add(Sub);

			TInlineComponentArray<UProceduralMeshComponent*> Procs(Actor);
			for (UProceduralMeshComponent* Proc : Procs)
			{
				const FString Skip = WhyNotComponent(Actor, Proc);
				if (!Skip.IsEmpty())
				{
					if (Skip != TEXT("empty")) { Plan.Skipped.Add(FString::Printf(TEXT("%s (%s)"), *Proc->GetName(), *Skip)); }
					continue;
				}
				const bool bCollides = Proc->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
				const bool bShown = Proc->GetVisibleFlag();
				TArray<FSectionRef> Parts[3];
				for (int32 i = 0; i < Proc->GetNumSections(); ++i)
				{
					const FProcMeshSection* Section = Proc->GetProcMeshSection(i);
					if (!Section || SectionTriangles(Proc, i) == 0) { continue; }
					FSectionRef Ref;
					Ref.Index = i;
					Ref.Material = Proc->GetMaterial(i);
					Ref.bCollision = bCollides && Section->bEnableCollision;
					if (bShown && Section->bSectionVisible)
					{
						Parts[int32(IsTranslucent(Ref.Material) ? EPart::Translucent : EPart::Opaque)].Add(Ref);
					}
					else if (Ref.bCollision)
					{
						Parts[int32(EPart::Collision)].Add(Ref);
					}
				}
				bool bAny = false;
				for (int32 p = 0; p < 3; ++p)
				{
					if (Parts[p].Num() == 0) { continue; }
					FJob& Job = Jobs.AddDefaulted_GetRef();
					Job.Actor = Actor;
					Job.Proc = Proc;
					Job.Part = EPart(p);
					Job.Sections = MoveTemp(Parts[p]);
					const FString AssetName = SafeName(Proc->GetName()) + PartSuffix(Job.Part);
					Prepare(Job, Folder / Sub / AssetName, AssetName);
					bAny = true;
				}
				if (bAny) { Plan.Procs.Add(Proc); }
			}
		}

		// 2. Build the meshes (Nanite, distance fields, Lumen cards) a batch at a time, then their collision. One
		// batch of all ~270 ran every build at once on every core and peaked at 100 GB of memory; 24 at a time
		// keeps the builds parallel and the peak a fraction of that.
		TArray<UStaticMesh*> Meshes;
		for (const FJob& Job : Jobs) { Meshes.AddUnique(Job.Mesh); }
		TSet<UPackage*> ToSave;
		MarkForNanite(Jobs, ToSave);
		constexpr int32 MeshesPerBatch = 24;
		for (int32 First = 0; First < Meshes.Num(); First += MeshesPerBatch)
		{
			TArray<UStaticMesh*> Batch(Meshes.GetData() + First, FMath::Min(MeshesPerBatch, Meshes.Num() - First));
			UStaticMesh::FBuildParameters Build;
			Build.bInSilent = true;
			UStaticMesh::BatchBuild(Batch, Build);
			FAssetCompilingManager::Get().FinishAllCompilation();
			if (GDistanceFieldAsyncQueue) { GDistanceFieldAsyncQueue->BlockUntilAllBuildsComplete(); }
			if (GCardRepresentationAsyncQueue) { GCardRepresentationAsyncQueue->BlockUntilAllBuildsComplete(); }
		}
		if (Meshes.Num() > 0)
		{
			for (UStaticMesh* Mesh : Meshes)
			{
				if (UBodySetup* Body = Mesh->GetBodySetup())
				{
					Body->InvalidatePhysicsData();
					Body->CreatePhysicsMeshes();
				}
				ToSave.Add(Mesh->GetPackage());
			}
		}

		// 3. The static mesh components in place of the procedural ones.
		int32 Made = 0;
		for (const FJob& Job : Jobs)
		{
			MakeComponent(Job);
			++Made;
		}
		int32 BakedActors = 0;
		for (const FActorPlan& Plan : Plans)
		{
			for (UProceduralMeshComponent* Proc : Plan.Procs) { Retire(Proc); }
			if (Plan.Procs.Num() > 0)
			{
				Plan.Actor->Modify();
				Plan.Actor->Tags.AddUnique(MuseeBake::BakedTag());
				++BakedActors;
			}
			int32 Triangles = 0, Parts = 0;
			for (const FJob& Job : Jobs)
			{
				if (Job.Actor == Plan.Actor) { Triangles += Job.Triangles; ++Parts; }
			}
			if (Plan.Procs.Num() > 0 || Plan.Skipped.Num() > 0)
			{
				UE_LOG(LogMuseeBake, Log, TEXT("Bake: %s: %d components -> %d meshes, %d triangles%s%s."), *Plan.Actor->GetActorNameOrLabel(),
					Plan.Procs.Num(), Parts, Triangles, Plan.Skipped.Num() ? TEXT("; left procedural: ") : TEXT(""),
					*FString::Join(Plan.Skipped, TEXT(", ")));
			}
		}

		// 4. Save the meshes (and any material newly marked for Nanite). The map is the caller's to save.
		int32 Saved = 0;
		for (UPackage* Package : ToSave) { Saved += Save(Package) ? 1 : 0; }
		UE_LOG(LogMuseeBake, Log, TEXT("Bake: %d actors baked (%d left procedural), %d static mesh components, %d packages saved to %s, in %.0f s."),
			BakedActors, SkippedActors, Made, Saved, *Folder, FPlatformTime::Seconds() - Start);
		return Made;
	}
}

int32 UMuseeBakeLibrary::BakeActor(AActor* Actor, const FString& Folder)
{
	return MuseeBakeImpl::BakeActors({Actor}, Folder);
}

int32 UMuseeBakeLibrary::BakeActors(const TArray<AActor*>& Actors, const FString& Folder)
{
	return MuseeBakeImpl::BakeActors(Actors, Folder);
}

int32 UMuseeBakeLibrary::BakeAll(const FString& Folder)
{
	UWorld* World = MuseeBakeImpl::EditorWorld();
	if (!World)
	{
		UE_LOG(LogMuseeBake, Error, TEXT("Bake: no editor world."));
		return 0;
	}
	TArray<AActor*> Actors;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (MuseeBake::IsBaked(*It) || MuseeBakeImpl::CanMakeProcs(*It)) { Actors.Add(*It); }
	}
	return MuseeBakeImpl::BakeActors(Actors, Folder);
}

bool UMuseeBakeLibrary::UnbakeActor(AActor* Actor)
{
	return MuseeBakeImpl::Unbake(Actor, true);
}

int32 UMuseeBakeLibrary::UnbakeAll()
{
	UWorld* World = MuseeBakeImpl::EditorWorld();
	int32 Count = 0;
	if (!World) { return 0; }
	TArray<AActor*> Baked;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (MuseeBake::IsBaked(*It)) { Baked.Add(*It); }
	}
	for (AActor* Actor : Baked) { Count += MuseeBakeImpl::Unbake(Actor, true) ? 1 : 0; }
	UE_LOG(LogMuseeBake, Log, TEXT("Bake: %d actors unbaked."), Count);
	return Count;
}

bool UMuseeBakeLibrary::IsBaked(const AActor* Actor)
{
	return MuseeBake::IsBaked(Actor);
}

FString UMuseeBakeLibrary::WhyNotBakeable(AActor* Actor)
{
	return MuseeBakeImpl::WhyNotActor(Actor);
}
