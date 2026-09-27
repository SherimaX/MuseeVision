#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "MotionChannelSpec.h"

#include "MotionDerivedChannel.generated.h"

/**
 * A channel a stage derives from another, kept as a relay keeps its contact: every frame it reads the channel it
 * follows and publishes the map's value (publishing a value the point already holds is silent). The Cube's mast
 * pinspots follow it: on while no work performs.
 */
UCLASS(ClassGroup = Musee)
class MUSEEMOTION_API UMotionDerivedChannel : public UActorComponent
{
	GENERATED_BODY()

public:
	UMotionDerivedChannel();

	/** Sets the channel up from its spec. Making time only. */
	void Configure(const FMotionChannelSpec& InSpec);

	[[nodiscard]] const FMotionChannelSpec& Spec() const { return ChannelSpec; }

	/** Publishes the derived value from the followed channel's present value. */
	void Advance();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	FMotionChannelSpec ChannelSpec;
};
