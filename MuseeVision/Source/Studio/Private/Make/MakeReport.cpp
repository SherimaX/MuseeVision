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
	}
}
