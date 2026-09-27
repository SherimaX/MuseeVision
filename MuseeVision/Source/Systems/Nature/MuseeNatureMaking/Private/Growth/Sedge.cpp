#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The sedge form (Carex), re-expressed from the as-built pond's tufts (windows/Source/MuseeVision/Salon/SalonPond.cpp,
 * Sedge): a tussock of narrow arching blades, a few browned at the tip, and a few culms standing through it, each with
 * three brown spikes at its top.
 *
 * Species colours: LeafEdgeColour the blades' pale feet; LeafColours their greens; LeafAgeColours [a browned tip];
 * StemColours [the culm's foot, its top]; SeedColours [the top spike (male), the lower spikes (female)].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** A blade leaning over the rim is drawn again, leaning further in, up to this many times. */
		constexpr int32 BladeTries = 5;
		/** A blade keeps this far inside the rim below the height where it may overhang it. */
		constexpr double BladeClearance = 0.015;
		/** A culm's spikes: three, 30 mm apart down its top. */
		constexpr int32 Spikes = 3;
		constexpr double SpikeStep = 0.03;

		void Blades(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			const int32 Count = Draw(Species.ShootsPerPlant, Random);
			const double Tussock = Draw(Species.ReachMeters, Random);
			for (int32 B = 0; B < Count; ++B)
			{
				for (int32 Try = 0; Try < BladeTries; ++Try)
				{
					const double Az = Random.FRandRange(0.0, Tau);
					const FVector Out2(FMath::Cos(Az), FMath::Sin(Az), 0.0);
					const FVector Dir = (FVector::UpVector + Out2 * Random.FRandRange(0.15, 0.55) + FVector(Site.Inward, 0.0) * (0.25 * Try)).GetSafeNormal();
					FBlade Blade;
					Blade.Length = Draw(Species.LeafLengthMeters, Random);
					Blade.Width = Draw(Species.LeafWidthMeters, Random);
					Blade.Droop = Random.FRandRange(2.0, 4.5);
					Blade.Twist = Random.FRandRange(-40.0, 40.0);
					Blade.TipShape = 1.2;
					Blade.BaseWidth = 0.8;
					Blade.Along = 9;
					Blade.Cup = 0.15;
					Blade.Root = FromHex(Species.LeafEdgeColour);
					Blade.Tip = Random.FRand() < 0.15 ? Colour(Species.LeafAgeColours, 0) : Vary(Colour(Species.LeafColours, 0), Random, 5.0, 0.08, 0.12);
					FNatureSurface Surface;
					const FVector Foot = Site.Crown + FVector(Random.FRandRange(-Tussock, Tussock), Random.FRandRange(-Tussock, Tussock), 0.0);
					AddBlade(Surface, Foot, Dir, FVector(-Dir.X, -Dir.Y, 1.0 - Dir.Z).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector), Blade);
					if (Site.IsClearOf(Surface, BladeClearance) || Try == BladeTries - 1)
					{
						Out.FindOrAdd(Organ::Leaf).Append(Surface);
						break;
					}
				}
			}
		}

		void GrowSedge(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			Blades(Species, Site, Random, Out);
			const int32 Culms = Random.FRand() < Species.FloweringShare ? Draw(Species.FlowersPerPlant, Random) : 0;
			for (int32 C = 0; C < Culms; ++C)
			{
				const FVector Foot = Site.Crown + FVector(Random.FRandRange(-0.02, 0.02), Random.FRandRange(-0.02, 0.02), 0.0);
				const FVector Top = Site.Crown + FVector(Site.Inward, 0.0) * Random.FRandRange(0.0, 0.08)
					+ FVector(Random.FRandRange(-0.06, 0.06), Random.FRandRange(-0.06, 0.06), Draw(Species.FlowerHeightMeters, Random));
				AddStalk(Out.FindOrAdd(Organ::Stem), Foot, Top, 0.0018, FVector(0.01, 0.0, 0.0), Colour(Species.StemColours, 0), Colour(Species.StemColours, 1));
				const FVector Axis = (Top - Foot).GetSafeNormal();
				for (int32 S = 0; S < Spikes; ++S)
				{
					const FVector Side = FVector(Random.FRandRange(-1.0, 1.0), Random.FRandRange(-1.0, 1.0), 0.0).GetSafeNormal();
					AddEllipsoid(Out.FindOrAdd(Organ::Seed), Top - Axis * (SpikeStep * S) + Side * 0.006, (Axis + Side * 0.4).GetSafeNormal(), 0.0032, 0.014, 6, 5,
						Colour(Species.SeedColours, S == 0 ? 0 : 1), 0.9);
				}
			}
		}

		TArray<FString> SedgeFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters"), Out);
			NeedRange(Species.LeafWidthMeters, TEXT("leafWidthMeters"), Out);
			NeedRange(Species.FlowerHeightMeters, TEXT("flowerHeightMeters (the culms)"), Out);
			NeedRange(Species.ShootsPerPlant, TEXT("shootsPerPlant (blades)"), Out);
			NeedRange(Species.FlowersPerPlant, TEXT("flowersPerPlant (culms)"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours({ Species.LeafEdgeColour }, 1, TEXT("leafEdgeColour"), Out);
			NeedColours(Species.LeafAgeColours, 1, TEXT("leafAgeColours"), Out);
			NeedColours(Species.StemColours, 2, TEXT("stemColours"), Out);
			NeedColours(Species.SeedColours, 2, TEXT("seedColours"), Out);
			return Out;
		}
	}

	const FForm& SedgeForm()
	{
		static const FForm Form{ TEXT("sedge"), { Organ::Leaf, Organ::Stem, Organ::Seed }, false, &GrowSedge, &SedgeFaults };
		return Form;
	}
}
