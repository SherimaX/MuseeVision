#include "Albion/AlbionStructure.h"

#include "Albion/AlbionBuild.h"
#include "Albion/AlbionSky.h"
#include "Albion/AlbionChaucer.h"
#include "Albion/AlbionWork.h"
#include "Catalog/MuseeFiles.h"
#include "Dom/JsonObject.h"
#include "Frames/MuseeFrame.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/CollisionProfile.h"
#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "MuseeVision.h"
#include "ProceduralMeshComponent.h"

#if !UE_BUILD_SHIPPING
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/DelayedAutoRegister.h"
#include "Misc/Parse.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#endif

namespace AlbionActor
{
	using namespace AlbionBuild;

	TSoftObjectPtr<UMaterialInterface> Soft(const TCHAR* Relative)
	{
		const FString Name = FPaths::GetCleanFilename(Relative);
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("/Game/Museum/Materials/%s.%s"), Relative, *Name)));
	}

	/** Each slot's section index in its component: the slots of a component in order. */
	int32 SectionOf(int32 Slot)
	{
		const EComponent C = ComponentOf(Slot);
		int32 Index = 0;
		for (int32 s = 0; s < Slot; ++s) { if (ComponentOf(s) == C) { ++Index; } }
		return Index;
	}
}

AAlbionStructure::AAlbionStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);

	auto Make = [this](const TCHAR* Name, bool bCollision, bool bShadow)
	{
		UProceduralMeshComponent* C = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		C->SetupAttachment(RootComponent);
		C->SetMobility(EComponentMobility::Static);
		C->bUseAsyncCooking = true;
		C->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		C->SetCastShadow(bShadow);
		return C;
	};
	Masonry = Make(TEXT("Masonry"), true, true);
	Exterior = Make(TEXT("Exterior"), true, true);
	Floor = Make(TEXT("Floor"), true, true);
	Iron = Make(TEXT("Iron"), false, true);
	Glass = Make(TEXT("Glass"), false, false);
	Furniture = Make(TEXT("Furniture"), true, true);

	for (int32 s = 0; s < AlbionBuild::SlotCount; ++s) { SlotMaterials.Add(AlbionActor::Soft(AlbionBuild::DefaultMaterial(s))); }
	Tags.AddUnique(MuseeBake::BakeableTag());
}

void AAlbionStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AAlbionStructure::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

TArray<FString> AAlbionStructure::GetReplacedImportPrims()
{
	// The Chinese Wing on the south door, all of it (its building, garden, works, lamps): Albion takes its place. Its code
	// stays (ChineseWing/); its native actor and garden plants are removed by albion_place.py.
	return {TEXT("/Museum/ChineseWing")};
}

FBox AAlbionStructure::GetCourtBox()
{
	namespace AP = AlbionPlan;
	return FBox(AP::At(AP::X0, AP::Y0, 0.0), AP::At(AP::X1, AP::Y1, AP::NaveCrown()));
}

void AAlbionStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	const double Start = FPlatformTime::Seconds();
	const AlbionBuild::FParts Parts = AlbionBuild::BuildAll();
	UProceduralMeshComponent* Components[AlbionBuild::CompCount] = {Masonry, Exterior, Floor, Iron, Glass, Furniture};
	const bool bCollision[AlbionBuild::CompCount] = {true, true, true, false, false, true};
	for (UProceduralMeshComponent* C : Components) { if (C) { C->ClearAllMeshSections(); } }
	int32 Triangles = 0;
	for (int32 s = 0; s < AlbionBuild::SlotCount; ++s)
	{
		const AlbionBuild::EComponent C = AlbionBuild::ComponentOf(s);
		Parts[s].Write(Components[C], AlbionActor::SectionOf(s), bCollision[C]);
		Triangles += Parts[s].Indices.Num() / 3;
	}
	// The guards are collision only: hidden (the bake keeps a hidden section with collision as collision, never drawn).
	if (Parts[AlbionBuild::SlotGuard].Indices.Num() > 0)
	{
		Components[AlbionBuild::ComponentOf(AlbionBuild::SlotGuard)]->SetMeshSectionVisible(AlbionActor::SectionOf(AlbionBuild::SlotGuard), false);
	}
	ApplyMaterials();
	UE_LOG(LogMusee, Log, TEXT("Albion: built %d triangles in %.2f s."), Triangles, FPlatformTime::Seconds() - Start);
}

void AAlbionStructure::ApplyMaterials()
{
	UProceduralMeshComponent* Components[AlbionBuild::CompCount] = {Masonry, Exterior, Floor, Iron, Glass, Furniture};
	for (int32 s = 0; s < AlbionBuild::SlotCount && s < SlotMaterials.Num(); ++s)
	{
		if (SlotMaterials[s].IsNull()) { continue; }
		if (UMaterialInterface* M = SlotMaterials[s].LoadSynchronous())
		{
			Components[AlbionBuild::ComponentOf(s)]->SetMaterial(AlbionActor::SectionOf(s), M);
		}
	}
}

// ==================================================================== Development aids (not in shipping builds)

#if !UE_BUILD_SHIPPING
/**
 * -MuseeAlbion: in a game world with no AAlbionStructure placed, spawns one at the origin (with Albion's companions)
 * and hides what it takes the place of: the Chinese Wing (its native actor, everything tagged musee.wing:ChineseWing),
 * the plants that stood round it inside Albion's footprint, and the lawn (whose blades the placed wing's survey will
 * clear from its floor). A preview before native.py places it.
 */
namespace AlbionDev
{
	/** The hang from assets/albion/hang.json (Scripts/albion_hang.py layout), as albion_hang.py place() puts it in the map. */
	int32 PreviewHang(UWorld* World)
	{
		FString Text;
		if (!MuseeFiles::Load(TEXT("assets/albion/hang.json"), Text)) { return 0; }
		TSharedPtr<FJsonObject> Root;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) { return 0; }
		const TArray<TSharedPtr<FJsonValue>>* Works = nullptr;
		if (!Root->TryGetArrayField(TEXT("works"), Works)) { return 0; }
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		int32 N = 0;
		for (const TSharedPtr<FJsonValue>& V : *Works)
		{
			const TSharedPtr<FJsonObject> E = V->AsObject();
			if (!E.IsValid()) { continue; }
			const FString Kind = E->GetStringField(TEXT("kind")), SightName = E->GetStringField(TEXT("sight")), Frame = E->GetStringField(TEXT("frame"));
			const FVector At(E->GetNumberField(TEXT("x")) * 100.0, E->GetNumberField(TEXT("y")) * 100.0, E->GetNumberField(TEXT("z")) * 100.0);
			const FTransform Xf(FRotator(0.0, E->GetNumberField(TEXT("yaw")), 0.0), At);
			AAlbionWork* W = World->SpawnActorDeferred<AAlbionWork>(AAlbionWork::StaticClass(), Xf, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!W) { continue; }
			W->Kind = Kind == TEXT("Tapestry") ? EAlbionWorkKind::Tapestry : Kind == TEXT("Wallpaper") ? EAlbionWorkKind::Wallpaper
				: Kind == TEXT("Dish") ? EAlbionWorkKind::Dish : Kind == TEXT("Vase") ? EAlbionWorkKind::Vase : EAlbionWorkKind::Canvas;
			W->Sight = SightName == TEXT("Arched") ? EAlbionSight::Arched : SightName == TEXT("Oval") ? EAlbionSight::Oval : EAlbionSight::Rect;
			W->Width = float(E->GetNumberField(TEXT("width")));
			W->Height = float(E->GetNumberField(TEXT("height")));
			const double Offset = E->GetNumberField(TEXT("canvas_offset"));
			W->CanvasOffset = Offset > 0.0 ? float(Offset) : 0.045f;
			W->FrameOuterHalfWidth = Frame != TEXT("None") ? float(0.5 * E->GetNumberField(TEXT("outer_w"))) : 0.f;
			W->FrameOuterTop = float(0.5 * E->GetNumberField(TEXT("outer_h")));
			W->RailAbove = float(E->GetNumberField(TEXT("rail_above")));
			const TArray<TSharedPtr<FJsonValue>>* Repeat = nullptr;
			if (E->TryGetArrayField(TEXT("repeat"), Repeat) && Repeat->Num() == 2) { W->PatternRepeat = FVector2D((*Repeat)[0]->AsNumber(), (*Repeat)[1]->AsNumber()); }
			const FString Mat = E->GetStringField(TEXT("material"));
			W->FaceMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Mat + TEXT(".") + FPaths::GetCleanFilename(Mat)));
			W->Tags.Add(FName(*(TEXT("work:") + E->GetStringField(TEXT("id")))));
			W->FinishSpawning(Xf);
			if (Frame != TEXT("None"))
			{
				AMuseeFrame* F = World->SpawnActor<AMuseeFrame>(AMuseeFrame::StaticClass(), Xf, Spawn);
				if (F)
				{
					F->SightWidth = W->Width;
					F->SightHeight = W->Height;
					F->CanvasOffset = W->CanvasOffset;
					F->Configure(Frame, float(E->GetNumberField(TEXT("frame_width"))), 0.f, FVector2D::ZeroVector, false);
					F->AttachToActor(W, FAttachmentTransformRules::KeepWorldTransform);
				}
			}
			++N;
		}
		return N;
	}

	void PreviewIn(UWorld* World)
	{
		if (!World || !World->IsGameWorld()) { return; }
		for (TActorIterator<AAlbionStructure> It(World); It; ++It) { return; }
		const FBox Footprint(AlbionPlan::At(-16.0, 10.3, -1.0), AlbionPlan::At(16.0, 55.5, 25.0));
		int32 Hidden = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			bool bHide = false;
			for (const FName& Tag : Actor->Tags)
			{
				const FString Text = Tag.ToString();
				bHide |= Text == TEXT("musee.wing:ChineseWing") || Text == TEXT("musee.lawn");
			}
			const FString Class = Actor->GetClass()->GetName();
			bHide |= Class == TEXT("ChineseWingStructure");
			if ((Class == TEXT("MuseeTree") || Class == TEXT("MuseePottedPlant") || Class == TEXT("MuseeTaihuRock") || Class == TEXT("MuseeWaterLilies"))
				&& Footprint.IsInside(Actor->GetActorLocation()))
			{
				bHide = true;
			}
			if (bHide)
			{
				Actor->SetActorHiddenInGame(true);
				Actor->SetActorEnableCollision(false);
				++Hidden;
			}
		}
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<AAlbionStructure>(AAlbionStructure::StaticClass(), FTransform::Identity, Spawn);
		bool bSky = false;
		for (TActorIterator<AAlbionSky> It(World); It; ++It) { bSky = true; }
		if (!bSky) { World->SpawnActor<AAlbionSky>(AAlbionSky::StaticClass(), FTransform::Identity, Spawn); }
		bool bBook = false;
		for (TActorIterator<AAlbionChaucer> It(World); It; ++It) { bBook = true; }
		if (!bBook)
		{
			World->SpawnActor<AAlbionChaucer>(AAlbionChaucer::StaticClass(), FTransform(AlbionPlan::At(AlbionPlan::LecternX, AlbionPlan::LecternY, 0.0)), Spawn);
		}
		const int32 Works = PreviewHang(World);
		UE_LOG(LogMusee, Log, TEXT("Albion preview: placed with %d works; %d actors of the Chinese Wing and its garden hidden."), Works, Hidden);
	}

	void PreviewCommand(const TArray<FString>& Args, UWorld* World) { PreviewIn(World); }

	FAutoConsoleCommandWithWorldAndArgs GPreviewCommand(
		TEXT("musee.Albion.Preview"),
		TEXT("Places Albion at the origin for this run (not saved) and hides the Chinese Wing it replaces."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&PreviewCommand));

	/** -MuseeAlbion: once a game world has begun play, preview Albion in it. */
	FDelayedAutoRegisterHelper GRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("MuseeAlbion"))) { return; }
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) -> bool
		{
			if (!GEngine) { return true; }
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && World->IsGameWorld() && World->HasBegunPlay())
				{
					PreviewIn(World);
					return false;
				}
			}
			return true;
		}), 0.25f);
	});
}
#endif
