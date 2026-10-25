#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MuseeLiveAssets.generated.h"

/**
 * -Live: material and texture packages an editor step saves (materials.py, a wing's materials script, a texture
 * reimport) are reloaded into the running game, so a materials round needs no restart. Only packages the game has
 * loaded are watched, never maps.
 *
 * A reload waits until the changed files have been quiet for 3 s, so a half-saved batch is never loaded, and until a
 * live shots request has finished. It goes a few packages per frame with every render
 * state taken down (the home project's reload once crashed a window under a drawing frame). Packages re-saved with the
 * same bytes are skipped. The live look's copies are remade on the reloaded instances and look.json applied again.
 *
 * The game lets go of its material packages' files after loading them (ResetLoaders), so an editor step can replace
 * them while the game runs. Textures keep their files open for lazy loading, so a step that rewrites a loaded texture
 * may still fail to save while a game holds it. The game's log lists each batch ("live: hot assets: …") and any errors;
 * -LiveNoHotAssets turns this off.
 */
UCLASS()
class UMuseeLiveAssets : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMuseeLiveAssets, STATGROUP_Tickables); }

private:
	bool bActive = false;
	double NextScan = 0.0;
	FString ContentDir;
	TMap<FString, FString> Seen;     // package name → its file's write time and size when last looked at
	TMap<FString, FString> Hashes;   // package name → hash of the bytes this game has
	TSet<FString> Released;          // material packages whose files the game has let go of
	FString PendingSig;
	double PendingSince = 0.0;
	TArray<FString> Queue;           // changed packages waiting to be reloaded, a few per frame
	TArray<FString> Reloaded, Errors;
	double BatchStart = 0.0;

	void Scan();
	void ReloadSome();
	void ReleaseFiles();
	FString FileOf(const FString& PackageName) const;
};
