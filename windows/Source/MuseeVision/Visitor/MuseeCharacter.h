#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Visitor/MuseeInteractable.h"
#include "MuseeCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
struct FMuseeArtwork;

/**
 * The visitor (iOS/WalkController.swift): walking pace, eyes at 1.6 m, solid walls, stairs and
 * multi-level floors; the elevator and the lifting pond carry you (they are moving bases).
 * WASD and the mouse, or a gamepad. Look at a work for its glass placard; look at a thing and
 * click (or press E) to use it. Buttons for what can be done where you stand are on 1–4.
 * Graphics (UMuseeGraphics): F10 or O (Start) opens the settings card, F6 and F7 a lighter or heavier
 * preset, F8 the frame-rate readout; Esc closes the card if it is open, and otherwise quits.
 */
UCLASS()
class MUSEEVISION_API AMuseeCharacter : public ACharacter
{
	GENERATED_BODY()

	/** The visitor in a headset (the MuseeVisionVR module) drives the same look, prompts and use. */
	friend class AMuseeVRCharacter;

public:
	AMuseeCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	/** Feet on the floor, in world space (cm). */
	FVector FeetLocation() const;
	/** Eye position in world space (cm). */
	FVector EyeLocation() const;

	/** A short line of text for the visitor (e.g. "The car is on its way down"); empty clears it. */
	void SetMessage(const FText& Text);
	const FText& Message() const { return MessageText; }

	/** What the visitor is looking at, for the HUD. */
	const FMuseeArtwork* LookedAtWork() const { return LookedWork; }
	float PlacardAlpha() const { return PlacardFade; }
	const FText& LookHint() const { return Hint; }
	const TArray<FMuseeActionPrompt>& CurrentPrompts() const { return Prompts; }
	bool InPhotoMode() const { return bPhoto; }
	/** Enter photo mode (path tracing), if not already in it. */
	void StartPhotoMode() { if (!bPhoto) { TogglePhoto(); } }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Musee")
	TObjectPtr<UCameraComponent> Camera;

	/** Walking pace (cm/s): the old brisk pace (the user: 140 felt slow); hold Shift for twice it. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float WalkSpeed = 260.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float BriskSpeed = 520.f;

	/** Space: a small hop, about 35 cm (JumpZVelocity² / 2g). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float JumpVelocity = 262.f;

	/** How far you can use things and read placards from (cm). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float ReachDistance = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float MouseSensitivity = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float GamepadLookRate = 110.f;

private:
	void BuildInput();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void LookGamepad(const FInputActionValue& Value);
	void Use();
	void BriskOn();
	void BriskOff();
	void Prompt(int32 Slot);
	/** Photo mode: the path tracer converges on the view while you hold still. */
	void TogglePhoto();
	/** A screenshot at twice the window's resolution, to Saved/Screenshots. */
	void Snapshot();
	/** Step the sky's time: now → 10:00 → 16:00 → 21:00 → now (musee.Hour). */
	void NextHour();
	/** Beijing ↔ New York: the museum's clock and sky (MuseeClock). */
	void ToggleCity();
	void Quit();
	void ToggleSettings();
	void LighterPreset();
	void HeavierPreset();
	/** A gamepad's one button for it: heavier, and round again from Cinematic to Performance. */
	void CyclePreset();
	void ToggleReadout();

	void UpdateLook(float DeltaSeconds);
	void UpdatePrompts();
	/** The work a hit belongs to, from the "work:<id>" tag on the actor or one it is attached to. */
	const FMuseeArtwork* WorkFor(const AActor* Actor) const;
	/** The interactable a hit belongs to (the actor or one it is attached to). */
	IMuseeInteractable* InteractableFor(AActor* Actor) const;

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> Context;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookPadAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> UseAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> BriskAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TArray<TObjectPtr<UInputAction>> PromptActions;
	UPROPERTY(Transient) TObjectPtr<UInputAction> PhotoAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SnapshotAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> HourAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> CityAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> QuitAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SettingsAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LighterAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> HeavierAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> CycleAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ReadoutAction;
	int32 HourStep = 0;
	bool bPhoto = false;

	FHitResult LookHit;
	TWeakObjectPtr<AActor> LookedActor;
	const FMuseeArtwork* LookedWork = nullptr;
	const FMuseeArtwork* DwellWork = nullptr;
	float Dwell = 0.f;
	float PlacardFade = 0.f;
	FText Hint;
	FText MessageText;
	TArray<FMuseeActionPrompt> Prompts;
	/** The provider of each prompt, in the same order. */
	TArray<TWeakObjectPtr<AActor>> PromptOwners;
	TArray<TWeakObjectPtr<AActor>> PromptProviders;
	float ProviderRefresh = 0.f;
};
