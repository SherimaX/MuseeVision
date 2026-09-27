#pragma once

#include "ArtHang.h"
#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"

namespace Musee::ArtMaking
{
	/** A finding of Art's about a stage's hanging plan. */
	[[nodiscard]] FMuseeFinding ArtFinding(const FMuseeStageContext& Stage, const FString& Summary);

	/**
	 * The stage's hanging plan, if it has one: false when the stage keeps no Hang.json, or when it doesn't read or
	 * names another stage (a finding).
	 */
	[[nodiscard]] bool ReadStageHang(const FMuseeStageContext& Stage, FArtHang& OutHang, TArray<FMuseeFinding>& OutFindings);
}
