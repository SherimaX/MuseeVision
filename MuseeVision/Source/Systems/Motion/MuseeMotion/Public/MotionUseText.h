#pragma once

#include "CoreMinimal.h"
#include "MuseeText.h"

#include "MotionUseText.generated.h"

/**
 * What a mechanism's control says to the visitor for one of its sequences: the verb the hands offer (`Raise`) and a
 * short sentence saying what will happen (`Raise the lily pond`). A stage writes it in its Mechanisms.json; without
 * it the control offers the joint's own verb (Raise and Lower for a vertical lift, Open and Close otherwise).
 */
USTRUCT()
struct MUSEEMOTION_API FMotionUseText
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeText Verb;

	UPROPERTY()
	FMuseeText Caption;
};
