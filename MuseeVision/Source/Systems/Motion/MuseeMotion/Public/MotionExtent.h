#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "MotionExtentSpec.h"

#include "MotionExtent.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;

/**
 * A piece of variable length at work: every frame it reads the length its channel asks for and sets its parts to it.
 * A telescope's stages ride carriages that slide along its axis, each by its own extension and those of the stages
 * before it; a column of turns shows as many turns, stacked back from its anchored end, as the length needs. The
 * dresser makes its carriages or its column from the parts Motion drew, at making time.
 */
UCLASS(ClassGroup = Musee)
class MUSEEMOTION_API UMotionExtent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMotionExtent();

	/** A telescope: its spec, each stage's carriage standing where the stage is drawn (home), and the length it is
	 *  drawn at (its channel's value where the stage is drawn). Making time only. */
	void ConfigureTelescope(const FMotionExtentSpec& InSpec, const TArray<USceneComponent*>& InCarriages, double DrawnLength);

	/** A column of turns: its spec, the column, its first turn's pose in the column, and the length it is drawn at.
	 *  Making time only. */
	void ConfigureTurns(const FMotionExtentSpec& InSpec, UInstancedStaticMeshComponent* InColumn, const FTransform& InFirstTurn, double DrawnLength);

	[[nodiscard]] const FMotionExtentSpec& Spec() const { return ExtentSpec; }

	/** The length it stands at, in metres. */
	[[nodiscard]] double LengthMeters() const { return Length; }

	/** Sets its parts to the length its channel asks for now. */
	void Advance();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void SetLength(double InLength);

	UPROPERTY()
	FMotionExtentSpec ExtentSpec;

	/** A telescope's carriages, one per stage, and where each stands at home relative to its parent. */
	UPROPERTY()
	TArray<TObjectPtr<USceneComponent>> Carriages;

	UPROPERTY()
	TArray<FTransform> CarriageHomes;

	/** A column's turns, and its first turn's pose. */
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> Column;

	UPROPERTY()
	FTransform FirstTurn;

	/** The length it stands at; saved with the level, so it is drawn at the length it was made at. */
	UPROPERTY()
	double Length = 0.0;

	/** Whether its parts have been set since it was loaded. */
	bool bSet = false;
};
