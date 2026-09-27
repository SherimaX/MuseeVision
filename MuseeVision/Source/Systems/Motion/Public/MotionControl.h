#pragma once

#include "Components/SphereComponent.h"
#include "CoreMinimal.h"
#include "MuseeUsable.h"

#include "MotionControl.generated.h"

class UMotionMechanism;

/**
 * The part of a machine the visitor works: a push-plate, a handle, the golden iris on the pond's coping. It is a small
 * sensed volume at the stage's Use anchor that the visitor's hands find by sight (it answers the Visibility channel
 * and nothing else, so it stops neither the body nor the camera). Its use and its words are its mechanism's. A
 * control on a moving body rides with it.
 */
UCLASS(ClassGroup = Musee)
class MUSEEMOTION_API UMotionControl : public USphereComponent, public IMuseeUsable
{
	GENERATED_BODY()

public:
	/**
	 * The radius of the sensed volume: an open hand's span (about 0.2 m, the hand breadth and length of the adult
	 * hand in Pheasant and Haslegrave, Bodyspace, 3rd ed., table 10.1) made a sphere, so a control is found as
	 * easily as a hand finds a fitting.
	 */
	static constexpr double RadiusMeters = 0.12;

	UMotionControl();

	/** Joins the control to the mechanism it works. Making time only. */
	void Bind(UMotionMechanism* InMechanism);

	[[nodiscard]] UMotionMechanism* Mechanism() const { return BoundMechanism; }

	virtual FMuseeUse Usage() const override;
	virtual void Use() override;

private:
	UPROPERTY()
	TObjectPtr<UMotionMechanism> BoundMechanism;
};
