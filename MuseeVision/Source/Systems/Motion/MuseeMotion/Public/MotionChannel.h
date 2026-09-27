#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "MotionChannelSpec.h"

#include "MotionChannel.generated.h"

/**
 * A control point a stage keeps. A derived one is kept as a relay keeps its contact: every frame it reads the channel it
 * follows and publishes the map's value (publishing a value the point already holds is silent). A point the stage's
 * sequences ramp is set at its rest when the museum opens and left to them.
 */
UCLASS(ClassGroup = Musee)
class MUSEEMOTION_API UMotionChannel : public UActorComponent
{
	GENERATED_BODY()

public:
	UMotionChannel();

	/** Sets the point up from its spec. Making time only. */
	void Configure(const FMotionChannelSpec& InSpec);

	[[nodiscard]] const FMotionChannelSpec& Spec() const { return ChannelSpec; }

	/** Sets the point where the museum opens it: at its rest, or at its derived value. BeginPlay does this. */
	void StandAtStart();

	/** Keeps a derived point: publishes its value from the followed channel's present one. */
	void Advance();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	FMotionChannelSpec ChannelSpec;
};
