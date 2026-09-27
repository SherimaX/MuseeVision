#pragma once

#include "ArtHangEntry.h"
#include "CoreMinimal.h"
#include "MuseeCaption.h"

/**
 * A work as the museum hangs it: the catalogue's facts (its id, title, artist, date, medium, where it is held), the
 * hanging plan's (its real size in metres, its support, its care class), and the image of its paint layer, found in
 * the repository's assets by the work's id.
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
	/** The image of the paint layer, a full path; empty when none was found. */
	FString ImagePath;

	/** The wall label: the title; the artist, date and medium; and where the work is held. */
	[[nodiscard]] FMuseeCaption Caption() const;
};

namespace Musee::Art
{
	/** The repository's root, which a hanging plan's `source` and the assets' paths are relative to: the project's parent. */
	[[nodiscard]] MUSEEART_API FString RepositoryRoot();

	/**
	 * Reads the work a hanging plan's entry names from the catalogue its `source` names (relative to the repository's
	 * root, or a full path), with the entry's size, support and care, and finds its image: under the catalogue's own
	 * folder, then under the repository's `assets`. Fails, saying why, on a catalogue that doesn't read or an id it
	 * doesn't hold; a work without an image has an empty ImagePath.
	 */
	[[nodiscard]] MUSEEART_API TValueOrError<FArtWork, FString> ReadWork(const FArtHangEntry& Entry);

	/**
	 * The image of a work's paint layer: the file under Root named for the work's id (jpg, jpeg or png). The
	 * high-resolution folder (`paintings_hires`) is preferred, then `paintings`, then any other in path order; empty if
	 * there is none.
	 */
	[[nodiscard]] MUSEEART_API FString FindImage(const FString& Root, const FString& WorkId);

	/**
	 * The palette key of a work's own paint layer, `paint.<work id>`: an instance of Matter's paint master carrying the
	 * work's image and size. None if the id can't be a key segment.
	 */
	[[nodiscard]] MUSEEART_API FName PaintMaterial(const FString& WorkId);
}
