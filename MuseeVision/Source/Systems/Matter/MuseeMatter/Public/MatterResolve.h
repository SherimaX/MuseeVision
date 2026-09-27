#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;

/**
 * Finding the material a surface is made of, at making time (Form and Studio call it; nothing calls it per frame).
 *
 * A material is named by a palette key, a working and a history, which a stage's finish schedule gives each surface.
 * Its instance was made by Matter's make step (MuseeMatterMake) at /Game/Generated/Matter/MI_<Palette>_<Working>_<History>
 * (Musee::Matter::InstancePackage). A spec that was never made, or that names nothing, resolves to nullptr and is
 * logged to LogMuseeMatter: the caller reports it as a finding. There is never a default or fallback material.
 */
namespace Musee::Matter
{
	/** The material instance for a palette key, a working and a history, or nullptr. */
	MUSEEMATTER_API UMaterialInterface* Resolve(FName PaletteKey, FName Working, FName History);

	/**
	 * The material a stage's finish schedule gives a surface: the schedule is `<ScheduleFolder>/Finishes.json`
	 * (ScheduleFolder relative to Data, as in Stages/nympheas, or absolute). Nullptr, logged, when the schedule can't
	 * be read, names no such surface, or names a material that resolves to nothing.
	 */
	MUSEEMATTER_API UMaterialInterface* ResolveFinish(const FString& ScheduleFolder, FName Surface);
}
