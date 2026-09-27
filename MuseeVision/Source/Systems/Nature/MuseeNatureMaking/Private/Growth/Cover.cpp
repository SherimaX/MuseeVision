#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The ground covers, grown once over their bed's region (its outline less the water in it):
 * - `moss` (Haplocladium): a close carpet of shoots a few millimetres to centimetres deep, drawn as the carpet's upper
 *   surface over the region at the shoots' length above the soil (Nature makes it as a face over the region, its tint
 *   varying from shoot to shoot). Species: leafLengthMeters the shoots' length, the carpet's depth; LeafColours.
 * - `grass` (a sward, Festuca): tufts of fine blades set at the species' density over the region, each tuft's blades
 *   arching out from its crown, a few withered, and in the season the flowering culms standing above the sward.
 *   Species: plantsPerSquareMeter the tufts; shootsPerPlant the blades of a tuft; leafLengthMeters, leafWidthMeters;
 *   flowerHeightMeters and flowersPerPlant the culms of a flowering tuft; LeafColours; LeafAgeColours [a withered
 *   blade]; StemColours [the culm]; SeedColours [the panicle].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** The share of withered blades in an uncut sward in early summer. */
		constexpr double WitheredShare = 0.12;
		/** A tuft's blades rise from a crown this wide. */
		constexpr double TuftRadiusMeters = 0.015;
		/** A culm's radius, and its panicle's length and radius. */
		constexpr double CulmRadiusMeters = 0.0008;
		constexpr double PanicleLengthMeters = 0.06;
		constexpr double PanicleRadiusMeters = 0.008;

		/** The region's bounds in plan. */
		FBox2D Bounds(const TArray<FVector2D>& Region)
		{
			FBox2D Box(ForceInit);
			for (const FVector2D& Corner : Region)
			{
				Box += Corner;
			}
			return Box;
		}

		/** The region's area, by the shoelace sum less the holes'. */
		double Area(const FPlantSite& Site)
		{
			const auto Shoelace = [](const TArray<FVector2D>& Corners)
			{
				double Sum = 0.0;
				for (int32 I = 0; I < Corners.Num(); ++I)
				{
					Sum += FVector2D::CrossProduct(Corners[I], Corners[(I + 1) % Corners.Num()]);
				}
				return 0.5 * FMath::Abs(Sum);
			};
			double Out = Shoelace(Site.Region);
			for (const TArray<FVector2D>& Hole : Site.Holes)
			{
				Out -= Shoelace(Hole);
			}
			return FMath::Max(0.0, Out);
		}

		void Tuft(const FNatureSpecies& Species, const FVector& Crown, FRandomStream& Random, FPlantParts& Out)
		{
			const int32 Blades = Draw(Species.ShootsPerPlant, Random);
			for (int32 B = 0; B < Blades; ++B)
			{
				const double Az = Random.FRandRange(0.0, Tau);
				const FVector Out2(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				const FVector Dir = (FVector::UpVector + Out2 * Random.FRandRange(0.1, 0.5)).GetSafeNormal();
				FBlade Blade;
				Blade.Length = Draw(Species.LeafLengthMeters, Random);
				Blade.Width = Draw(Species.LeafWidthMeters, Random);
				Blade.Droop = Random.FRandRange(1.5, 4.0);
				Blade.Twist = Random.FRandRange(-30.0, 30.0);
				Blade.TipShape = 1.2;
				Blade.BaseWidth = 0.8;
				Blade.Across = 1;
				Blade.Along = 3;
				Blade.Root = Colour(Species.LeafColours, 0);
				Blade.Tip = Random.FRand() < WitheredShare ? Colour(Species.LeafAgeColours, 0) : Vary(AnyColour(Species.LeafColours, Random), Random, 5.0, 0.1, 0.12);
				const FVector Foot = Crown + Out2 * Random.FRandRange(0.0, TuftRadiusMeters);
				AddBlade(Out.FindOrAdd(Organ::Leaf), Foot, Dir, FVector(-Dir.X, -Dir.Y, 1.0 - Dir.Z).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector), Blade);
			}
			const int32 Culms = Random.FRand() < Species.FloweringShare ? Draw(Species.FlowersPerPlant, Random) : 0;
			for (int32 C = 0; C < Culms; ++C)
			{
				const FVector Top = Crown + FVector(Random.FRandRange(-0.03, 0.03), Random.FRandRange(-0.03, 0.03), Draw(Species.FlowerHeightMeters, Random));
				AddStalk(Out.FindOrAdd(Organ::Stem), Crown, Top, CulmRadiusMeters, FVector(0.01, 0.0, 0.0), Colour(Species.StemColours, 0), Colour(Species.StemColours, 0));
				AddEllipsoid(Out.FindOrAdd(Organ::Seed), Top + FVector(0.0, 0.0, 0.5 * PanicleLengthMeters), FVector::UpVector, PanicleRadiusMeters,
					0.5 * PanicleLengthMeters, 5, 4, AnyColour(Species.SeedColours, Random), 0.9);
			}
		}

		void GrowGrass(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			if (Site.Region.Num() < 3)
			{
				return;
			}
			const FBox2D Box = Bounds(Site.Region);
			const int32 Tufts = FMath::RoundToInt(Area(Site) * Draw(Species.PlantsPerSquareMeter, Random));
			for (int32 T = 0, Tries = 0; T < Tufts && Tries < 4 * Tufts; ++Tries)
			{
				const FVector2D At(Random.FRandRange(Box.Min.X, Box.Max.X), Random.FRandRange(Box.Min.Y, Box.Max.Y));
				if (Site.InRegion(At))
				{
					Tuft(Species, FVector(At.X, At.Y, 0.0), Random, Out);
					++T;
				}
			}
		}

		TArray<FString> MossFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters (the shoots', the carpet's depth)"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			return Out;
		}

		TArray<FString> GrassFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.PlantsPerSquareMeter, TEXT("plantsPerSquareMeter (tufts)"), Out);
			NeedRange(Species.ShootsPerPlant, TEXT("shootsPerPlant (blades of a tuft)"), Out);
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters"), Out);
			NeedRange(Species.LeafWidthMeters, TEXT("leafWidthMeters"), Out);
			NeedRange(Species.FlowerHeightMeters, TEXT("flowerHeightMeters (the culms)"), Out);
			NeedRange(Species.FlowersPerPlant, TEXT("flowersPerPlant (culms of a flowering tuft)"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours(Species.LeafAgeColours, 1, TEXT("leafAgeColours (a withered blade)"), Out);
			NeedColours(Species.StemColours, 1, TEXT("stemColours (the culm)"), Out);
			NeedColours(Species.SeedColours, 1, TEXT("seedColours (the panicle)"), Out);
			return Out;
		}
	}

	const FForm& MossForm()
	{
		// A carpet: Nature lays its surface over the region itself, so it has no grower of its own.
		static const FForm Form{ TEXT("moss"), { Organ::Leaf }, false, nullptr, &MossFaults, EPlanting::OverRegion };
		return Form;
	}

	const FForm& GrassForm()
	{
		static const FForm Form{ TEXT("grass"), { Organ::Leaf, Organ::Stem, Organ::Seed }, false, &GrowGrass, &GrassFaults, EPlanting::OverRegion };
		return Form;
	}
}
