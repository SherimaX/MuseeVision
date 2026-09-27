#pragma once

#include "Commandlets/Commandlet.h"
#include "MuseeMatterMakeCommandlet.generated.h"

/**
 * Matter's make step, unattended, with the editor closed: -run=MuseeMatterMake.
 *
 * It makes MPC_Moment, every master (compiled for PCD3D_SM6, the museum's desktop platform, without an RHI) and every
 * palette finish's instance (Musee::Matter::MakeMatter), reports each asset's size, BLAKE3 hash and whether it was
 * written, and returns 1 if anything failed to validate, fill, save or compile, 0 otherwise. A second run with nothing
 * changed writes nothing. It runs with -nullrhi.
 */
UCLASS()
class UMuseeMatterMakeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMuseeMatterMakeCommandlet();

	virtual int32 Main(const FString& Params) override;
};
