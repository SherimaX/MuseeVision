#pragma once

#include "ArtCastScan.h"
#include "CoreMinimal.h"
#include "Make/MakeReport.h"
#include "MuseeStageDresser.h"

namespace Musee::Studio
{
	struct FStageMakeOptions;

	/**
	 * Imports the scans a stage's works are cast from, before its pieces are made, since Art casts each work from its scan
	 * (Art's FArtCastScan, D-015: Art defines, Studio writes). Each glTF binary comes in through the engine's own glTF
	 * import (Interchange) as a static mesh at <Root>/Art/Casts/SM_Scan_<Scan> (Musee::ArtMaking::ScanPackage), which for
	 * the museum is where Art's piece source reads it. Only a scan whose file changed is imported again. A scan the import
	 * can't read is a failed package.
	 */
	void MakeCastScans(const FMuseeStageContext& Stage, const FStageMakeOptions& Options, FMakeReport& Report);

	/**
	 * Imports one scan as the static mesh at Package, unless the one there was imported from the same bytes by the same
	 * rules. Only the mesh comes in: no materials, textures or animation, no Nanite and no collision (the scan is the
	 * source a cast is made from, never placed itself), its normals as the scan gives them. Its package is written as every
	 * made asset is (the input hash in its metadata, a GUID derived from its name and inputs).
	 */
	void ImportScan(const FArtCastScan& Scan, const FString& Package, FMakeReport& Report);
}
