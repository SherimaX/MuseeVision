#pragma once

#include "CoreMinimal.h"
#include "MatterFilm.h"
#include "MatterQuantity.h"
#include "MatterTriple.h"
#include "MuseeKey.h"

#include "MatterHistory.generated.h"

/**
 * What time has done to a surface, as Matter's builders read it: a history regime for one master (`stone.worn`,
 * `metal.tarnished`, `glass.soiled`), which a finish names by its last word (`worn`).
 *
 * A regime gathers the mechanisms the research records describe (`Data/Matter/Histories/<family>.<mechanism>.json`,
 * listed in Mechanisms) into the state and rates History.ush's law reads: how old the face is, how many passages cross
 * it, how much sky reaches it, how deposit settles and is washed off, and what abrasion carries it toward. Its typed
 * record lives at `Data/Matter/Shading/Histories/<master>.<regime>.json`.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterHistory
{
	GENERATED_BODY()

	/** `musee.matter.shading.history/1`. */
	UPROPERTY(meta = (MuseeRequired))
	FString Schema;

	/** `<master>.<regime>`: stone.worn. A layer working's regimes are keyed by the working: gilt.worn. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	UPROPERTY(meta = (MuseeRequired))
	FString Name;

	/** The research histories the regime gathers. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<FMuseeKey> Mechanisms;

	/** Years since the face was worked (unit year). */
	UPROPERTY()
	FMatterQuantity Age;

	/** Feet or hands passing over the face each year (unit 1/year). */
	UPROPERTY()
	FMatterQuantity PassagesPerYear;

	/** Fraction of free-field precipitation that reaches the face (unit 1). */
	UPROPERTY()
	FMatterQuantity SkyExposure;

	/** Fraction of its water capacity the face holds whatever the weather, kept wet by standing water beside it (unit 1). */
	UPROPERTY()
	FMatterQuantity KeptWet;

	/** First-order rate at which deposit covers bare area (unit 1/year). */
	UPROPERTY()
	FMatterQuantity Deposition;

	/** First-order rate at which a full rain wash clears deposit (unit 1/year). */
	UPROPERTY()
	FMatterQuantity WashRemoval;

	/** Diffuse albedo of the deposit the place lays down. */
	UPROPERTY()
	FMatterTriple SoilAlbedo;

	/** The steady roughness abrasion carries the face toward (unit 1). */
	UPROPERTY()
	FMatterQuantity WornRoughness;

	/** The steady haziness of the worn face (unit 1). */
	UPROPERTY()
	FMatterQuantity WornHaziness;

	/** Passages that carry the face 63 % of the way to its worn state; 0 for a face that never wears (unit count). */
	UPROPERTY()
	FMatterQuantity WearPassages;

	/** The fraction of a metal the tarnish film covers after this history (unit 1). */
	UPROPERTY()
	FMatterQuantity TarnishCover;
};
