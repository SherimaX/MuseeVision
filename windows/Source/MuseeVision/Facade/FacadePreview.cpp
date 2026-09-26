#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Facade/MuseeFacadeStructure.h"
#include "Facade/MuseeLandscape.h"
#include "HAL/IConsoleManager.h"
#include "MuseeVision.h"
#include "Nature/MuseeTree.h"
#include "Plan/MuseePlan.h"

/**
 * musee.Facade.Preview [nofacade] [nogrounds] [notrees]: for a test run (nothing is saved), places the exterior dress
 * (AMuseeFacadeStructure) and the grounds (AMuseeLandscape) at the origin if the map has none, and grows the grounds'
 * trees (AMuseeLandscape::GetTreePlacements) where nature.py hasn't placed them.
 */
namespace FacadePreview
{
	template <class T>
	T* FindFirst(UWorld* World)
	{
		for (TActorIterator<T> It(World); It; ++It) { return *It; }
		return nullptr;
	}

	void Preview(const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		auto Has = [&Args](const TCHAR* Word) { return Args.ContainsByPredicate([Word](const FString& A) { return A.Equals(Word, ESearchCase::IgnoreCase); }); };
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		int32 Placed = 0, Trees = 0, TreeTriangles = 0;
		if (!Has(TEXT("nofacade")) && !FindFirst<AMuseeFacadeStructure>(World))
		{
			Placed += World->SpawnActor<AMuseeFacadeStructure>(FVector::ZeroVector, FRotator::ZeroRotator, Params) ? 1 : 0;
		}
		if (!Has(TEXT("nogrounds")) && !FindFirst<AMuseeLandscape>(World))
		{
			Placed += World->SpawnActor<AMuseeLandscape>(FVector::ZeroVector, FRotator::ZeroRotator, Params) ? 1 : 0;
		}
		if (!Has(TEXT("notrees")))
		{
			TSet<FString> Existing;
			for (TActorIterator<AMuseeTree> It(World); It; ++It)
			{
				for (const FName& Tag : It->Tags)
				{
					const FString Text = Tag.ToString();
					if (Text.StartsWith(TEXT("nature:"))) { Existing.Add(Text.Mid(7)); }
				}
			}
			for (const FMuseeGroundsTree& T : AMuseeLandscape::GetTreePlacements())
			{
				if (Existing.Contains(T.Name)) { continue; }
				AMuseeTree* Tree = World->SpawnActor<AMuseeTree>(MuseePlan::At(T.Position.X, T.Position.Y, 0.0), FRotator::ZeroRotator, Params);
				if (!Tree) { continue; }
				Tree->Species = T.Species;
				Tree->TreeHeight = T.TreeHeight;
				Tree->CrownHeight = T.CrownHeight;
				Tree->CrownRadii = T.CrownRadii;
				Tree->LeafDensity = T.LeafDensity;
				Tree->Seed = T.Seed;
				Tree->Culms = T.Culms;
				Tree->Keep = T.Keep;
				Tree->Season = EMuseeNatureSeason::Summer;
				Tree->Tags.Add(FName(TEXT("musee.grounds.preview")));
				Tree->Tags.Add(FName(*(TEXT("nature:") + T.Name)));
				Tree->Regrow();
				TreeTriangles += Tree->TriangleCount;
				UE_LOG(LogMusee, Log, TEXT("Facade preview: %s %d triangles."), *T.Name, Tree->TriangleCount);
				++Trees;
			}
		}
		UE_LOG(LogMusee, Log, TEXT("Facade preview: %d actors placed, %d trees grown (%d triangles; not saved)."), Placed, Trees, TreeTriangles);
	}

	/** musee.Facade.WhoIsAt X Y Z (plan metres): logs the visible primitives whose bounds hold the point (a debugging aid). */
	void WhoIsAt(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Args.Num() < 3) { return; }
		const FVector P = MuseePlan::At(FCString::Atod(*Args[0]), FCString::Atod(*Args[1]), FCString::Atod(*Args[2]));
		const bool bAll = Args.Num() > 3 && Args[3].Equals(TEXT("all"), ESearchCase::IgnoreCase);
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->IsHidden() && !bAll) { continue; }
			TArray<UPrimitiveComponent*> Prims;
			It->GetComponents<UPrimitiveComponent>(Prims);
			for (const UPrimitiveComponent* C : Prims)
			{
				if (!C || (!C->IsVisible() && !bAll) || !C->Bounds.GetBox().ExpandBy(10.0).IsInside(P)) { continue; }
				if (bAll)
				{
					UE_LOG(LogMusee, Log, TEXT("WhoIsAt(all): %s %s hidden %d visible %d castshadow %d hiddenshadow %d"), *It->GetActorNameOrLabel(), *C->GetName(),
						   It->IsHidden() ? 1 : 0, C->IsVisible() ? 1 : 0, C->CastShadow ? 1 : 0, C->bCastHiddenShadow ? 1 : 0);
				}
				FString TagText;
				for (const FName& T : It->Tags) { TagText += T.ToString() + TEXT(" "); }
				UE_LOG(LogMusee, Log, TEXT("WhoIsAt: %s (%s) %s [%s] bounds %s"), *It->GetActorNameOrLabel(), *It->GetClass()->GetName(), *C->GetName(), *TagText,
					   *C->Bounds.GetBox().ToString());
			}
		}
	}

	/** musee.Facade.Fov <degrees>: the visitor's lens for a shot (a zoom to inspect a detail far off). */
	void Fov(const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		UCameraComponent* Camera = Pawn ? Pawn->FindComponentByClass<UCameraComponent>() : nullptr;
		if (Camera && Args.Num() > 0) { Camera->SetFieldOfView(FMath::Clamp(FCString::Atof(*Args[0]), 5.f, 120.f)); }
	}

	FAutoConsoleCommandWithWorldAndArgs GFovCommand(
		TEXT("musee.Facade.Fov"),
		TEXT("Sets the visitor's horizontal field of view (degrees), for zoomed inspection shots."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Fov));

	/** musee.Facade.Debug hide|noshadow <component name part>: hides, or stops the shadow of, matching primitives (for tests). */
	void DebugPrims(const TArray<FString>& Args, UWorld* World)
	{
		if (!World || Args.Num() < 2) { return; }
		const bool bHide = Args[0].Equals(TEXT("hide"), ESearchCase::IgnoreCase);
		int32 N = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			TArray<UPrimitiveComponent*> Prims;
			It->GetComponents<UPrimitiveComponent>(Prims);
			for (UPrimitiveComponent* C : Prims)
			{
				if (!C || !C->GetName().Contains(Args[1])) { continue; }
				if (bHide) { C->SetVisibility(false); }
				else { C->SetCastShadow(false); }
				++N;
			}
		}
		UE_LOG(LogMusee, Log, TEXT("Facade debug: %s on %d primitives named *%s*."), *Args[0], N, *Args[1]);
	}

	FAutoConsoleCommandWithWorldAndArgs GDebugCommand(
		TEXT("musee.Facade.Debug"),
		TEXT("Test aid: 'hide <name part>' hides primitives whose component name contains it; 'noshadow <name part>' stops their shadows."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&DebugPrims));

	FAutoConsoleCommandWithWorldAndArgs GWhoIsAtCommand(
		TEXT("musee.Facade.WhoIsAt"),
		TEXT("Logs the visible primitives whose bounds hold a plan point: X Y Z in metres."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&WhoIsAt));

	FAutoConsoleCommandWithWorldAndArgs GPreviewCommand(
		TEXT("musee.Facade.Preview"),
		TEXT("Places the exterior dress and the grounds at the origin for this run (not saved), with the grounds' trees. Args: nofacade, nogrounds, notrees."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Preview));
}

#endif
