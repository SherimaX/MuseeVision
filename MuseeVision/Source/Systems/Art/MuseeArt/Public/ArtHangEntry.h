#pragma once

#include "ArtAfter.h"
#include "ArtCarrier.h"
#include "ArtScanPlacement.h"
#include "ArtScrollMount.h"
#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "ArtHangEntry.generated.h"

/**
 * One object in a stage's hanging plan: which work (its id in the catalogue that owns it, named by `source`, a path
 * from the repository's root), the anchor it is set at, where (`along` the anchor's x to its start edge, its lower edge
 * `height` above the floor), its real size, its support and frame, and its care class (a key of Data/Art/Care.json).
 *
 * A work split across several objects (the Orangerie's willows, Monet's Rouen canvases, Albion's twelve Tristram
 * panels, Chenghuai's twelve stones) has one entry per object, sharing `work` and each with its own `canvas` index.
 * Without an `image` the canvases carry the work's one image side by side in index order; with one, each shows its own.
 *
 * Three kinds of entry are not the museum's own original at its own size:
 * - `"built": "form"`: the stage builds the work itself (a screen wall's heart, stones in a wall, furniture). Art
 *   makes nothing of it and adds only its caption and care class to the pieces the stage made for it.
 * - `"reproduction": "<kind>"` (a key of Care.json's reproductions, `exhibition-facsimile`): what hangs is a copy of
 *   the original, which stays with its holder. The care class stays the original's.
 * - `"after": {...}`: the work is made at another size than the original's, which it records.
 *
 * The rest of the fields place and furnish works Art makes in later rounds (scrolls, cases, casts in the round),
 * written as the stage's sheet gives them. A size's dimension written 0 is the object's own (a scan's width, a grown
 * rock's), measured from it.
 */
USTRUCT()
struct MUSEEART_API FArtHangEntry
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FString Work;

	UPROPERTY(meta = (MuseeRequired))
	FString Source;

	/** The anchor the work is set at: a Wall, a Glazing light, a Mount, a case's bed. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Anchor;

	/** The distance along the anchor's x (its arc length on a curved wall) from the anchor to the work's start edge. */
	UPROPERTY(meta = (Units = "Meters"))
	float Along = 0.0f;

	/** The work's lower edge above the floor, which the anchor's origin stands on (above the light's sill for glass). */
	UPROPERTY(meta = (Units = "Meters"))
	float Height = 0.0f;

	/** The work's width and height [w, h] in metres, its real size as it is shown. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Size;

	/** `canvas`, `glass`, `paper`, `panel`, `silk`, `cast`, `stone`...: what the work is made on or of. */
	UPROPERTY(meta = (MuseeRequired))
	FName Support;

	/**
	 * `none`, or the surface in the stage's Finishes.json whose finish the work's frame (or a scroll's mount) takes
	 * (`salon.frame.gilt`).
	 */
	UPROPERTY()
	FName Frame;

	/** Which of the work's objects this is, in the plan's numbering; a work of one object leaves it 0. */
	UPROPERTY()
	int32 Canvas = 0;

	UPROPERTY(meta = (MuseeRequired))
	FName Care;

	UPROPERTY()
	FString Note;

	/** `form` when the stage builds the work itself; empty for a work Art makes. */
	UPROPERTY()
	FName Built;

	/** The kind of reproduction that hangs in the original's place (a key of Care.json's reproductions), or empty. */
	UPROPERTY()
	FName Reproduction;

	/** The original, when the work is shown at another size than its. */
	UPROPERTY()
	FArtAfter After;

	/** The image this object shows, by its file's name without extension, when it is not a share of the work's one image. */
	UPROPERTY()
	FString Image;

	/**
	 * The surface in the stage's Finishes.json the work stands on or is made in: the glass a stained-glass panel is
	 * fired in (`albion.band.glass`), a transparency's plate (`hall-of-light.autochrome.plate`), the stone of a built
	 * work, a cast's carrier, a case's lining.
	 */
	UPROPERTY()
	FName Wall;

	/**
	 * What the object is, where its support doesn't say: `hanging-scroll`, `handscroll`, `dish`, `vase`, `book` (an open
	 * book: its opening whole, or its verso and recto as a work's objects 0 and 1), `leaf` (a loose sheet lying flat),
	 * `stained-glass`.
	 */
	UPROPERTY()
	FName Kind;

	/** Which part of a work of several parts this is (`catalogue`, `charivari`, `handling-copy`). */
	UPROPERTY()
	FName Part;

	/**
	 * The work's centre in the stage's frame [x, y, z], as the sheet computed it: a cross-check of a work on a wall; where an
	 * object in the round stands on its Mount anchor (the foot of a dish or vessel on a case's bed); where a book or a
	 * leaf lies on it (the middle of its underside: a book's spine, on the bed or the desk).
	 */
	UPROPERTY()
	TArray<double> Centre;

	/**
	 * The way a work in the round or on a reader faces: a word (`south`), and its bearing from +x towards +y. A book or a
	 * leaf lying on its Mount anchor faces its reader with its foot, its head away.
	 */
	UPROPERTY()
	FName Facing;

	UPROPERTY(meta = (Units = "Degrees"))
	float Facing_Deg = 0.0f;

	/** How far a work on a sloping reader tilts back from upright. */
	UPROPERTY(meta = (Units = "Degrees"))
	float Tilt_Deg = 0.0f;

	/**
	 * How far the back of a work that hangs against its wall stands off the Wall anchor's plane: a tapestry on the batten
	 * and lining behind it, a scroll hanging by its cord in front of its case's silk, a mounted work's board and frame on
	 * their hanging plates. A mounted work that gives none lies with its mount's face on the anchor's plane, a hanging
	 * plane in the room.
	 */
	UPROPERTY(meta = (Units = "Meters"))
	float OffWall = 0.0f;

	/** Whether the work is glazed in its frame or case. */
	UPROPERTY()
	bool Glazed = false;

	/** The surface in the stage's Finishes.json of the glass a glazed work lies under (`hall-of-light.frame.glazing`). */
	UPROPERTY()
	FName Glazing;

	/**
	 * The surface in the stage's Finishes.json of the diffusing pane behind a transparency, through which the daylight
	 * behind it lights it (`hall-of-light.autochrome.backing`).
	 */
	UPROPERTY()
	FName Backing;

	/** The frame's pattern (`watts`, `reeded`), its moulding's width, and its sight's shape (`rect`, `arched`, `oval`). */
	UPROPERTY()
	FName FrameStyle;

	UPROPERTY(meta = (Units = "Meters"))
	float FrameWidth = 0.0f;

	UPROPERTY()
	FName Sight;

	/** The frame's outer size [w, h] and its moulding's width, metres, where the sheet gives them so. */
	UPROPERTY()
	TArray<double> FrameOuter;

	UPROPERTY(meta = (Units = "Meters"))
	float FrameMoulding = 0.0f;

	/** The opening a book lies open at, counted from its first (0: the first leaf's verso and the second's recto). */
	UPROPERTY()
	int32 Opening = 0;

	/** Whether the object stands on a carved stand. */
	UPROPERTY()
	bool Stand = false;

	/** A scroll's silk mount. */
	UPROPERTY()
	FArtScrollMount Mount;

	/**
	 * The surface of the board a print or transparency is mounted on or matted in, the board's size [w, h] and its
	 * depth, metres: its face is the Wall anchor's plane, its back that depth behind it.
	 */
	UPROPERTY()
	FName Mat;

	UPROPERTY()
	TArray<double> MatSize;

	UPROPERTY(meta = (Units = "Meters"))
	float MatDepth = 0.0f;

	/** How a work made from a scan is set. */
	UPROPERTY()
	FArtScanPlacement Scan;

	/** What a work in the round stands on. */
	UPROPERTY()
	FArtCarrier Carrier;

	/** Whether the stage builds the work itself (`"built": "form"`). */
	[[nodiscard]] bool IsBuilt() const { return Built == TEXT("form"); }

	/** Whether what hangs is a reproduction of the original. */
	[[nodiscard]] bool IsReproduction() const { return !Reproduction.IsNone(); }

	/** The work's size as shown, metres; zero unless two numbers are written. */
	[[nodiscard]] FVector2D SizeMeters() const { return Size.Num() == 2 ? FVector2D(Size[0], Size[1]) : FVector2D::ZeroVector; }

	/**
	 * How many times the original's size the work is shown at, across and up: 1 for a work at its true size, the
	 * hung size over the original's for one made after it.
	 */
	[[nodiscard]] FVector2D TimesOriginal() const
	{
		const FVector2D Original = After.OriginalMeters();
		return Original.X > 0.0 && Original.Y > 0.0 ? SizeMeters() / Original : FVector2D::UnitVector;
	}
};
