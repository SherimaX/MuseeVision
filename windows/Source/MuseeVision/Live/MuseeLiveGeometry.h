#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MuseeLiveGeometry.generated.h"

/**
 * -Live: a wing's geometry follows its tuned numbers while the game runs. Today Chenghuai: Saved/Live/chenghuai.json
 * (ChenghuaiTune) changes, the wing is built again with them (AChenghuaiStructure::RebuildLive) and the live look is
 * applied again on its materials. Saved/Live/geometry_state.json reports each rebuild: the count, the milliseconds and
 * the file's keys that no build asked for; Saved/Live/chenghuai_keys.json lists every tunable key with its default.
 * With no chenghuai.json the wing stays as it started.
 */
UCLASS()
class UMuseeLiveGeometry : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMuseeLiveGeometry, STATGROUP_Tickables); }

private:
	bool bActive = false;
	double NextPoll = 0.0;
	FString Key;   // chenghuai.json's write time and size when last built from
	int32 Applied = 0;
};
