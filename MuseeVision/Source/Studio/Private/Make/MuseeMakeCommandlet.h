#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"

#include "MuseeMakeCommandlet.generated.h"

/**
 * The make, run with the editor closed: UnrealEditor-Cmd <project> -run=MuseeMake. It makes Studio's proof stage and
 * its museum level, logs the make report, and returns 0 when every package was made or left unchanged.
 */
UCLASS()
class UMuseeMakeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMuseeMakeCommandlet();

	virtual int32 Main(const FString& Params) override;
};
