#pragma once

#include "Commandlets/Commandlet.h"
#include "MatterMakeCommandlet.generated.h"

/**
 * Makes Matter's masters from their recipes, unattended, with the editor closed: -run=MatterMake.
 *
 * For each master it builds and saves the asset under /Game/Generated/Matter, compiles its shaders for Shader Model 6
 * (PCD3D_SM6, the museum's desktop platform) and reports its StateId and the BLAKE3 hash of the saved package, so two
 * runs can be compared byte for byte. It returns 1 if any master fails to validate, save or compile, and 0 otherwise.
 * It runs without an RHI (-nullrhi).
 */
UCLASS()
class UMatterMakeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMatterMakeCommandlet();

	virtual int32 Main(const FString& Params) override;
};
