#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"
#include "UObject/Class.h"

/**
 * The record reader: the one way the museum's specifications, written as text under `<Project>/Data/<System>/`, become
 * typed structs. Every number in the museum that isn't a physical constant comes through it.
 *
 * A record file is one JSON object. Its fields are the struct's properties by name; a key is written as a string;
 * nested structs are objects and arrays are arrays. A point or a size, an `FVector` or `FVector2D`, is written as its
 * numbers, `[x, y, z]` or `[x, y]` (or as the object `{ "X": x, ... }`), so an outline is `[[x, y], [x, y], ...]`.
 * Keys beginning with `$` are the file's declarations, not fields:
 * - `"$records": [ {...}, {...} ]` holds the file's records when it has more than one; otherwise the object itself is
 *   the one record.
 * - `"$units": { "HeightMeters": "m", "IlluminanceLux": "lx" }` declares the unit each named field is written in,
 *   wherever the field appears in the file. A unit is any name or symbol the engine's unit table knows (`m`, `Meters`,
 *   `lx`, `cd/m2`, `K`, `kg`, `s`, `deg`, `EV`, `dB`).
 * - `"$sources": "..."` or `[ "...", ... ]` says where the file's numbers come from. The reader keeps no copy; it is
 *   for the people who check the numbers.
 *
 * Reading fails, naming the file and the field, when:
 * - a field whose UPROPERTY carries `meta = (MuseeRequired)` is missing or null (a required key written empty is
 *   not a key);
 * - a range (`meta = (MuseeRange)`) isn't [least, most] with at most one end null;
 * - a declared unit is not the unit of the field's `Units` metadata (the same unit under another name agrees), or the
 *   field has no `Units` metadata, or no such field exists;
 * - a field doesn't exist on the struct, or its value has the wrong JSON type, or a key breaks the key rule, or a
 *   vector written as an array has the wrong count of numbers;
 * - the file is missing, isn't JSON, or declares something other than the three declarations above.
 * The required and unit checks read reflection metadata, which exists where records are made (the editor and its
 * commandlets). A cooked game has no metadata; there the reader converts and checks types only.
 *
 * An optional key may be written `""`: it reads as the none key, which the stage contract writes for a field that names
 * nothing (a bed that rides no body).
 *
 * Every file read, whether or not it converts, is added with the hash of its content to each FMuseeDependencySet in
 * scope on the reading thread.
 *
 * A path is relative to `<Project>/Data`; an absolute path is read as given (the reader's own test fixtures).
 */
namespace Musee
{
	namespace Records
	{
		/** The metadata key that makes a field required: `UPROPERTY(meta = (MuseeRequired))`. */
		inline constexpr const TCHAR* RequiredMetadata = TEXT("MuseeRequired");

		/**
		 * The metadata key that makes an array of numbers a range [least, most]: `UPROPERTY(meta = (MuseeRange))`. Either end
		 * may be written null for an open end (`"lux": [50, null]`, at least 50), which reads as -infinity for the least end
		 * and +infinity for the most (the mark is metadata, so only where records are made); both ends null, a count other than two, or the least above the most is refused.
		 */
		inline constexpr const TCHAR* RangeMetadata = TEXT("MuseeRange");

		/**
		 * The folder records live in, `<Project>/Data`, as a full path: DataRootOf this process's executable folder and
		 * project folder. The editor and a packaged game resolve it the same way.
		 */
		[[nodiscard]] MUSEEFOUNDATION_API FString DataRoot();

		/**
		 * The data root of a process whose executable is in BaseDir and whose project folder is ProjectDir, as the engine
		 * gives them (FPlatformProcess::BaseDir, FPaths::ProjectDir): ProjectDir/Data made full against BaseDir. The
		 * editor's project folder is absolute; a packaged game's is relative to its executable (`../../../<Project>/`),
		 * and packaging stages Data/ there as loose, non-UFS files (the Foundation module's runtime dependency), so the
		 * game reads the same records at the same place under its project as the editor.
		 */
		[[nodiscard]] MUSEEFOUNDATION_API FString DataRootOf(const FString& BaseDir, const FString& ProjectDir);

		/**
		 * The untyped reader behind ReadRecords and ReadRecord. Reads the records at Path, which names a file or a
		 * folder of `*.json` files (read in name order), and converts each into the struct Emplace returns. With
		 * bExactlyOne, the path must name one file holding one record.
		 */
		[[nodiscard]] MUSEEFOUNDATION_API bool ReadInto(const UScriptStruct& Type, const FString& Path, bool bExactlyOne, TFunctionRef<void*()> Emplace, FString& OutError);
	}

	/** Every record at a path: one file (one record, or its `$records`), or every `*.json` file in a folder. */
	template <typename T>
	[[nodiscard]] TValueOrError<TArray<T>, FString> ReadRecords(const FString& Path)
	{
		TArray<T> Result;
		FString Error;
		if (!Records::ReadInto(*T::StaticStruct(), Path, false, [&Result]() -> void* { return &Result.AddDefaulted_GetRef(); }, Error))
		{
			return MakeError(MoveTemp(Error));
		}
		return MakeValue(MoveTemp(Result));
	}

	/** The one record in a file. */
	template <typename T>
	[[nodiscard]] TValueOrError<T, FString> ReadRecord(const FString& Path)
	{
		T Result;
		FString Error;
		if (!Records::ReadInto(*T::StaticStruct(), Path, true, [&Result]() -> void* { return &Result; }, Error))
		{
			return MakeError(MoveTemp(Error));
		}
		return MakeValue(MoveTemp(Result));
	}
}
