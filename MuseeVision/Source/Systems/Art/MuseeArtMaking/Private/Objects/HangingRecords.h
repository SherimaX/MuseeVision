#pragma once

#include "CoreMinimal.h"

#include "HangingRecords.generated.h"

/**
 * How Art makes the fittings of a Chinese hanging scroll (`Data/Art/Scrolls.json`), the parts every mount shares
 * whatever its proportions: the mount's thickness (the silk, or the painting, on its paper backing), the half-round top
 * stick (tiangan) the heaven is folded over, the roller (dizhou) the earth is wound on and its knobs (zhoutou), and the
 * cord the scroll hangs by, tied to the stick.
 */
USTRUCT()
struct FArtScrollFittings
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Scrolls.json");

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double MountMeters = 0.0;

	/** The top stick: a half-round, its flat against the mount's back, of this radius. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double StickRadiusMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double RollerRadiusMeters = 0.0;

	/** Each knob: a turned end of this radius, standing this far past the mount's edge. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double KnobRadiusMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double KnobLengthMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double CordRadiusMeters = 0.0;

	/** Where the cord is tied on the stick: this share of the stick's length in from each end. */
	UPROPERTY(meta = (MuseeRequired))
	double CordEyeShare = 0.0;
};

/**
 * How Art makes a painting on a wooden panel (`Data/Art/Panel.json`): the board, the ground and paint laid on it, and
 * the cradle glued to its back to hold it flat, members along the grain with slides running across them through their
 * slots. The grain runs along the panel's longer side, as panel makers cut the board.
 */
USTRUCT()
struct FArtPanelConstruction
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Panel.json");

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double BoardMeters = 0.0;

	/** The gesso ground and the paint over it. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double GroundMeters = 0.0;

	/** The cradle's fixed members, along the grain: their depth off the board, their width, and the most they stand apart. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double CradleDepthMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double MemberWidthMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double MemberSpacingMeters = 0.0;

	/** The slides across the grain, flush with the cradle's back: their width, depth, and the most they stand apart. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double SlideWidthMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double SlideDepthMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double SlideSpacingMeters = 0.0;

	/** How far in from the panel's edges the outermost members and slides stand. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double EdgeMeters = 0.0;
};

/** A palette finish a record names directly: the entry, its working and its history. */
USTRUCT()
struct FArtFinishSpec
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FName Palette;

	UPROPERTY(meta = (MuseeRequired))
	FName Working;

	UPROPERTY(meta = (MuseeRequired))
	FName History;
};

/**
 * A bound book as it lies open (`Data/Art/Books.json`, one record a work): how many leaves its text block holds and how
 * thick the block is, its boards' thickness and how far they stand past the leaves (the square), what its covers are
 * covered in and what its leaves' edges are. The opening it lies open at (the plan's `opening`) shares the block
 * between its two sides.
 */
USTRUCT()
struct FArtBook
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Books.json");

	UPROPERTY(meta = (MuseeRequired))
	FString Work;

	UPROPERTY(meta = (MuseeRequired))
	int32 Leaves = 0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double BlockMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double BoardMeters = 0.0;

	UPROPERTY(meta = (Units = "Meters"))
	double SquareMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	FArtFinishSpec Cover;

	UPROPERTY(meta = (MuseeRequired))
	FArtFinishSpec Edges;
};
