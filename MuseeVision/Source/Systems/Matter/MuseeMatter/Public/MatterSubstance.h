#pragma once

#include "CoreMinimal.h"
#include "MatterConductor.h"
#include "MatterDielectric.h"
#include "MatterFibre.h"
#include "MatterFilm.h"
#include "MatterMedium.h"
#include "MatterSheet.h"
#include "MatterStructure.h"
#include "MatterTissue.h"
#include "MatterWood.h"
#include "MuseeKey.h"

#include "MatterSubstance.generated.h"

/**
 * What a thing in the museum is made of, as Matter's builders read it: the real substance (travertine, lime plaster,
 * the one bronze, low-iron glass, still water) and the numbers its master material needs, each with its unit and
 * source.
 *
 * The research record at `Data/Matter/Substances/<key>.json` is the evidence: how the substance formed, its measured
 * optics, its physics and its sources, in the schema of Docs/Matter/Schema.md. This typed record, at
 * `Data/Matter/Shading/Substances/<key>.json`, draws from it the numbers a master reads, naming the research field each
 * came from (grade `record`), how it was derived, or why no source measures it yet (grade `unverified`). It holds only
 * the blocks its master reads: a stone its dielectric optics and structure, a metal its reflection and its tarnish, a
 * glass or water its medium, a wood its figure, a cloth its fibre, a thin sheet its transmittance, a living tissue its
 * cuticle and transmission.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterSubstance
{
	GENERATED_BODY()

	/** `musee.matter.shading.substance/1`. */
	UPROPERTY(meta = (MuseeRequired))
	FString Schema;

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** English display text; the museum shows FText, never this. */
	UPROPERTY(meta = (MuseeRequired))
	FString Name;

	/** The D-014 family: stone, white, bronze, oak, accent, glass, water or living. */
	UPROPERTY(meta = (MuseeRequired))
	FString Family;

	/** For an accent, the core family it ties back to. */
	UPROPERTY()
	FString AccentOf;

	/** The research record the numbers come from, relative to Data/Matter. */
	UPROPERTY(meta = (MuseeRequired))
	FString Record;

	/** The master that shades it: stone, marble, white, metal, glass, water, paint, wood, cloth, sheet or living
	 *  (Musee::Matter::Masters). */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Master;

	UPROPERTY()
	FMatterDielectric Dielectric;

	UPROPERTY()
	FMatterStructure Structure;

	UPROPERTY()
	FMatterConductor Conductor;

	/** The film the substance's own history grows on it (a copper alloy's tarnish). */
	UPROPERTY()
	FMatterFilm Tarnish;

	UPROPERTY()
	FMatterMedium Medium;

	/** A wood's growth rings and rays. */
	UPROPERTY()
	FMatterWood Wood;

	/** A textile's fibres. */
	UPROPERTY()
	FMatterFibre Fibre;

	/** A thin scattering sheet's transmittance. */
	UPROPERTY()
	FMatterSheet Sheet;

	/** A living tissue's cuticle and transmission. */
	UPROPERTY()
	FMatterTissue Tissue;
};
