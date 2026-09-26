#pragma once

#include "CoreMinimal.h"
#include "Visitor/MuseeCharacter.h"
#include "MuseeVRCharacter.generated.h"

class AMuseeVRButton;
class FMuseeVRExposure;
class SMuseePlacard;
class SMuseeReticle;
class STextBlock;
class SWidget;
class UMotionControllerComponent;
class UWidgetComponent;

/**
 * The visitor in a headset: MuseeVisionVR.exe through SteamVR, and on the Vision Pro through ALVR,
 * whose hand gestures arrive as controller buttons. The game mode picks it whenever the VR build
 * runs; with no headset connected it is the desktop visitor (-MuseeVR forces the headset's
 * controls and signs on the desktop, for testing).
 *
 *   Look at a thing and pinch (right thumb and index) to use it; the buttons for where you stand
 *   float in front of you, lower right: look at one and pinch.
 *   Pinch and hold anywhere else: a gilt ring shows where your right hand points on the floor (or
 *   your gaze, if the hand isn't tracked); let go to step there.
 *   Pinch and hold with the left hand: walk where you look.
 *   Thumb to middle finger: turn 30° (the right hand turns right, the left hand left).
 *   Left thumb to ring finger: another hour of the sky. Left thumb to little finger: recentre.
 *   Thumbsticks (PS VR2 controllers, or ALVR's stick gesture): walk with the left, turn with the right.
 *
 * The eyes are put at 1.6 m however you sit or stand (recentring puts them back). Walking about
 * the room carries the visitor with you; walls stop the body, not the head.
 */
UCLASS(NotPlaceable)
class AMuseeVRCharacter : public AMuseeCharacter
{
	GENERATED_BODY()

public:
	AMuseeVRCharacter();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	/** The headset's tracking space: at eye height over the feet, turned with the visitor. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<USceneComponent> TrackingOrigin;

	/** The hands' pointing rays (OpenXR aim poses). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UMotionControllerComponent> RightHand;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UMotionControllerComponent> LeftHand;

	/** The dot at the centre of the view, and the hint under it. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UWidgetComponent> DotPanel;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UWidgetComponent> HintPanel;

	/** The wall label, low and to the left of the view, following it lazily. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UWidgetComponent> LabelPanel;

	/** The museum's message, above the view. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UWidgetComponent> MessagePanel;

	/** The ring on the floor where a step would land. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UWidgetComponent> StepRing;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float TurnDegrees = 30.f;

	/** How far a step can reach (cm). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float StepRange = 1500.f;

private:
	void BuildVRInput();
	/** The triggers (buttons), ignored for a hand whose joints are tracked: its pinches come from UpdateHands. */
	void TriggerRightStarted();
	void TriggerRightEnded();
	void TriggerLeftStarted();
	void TriggerLeftEnded();
	void PinchRightStarted();
	void PinchRightEnded();
	void PinchLeftStarted();
	void PinchLeftEnded();
	void StickMove(const FInputActionValue& Value);
	void StickTurn(const FInputActionValue& Value);
	void StickTurnEnded();
	void TurnRight();
	void TurnLeft();
	void Turn(float Degrees);
	void Recentre();
	void SkyHour();
	/** A (the ring pinch): use what the right hand points at, else what the eyes rest on. */
	void UsePointed();
	/** The right stick pushed forward: aim a step; let go to take it. */
	void StickAim(const FInputActionValue& Value);
	void StickAimEnded();
	void City();
	void BriskStart();
	void BriskEnd();
	/**
	 * Bare hands (the Quest 3's hand tracking through Steam Link, OpenXR's hand joints): pinches read
	 * from the fingertips. Thumb to index is the trigger, thumb to middle turns (right hand right,
	 * left hand left), right thumb to ring is another hour, left thumb to ring recentres.
	 */
	void UpdateHands();
	/** The right hand's beam: dots along its ray (or the step's arc) and a ring where it lands. */
	void UpdatePointer();
	void HidePointer();

	/** Keep the body under the head as you walk about the room. */
	void FollowHead();
	/** Log what the hands send (-MuseeVRInputLog, or always for the first minutes in a headset). */
	void LogInput(float DeltaSeconds);
	/** The right hand's ray, or the gaze's when the hand isn't tracked. */
	void AimRay(FVector& From, FVector& Direction) const;
	/** Use what the ray points at, if it can be used now. */
	bool UseAlong(const FVector& From, const FVector& Direction);
	/** Put the eyes back over the body (after a step or a turn). */
	void CentreHead();
	void UpdateStep();
	void StepTo(const FVector& Floor);
	void UpdateButtons();
	void ClearButtons();
	void UpdatePanels(float DeltaSeconds);
	/** Glide a world-placed panel toward its place in the view (snap when it first appears). */
	void Follow(UWidgetComponent& Panel, const FVector& Offset, float DeltaSeconds, bool bSnap) const;
	FVector HeadForward() const;

	/** The headset's controls and signs are on. */
	bool bVR = false;
	/** A headset is tracking the head (not the -MuseeVR desktop test). */
	bool bTracked = false;

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> VRContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PinchRightAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PinchLeftAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> StickMoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> StickTurnAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> TurnRightAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> TurnLeftAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> RecentreAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SkyHourAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> VRUseAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> StickAimAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> VRCityAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> VRBriskAction;

	UPROPERTY(Transient) TArray<TObjectPtr<UWidgetComponent>> BeamDots;
	UPROPERTY(Transient) TObjectPtr<UWidgetComponent> Cursor;
	TSharedPtr<SMuseeReticle> CursorRing;
	/** The step's arc, for the beam. */
	TArray<FVector> ArcPoints;
	/** What the beam points at can be used: its hint, in place of the eyes'. */
	FText PointerHint;
	bool bStickAiming = false;

	struct FHandPinches
	{
		bool bTracked = false;
		bool bIndex = false;
		bool bMiddle = false;
		bool bRing = false;
		/** Between the thumb and index tips, in the world. */
		FVector PinchPoint = FVector::ZeroVector;
	};
	FHandPinches LeftPinches, RightPinches;

	UPROPERTY(Transient) TArray<TObjectPtr<AMuseeVRButton>> Buttons;
	TArray<FMuseeActionPrompt> ButtonPrompts;
	/** Which way the buttons were put out (yaw, degrees). */
	float ButtonsYaw = 0.f;

	bool bAiming = false;
	bool bGliding = false;
	bool bStickTurned = false;
	TOptional<FVector> StepTarget;
	FTimerHandle StepTimer;

	TSharedPtr<SMuseeReticle> Dot;
	TSharedPtr<STextBlock> HintLabel;
	TSharedPtr<SWidget> HintSign;
	TSharedPtr<SMuseePlacard> Placard;
	TSharedPtr<STextBlock> MessageLine;
	TSharedPtr<SWidget> MessageSign;
	TSharedPtr<SMuseeReticle> Ring;
	const FMuseeArtwork* ShownWork = nullptr;
	FString ShownHint;
	float HintAlpha = 0.f;
	FString ShownMessage;
	float MessageAlpha = 0.f;
	bool bLabelShown = false;
	bool bMessageShown = false;
	/** Recentred once the headset reports a pose (at BeginPlay it has none yet). */
	bool bRecentred = false;
	float InputLogTime = 0.f;
	float InputLogFor = 0.f;
	TMap<FName, bool> LoggedKeys;
	/** The view's exposure, to keep the signs as bright as on the desktop. */
	TSharedPtr<FMuseeVRExposure, ESPMode::ThreadSafe> Exposure;
};
