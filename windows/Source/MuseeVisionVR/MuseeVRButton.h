#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Visitor/MuseeInteractable.h"
#include "MuseeVRButton.generated.h"

class UWidgetComponent;

/**
 * One of the buttons for where you stand (the elevator panel, the pond), floating in front of the
 * visitor in a headset: look at it and pinch. The desktop's [1] CALL THE CAR, as a thing in the room.
 */
UCLASS(NotPlaceable)
class AMuseeVRButton : public AActor, public IMuseeInteractable
{
	GENERATED_BODY()

public:
	AMuseeVRButton();

	/** The prompt this button runs, and the actor that offered it. */
	void SetPrompt(const FMuseeActionPrompt& InPrompt, AActor* InProvider);
	const FMuseeActionPrompt& GetPrompt() const { return Prompt; }

	// IMuseeInteractable
	virtual void Interact(AMuseeCharacter* Visitor, const FHitResult& Hit) override;
	virtual FText InteractHint(const AMuseeCharacter* Visitor, const FHitResult& Hit) const override { return Prompt.Title; }

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UWidgetComponent> Sign;

private:
	FMuseeActionPrompt Prompt;
	TWeakObjectPtr<AActor> Provider;
};
