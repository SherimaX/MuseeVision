#include "MuseeDependencySet.h"

#include "Misc/SecureHash.h"

namespace Musee
{
	namespace
	{
		/** The sets in scope on this thread, innermost last. */
		thread_local TArray<FMuseeDependencySet*> ScopedSets;
	}
}

FMuseeDependencySet::FScope::FScope(FMuseeDependencySet& InSet)
	: Set(InSet)
{
	Musee::ScopedSets.Add(&Set);
}

FMuseeDependencySet::FScope::~FScope()
{
	check(!Musee::ScopedSets.IsEmpty() && Musee::ScopedSets.Last() == &Set);
	Musee::ScopedSets.Pop(EAllowShrinking::No);
}

void FMuseeDependencySet::Add(const FString& Path, const FString& ContentHash)
{
	FileHashes.Add(Path, ContentHash);
}

void FMuseeDependencySet::Append(const FMuseeDependencySet& Other)
{
	FileHashes.Append(Other.FileHashes);
}

FString FMuseeDependencySet::Hash() const
{
	TArray<FString> Paths;
	FileHashes.GetKeys(Paths);
	Paths.Sort();
	FSHA1 Hasher;
	for (const FString& Path : Paths)
	{
		const FTCHARToUTF8 Line(*FString::Printf(TEXT("%s\n%s\n"), *Path, *FileHashes[Path]));
		Hasher.Update(reinterpret_cast<const uint8*>(Line.Get()), Line.Length());
	}
	Hasher.Final();
	FSHAHash Digest;
	Hasher.GetHash(Digest.Hash);
	return Digest.ToString();
}

void FMuseeDependencySet::Record(const FString& Path, const FString& ContentHash)
{
	for (FMuseeDependencySet* Set : Musee::ScopedSets)
	{
		Set->Add(Path, ContentHash);
	}
}
