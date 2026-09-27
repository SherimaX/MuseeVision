#pragma once

#include "CoreMinimal.h"

/**
 * The records a piece of work read: each record file with the hash of its content. Studio opens a set around each
 * make (a stage, a substance, a joint) and hashes it with the generator's version, so a change to a record remakes
 * exactly what read it and nothing else.
 *
 * The record reader adds every file it reads to each set in scope on the reading thread (FScope). A path inside the
 * project's Data folder is kept relative to it, with forward slashes (`Site/PortClasses.json`).
 */
class MUSEEFOUNDATION_API FMuseeDependencySet
{
public:
	/**
	 * While a scope lives, every record read on its thread is added to its set, and to the sets of the scopes that
	 * enclose it, so a make that contains another make depends on everything the inner one read.
	 */
	class MUSEEFOUNDATION_API FScope
	{
	public:
		explicit FScope(FMuseeDependencySet& InSet);
		~FScope();

		FScope(const FScope&) = delete;
		FScope& operator=(const FScope&) = delete;

	private:
		FMuseeDependencySet& Set;
	};

	/** Adds a file and the hash of its content, replacing the hash it had if it was read before. */
	void Add(const FString& Path, const FString& ContentHash);

	/** Adds everything another set holds. */
	void Append(const FMuseeDependencySet& Other);

	/** Every file read, with the hash of its content. */
	[[nodiscard]] const TMap<FString, FString>& Files() const { return FileHashes; }

	[[nodiscard]] bool IsEmpty() const { return FileHashes.IsEmpty(); }

	/** One SHA-1 over every path and content hash, taken in path order, so the order of reading doesn't matter. */
	[[nodiscard]] FString Hash() const;

	/** Adds a file to every set in scope on this thread. The record reader calls it for each file it reads. */
	static void Record(const FString& Path, const FString& ContentHash);

private:
	TMap<FString, FString> FileHashes;
};
