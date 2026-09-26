#pragma once

#include "CoreMinimal.h"
#include "MuseeUse.h"
#include "UObject/Interface.h"

#include "MuseeUsable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UMuseeUsable : public UInterface
{
	GENERATED_BODY()
};

/**
 * A thing the visitor can use: a mechanism's control, a door handle, a bench. Visitor's hands target it and offer its
 * use; the owner (Motion, Art, Site) decides what the use does. Neither side sees the other.
 */
class MUSEEFOUNDATION_API IMuseeUsable
{
	GENERATED_BODY()

public:
	/** How the thing can be used now. */
	[[nodiscard]] virtual FMuseeUse Usage() const = 0;

	/** Uses the thing. Does nothing while its use isn't available. */
	virtual void Use() = 0;
};
