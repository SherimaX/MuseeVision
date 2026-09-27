#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "MuseeUnits.h"

/**
 * The visitor's pace: how fast the body walks, and how fast it strides while the stride key is held. The visitor
 * chooses it with the setting `visitor.pace` (its command is `musee.visitor.set-pace`), which Visitor registers when
 * its module starts:
 * - `as-built`, the default: the feel of the as-built museum, a brisk 2.6 m/s and 5.2 m/s striding;
 * - `physical`: a real person's gallery walk, 1.35 m/s, striding at the walk-to-run transition, 2.0 m/s.
 */
struct MUSEEVISITOR_API FVisitorPace
{
	/** The walking speed. */
	Musee::MetersPerSecond Walk;
	/** The striding speed. */
	Musee::MetersPerSecond Stride;

	/** The setting's key, `visitor.pace`. */
	[[nodiscard]] static FMuseeKey SettingKey();

	/** The setting's choices. */
	[[nodiscard]] static FName AsBuilt();
	[[nodiscard]] static FName Physical();

	/** The pace a choice names; the default, as-built, for any other name. */
	[[nodiscard]] static FVisitorPace Of(FName Choice);

	/** The pace chosen now: the setting's, or the default while the setting isn't registered. */
	[[nodiscard]] static FVisitorPace Chosen();

	/** Registers the setting (Visitor's module startup) and unregisters it (its shutdown). */
	static void Register();
	static void Unregister();
};
