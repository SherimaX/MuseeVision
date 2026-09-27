#include "ArtWork.h"

#include "ArtCare.h"
#include "ArtCatalogue.h"
#include "Scan/ArtScanCredits.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "MuseeKey.h"
#include "MuseeRecords.h"

namespace Musee::Art
{
	namespace
	{
		/** The folders a work's image is looked for in first: the high-resolution scans, then the working copies. */
		const TCHAR* const PreferredImageFolders[] = { TEXT("paintings_hires"), TEXT("paintings") };

		const TCHAR* const ImageExtensions[] = { TEXT("jpg"), TEXT("jpeg"), TEXT("png") };

		/** The collection's folder under a repository's root, as a schedule's `source` spells it. */
		const TCHAR* const AssetsFolder = TEXT("assets");

		/** How well a folder of images is preferred: lower is better. */
		int32 FolderPreference(const FString& FolderPath)
		{
			const FString Folder = FPaths::GetCleanFilename(FolderPath);
			for (int32 Rank = 0; Rank < UE_ARRAY_COUNT(PreferredImageFolders); ++Rank)
			{
				if (Folder == PreferredImageFolders[Rank])
				{
					return Rank;
				}
			}
			return UE_ARRAY_COUNT(PreferredImageFolders);
		}

		/** How well a found image's folder is preferred: lower is better. */
		int32 Preference(const FString& Path)
		{
			return FolderPreference(FPaths::GetPath(Path));
		}
	}

	FString RepositoryRoot()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT(".."));
	}

	namespace
	{
		/**
		 * The repository's main working tree when this working copy is a linked worktree, whose `.git` is a file naming
		 * its own git folder (`gitdir: <repo>/.git/worktrees/<name>`), which names the shared one in `commondir`; the main
		 * tree is that shared folder's parent. Empty when this copy is the main tree or isn't a repository.
		 */
		FString MainWorkingTree()
		{
			FString Link;
			const FString Prefix = TEXT("gitdir:");
			if (!FFileHelper::LoadFileToString(Link, *(RepositoryRoot() / TEXT(".git")), FFileHelper::EHashOptions::None, FILEREAD_Silent)
				|| !Link.StartsWith(Prefix))
			{
				return FString();
			}
			const FString GitDir = Link.RightChop(Prefix.Len()).TrimStartAndEnd();
			FString Common;
			if (!FFileHelper::LoadFileToString(Common, *(GitDir / TEXT("commondir")), FFileHelper::EHashOptions::None, FILEREAD_Silent))
			{
				return FString();
			}
			Common = Common.TrimStartAndEnd();
			FString Shared = FPaths::IsRelative(Common) ? GitDir / Common : Common;
			FPaths::CollapseRelativeDirectories(Shared);
			return FPaths::ConvertRelativePathToFull(FPaths::GetPath(Shared));
		}

		/** The repositories a catalogue or image may be read from: this working copy, then its main working tree. */
		TArray<FString> RepositoryRoots()
		{
			TArray<FString> Roots = { RepositoryRoot() };
			const FString Main = MainWorkingTree();
			if (!Main.IsEmpty() && !FPaths::IsSamePath(Main, Roots[0]))
			{
				Roots.Add(Main);
			}
			return Roots;
		}

		/** The collection's folder: MUSEE_COLLECTION, else the first repository that holds assets/. */
		FString CollectionRoot()
		{
			const FString Named = FPlatformMisc::GetEnvironmentVariable(TEXT("MUSEE_COLLECTION"));
			if (!Named.IsEmpty())
			{
				return FPaths::ConvertRelativePathToFull(Named);
			}
			for (const FString& Root : RepositoryRoots())
			{
				if (IFileManager::Get().DirectoryExists(*(Root / AssetsFolder)))
				{
					return Root / AssetsFolder;
				}
			}
			return RepositoryRoot() / AssetsFolder;
		}

		FName KeyName(const FString& Text, int32 Segments)
		{
			const TOptional<FMuseeKey> Key = FMuseeKey::Parse(Text);
			return Key.IsSet() && Key->Segments().Num() == Segments ? Key->ToName() : NAME_None;
		}

		/**
		 * The credit of the scan a work is made from: the record's line for its mesh, or for the folder that holds it (a
		 * line naming a folder ends in `/`); an error when the record doesn't read or has no line for it.
		 */
		TValueOrError<FString, FString> ScanCreditOf(const FString& Mesh)
		{
			const TValueOrError<FArtScanCredits, FString> Credits = ReadRecord<FArtScanCredits>(FArtScanCredits::Record);
			if (Credits.HasError())
			{
				return MakeError(Credits.GetError());
			}
			const FArtScanCredit* Found = Credits.GetValue().Scans.FindByPredicate([&Mesh](const FArtScanCredit& Credit)
			{
				return Credit.Mesh == Mesh || (Credit.Mesh.EndsWith(TEXT("/")) && Mesh.StartsWith(Credit.Mesh));
			});
			if (!Found)
			{
				return MakeError(FString::Printf(TEXT("Data/%s gives no credit for its scan %s"), FArtScanCredits::Record, *Mesh));
			}
			return MakeValue(Found->Credit);
		}
	}

	FString SourcePath(const FString& Source)
	{
		if (!FPaths::IsRelative(Source))
		{
			return Source;
		}
		const FString Assets = FString::Printf(TEXT("%s/"), AssetsFolder);
		if (Source.StartsWith(Assets))
		{
			return CollectionRoot() / Source.RightChop(Assets.Len());
		}
		for (const FString& Root : RepositoryRoots())
		{
			if (IFileManager::Get().FileExists(*(Root / Source)))
			{
				return Root / Source;
			}
		}
		return RepositoryRoot() / Source;
	}

	FString FindImage(const FString& Root, const FString& Name)
	{
		TArray<FString> Found;
		for (const TCHAR* Extension : ImageExtensions)
		{
			TArray<FString> Files;
			IFileManager::Get().FindFilesRecursive(Files, *Root, *FString::Printf(TEXT("%s.%s"), *Name, Extension), true, false);
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

	namespace
	{
		/** The numbered photographs `<Name>-<n>` under Root, by folder and then by number. */
		TMap<FString, TMap<int32, FString>> NumberedImages(const FString& Root, const FString& Name)
		{
			TMap<FString, TMap<int32, FString>> Out;
			const FString Prefix = Name + TEXT("-");
			for (const TCHAR* Extension : ImageExtensions)
			{
				TArray<FString> Files;
				IFileManager::Get().FindFilesRecursive(Files, *Root, *FString::Printf(TEXT("%s*.%s"), *Prefix, Extension), true, false);
				for (const FString& File : Files)
				{
					const FString Number = FPaths::GetBaseFilename(File).RightChop(Prefix.Len());
					if (!Number.IsEmpty() && Number.IsNumeric() && !Number.Contains(TEXT(".")))
					{
						Out.FindOrAdd(FPaths::GetPath(File)).Add(FCString::Atoi(*Number), FPaths::ConvertRelativePathToFull(File));
					}
				}
			}
			return Out;
		}

		/** A folder's run of photographs from 1, when it is two or more distinct files; empty otherwise. */
		TArray<FString> DistinctRun(const TMap<int32, FString>& Numbered)
		{
			TArray<FString> Run;
			TSet<FString> Hashes;
			for (int32 Number = 1; const FString* Path = Numbered.Find(Number); ++Number)
			{
				bool bCopy = false;
				Hashes.Add(LexToString(FMD5Hash::HashFile(**Path)), &bCopy);
				if (bCopy)
				{
					return {};
				}
				Run.Add(*Path);
			}
			return Run.Num() >= 2 ? Run : TArray<FString>();
		}
	}

	TArray<FString> FindSectionImages(const FArtHangEntry& Entry)
	{
		const FString& Name = Entry.Image.IsEmpty() ? Entry.Work : Entry.Image;
		const FString Catalogue = FPaths::GetPath(FPaths::ConvertRelativePathToFull(SourcePath(Entry.Source)));
		for (const FString& Root : { Catalogue, CollectionRoot() })
		{
			TMap<FString, TMap<int32, FString>> ByFolder = NumberedImages(Root, Name);
			TArray<FString> Folders;
			ByFolder.GetKeys(Folders);
			Folders.Sort([](const FString& A, const FString& B)
			{
				const int32 RankA = FolderPreference(A);
				const int32 RankB = FolderPreference(B);
				return RankA != RankB ? RankA < RankB : A < B;
			});
			for (const FString& Folder : Folders)
			{
				TArray<FString> Run = DistinctRun(ByFolder[Folder]);
				if (!Run.IsEmpty())
				{
					return Run;
				}
			}
		}
		return {};
	}

	FName PaintMaterial(const FString& WorkId)
	{
		return KeyName(FString::Printf(TEXT("paint.%s"), *WorkId), 2);
	}

	FName PaintMaterial(const FString& WorkId, int32 Canvas)
	{
		return KeyName(FString::Printf(TEXT("paint.%s.%d"), *WorkId, Canvas), 3);
	}

	TValueOrError<FArtWork, FString> ReadWork(const FArtHangEntry& Entry)
	{
		const FString Catalogue = SourcePath(Entry.Source);
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
		if (Entry.IsReproduction())
		{
			TValueOrError<FString, FString> Credit = ReproductionCredit(Entry.Reproduction);
			if (Credit.HasError())
			{
				return MakeError(Credit.StealError());
			}
			Work.Reproduction = Entry.Reproduction;
			Work.ReproductionCredit = Credit.StealValue();
		}
		if (!Entry.Scan.Mesh.IsEmpty())
		{
			TValueOrError<FString, FString> Credit = ScanCreditOf(Entry.Scan.Mesh);
			if (Credit.HasError())
			{
				return MakeError(Credit.StealError());
			}
			Work.ScanCredit = Credit.StealValue();
		}
		Work.Id = Found->Id;
		Work.Title = Found->Title;
		Work.Artist = Found->Artist;
		Work.Date = Found->Year;
		Work.Medium = Found->Medium;
		Work.Collection = Found->Collection;
		Work.City = Found->City;
		Work.Notes = Found->Notes;
		Work.SizeMeters = Entry.SizeMeters();
		Work.Support = Entry.Support;
		Work.Care = Entry.Care;
		// A catalogue's images lie beside it (assets/collection.json and assets/paintings/), or in the collection.
		const FString& Image = Entry.Image.IsEmpty() ? Work.Id : Entry.Image;
		Work.ImagePath = FindImage(FPaths::GetPath(FPaths::ConvertRelativePathToFull(Catalogue)), Image);
		Work.ImagePath = Work.ImagePath.IsEmpty() ? FindImage(CollectionRoot(), Image) : Work.ImagePath;
		return MakeValue(MoveTemp(Work));
	}
}

FMuseeCaption FArtWork::Caption() const
{
	FMuseeCaption Out;
	Out.Title = FText::FromString(Title);
	const FString Line = FString::Join(TArray<FString>({ Artist, Date, Medium }).FilterByPredicate([](const FString& S) { return !S.IsEmpty(); }), TEXT(", "));
	Out.Body = FText::FromString(Notes.IsEmpty() ? Line : Line + TEXT(". ") + Notes);
	const FString Holder = FString::Join(TArray<FString>({ Collection, City }).FilterByPredicate([](const FString& S) { return !S.IsEmpty(); }), TEXT(", "));
	const FString Held = Reproduction.IsNone() ? Holder : FString::Printf(TEXT("%s %s"), *ReproductionCredit, *Holder);
	Out.Credit = FText::FromString(ScanCredit.IsEmpty() ? Held : FString::Printf(TEXT("%s. %s"), *Held, *ScanCredit));
	return Out;
}
