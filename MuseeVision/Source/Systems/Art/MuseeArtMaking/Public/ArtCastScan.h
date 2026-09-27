#pragma once

#include "CoreMinimal.h"
#include "MuseeStageDresser.h"

/**
 * A scan a stage's works in the round are cast from, as Studio imports it (D-015: Art defines, Studio writes): the
 * collection's glTF mesh of the work (assets/classical/<scan>.glb), brought in by the engine's own glTF import
 * (Interchange) as a static mesh at ScanPackage, from which Art's piece source casts the work at its true height, on its
 * carrier and in its medium. The collection's `.mvm` meshes (the old museum's format) are read by Art itself and need
 * no import.
 */
struct FArtCastScan
{
	/** The scan's name: its mesh's file name without its extension (`augustus-prima-porta`). */
	FString Scan;

	/** Its mesh, a full path in the collection (Musee::Art::SourcePath). */
	FString MeshPath;
};

namespace Musee::ArtMaking
{
	/** The root the museum's made assets live under, where Art's piece source finds the imported scans. */
	inline constexpr const TCHAR* MuseumAssetsRoot = TEXT("/Game/Generated");

	/** The package a scan is imported as, under a make's root: <Root>/Art/Casts/SM_Scan_<Scan in PascalCase>. */
	[[nodiscard]] MUSEEARTMAKING_API FString ScanPackage(const FString& Root, const FString& Scan);

	/**
	 * The glTF scans a stage's hanging plan casts works from, each once, in the plan's order. A plan that doesn't read
	 * has none; the piece source reports why.
	 */
	[[nodiscard]] MUSEEARTMAKING_API TArray<FArtCastScan> CastScans(const FMuseeStageContext& Stage);
}
