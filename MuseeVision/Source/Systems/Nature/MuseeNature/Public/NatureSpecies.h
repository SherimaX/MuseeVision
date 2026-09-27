#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "NatureOrgan.h"
#include "NatureTreeHabit.h"

#include "NatureSpecies.generated.h"

/**
 * A plant species as the museum grows it, one record in `Data/Nature/Species/`: its botanical measures (each a range
 * [least, most] a grown plant falls in, with its sources in the record), its colours as sRGB hex (`#3F6B2E`), and the
 * form that grows it: `water-lily` (Nymphaea: floating pads and flowers on the water), `iris` (fans of sword leaves and
 * flowers on stalks), `sedge` (a tussock of arching blades and fruiting culms), `forget-me-not` (low leafy stems and
 * coiled sprays of small flowers), `tree` (a trunk, orders of branches, leaves and flowers on the twigs, by its habit),
 * `bamboo` (a grove of jointed culms with branches and leaf sprays), `lotus` (round leaves and flowers held above the
 * water), `moss` (a carpet over the ground), `grass` (a sward of tufts), `herb` (a meadow flower or a bulb: basal
 * leaves and flowering stems), `hedge` (a clipped hedge along a line) and `vine` (a climber trained along its paths).
 *
 * What each measure means depends on the form: a lily's leaf length is its pad's radius and its reach the colony's
 * radius on the water; an iris's leaf length is its sword's length from the soil; a forget-me-not's is its stem's; a
 * lotus's is its leaf's radius; a moss's is its shoots' length, the carpet's depth. The museum shows its plants in early
 * summer, from Mangzhong to Xiazhi (June): what flowers then, and how much of the planting, is the floweringShare.
 */
USTRUCT()
struct MUSEENATURE_API FNatureSpecies
{
	GENERATED_BODY()

	/** The record's key (`nymphaea-alba`), which seeds its plants with the bed's key. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The scientific name a planting plan calls it by (`Nymphaea alba`). */
	UPROPERTY(meta = (MuseeRequired))
	FString Name;

	UPROPERTY()
	FString Common;

	/** The form that grows it, one of those above (`water-lily`, `tree`, `vine`...). */
	UPROPERTY(meta = (MuseeRequired))
	FName Form;

	/** The tissue of each organ the form grows. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<FNatureOrgan> Organs;

	/** How far one plant spreads from its crown: a lily colony's radius on the water, a clump's radius. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<float> ReachMeters;

	/** A leaf's length (a pad's radius for a water lily). */
	UPROPERTY(meta = (MuseeRequired))
	TArray<float> LeafLengthMeters;

	/** A leaf's width (unused by a water lily, whose pad is round). */
	UPROPERTY()
	TArray<float> LeafWidthMeters;

	/** How high a leaf is held above the water or soil (a lotus's leaves stand above the water on their stalks). */
	UPROPERTY()
	TArray<float> LeafHeightMeters;

	/** The plant's height: a tree's, a bamboo culm's, the shoots of a hedge left unclipped. */
	UPROPERTY()
	TArray<float> HeightMeters;

	/** A stem's radius at its foot: a bamboo culm's, a vine's stem and its canes. */
	UPROPERTY()
	TArray<float> StemRadiusMeters;

	/** How many plants (tufts, shoots) stand on a square metre of a cover. */
	UPROPERTY()
	TArray<float> PlantsPerSquareMeter;

	/** How many leaves (needles) a square metre of a clipped hedge's surface carries. */
	UPROPERTY()
	TArray<float> LeavesPerSquareMeter;

	/** How many leaves a metre of shoot carries (a vine's canes). */
	UPROPERTY()
	TArray<float> LeavesPerMeter;

	/** How many petals (tepals, rays) a flower has. */
	UPROPERTY()
	TArray<int32> PetalsPerFlower;

	/**
	 * How a flower's petals stand: `cup` (rising, a buttercup's, a tulip's), `flat` (spread, a mallow's, a narcissus's
	 * perianth), `reflexed` (swept back, a sneezeweed's rays) or `head` (a scabious's flat head of florets).
	 */
	UPROPERTY()
	FName FlowerShape;

	/** A narcissus's corona, as a share of the petals' length; zero for none. */
	UPROPERTY()
	float CoronaShare = 0.0f;

	/** A tree's habit, for the `tree` form. */
	UPROPERTY()
	FNatureTreeHabit Tree;

	/** A flower's size across. */
	UPROPERTY()
	TArray<float> FlowerSizeMeters;

	/** How high a flower is held above the soil (above the water for a water lily). */
	UPROPERTY()
	TArray<float> FlowerHeightMeters;

	/** How many shoots a plant raises: an iris's fans, a sedge's blades, a forget-me-not's stems. */
	UPROPERTY()
	TArray<int32> ShootsPerPlant;

	/** How many leaves a shoot carries: the swords of an iris's fan, the leaves up a forget-me-not's stem. */
	UPROPERTY()
	TArray<int32> LeavesPerShoot;

	/** How many flowers a flowering plant carries (a sedge's fruiting culms; a forget-me-not's florets in one spray). */
	UPROPERTY()
	TArray<int32> FlowersPerPlant;

	/** The share of plants in flower in the museum's season. */
	UPROPERTY()
	float FloweringShare = 0.0f;

	/** The leaves' greens, one drawn per leaf. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<FString> LeafColours;

	/** A young leaf's colour and an old one's (a lily's bronze new pad, its yellowing old one). */
	UPROPERTY()
	TArray<FString> LeafAgeColours;

	/** The colour at a leaf's edge or foot (a pad's red rim, an iris's brown sheath). */
	UPROPERTY()
	FString LeafEdgeColour;

	/** The colour a leaf pales to at its middle (a pad's centre, where its cuticle is youngest). */
	UPROPERTY()
	FString LeafMiddleColour;

	/** The flower's colours: its ground, its tip (or deeper vein), and a pale third where the form has one. */
	UPROPERTY()
	TArray<FString> FlowerColours;

	/** The flower's eye: stamens, a signal, a yellow throat. */
	UPROPERTY()
	TArray<FString> EyeColours;

	/** The stem's colours from its foot to its top. */
	UPROPERTY()
	TArray<FString> StemColours;

	/** Seed heads and fruiting spikes. */
	UPROPERTY()
	TArray<FString> SeedColours;

	UPROPERTY()
	FString Note;

	/** The palette key of an organ's tissue, or none if the record doesn't give it. */
	[[nodiscard]] FName MaterialOf(FName Organ) const
	{
		const FNatureOrgan* Found = Organs.FindByPredicate([Organ](const FNatureOrgan& O) { return O.Organ == Organ; });
		return Found ? Found->Material : NAME_None;
	}
};
