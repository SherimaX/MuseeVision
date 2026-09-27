#include "ArtCastScan.h"
#include "ArtPieces.h"
#include "Cast/CastMesh.h"
#include "Engine/StaticMesh.h"
#include "FormMesh.h"
#include "MatterNames.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "Objects/ObjectMaking.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** Where Studio imports the collection's scans, under a make's root. */
		const TCHAR* const CastsFolder = TEXT("Art/Casts");

		/** A scan's asset is named SM_Scan_<scan>. */
		const TCHAR* const ScanAssetPrefix = TEXT("SM_Scan_");

		bool IsGltf(const FArtHangEntry& Entry)
		{
			return FPaths::GetExtension(Entry.Scan.Mesh).Equals(GltfExtension, ESearchCase::IgnoreCase);
		}

		/** A work's scan, read from its `.mvm` or from the static mesh Studio imported its glTF as. */
		bool ReadScan(const FArtHangEntry& Entry, FArtScanMesh& Out, FString& OutWhy)
		{
			if (!IsGltf(Entry))
			{
				return ReadMvm(Musee::Art::SourcePath(Entry.Scan.Mesh), Out, OutWhy);
			}
			const FString Package = ScanPackage(MuseumAssetsRoot, ScanName(Entry));
			UStaticMesh* Mesh = FPackageName::DoesPackageExist(Package)
				? LoadObject<UStaticMesh>(nullptr, *Musee::Matter::ObjectPathOf(Package), nullptr, LOAD_NoWarn | LOAD_Quiet) : nullptr;
			if (!Mesh)
			{
				OutWhy = FString::Printf(TEXT("its scan %s has not been imported at %s (Studio's make imports it)"), *Entry.Scan.Mesh, *Package);
				return false;
			}
			return ReadImportedScan(*Mesh, Out, OutWhy);
		}

		/** The record's parts of this work cast in another material, in the record's order. */
		TArray<const FArtCastPart*> PartsOf(const FArtCasts& Casts, const FArtHangEntry& Entry)
		{
			TArray<const FArtCastPart*> Out;
			for (const FArtCastPart& Part : Casts.Parts)
			{
				if (Part.Work == Entry.Work)
				{
					Out.Add(&Part);
				}
			}
			return Out;
		}
	}

	FString ScanName(const FArtHangEntry& Entry)
	{
		return FPaths::GetBaseFilename(Entry.Scan.Mesh);
	}

	FString ScanPackage(const FString& Root, const FString& Scan)
	{
		return FString::Printf(TEXT("%s/%s/%s%s"), *Root, CastsFolder, ScanAssetPrefix, *Musee::Matter::PascalCase(Scan));
	}

	TArray<FArtCastScan> CastScans(const FMuseeStageContext& Stage)
	{
		TArray<FArtCastScan> Out;
		FArtHang Hang;
		TArray<FMuseeFinding> Reported;
		if (!ReadStageHang(Stage, Hang, Reported))
		{
			return Out;
		}
		for (const FArtHangEntry& Entry : Hang.Works)
		{
			if (Entry.IsBuilt() || !IsCast(Entry) || !IsGltf(Entry))
			{
				continue;
			}
			const FString Scan = ScanName(Entry);
			if (!Out.ContainsByPredicate([&Scan](const FArtCastScan& Found) { return Found.Scan == Scan; }))
			{
				Out.Add({ Scan, Musee::Art::SourcePath(Entry.Scan.Mesh) });
			}
		}
		return Out;
	}

	const TValueOrError<FArtCasts, FString>& FObjectMaking::Casts()
	{
		if (!CastRecord.IsSet())
		{
			CastRecord.Emplace(ReadRecord<FArtCasts>(FArtCasts::Record));
		}
		return CastRecord.GetValue();
	}

	void MakeCast(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const auto Refuse = [&Making, &Entry, &OutFindings](const FString& Why)
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: it can't be cast: %s."), *Entry.Work, *Why)));
		};
		const TValueOrError<FArtCasts, FString>& Casts = Making.Casts();
		FArtScanMesh Scan;
		FString Why;
		if (Casts.HasError() || !ReadScan(Entry, Scan, Why))
		{
			Refuse(Casts.HasError() ? Casts.GetError() : Why);
			return;
		}
		const FCastFit Fit = FitCast(Scan, Entry.Size[1], Casts.GetValue().BaseSliceMeters);
		if (!Fit.IsValid())
		{
			Refuse(FString::Printf(TEXT("its scan %s has no height"), *Entry.Scan.Mesh));
			return;
		}
		if (Entry.Size[0] > 0.0 && FMath::Abs(Fit.WidthMeters - Entry.Size[0]) > Casts.GetValue().WidthToleranceMeters)
		{
			Refuse(FString::Printf(TEXT("at its true height of %.3f m its scan stands %.3f m wide, and the plan gives %.3f m"), Entry.Size[1],
				Fit.WidthMeters, Entry.Size[0]));
			return;
		}
		const TArray<const FArtCastPart*> Parts = PartsOf(Casts.GetValue(), Entry);
		TArray<FCastBand> Bands;
		for (const FArtCastPart* Part : Parts)
		{
			Bands.Add({ Part->Band[0], Part->Band[1] });
		}
		TArray<FFormMesh> Meshes;
		BuildCast(Scan, Fit, Bands, Meshes);
		const FMuseeFrame Frame = RoundFrame(Entry, Making.Stage.FindAnchor(Entry.Anchor, MountAnchorKind)->Frame);
		const bool bSplit = Plan.IsSplit();
		for (int32 Index = 0; Index < Meshes.Num(); ++Index)
		{
			if (Meshes[Index].IsEmpty())
			{
				Refuse(Index == 0 ? FString(TEXT("its scan leaves nothing of its body"))
					: FString::Printf(TEXT("its part %s takes no triangle of its scan"), *Parts[Index - 1]->Part.ToString()));
				continue;
			}
			const FArtCastPart* Part = Index == 0 ? nullptr : Parts[Index - 1];
			FFormMadePiece Piece = Part
				? ArtPiece(PartName(Entry, bSplit, *Part->Part.ToString()), Supports::Cast, { Part->Palette, Part->Working, Part->History }, Frame, true)
				: ArtPiece(FName(ObjectName(Entry, bSplit)), Supports::Cast, Canvas.Finish, Frame, true);
			Meshes[Index].Write(Piece.Mesh);
			OutPieces.Add(MoveTemp(Piece));
		}
	}
}
