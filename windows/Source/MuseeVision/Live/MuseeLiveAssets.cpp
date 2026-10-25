#include "Live/MuseeLiveAssets.h"
#include "Live/MuseeLiveLook.h"
#include "Live/MuseeLiveShots.h"
#include "MuseeVision.h"
#include "ComponentRecreateRenderStateContext.h"
#include "Components/MeshComponent.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UObject/Linker.h"
#include "UObject/Package.h"
#include "UObject/PackageReload.h"
#include "UObject/UObjectIterator.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

namespace
{
/** A file's exact write time and size ("missing" when it isn't there). */
FString FileKey(const FString& Path)
{
#if PLATFORM_WINDOWS
	WIN32_FILE_ATTRIBUTE_DATA A;
	if (!GetFileAttributesExW(*Path, GetFileExInfoStandard, &A)) { return TEXT("missing"); }
	return FString::Printf(TEXT("%u.%u:%u.%u"), A.ftLastWriteTime.dwHighDateTime, A.ftLastWriteTime.dwLowDateTime, A.nFileSizeHigh, A.nFileSizeLow);
#else
	const FFileStatData S = IFileManager::Get().GetStatData(*Path);
	return S.bIsValid ? FString::Printf(TEXT("%lld:%lld"), S.ModificationTime.GetTicks(), S.FileSize) : TEXT("missing");
#endif
}

FString HashFile(const FString& Path)
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent)) { return FString(); }
	FSHAHash H;
	FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), H.Hash);
	return H.ToString();
}

/** The /Game packages worth watching: materials, material instances and textures, never maps. */
bool Watched(const UObject* O)
{
	return (O->IsA<UMaterialInterface>() || O->IsA<UTexture>()) && O->IsAsset();
}

bool IsGamePackage(const FString& Name) { return Name.StartsWith(TEXT("/Game/")) && !Name.StartsWith(TEXT("/Game/Maps/")); }

constexpr int32 PerFrame = 6;   // packages reloaded per frame: a big batch stutters instead of freezing
}

bool UMuseeLiveAssets::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("Live")) || FParse::Param(FCommandLine::Get(), TEXT("LiveNoHotAssets"))) { return false; }
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE);
}

void UMuseeLiveAssets::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	ContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	bActive = true;
	NextScan = FPlatformTime::Seconds() + 2.0;
}

FString UMuseeLiveAssets::FileOf(const FString& PackageName) const
{
	return ContentDir / PackageName.Mid(6) + FPackageName::GetAssetPackageExtension();   // "/Game/" → Content/
}

void UMuseeLiveAssets::ReleaseFiles()
{
	// editor builds keep a loaded package's file open for lazy loading; materials have nothing to load lazily, so the
	// game lets go of their files and an editor step can replace them while it runs
	TArray<UObject*> Outers;
	for (TObjectIterator<UMaterialInterface> It; It; ++It)
	{
		UPackage* P = It->GetPackage();
		if (!P || !It->IsAsset()) { continue; }
		const FString Name = P->GetName();
		if (!IsGamePackage(Name) || Released.Contains(Name)) { continue; }
		Released.Add(Name);
		Outers.AddUnique(P);
	}
	if (Outers.Num()) { ResetLoaders(Outers); }
}

void UMuseeLiveAssets::Tick(float DeltaTime)
{
	if (!bActive) { return; }
	if (Queue.Num())
	{
		ReloadSome();
		return;
	}
	const double Now = FPlatformTime::Seconds();
	if (Now < NextScan) { return; }
	NextScan = Now + 1.0;
	Scan();
}

void UMuseeLiveAssets::Scan()
{
	ReleaseFiles();   // what loaded since the last scan (a room walked into) lets go of its files too
	TArray<FString> Changed, Keys;
	TSet<FString> Names;
	for (TObjectIterator<UObject> It; It; ++It)
	{
		if (!Watched(*It)) { continue; }
		const UPackage* P = It->GetPackage();
		const FString Name = P ? P->GetName() : FString();
		if (!IsGamePackage(Name) || Names.Contains(Name)) { continue; }
		Names.Add(Name);
		const FString K = FileKey(FileOf(Name));
		const FString* Old = Seen.Find(Name);
		if (!Old)   // first seen: the version the game loaded (a material's bytes hashed, so a same-bytes re-save is skipped)
		{
			Seen.Add(Name, K);
			if (It->IsA<UMaterialInterface>()) { Hashes.Add(Name, HashFile(FileOf(Name))); }
			continue;
		}
		if (*Old == K || K == TEXT("missing")) { continue; }
		Changed.Add(Name);
		Keys.Add(K);
	}
	if (!Changed.Num()) { PendingSig.Reset(); return; }
	// wait until nothing changed for 3 s, so a batch an editor step is still saving is never loaded half-way, and until
	// a shots request is done. (Not "no UnrealEditor-Cmd running": another worktree's commandlets would block it.)
	const double Now = FPlatformTime::Seconds();
	const FString Sig = FString::Join(Keys, TEXT("|"));
	if (Sig != PendingSig) { PendingSig = Sig; PendingSince = Now; return; }
	if (Now - PendingSince < 3.0) { return; }
	if (const UMuseeLiveShots* Shots = GetWorld()->GetSubsystem<UMuseeLiveShots>(); Shots && Shots->IsBusy()) { return; }
	PendingSig.Reset();
	int32 Same = 0;
	for (int32 i = 0; i < Changed.Num(); ++i)
	{
		const FString& Name = Changed[i];
		Seen.Add(Name, Keys[i]);
		// an editor step re-saves packages it didn't change: the same bytes as this game's version, nothing to reload
		const FString H = HashFile(FileOf(Name));
		if (!Hashes.Contains(Name)) { Hashes.Add(Name, FString()); }
		if (H.IsEmpty() || Hashes[Name] == H) { ++Same; continue; }
		Hashes.Add(Name, H);
		Queue.Add(Name);
	}
	if (Same) { UE_LOG(LogMusee, Display, TEXT("live: hot assets: %d re-saved with the same bytes, skipped"), Same); }
	Reloaded.Reset();
	Errors.Reset();
	BatchStart = FPlatformTime::Seconds();
}

void UMuseeLiveAssets::ReloadSome()
{
	TArray<UPackage*> Batch;
	while (Queue.Num() && Batch.Num() < PerFrame)
	{
		const FString Name = Queue[0];
		Queue.RemoveAt(0);
		if (UPackage* P = FindPackage(nullptr, *Name))
		{
			Batch.Add(P);
			Reloaded.Add(FPackageName::GetShortName(Name));
		}
	}
	if (Batch.Num())
	{
		// no primitive draws while its materials are swapped: every render state is taken down for the pass (the
		// context flushes the render thread) and made again after it
		{
			FGlobalComponentRecreateRenderStateContext Recreate;
			for (UPackage* P : Batch) { ResetLoaders(P); }
			SortPackagesForReload(Batch);
			TArray<FReloadPackageData> Data;
			for (UPackage* P : Batch) { Data.Emplace(P, LOAD_None); }
			TArray<UPackage*> Out;
			ReloadPackages(Data, Out, Data.Num());
			for (int32 i = 0; i < Out.Num(); ++i)
			{
				if (!Out[i]) { Errors.Add(FString::Printf(TEXT("hot assets: %s didn't reload"), *Batch[i]->GetName())); }
			}
			const UWorld* World = GetWorld();
			for (TObjectIterator<UMeshComponent> It; It; ++It)
			{
				if (It->GetWorld() != World) { continue; }
				for (int32 m = 0; m < It->GetNumMaterials(); ++m)
				{
					if (UMaterialInterface* M = It->GetMaterial(m)) { M->RecacheUniformExpressions(true); }
				}
			}
		}
	}
	if (Queue.Num()) { return; }
	// the batch is in: the reloaded packages let go of their files again, and the live look is remade on them
	Released.Reset();
	ReleaseFiles();
	if (UMuseeLiveLook* Look = GetWorld()->GetSubsystem<UMuseeLiveLook>()) { Look->Reapply(); }
	const int32 Shown = FMath::Min(Reloaded.Num(), 6);
	UE_LOG(LogMusee, Display, TEXT("live: hot assets: %d from the editor (%s%s) in %.0f ms"), Reloaded.Num(),
		*FString::Join(TArrayView<const FString>(Reloaded.GetData(), Shown), TEXT(", ")),
		Reloaded.Num() > Shown ? *FString::Printf(TEXT(" and %d more"), Reloaded.Num() - Shown) : TEXT(""),
		(FPlatformTime::Seconds() - BatchStart) * 1000.0);
	for (const FString& E : Errors) { UE_LOG(LogMusee, Warning, TEXT("live: %s"), *E); }
}
