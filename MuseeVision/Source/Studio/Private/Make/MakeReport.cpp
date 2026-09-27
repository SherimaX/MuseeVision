#include "Make/MakeReport.h"

#include "Algo/Count.h"
#include "StudioLog.h"

namespace Musee::Studio
{
	int32 FMakeReport::Count(EMadeOutcome Outcome) const
	{
		return Algo::CountIf(Packages, [Outcome](const FMadePackage& Package) { return Package.Outcome == Outcome; });
	}

	double FMakeReport::Seconds() const
	{
		double Total = 0.0;
		for (const FMakeStep& Step : Steps)
		{
			Total += Step.Seconds;
		}
		return Total;
	}

	void FMakeReport::Log() const
	{
		UE_LOG(LogMuseeStudio, Display, TEXT("Make: %d packages: %d written, %d unchanged, %d failed, in %.2f s."),
			Packages.Num(), Count(EMadeOutcome::Written), Count(EMadeOutcome::Unchanged), Count(EMadeOutcome::Failed), Seconds());
		for (const FMakeStep& Step : Steps)
		{
			UE_LOG(LogMuseeStudio, Display, TEXT("Make: step %s took %.2f s."), *Step.Name, Step.Seconds);
		}
		UE_LOG(LogMuseeStudio, Display, TEXT("Make: cast built %d meshes (%lld triangles) in %d batches; PeakPhysMemoryMB %.0f."),
			Cast.PartsBuilt, Cast.TrianglesBuilt, Cast.Batches, PeakPhysMemoryMB);
		UE_LOG(LogMuseeStudio, Display, TEXT("Make: DDC during cast: mesh %lld hits, %lld misses; distance field %lld hits, %lld misses; cards %lld hits, %lld misses."),
			DerivedData.MeshHits, DerivedData.MeshMisses, DerivedData.FieldHits, DerivedData.FieldMisses, DerivedData.CardHits, DerivedData.CardMisses);
		for (const FMadePackage& Package : Packages)
		{
			if (Package.Outcome == EMadeOutcome::Written)
			{
				UE_LOG(LogMuseeStudio, Display, TEXT("Make: wrote %s."), *Package.PackageName);
			}
			else if (Package.Outcome == EMadeOutcome::Failed)
			{
				UE_LOG(LogMuseeStudio, Error, TEXT("Make: %s failed: %s."), *Package.PackageName, *Package.Why);
			}
		}
		UE_LOG(LogMuseeStudio, Display, TEXT("Make: %d findings."), Findings.Num());
		for (const FMuseeFinding& Finding : Findings)
		{
			UE_LOG(LogMuseeStudio, Error, TEXT("Make: finding: %s"), *Finding.ToString());
		}
	}

	bool FMakeReport::HasFailed(const FSoftObjectPath& Object) const
	{
		const FString Package = Object.GetLongPackageName();
		return Packages.ContainsByPredicate([&Package](const FMadePackage& Made)
		{
			return Made.PackageName == Package && Made.Outcome == EMadeOutcome::Failed;
		});
	}

	void FMakeReport::Append(const FMakeReport& Other)
	{
		Packages.Append(Other.Packages);
		Steps.Append(Other.Steps);
		Findings.Append(Other.Findings);
		Cast.PartsBuilt += Other.Cast.PartsBuilt;
		Cast.TrianglesBuilt += Other.Cast.TrianglesBuilt;
		Cast.Batches += Other.Cast.Batches;
		Cast.PeakPhysMemoryMB = FMath::Max(Cast.PeakPhysMemoryMB, Other.Cast.PeakPhysMemoryMB);
		DerivedData.MeshHits += Other.DerivedData.MeshHits;
		DerivedData.MeshMisses += Other.DerivedData.MeshMisses;
		DerivedData.FieldHits += Other.DerivedData.FieldHits;
		DerivedData.FieldMisses += Other.DerivedData.FieldMisses;
		DerivedData.CardHits += Other.DerivedData.CardHits;
		DerivedData.CardMisses += Other.DerivedData.CardMisses;
		PeakPhysMemoryMB = FMath::Max(PeakPhysMemoryMB, Other.PeakPhysMemoryMB);
	}
}
