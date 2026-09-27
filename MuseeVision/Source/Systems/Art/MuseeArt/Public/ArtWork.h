#pragma once

#include "ArtHangEntry.h"
#include "CoreMinimal.h"
#include "MuseeCaption.h"

/**
 * A work as the museum hangs it: the catalogue's facts (its id, title, artist, date, medium, where it is held), the
 * hanging plan's (its real size in metres, its support, its care class, what hangs if it is a reproduction), and the
 * image of its paint layer, found in the collection by the work's id or by the image the entry names.
 */
struct MUSEEART_API FArtWork
{
	FString Id;
	FString Title;
	FString Artist;
	FString Date;
	FString Medium;
	FString Collection;
	FString City;
	FString Notes;
	/** Width and height, metres. */
	FVector2D SizeMeters = FVector2D::ZeroVector;
	FName Support;
	FName Care;
	/** The kind of reproduction that hangs in the original's place, or none; and the label's words for it. */
	FName Reproduction;
	FString ReproductionCredit;
	/** The image of the paint layer, a full path; empty when none was found. */
	FString ImagePath;
	/** For a work made from a scan, the scan's credit as its licence asks (Data/Art/Scans.json); empty otherwise. */
	FString ScanCredit;
	/** For a work made from photographs, their credit as their licences ask (its catalogue's `credit`); empty otherwise. */
	FString ImageCredit;

	/**
	 * The label: the title; the artist, date and medium; and where the work is held, or for a reproduction what it is
	 * and where the original is held ("Exhibition facsimile after the original in ..."), then the scan's credit and the
	 * photographs'.
	 */
	[[nodiscard]] FMuseeCaption Caption() const;
};

namespace Musee::Art
{
	/** The repository's root, which a hanging plan's `source` and the assets' paths are relative to: the project's parent. */
	[[nodiscard]] MUSEEART_API FString RepositoryRoot();

	/**
	 * A file a hanging plan names from the repository's root (a catalogue, a scan's mesh), as a full path: `assets/...` in
	 * the collection, any other (data/albion.json) in the first repository that holds it; a full path as it is.
	 */
	[[nodiscard]] MUSEEART_API FString SourcePath(const FString& Source);

	/**
	 * Reads the work a hanging plan's entry names from the catalogue its `source` names (relative to the repository's
	 * root, or a full path), with the entry's size, support, care and reproduction, and finds its image: the entry's own
	 * `image` if it names one, otherwise the work's id; under the catalogue's own folder, then in the collection. Fails,
	 * saying why, on a catalogue that doesn't read, an id it doesn't hold, or a reproduction Care.json doesn't list; a
	 * work without an image has an empty ImagePath. A work made from a scan carries the scan's credit, and fails without
	 * one (Data/Art/Scans.json): a scan is shown only on its licence's terms.
	 *
	 * The collection (assets/, with its catalogues and images) and the older wings' catalogues (data/) are the museum's
	 * reserve, kept out of a working copy that doesn't check them out. They are read from, in order, the environment's
	 * MUSEE_COLLECTION (the collection's folder), this working copy, and the repository's main working tree when this
	 * copy is one of its linked worktrees. The make only ever reads them.
	 */
	[[nodiscard]] MUSEEART_API TValueOrError<FArtWork, FString> ReadWork(const FArtHangEntry& Entry);

	/**
	 * The image of a work's paint layer: the file under Root named for the work's id or an entry's image (jpg, jpeg or
	 * png). The high-resolution folder (`paintings_hires`) is preferred, then `paintings`, then any other in path order;
	 * empty if there is none.
	 */
	[[nodiscard]] MUSEEART_API FString FindImage(const FString& Root, const FString& Name);

	/**
	 * The palette key of a work's own paint layer, `paint.<work id>`: an instance of Matter's paint master carrying the
	 * work's image and size. None if the id can't be a key segment.
	 */
	[[nodiscard]] MUSEEART_API FName PaintMaterial(const FString& WorkId);

	/**
	 * The palette key of the layer one object of a work shows when it has its own image (a Rouen canvas, a Tristram
	 * panel): `paint.<work id>.<canvas>`. None if the id can't be a key segment.
	 */
	[[nodiscard]] MUSEEART_API FName PaintMaterial(const FString& WorkId, int32 Canvas);
}
