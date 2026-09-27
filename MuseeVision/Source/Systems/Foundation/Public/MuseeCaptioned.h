#pragma once

#include "CoreMinimal.h"
#include "MuseeCaption.h"
#include "UObject/Interface.h"

#include "MuseeCaptioned.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UMuseeCaptioned : public UInterface
{
	GENERATED_BODY()
};

/**
 * A thing that carries a caption: a work, a fitting, a mechanism. Visitor's hands find it by gaze or beam and the
 * overlay shows its caption, without either knowing which system owns the thing.
 */
class MUSEEFOUNDATION_API IMuseeCaptioned
{
	GENERATED_BODY()

public:
	/** The caption to show for this thing now. */
	[[nodiscard]] virtual FMuseeCaption Caption() const = 0;
};
