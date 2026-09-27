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

	/** The walking and striding speeds, and the take-off speed of a hop that lifts the feet one step's rise. */
	[[nodiscard]] static Musee::MetersPerSecond WalkSpeed();
	[[nodiscard]] static Musee::MetersPerSecond StrideSpeed();
	[[nodiscard]] static Musee::MetersPerSecond HopSpeed();

	/** The thing under the visitor's gaze and within reach, found anew each frame: what the use key would use. */
	[[nodiscard]] UObject* LookedAt() const { return Looked.Get(); }

	/** Looks along the view now, updating what is looked at and the overlay. */
	void Look();

	/** Uses the thing looked at, if it can be used now. */
	void UseLooked();

	UCameraComponent* Camera() const { return EyeCamera; }
	UVisitorEye* Eye() const { return VisitorEye; }
	UVisitorOverlay* Overlay() const { return VisitorOverlay; }

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

private:
	void BuildInput();
	void Walk(const FInputActionValue& Value);
	void Turn(const FInputActionValue& Value);
	void StrideOn();
	void StrideOff();
	void RunCommand(const TCHAR* Name);
	void AnotherHour();
	void OtherCity();
	void Leave();

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
};
