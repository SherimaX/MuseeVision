#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MuseeLiveLook.generated.h"

class APostProcessVolume;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * -Live: the look follows a file while the game runs, so a change to the exposure, the post-process, a console
 * variable, a light or a material parameter shows within a fraction of a second, with no editor step and no relaunch.
 * Without -Live the game is exactly as before. Ported from the home project's LiveScene (ApplyLook).
 *
 *   Saved/Live/look.json        {"post": {Setting: value}, "cvars": {r.Name: value},
 *                               "lights": {label or name pattern: {Property: value}},
 *                               "materials": {MI name or /Game/... path: {Param: value}}}
 *                               (Scripts/live.py writes it; a key taken out goes back to what the level has)
 *   Saved/Live/state.json       the game's report after every change: the count of changes applied, the time it took,
 *                               the errors, and the lights by label
 *   Saved/Live/materials.json   every material instance in the scene with its parameters, written once at the start
 *
 * Nothing is saved to an asset: the post-process goes on an unbound volume of its own (priority 1000, over the
 * museum's at 10 and Chenghuai's at 6), materials on dynamic copies swapped in where the instance is used.
 */
UCLASS()
class UMuseeLiveLook : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMuseeLiveLook, STATGROUP_Tickables); }
	/** After material packages were reloaded (UMuseeLiveAssets): materials.json lists the reloaded values, and the live
	 *  copies are remade on the reloaded instances with look.json applied again on the next tick. */
	void Reapply() { WriteMaterials(); Mids.Empty(); LookKey.Reset(); }

private:
	FString LiveDir;
	bool bActive = false;
	double NextPoll = 0.0;
	FString LookKey;   // look.json's write time and size when last applied

	int32 Applied = 0;   // changes applied so far (live.py waits for this to move)
	double LastMs = 0.0;
	TArray<FString> Errors;

	UPROPERTY(Transient) TObjectPtr<APostProcessVolume> LookVolume;
	UPROPERTY(Transient) TMap<FString, TObjectPtr<UMaterialInstanceDynamic>> Mids;   // MI path → its live copy
	TMap<FString, FString> CVarDefaults;
	struct FPropDefault { TWeakObjectPtr<UObject> Obj; FString Prop, Text; };
	TArray<FPropDefault> PropDefaults;   // the lights' properties as the level had them, before look.json

	void ApplyLook();
	void SwapLiveMaterials();
	UMaterialInterface* FindMaterial(const FString& Name, FString& OutPath) const;
	void WriteState();
	void WriteMaterials();
};
