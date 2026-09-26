#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MuseeInteractable.generated.h"

class AMuseeCharacter;

UINTERFACE(MinimalAPI, BlueprintType)
class UMuseeInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Something you can look at and click (or press E) to use: the golden lily, a rack, the elevator
 * car, the chicken cup. The iPhone's tap; the Vision Pro's gaze and pinch.
 */
class MUSEEVISION_API IMuseeInteractable
{
	GENERATED_BODY()

public:
	/** Whether it can be used now, from where the visitor stands. */
	virtual bool CanInteract(const AMuseeCharacter* Visitor, const FHitResult& Hit) const { return true; }

	/** Use it. */
	virtual void Interact(AMuseeCharacter* Visitor, const FHitResult& Hit) = 0;

	/** Short hint shown at the centre of the view while it is looked at ("Wake the lily"). */
	virtual FText InteractHint(const AMuseeCharacter* Visitor, const FHitResult& Hit) const { return FText::GetEmpty(); }
};

/** Buttons offered where the visitor stands (the elevator panel), like the iPhone's action prompts. */
struct FMuseeActionPrompt
{
	FName Id;
	FText Title;
	/** Key shown beside it: 1, 2, 3… */
	int32 Slot = 0;
};

UINTERFACE(MinimalAPI)
class UMuseePromptProvider : public UInterface
{
	GENERATED_BODY()
};

class MUSEEVISION_API IMuseePromptProvider
{
	GENERATED_BODY()

public:
	/** Prompts for the visitor where they stand now (empty if none). */
	virtual void GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const = 0;
	/** Run a prompt chosen by the visitor. */
	virtual void RunPrompt(AMuseeCharacter* Visitor, FName Id) = 0;
};
