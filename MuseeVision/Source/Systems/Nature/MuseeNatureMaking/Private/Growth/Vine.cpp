#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The vine form (Vitis vinifera), re-expressed from the as-built pergola's vines (windows/Source/MuseeVision/Salon/
 * SalonStructure.cpp): a climber trained as a gardener ties it in, along the paths its bed gives. Its stem rises from the
 * soil along the first path, thick and twisting at the foot; each further path is a cane tied along a bar. From the
 * canes, at their nodes, short spurs carry the leaves in clusters with gaps between them (the dapple the pergola's shade
 * is known by): palmate leaves on their stalks, turned up to the sky, some hanging.
 *
 * Species: stemRadiusMeters [a cane's, the stem's at its foot]; leavesPerMeter of cane; leafLengthMeters a leaf's
 * blade. Colours: StemColours [the bark]; LeafColours; LeafAgeColours [a young leaf at a cane's tip].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** A spur every so far along a cane: the vine's internode. */
		constexpr double InternodeMeters = 0.16;
		/** A leaf cluster's spread about its spur. */
		constexpr double ClusterRadiusMeters = 0.18;
		/** A leaf's side lobes, as shares of its length, and their spread from the midrib. */
		constexpr double SideLobeShare = 0.75;
		constexpr double SideLobeDegrees = 50.0;
		/** A lobe's width as a share of the leaf's length. */
		constexpr double LobeWidthShare = 0.45;
		/**
		 * The share of spurs that bear a cluster: the as-built canopy kept a cluster where a slow noise stood above its
		 * threshold, about two spurs in three, which leaves the gaps the dapple comes through.
		 */
		constexpr double ClusterShare = 0.66;
		/** How far a stem's radius falls by its top, as a share. */
		constexpr double StemTaperShare = 0.4;

		/** A tube along a path, tapering from one radius to another. */
		void Along(const TArray<FVector>& Path, double From, double To, const FLinearColor& Tint, FNatureSurface& Out)
		{
			TArray<FTubeRing> Rings;
			for (int32 I = 0; I < Path.Num(); ++I)
			{
				const FVector Dir = (Path[FMath::Min(I + 1, Path.Num() - 1)] - Path[FMath::Max(I - 1, 0)]).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
				Rings.Add({ Path[I], Dir, FMath::Lerp(From, To, double(I) / FMath::Max(1, Path.Num() - 1)), Tint });
			}
			AddTube(Out, Rings, 6, true);
		}

		/** A palmate leaf: its middle lobe and two side lobes fanned from the stalk's end, facing up. */
		void Leaf(const FVector& Base, const FVector& Dir, const FVector& Normal, double Length, const FLinearColor& Tint, double Occlusion, FNatureSurface& Out)
		{
			AddCard(Out, Base, Dir, Normal, Length, LobeWidthShare * Length, 0.25, 0.15, 2, Tint, Occlusion);
			for (const double Side : { -1.0, 1.0 })
			{
				const FVector Lobe = RotateAbout(Dir, Normal, FMath::DegreesToRadians(Side * SideLobeDegrees));
				AddCard(Out, Base, Lobe, Normal, SideLobeShare * Length, LobeWidthShare * Length, 0.25, 0.15, 2, Tint, Occlusion);
			}
		}

		void Cluster(const FNatureSpecies& Species, const FVector& Spur, bool bYoung, FRandomStream& Random, FPlantParts& Out)
		{
			const int32 Leaves = FMath::Max(1, FMath::RoundToInt(Draw(Species.LeavesPerMeter, Random) * InternodeMeters));
			for (int32 K = 0; K < Leaves; ++K)
			{
				const double Az = Random.FRandRange(0.0, Tau);
				const FVector Out2(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				const FVector Base = Spur + Out2 * Random.FRandRange(0.02, ClusterRadiusMeters) + FVector(0.0, 0.0, Random.FRandRange(-0.08, 0.04));
				AddStalk(Out.FindOrAdd(Organ::Stem), Spur, Base, 0.0015, FVector(0.0, 0.0, 0.02), Colour(Species.StemColours, 0), Colour(Species.LeafColours, 0));
				const FVector Dir = (Out2 + FVector(0.0, 0.0, Random.FRandRange(-0.6, 0.15))).GetSafeNormal();
				FVector Normal = (FVector::UpVector + FVector(Random.FRandRange(-0.45, 0.45), Random.FRandRange(-0.45, 0.45), 0.0)).GetSafeNormal();
				Normal = (Normal - Dir * FVector::DotProduct(Normal, Dir)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
				const FLinearColor Tint = Vary(bYoung && Random.FRand() < 0.5 ? Colour(Species.LeafAgeColours, 0) : AnyColour(Species.LeafColours, Random), Random, 6.0, 0.12, 0.14);
				Leaf(Base, Dir, Normal, Draw(Species.LeafLengthMeters, Random), Tint, 0.85, Out.FindOrAdd(Organ::Leaf));
			}
		}

		/** The points along a path, at a spacing. */
		TArray<FVector> Stations(const TArray<FVector>& Path, double Step)
		{
			TArray<FVector> Out;
			for (int32 I = 0; I + 1 < Path.Num(); ++I)
			{
				const double Span = FVector::Dist(Path[I], Path[I + 1]);
				for (double S = 0.5 * Step; S < Span; S += Step)
				{
					Out.Add(FMath::Lerp(Path[I], Path[I + 1], S / Span));
				}
			}
			return Out;
		}

		void GrowVine(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			if (Site.Trained.IsEmpty())
			{
				return;
			}
			const double Cane = Species.StemRadiusMeters[0];
			const double Foot = Species.StemRadiusMeters[1];
			const FLinearColor Bark = Colour(Species.StemColours, 0);
			Along(Site.Trained[0], Foot, Foot * (1.0 - StemTaperShare), Bark, Out.FindOrAdd(Organ::Trunk));
			for (int32 C = 1; C < Site.Trained.Num(); ++C)
			{
				const TArray<FVector>& Path = Site.Trained[C];
				Along(Path, FMath::Max(Cane, Foot * (1.0 - StemTaperShare) * 0.5), Cane, Bark, Out.FindOrAdd(Organ::Stem));
				const TArray<FVector> Spurs = Stations(Path, InternodeMeters);
				for (int32 S = 0; S < Spurs.Num(); ++S)
				{
					if (Random.FRand() < ClusterShare)
					{
						Cluster(Species, Spurs[S], S >= Spurs.Num() - 2, Random, Out);
					}
				}
			}
		}

		TArray<FString> VineFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.StemRadiusMeters, TEXT("stemRadiusMeters [a cane's, the stem's at its foot]"), Out);
			NeedRange(Species.LeavesPerMeter, TEXT("leavesPerMeter (of cane)"), Out);
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters"), Out);
			NeedColours(Species.StemColours, 1, TEXT("stemColours (the bark)"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours(Species.LeafAgeColours, 1, TEXT("leafAgeColours (a young leaf)"), Out);
			return Out;
		}
	}

	const FForm& VineForm()
	{
		static const FForm Form{ TEXT("vine"), { Organ::Trunk, Organ::Stem, Organ::Leaf }, false, &GrowVine, &VineFaults };
		return Form;
	}
}
