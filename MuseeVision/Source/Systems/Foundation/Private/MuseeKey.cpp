#include "MuseeKey.h"

#include "Containers/StringConv.h"
#include "MuseeFoundationLog.h"

namespace Musee
{
	namespace
	{
		constexpr TCHAR SegmentSeparator = TEXT('.');
		constexpr TCHAR WordSeparator = TEXT('-');

		/** A key is stored as a name, and a name holds at most NAME_SIZE characters with its terminator. */
		constexpr int32 LongestKey = NAME_SIZE - 1;

		bool IsLetterOrDigit(TCHAR Character)
		{
			return (Character >= TEXT('a') && Character <= TEXT('z')) || (Character >= TEXT('0') && Character <= TEXT('9'));
		}

		bool Fail(FString* OutWhy, const FString& Why)
		{
			if (OutWhy)
			{
				*OutWhy = Why;
			}
			return false;
		}

		bool IsValidSegment(FStringView Segment, FString* OutWhy)
		{
			if (Segment.IsEmpty())
			{
				return Fail(OutWhy, TEXT("a segment is empty"));
			}
			if (Segment[0] == WordSeparator || Segment[Segment.Len() - 1] == WordSeparator)
			{
				return Fail(OutWhy, FString::Printf(TEXT("segment '%.*s' starts or ends with a hyphen"), Segment.Len(), Segment.GetData()));
			}
			for (int32 Index = 0; Index < Segment.Len(); ++Index)
			{
				const TCHAR Character = Segment[Index];
				if (Character == WordSeparator && Segment[Index - 1] == WordSeparator)
				{
					return Fail(OutWhy, FString::Printf(TEXT("segment '%.*s' has two hyphens together"), Segment.Len(), Segment.GetData()));
				}
				if (Character != WordSeparator && !IsLetterOrDigit(Character))
				{
					return Fail(OutWhy, FString::Printf(TEXT("'%c' is not a lowercase letter, a digit or a hyphen"), Character));
				}
			}
			return true;
		}
	}
}

FMuseeKey::FMuseeKey(FStringView Text)
{
	FString Why;
	if (ensureMsgf(IsValid(Text, &Why), TEXT("'%.*s' is not a key: %s."), Text.Len(), Text.GetData(), *Why))
	{
		Name = FName(Text.Len(), Text.GetData());
	}
}

TOptional<FMuseeKey> FMuseeKey::Parse(FStringView Text)
{
	if (!IsValid(Text))
	{
		return {};
	}
	FMuseeKey Key;
	Key.Name = FName(Text.Len(), Text.GetData());
	return Key;
}

bool FMuseeKey::IsValid(FStringView Text, FString* OutWhy)
{
	using namespace Musee;
	if (Text.IsEmpty())
	{
		return Fail(OutWhy, TEXT("a key is not empty"));
	}
	if (Text.Len() > LongestKey)
	{
		return Fail(OutWhy, FString::Printf(TEXT("a key has at most %d characters"), LongestKey));
	}
	int32 SegmentStart = 0;
	for (int32 Index = 0; Index <= Text.Len(); ++Index)
	{
		if (Index == Text.Len() || Text[Index] == SegmentSeparator)
		{
			if (!IsValidSegment(Text.Mid(SegmentStart, Index - SegmentStart), OutWhy))
			{
				return false;
			}
			SegmentStart = Index + 1;
		}
	}
	return true;
}

FString FMuseeKey::ToString() const
{
	// A cooked game keeps one spelling per name, whichever was made first; a key's text is lowercase by its rule.
	return IsNone() ? FString() : Name.ToString().ToLower();
}

TArray<FString> FMuseeKey::Segments() const
{
	TArray<FString> Result;
	ToString().ParseIntoArray(Result, TEXT("."));
	return Result;
}

FMuseeKey FMuseeKey::Parent() const
{
	const FString Text = ToString();
	int32 Separator = INDEX_NONE;
	if (!Text.FindLastChar(Musee::SegmentSeparator, Separator))
	{
		return FMuseeKey();
	}
	FMuseeKey Key;
	Key.Name = FName(Separator, *Text);
	return Key;
}

FMuseeKey FMuseeKey::Child(FStringView Segment) const
{
	if (!Musee::IsValidSegment(Segment, nullptr))
	{
		return FMuseeKey();
	}
	const FString Text = IsNone() ? FString(Segment) : FString::Printf(TEXT("%s%c%.*s"), *ToString(), Musee::SegmentSeparator, Segment.Len(), Segment.GetData());
	return Parse(Text).Get(FMuseeKey());
}

bool FMuseeKey::ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText)
{
	// The token runs to a closing quote if it opens with one, and otherwise to the next space, comma or bracket.
	const TCHAR* Cursor = Buffer;
	const bool bQuoted = *Cursor == TEXT('"');
	if (bQuoted)
	{
		++Cursor;
	}
	const TCHAR* Start = Cursor;
	while (*Cursor && (bQuoted ? *Cursor != TEXT('"') : !FChar::IsWhitespace(*Cursor) && *Cursor != TEXT(',') && *Cursor != TEXT(')')))
	{
		++Cursor;
	}
	if (bQuoted && *Cursor != TEXT('"'))
	{
		return false;
	}
	const FStringView Text(Start, UE_PTRDIFF_TO_INT32(Cursor - Start));
	FMuseeKey Key;
	if (!Text.IsEmpty())
	{
		FString Why;
		if (!IsValid(Text, &Why))
		{
			if (ErrorText)
			{
				ErrorText->Logf(ELogVerbosity::Warning, TEXT("'%.*s' is not a key: %s."), Text.Len(), Text.GetData(), *Why);
			}
			return false;
		}
		Key.Name = FName(Text.Len(), Text.GetData());
	}
	*this = Key;
	Buffer = bQuoted ? Cursor + 1 : Cursor;
	return true;
}

bool FMuseeKey::ExportTextItem(FString& ValueStr, const FMuseeKey& DefaultValue, UObject* Parent, int32 PortFlags, UObject* ExportRootScope) const
{
	ValueStr += ToString();
	return true;
}

int32 Musee::Seed(const FMuseeKey& Key)
{
	// FNV-1a, 64-bit: G. Fowler, L. C. Noll and K.-P. Vo, as specified in IETF draft-eastlake-fnv.
	constexpr uint64 FnvOffsetBasis = 0xcbf29ce484222325ull;
	constexpr uint64 FnvPrime = 0x00000100000001b3ull;
	// The SplitMix64 finaliser: G. Steele, D. Lea and C. Flood, "Fast splittable pseudorandom number generators",
	// OOPSLA 2014, with the multipliers and shifts (30, 27, 31) of S. Vigna's reference implementation.
	constexpr uint64 MixMultiplierA = 0xbf58476d1ce4e5b9ull;
	constexpr uint64 MixMultiplierB = 0x94d049bb133111ebull;

	ensureMsgf(!Key.IsNone(), TEXT("Every seed belongs to a thing; the none key names nothing."));
	const FTCHARToUTF8 Utf8(*Key.ToString());
	uint64 Hash = FnvOffsetBasis;
	for (int32 Index = 0; Index < Utf8.Length(); ++Index)
	{
		Hash ^= static_cast<uint8>(Utf8.Get()[Index]);
		Hash *= FnvPrime;
	}
	Hash = (Hash ^ (Hash >> 30)) * MixMultiplierA;
	Hash = (Hash ^ (Hash >> 27)) * MixMultiplierB;
	Hash = Hash ^ (Hash >> 31);
	// Fold both halves into the 32 bits of FRandomStream's seed.
	return static_cast<int32>(static_cast<uint32>(Hash ^ (Hash >> 32)));
}
