#pragma once

#include "CoreMinimal.h"
#include "MuseeStageDresser.h"

/**
 * Light's step in furnishing a stage while Studio makes it (stage dresser order 40): it reads the stage's lighting
 * schedule and places each luminaire at its mount, on the moving body it rides, aimed and operated as the schedule says.
 * Anything the schedule names that doesn't exist is a finding.
 */
namespace Musee::Light
{
	/** Light's place in the order of dressers: after Motion (10), Nature (20) and Art (30). */
	inline constexpr int32 DresserOrder = 40;

	/** The tag each placed luminaire carries: `musee.luminaire:<id>`. */
	inline constexpr const TCHAR* LuminaireTagPrefix = TEXT("musee.luminaire:");

	void DressStage(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings);
}
