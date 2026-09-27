#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "VisitorHeadset.generated.h"

class AVisitorBody;
class STextBlock;
class UInputAction;
class UInputMappingContext;
class UMotionControllerComponent;
class UWidgetComponent;
struct FInputActionValue;

/**
 * The visitor in a headset (Quest 3 with Touch controllers through OpenXR; the Vision Pro through ALVR, whose gestures
 * arrive as the same buttons). The headset adapter adds it to the body when an OpenXR session runs. The eyes follow the
 * headset from standing eye height, the body follows the head about the room as far as the walls let it, and the
 * overlay's words float on a panel in front of the eyes. Quest Touch controls:
 *  - left stick: walk where the head faces; left stick click: stride while held;
 *  - right stick left or right: turn by a comfort step;
 *  - right trigger: use what the right hand points at; pointing at the floor, hold and let go to step there;
 *  - A: use what the eyes rest on; B: another hour; X: the other city; Y or the menu button: recentre.
 */
UCLASS(ClassGroup = (Musee))
class UVisitorHeadset final : public UActorComponent
{
	GENERATED_BODY()

public:
	UVisitorHeadset();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:
	void TakeTheEyes(AVisitorBody& Body);
	void MakePanel(AVisitorBody& Body);
	void BuildControls();
	void AddControls(AVisitorBody& Body);
	void Walk(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void TurnReleased();
	void PointStarted();
	void PointEnded();
	void UseGazed();
	void AnotherHour();
	void OtherCity();
	void StrideOn();
	void StrideOff();
	void Recentre();
	void FollowHead();
	TOptional<FVector> StepTarget() const;
	void StepTo(const FVector& Floor);
	AVisitorBody* Body() const;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> TrackingOrigin;

	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> RightHand;

	UPROPERTY(Transient)
	TObjectPtr<UMotionControllerComponent> LeftHand;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> Panel;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Controls;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UInputAction>> Actions;

	TSharedPtr<STextBlock> PanelText;
	FDelegateHandle ControlsHandle;
	bool bAimingStep = false;
	bool bTurned = false;
};
