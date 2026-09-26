#pragma once

#include "CoreMinimal.h"

#include "MuseeKey.generated.h"

/**
 * The name of a thing in the museum's data: a substance, a room, a port, a luminaire, a mechanism, a setting. Things
 * find each other by key, never by compile-time reference, and every seed is drawn from a key.
 *
 * A key is lowercase and dot-scoped, with kebab-case inside a segment: `travertine`, `rotunda.west`,
 * `track-spot.3000k.25deg`. A segment holds the letters a to z, the digits and single hyphens, and neither starts nor
 * ends with a hyphen. Display text is FText and never a key.
 *
 * In a record a key is written as a plain string. The default key is the none key, which names nothing.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeKey
{
	GENERATED_BODY()

	/** The none key. */
	FMuseeKey() = default;

	/**
	 * A key the program itself spells, such as a literal. Text that breaks the key rule is a programming error: it is
	 * reported by an ensure and yields the none key. Text from outside the program goes through Parse.
	 */
	explicit FMuseeKey(FStringView Text);

	/** The key the text spells, or nothing if the text breaks the key rule. */
	[[nodiscard]] static TOptional<FMuseeKey> Parse(FStringView Text);

	/** Whether the text is a key; if not, OutWhy says which part of the rule it breaks. */
	[[nodiscard]] static bool IsValid(FStringView Text, FString* OutWhy = nullptr);

	[[nodiscard]] bool IsNone() const { return Name.IsNone(); }

	/** The key as written, always lowercase. */
	[[nodiscard]] FString ToString() const;

	/** The key as a name, for engine interfaces that take one. */
	[[nodiscard]] FName ToName() const { return Name; }

	/** The key's dot-separated segments, outermost first. The none key has none. */
	[[nodiscard]] TArray<FString> Segments() const;

	/** The key one scope out (`rotunda` for `rotunda.west`), or the none key for a single segment. */
	[[nodiscard]] FMuseeKey Parent() const;

	/** The key one scope in (`rotunda.west` from `rotunda` and `west`); the none key if the segment breaks the rule. */
	[[nodiscard]] FMuseeKey Child(FStringView Segment) const;

	bool operator==(const FMuseeKey& Other) const { return Name == Other.Name; }

	friend uint32 GetTypeHash(const FMuseeKey& Key) { return GetTypeHash(Key.Name); }

	/** Reads a key from text (a record's string, a config value); fails on text that breaks the key rule. */
	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);

	/** Writes the key as its plain text. */
	bool ExportTextItem(FString& ValueStr, const FMuseeKey& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const;

private:
	/** Lowercase by construction, since every key is validated before it is stored. */
	UPROPERTY()
	FName Name;
};

template <>
struct TStructOpsTypeTraits<FMuseeKey> : public TStructOpsTypeTraitsBase2<FMuseeKey>
{
	enum
	{
		WithIdenticalViaEquality = true,
		WithImportTextItem = true,
		WithExportTextItem = true,
	};
};

namespace Musee
{
	/**
	 * The seed of the random stream that belongs to a key: every random choice in the museum (a stone's veining, a
	 * plant's branching, a course's joints) is drawn from the stream of the thing it belongs to, so the museum is the
	 * same on every run and every machine. There is no unseeded randomness.
	 *
	 * The seed is the 64-bit FNV-1a hash of the key's UTF-8 text, mixed by the SplitMix64 finaliser and folded to 32
	 * bits (the width of FRandomStream's seed). The algorithm is fixed forever: changing it would change every made
	 * piece. Distinct things take distinct keys; `Child` scopes a key to a part.
	 */
	[[nodiscard]] MUSEEFOUNDATION_API int32 Seed(const FMuseeKey& Key);
}
