#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "MuseeUsable.h"

#include "VisitorTestUsable.generated.h"

/** A usable volume for Visitor's tests: it offers one use and counts the times it is used. */
UCLASS(NotBlueprintable, HideDropdown)
class UVisitorTestUsable final : public UBoxComponent, public IMuseeUsable
{
	GENERATED_BODY()

public:
	virtual FMuseeUse Usage() const override
	{
		return FMuseeUse::Available(NSLOCTEXT("MuseeVisitorTests", "Verb", "Wake"), NSLOCTEXT("MuseeVisitorTests", "Caption", "A line of light runs round the rim."));
	}

	virtual void Use() override { ++Uses; }

	int32 Uses = 0;
};
