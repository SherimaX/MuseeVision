#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MuseeUnits.h"

#include "VisitorBody.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UVisitorEye;
class UVisitorOverlay;
class FMuseeSetting;
struct FInputActionValue;

/**
 * The person in the museum: a body of human size (every measure from FMuseeHumanMeasure, none set here) that walks,
 * strides, hops and looks, with the eye at standing eye height. Looking at a thing within reach shows its caption and
 * what using it does; the use key uses it (IMuseeUsable). The hour key and the city key ask Time through its commands.
 * The body creates the Eye (exposure) and the overlay. Desktop controls: W A S D to walk, the mouse to look, the left
 * button or E to use, Shift to stride, Space to hop, H for another hour, C for the other city, Esc to leave.
 */
UCLASS(ClassGroup = (Musee))
class MUSEEVISITOR_API AVisitorBody : public ACharacter
{
	GENERATED_BODY()

public:
	AVisitorBody();

	/** The eye's point, in the world. */
	[[nodiscard]] FVector EyeLocation() const;

	/** How far from the eye the visitor can use a thing: one pace towards it and an arm's reach, down to the floor. */
	[[nodiscard]] static Musee::Meters UseDistance();

	/**
	 * The walking and striding speeds of the pace chosen now (FVisitorPace, the setting `visitor.pace`), and the take-off
	 * speed of a hop that lifts the feet one step's rise.
	 */
	[[nodiscard]] static Musee::MetersPerSecond WalkSpeed();
	[[nodiscard]] static Musee::MetersPerSecond StrideSpeed();
	[[nodiscard]] static Musee::MetersPerSecond HopSpeed();

	/** The thing under the visitor's gaze and within reach, found anew each frame: what the use key would use. */
	[[nodiscard]] UObject* LookedAt() const { return Looked.Get(); }

	/**
	 * What lies along a ray within reach of the visitor: the first usable thing (IMuseeUsable) and the first captioned
	 * thing (IMuseeCaptioned), each null if there is none. A thing answers the Visibility trace, by blocking or
	 * overlapping it, on its own component, a component it hangs from, or its actor.
	 */
	void FindAlong(const FVector& From, const FVector& Direction, UObject*& OutUsable, UObject*& OutCaptioned) const;

	/** Looks along the view now, updating what is looked at and the overlay. */
	void Look();

	/** Uses the thing looked at, if it can be used now. */
	void UseLooked();

	/** Announces that the body's controls are set up, so an adapter can add its own to them. */
	using FControlsReady = TMulticastDelegate<void(AVisitorBody&)>;
	FControlsReady& OnControlsReady() { return ControlsReady; }

	/** The name of the adapter that adopted the body, or none for the screen's own controls. */
	[[nodiscard]] FName Adapter() const { return AdapterName; }

	/** Runs a museum command and shows what it said on the overlay. */
	void RunCommand(const TCHAR* Name);

	/** The hour key and the city key. */
	void AnotherHour();
	void OtherCity();

	/** Striding, while the stride key is held. */
	void StrideOn();
	void StrideOff();

	/** Turns the body about the vertical, for adapters without a mouse. */
	void TurnBy(Musee::Degrees Angle);

	UCameraComponent* Camera() const { return EyeCamera; }
	UVisitorEye* Eye() const { return VisitorEye; }
	UVisitorOverlay* Overlay() const { return VisitorOverlay; }

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

private:
	void BuildInput();
	void Walk(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void Leave();
	/** Sets the body's speed to the chosen pace, walking or striding. */
	void FollowPace();
	void FollowPace(const FMuseeSetting& Changed);

	UPROPERTY(VisibleAnywhere, Category = "Visitor")
	TObjectPtr<UCameraComponent> EyeCamera;

	UPROPERTY(VisibleAnywhere, Category = "Visitor")
	TObjectPtr<UVisitorEye> VisitorEye;

	UPROPERTY(VisibleAnywhere, Category = "Visitor")
	TObjectPtr<UVisitorOverlay> VisitorOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Controls;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UInputAction>> Actions;

	TWeakObjectPtr<UObject> Looked;
	bool bStriding = false;
	FDelegateHandle PaceHandle;
	FControlsReady ControlsReady;
	FName AdapterName;
};
