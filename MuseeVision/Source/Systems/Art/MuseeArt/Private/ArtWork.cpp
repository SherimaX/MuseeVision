#include "ArtWork.h"

#include "ArtCatalogue.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "MuseeKey.h"
#include "MuseeRecords.h"

namespace Musee::Art
{
	namespace
	{
		/** The folders a work's image is looked for in first: the high-resolution scans, then the working copies. */
		const TCHAR* const PreferredImageFolders[] = { TEXT("paintings_hires"), TEXT("paintings") };

		const TCHAR* const ImageExtensions[] = { TEXT("jpg"), TEXT("jpeg"), TEXT("png") };

		/** How well a found image's folder is preferred: lower is better. */
		int32 Preference(const FString& Path)
		{
			const FString Folder = FPaths::GetCleanFilename(FPaths::GetPath(Path));
			for (int32 Rank = 0; Rank < UE_ARRAY_COUNT(PreferredImageFolders); ++Rank)
			{
				if (Folder == PreferredImageFolders[Rank])
				{
					return Rank;
				}
			}
			return UE_ARRAY_COUNT(PreferredImageFolders);
		}
	}

	FString RepositoryRoot()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT(".."));
	}

	FString FindImage(const FString& Root, const FString& WorkId)
	{
		TArray<FString> Found;
		for (const TCHAR* Extension : ImageExtensions)
		{
			TArray<FString> Files;
			IFileManager::Get().FindFilesRecursive(Files, *Root, *FString::Printf(TEXT("%s.%s"), *WorkId, Extension), true, false);
			Found.Append(Files);
		}
		Found.Sort([](const FString& A, const FString& B)
		{
			const int32 RankA = Preference(A);
			const int32 RankB = Preference(B);
			return RankA != RankB ? RankA < RankB : A < B;
		});
		return Found.IsEmpty() ? FString() : FPaths::ConvertRelativePathToFull(Found[0]);
	}

	FName PaintMaterial(const FString& WorkId)
	{
		const TOptional<FMuseeKey> Key = FMuseeKey::Parse(FString::Printf(TEXT("paint.%s"), *WorkId));
		return Key.IsSet() && Key->Segments().Num() == 2 ? Key->ToName() : NAME_None;
	}

	TValueOrError<FArtWork, FString> ReadWork(const FArtHangEntry& Entry)
	{
		const FString Catalogue = FPaths::IsRelative(Entry.Source) ? RepositoryRoot() / Entry.Source : Entry.Source;
		const TValueOrError<FArtCatalogue, FString> Read = ReadRecord<FArtCatalogue>(FPaths::ConvertRelativePathToFull(Catalogue));
		if (Read.HasError())
		{
			return MakeError(Read.GetError());
		}
		const FArtCatalogueEntry* Found = Read.GetValue().Find(Entry.Work);
		if (!Found)
		{
			return MakeError(FString::Printf(TEXT("%s holds no work '%s'"), *Entry.Source, *Entry.Work));
		}
		FArtWork Work;
		Work.Id = Found->Id;
		Work.Title = Found->Title;
		Work.Artist = Found->Artist;
		Work.Date = Found->Year;
		Work.Medium = Found->Medium;
		Work.Collection = Found->Collection;
		Work.City = Found->City;
		Work.Notes = Found->Notes;
		Work.SizeMeters = Entry.Size.Num() == 2 ? FVector2D(Entry.Size[0], Entry.Size[1]) : FVector2D::ZeroVector;
		Work.Support = Entry.Support;
		Work.Care = Entry.Care;
		// A catalogue's images lie beside it (assets/collection.json and assets/paintings/), or in the assets.
		Work.ImagePath = FindImage(FPaths::GetPath(FPaths::ConvertRelativePathToFull(Catalogue)), Work.Id);
		Work.ImagePath = Work.ImagePath.IsEmpty() ? FindImage(RepositoryRoot() / TEXT("assets"), Work.Id) : Work.ImagePath;
		return MakeValue(MoveTemp(Work));
	}
}

FMuseeCaption FArtWork::Caption() const
{
	FMuseeCaption Out;
	Out.Title = FText::FromString(Title);
	const FString Line = FString::Join(TArray<FString>({ Artist, Date, Medium }).FilterByPredicate([](const FString& S) { return !S.IsEmpty(); }), TEXT(", "));
	Out.Body = FText::FromString(Notes.IsEmpty() ? Line : Line + TEXT(". ") + Notes);
	Out.Credit = FText::FromString(FString::Join(TArray<FString>({ Collection, City }).FilterByPredicate([](const FString& S) { return !S.IsEmpty(); }), TEXT(", ")));
	return Out;
}
