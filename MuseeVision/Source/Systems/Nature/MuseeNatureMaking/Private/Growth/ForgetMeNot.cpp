#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The forget-me-not form (Myosotis), re-expressed from the as-built pond's patches
 * (windows/Source/MuseeVision/Salon/SalonPond.cpp, ForgetMeNots): a patch of low leafy stems leaning out, oblong
 * leaves alternate up each, and at each top a coiled spray of small flowers opening from the outside in, the youngest
 * still pink buds in the coil.
 *
 * Species colours: LeafColours the leaves' greens; StemColours [the stem's foot, its top]; FlowerColours [the open
 * flower's blue, the bud's pink].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** The spray coils through 3.4 radians from its oldest flower to its youngest bud, the as-built scorpioid cyme. */
		constexpr double CoilRadians = 3.4;
		/** The last three tenths of the spray are still buds. */
		constexpr double BudShare = 0.7;

		void Spray(const FNatureSpecies& Species, const FVector& Top, FRandomStream& Random, FPlantParts& Out)
		{
			const int32 Flowers = Draw(Species.FlowersPerPlant, Random);
			const double Az0 = Random.FRandRange(0.0, Tau);
			for (int32 F = 0; F < Flowers; ++F)
			{
				const double T = double(F) / Flowers;
				const double Angle = Az0 + T * CoilRadians;
				const FVector Around(FMath::Cos(Angle), FMath::Sin(Angle), 0.0);
				const FVector Centre = Top + Around * (0.012 * (1.0 - 0.6 * T)) + FVector(0.0, 0.0, 0.012 * T - 0.004 * (F % 2));
				const FVector Facing = (FVector::UpVector + Around * 0.7).GetSafeNormal();
				if (T > BudShare)
				{
					AddEllipsoid(Out.FindOrAdd(Organ::Flower), Centre, Facing, 0.0018, 0.0025, 5, 4, Colour(Species.FlowerColours, 1), 1.0);
				}
				else
				{
					AddFlowerCard(Out.FindOrAdd(Organ::Flower), Centre, Facing, Random.FRandRange(0.0, Tau), Draw(Species.FlowerSizeMeters, Random), 0.12,
						Vary(Colour(Species.FlowerColours, 0), Random, 6.0, 0.08, 0.06), 1.0);
				}
			}
		}

		void GrowForgetMeNot(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			const double Radius = Draw(Species.ReachMeters, Random);
			const int32 Stems = Draw(Species.ShootsPerPlant, Random);
			for (int32 K = 0; K < Stems; ++K)
			{
				const double A = Random.FRandRange(0.0, Tau);
				const double R = Radius * FMath::Sqrt(Random.FRand());
				const FVector Foot = Site.Crown + FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.0);
				const double Lean = Random.FRandRange(0.1, 0.5);
				const FVector2D LeanTo = (FVector2D(FMath::Cos(A), FMath::Sin(A)) * 0.5 + Site.Inward).GetSafeNormal();
				const FVector Top = Foot + FVector(LeanTo.X * Lean * 0.2, LeanTo.Y * Lean * 0.2, Draw(Species.FlowerHeightMeters, Random));
				AddStalk(Out.FindOrAdd(Organ::Stem), Foot, Top, 0.0016, FVector::ZeroVector, Colour(Species.StemColours, 0), Colour(Species.StemColours, 1));
				// Oblong leaves, alternate up the stem.
				const int32 Leaves = Draw(Species.LeavesPerShoot, Random);
				for (int32 L = 0; L < Leaves; ++L)
				{
					const FVector At = FMath::Lerp(Foot, Top, 0.25 + 0.6 * L / Leaves);
					const double Az = Random.FRandRange(0.0, Tau);
					const FVector Dir = (FVector(FMath::Cos(Az), FMath::Sin(Az), 0.0) + FVector::UpVector * 0.5).GetSafeNormal();
					FVector N = FVector::CrossProduct(Dir, FVector(-FMath::Sin(Az), FMath::Cos(Az), 0.0)).GetSafeNormal();
					N = N.Z < 0.0 ? -N : N;
					AddCard(Out.FindOrAdd(Organ::Leaf), At, Dir, N, Draw(Species.LeafLengthMeters, Random), Draw(Species.LeafWidthMeters, Random), 0.25, 0.15, 2,
						Vary(Colour(Species.LeafColours, 0), Random, 4.0, 0.08, 0.1), 0.9);
				}
				Spray(Species, Top, Random, Out);
			}
		}

		TArray<FString> ForgetMeNotFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters"), Out);
			NeedRange(Species.LeafWidthMeters, TEXT("leafWidthMeters"), Out);
			NeedRange(Species.FlowerSizeMeters, TEXT("flowerSizeMeters"), Out);
			NeedRange(Species.FlowerHeightMeters, TEXT("flowerHeightMeters (the stems)"), Out);
			NeedRange(Species.ShootsPerPlant, TEXT("shootsPerPlant (stems)"), Out);
			NeedRange(Species.LeavesPerShoot, TEXT("leavesPerShoot"), Out);
			NeedRange(Species.FlowersPerPlant, TEXT("flowersPerPlant (a spray's flowers)"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours(Species.StemColours, 2, TEXT("stemColours"), Out);
			NeedColours(Species.FlowerColours, 2, TEXT("flowerColours"), Out);
			return Out;
		}
	}

	const FForm& ForgetMeNotForm()
	{
		static const FForm Form{ TEXT("forget-me-not"), { Organ::Leaf, Organ::Flower, Organ::Stem }, false, &GrowForgetMeNot, &ForgetMeNotFaults };
		return Form;
	}
}
