#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "LightSkyPlacement.generated.h"

/** Places the one sky in every game world as play begins, unless the world already has it. */
UCLASS()
class ULightSkyPlacement final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& World) override;
};
