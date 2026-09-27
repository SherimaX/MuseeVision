#pragma once

#include "CoreMinimal.h"
#include "MatterConductor.h"
#include "MatterFilm.h"
#include "MatterMedium.h"
#include "MatterQuantity.h"
#include "MuseeKey.h"

#include "MatterWorking.generated.h"

/**
 * How a substance was worked, as Matter's builders read it: the face a craft leaves (honed, trowelled, brushed, cast),
 * a layer it lays over another working (gilt), a film it grows (patinated), a make-up (a laminated pane) or a treatment
 * (a pond's dye).
 *
 * The research record at `Data/Matter/Workings/<key>.json` is the evidence; this typed record, at
 * `Data/Matter/Shading/Workings/<key>.json`, draws from it the numbers a master reads, each with its source.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterWorking
{
	GENERATED_BODY()

	/** `musee.matter.shading.working/1`. */
	UPROPERTY(meta = (MuseeRequired))
	FString Schema;

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	UPROPERTY(meta = (MuseeRequired))
	FString Name;

	/** The research record the numbers come from, relative to Data/Matter. */
	UPROPERTY(meta = (MuseeRequired))
	FString Record;

	/** `face` for a working that finishes the substance's own face; `layer` for one that lays another material over a
	 *  host working (gilt: leaf over the host), which the master carries as a Substrate vertical layer. */
	UPROPERTY(meta = (MuseeRequired))
	FName Kind;

	/** The Slab roughness of the worked face; along the grain on a brushed face (unit 1). */
	UPROPERTY()
	FMatterQuantity Roughness;

	/** The Slab roughness across the grain; equal to Roughness on a face with no grain (unit 1). */
	UPROPERTY()
	FMatterQuantity RoughnessAcross;

	/** The weight of the face's wide second specular lobe (unit 1). */
	UPROPERTY()
	FMatterQuantity Haziness;

	/** Mean texture depth: the water a face holds as a film before it is continuous (unit mm). */
	UPROPERTY()
	FMatterQuantity TextureDepth;

	/** Amplitude of the worked face's relief (unit mm). */
	UPROPERTY()
	FMatterQuantity Relief;

	/** A layer working's material: gold leaf's reflection. */
	UPROPERTY()
	FMatterConductor Leaf;

	/** A layer working's host: for each master family it is laid on, the working of the face beneath it. */
	UPROPERTY()
	TMap<FString, FMuseeKey> Hosts;

	/** A film the working lays on a metal (a statuary patina). */
	UPROPERTY()
	FMatterFilm Film;

	/** The glass a glazing make-up puts in the light's path (unit mm). */
	UPROPERTY()
	FMatterQuantity PaneThickness;

	/** What a treatment adds to a medium: a dye's absorption. */
	UPROPERTY()
	FMatterMedium Medium;
};
