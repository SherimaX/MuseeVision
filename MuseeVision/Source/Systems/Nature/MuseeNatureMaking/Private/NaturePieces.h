#pragma once

#include "CoreMinimal.h"
#include "FormMadePiece.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"
#include "NaturePlanting.h"
#include "NatureSpecies.h"
#include "NatureWater.h"

namespace Musee::NatureMaking
{
	/** Nature's pieces are gathered after the stage's own (order 20): its water, its ground and its plants. */
	inline constexpr int32 PieceSourceOrder = 20;

	/**
	 * Nature's piece source: reads the stage's Planting.json (a stage without one grows nothing) and makes the surface of
	 * each body of water, the ground of each bed that gives one, and the plants of each bed, as pieces on the bodies that
	 * carry them. Every plant is drawn from the stream of its bed, species and place, so a make is the same every time.
	 */
	void GatherNature(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings);

	/** Every species record in Data/Nature/Species, or why they can't be read. */
	[[nodiscard]] TValueOrError<TArray<FNatureSpecies>, FString> ReadSpecies();

	/** What is wrong with a species record: an unknown form, a missing measure or colour, an organ without its tissue. */
	[[nodiscard]] TArray<FString> SpeciesFaults(const FNatureSpecies& Species);

	/**
	 * A piece's frame from an anchor's frame in the stage: the anchor's own frame when the piece stands still, or the
	 * anchor seen from its body's anchor when a body carries it. Fails, saying why, if the body isn't a Body anchor.
	 */
	[[nodiscard]] bool PieceFrame(const FMuseeStageContext& Stage, const FMuseeFrame& AnchorFrame, const FMuseeKey& Body, FMuseeFrame& OutFrame, FString& OutWhy);

	/**
	 * The surface of a body of water: its outline at the level of its datum, as one flat piece (non-Nanite, colliding
	 * with nothing) in the entry's own material. Fails, saying why, on an anchor or body the stage doesn't declare.
	 */
	[[nodiscard]] bool MakeWaterPiece(const FMuseeStageContext& Stage, const FNatureWater& Water, FFormMadePiece& OutPiece, FString& OutWhy);

	/** A bed's region in its own frame: its outline, and the water lying wholly inside it as holes, as plan points. */
	struct FBedRegion
	{
		TArray<FVector2D> Outline;
		TArray<TArray<FVector2D>> Holes;
	};

	/** An outline as plan points of the stage: a polygon as it is written, an ellipse divided about its anchor. */
	[[nodiscard]] TArray<FVector2D> PlanPolygon(const FNatureOutline& Outline, const FMuseeFrame& Anchor);

	/** A bed's region, in its frame; empty when the entry gives no outline. */
	[[nodiscard]] FBedRegion RegionOf(const FMuseeStageContext& Stage, const FNaturePlanting& Plan, const FNatureBed& Entry, const FMuseeFrame& Bed);

	/**
	 * A bed's ground: its region as a flat face at the soil (the bed anchor's height), in the ground's own material,
	 * walkable. Fails, saying why, on a ground without its working and history, or a bed without an outline.
	 */
	[[nodiscard]] bool MakeGroundPiece(const FMuseeStageContext& Stage, const FNatureBed& Entry, const FBedRegion& Region, const FMuseeFrame& Bed,
		FFormMadePiece& OutPiece, FString& OutWhy);

	/**
	 * A carpet (a moss, or a sward mown close) over a bed's region, or over the top of the stage's piece the bed lies on
	 * (one of StagePieces, the pieces made so far): its upper surface, the shoots' length above the soil (the mown height,
	 * for a sward), in its leaves' tissue, its greens varying over it, colliding with nothing.
	 */
	[[nodiscard]] bool MakeCarpetPiece(const FMuseeStageContext& Stage, const FNatureBed& Entry, const FNatureSpecies& Species, const FBedRegion& Region,
		const FMuseeFrame& Bed, TConstArrayView<FFormMadePiece> StagePieces, FFormMadePiece& OutPiece, FString& OutWhy);
}
