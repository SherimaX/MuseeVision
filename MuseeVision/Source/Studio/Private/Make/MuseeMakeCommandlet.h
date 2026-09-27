#pragma once

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"

#include "MuseeMakeCommandlet.generated.h"

/**
 * The make, run with the editor closed: UnrealEditor-Cmd <project> -run=MuseeMake -Stage=<stage|site|all>. It makes a
 * stage's level, the museum level, or every stage of the site plan and then the museum level, logs the make report,
 * and returns 0 when every package was made or left unchanged and nothing was found wrong, 1 when something was, and 2
 * when it was asked for something that isn't a stage.
 */
UCLASS()
class UMuseeMakeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMuseeMakeCommandlet();

	virtual int32 Main(const FString& Params) override;
};
