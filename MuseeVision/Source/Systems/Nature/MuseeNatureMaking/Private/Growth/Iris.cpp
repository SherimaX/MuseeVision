#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The iris form (Iris laevigata and its kin), re-expressed from the as-built pond's marginal planting
 * (windows/Source/MuseeVision/Salon/SalonPond.cpp, IrisClump and IrisFlower): a clump of fans of sword leaves, each
 * fan in its own plane, leaning a little out over the water; flowers on stalks a little below the leaves' tips, each
 * with three broad falls (a white signal down each, yellow at its throat), three upright standards and three pale
 * style arms, in a spathe of two green bracts; a furled bud or two.
 *
 * Species colours: LeafColours the swords' greens; LeafEdgeColour their sheathing feet; LeafAgeColours [a browned
 * tip]; FlowerColours [the petals' violet, its deep veining, the style arms' pale]; EyeColours [the throat's yellow,
 * the signal's white]; StemColours [the stalk's foot, its top].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** The span across the falls the petal proportions below are drawn at (the as-built flower). */
		constexpr double DrawnFlowerSpan = 0.13;
		/** A leaf leaning over the rim is drawn again, leaning further in, up to this many times. */
		constexpr int32 LeafTries = 5;
		/** A leaf keeps this far inside the rim below the height where it may overhang it. */
		constexpr double LeafClearance = 0.02;

		void Flower(const FNatureSpecies& Species, FPlantParts& Out, const FVector& Top, const FVector& Axis, double Scale, FRandomStream& Random)
		{
			const FLinearColor Violet = Vary(Colour(Species.FlowerColours, 0), Random, 6.0, 0.06, 0.1);
			const FLinearColor Deep = Mix(Violet, Colour(Species.FlowerColours, 1), 0.55);
			const FLinearColor Pale = Mix(Violet, Colour(Species.FlowerColours, 2), 0.55);
			const FLinearColor Throat = Colour(Species.EyeColours, 0), Signal = Colour(Species.EyeColours, 1);
			FNatureSurface& Petals = Out.FindOrAdd(Organ::Flower);
			const double Az0 = Random.FRandRange(0.0, Tau);
			for (int32 K = 0; K < 3; ++K)
			{
				const double Az = Az0 + Tau * K / 3 + Random.FRandRange(-0.12, 0.12);
				const FVector Radial(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				// The fall: broad, out and down, the signal a narrow white line (yellow at its throat) down its middle.
				const FPetal Fall{ 0.072 * Scale, 0.042 * Scale, 28.0 + Random.FRandRange(-6.0, 6.0), -78.0, 2.0, -0.06, 0.72, 0.16, 0.05, 8, 10 };
				AddPetal(Petals, Top + Radial * 0.006, Radial, Fall, [&](double X, double S)
				{
					const double Line = FMath::Clamp(1.0 - FMath::Abs(X) / 0.34, 0.0, 1.0) * FMath::Clamp((0.62 - S) / 0.2, 0.0, 1.0);
					return Mix(Mix(Violet, Deep, 0.35 * S + 0.25 * FMath::Abs(X)), Mix(Throat, Signal, FMath::Clamp(S / 0.25, 0.0, 1.0)), Line);
				});
				// The standard between the falls: narrow, upright.
				const FVector Between(FMath::Cos(Az + Tau / 6), FMath::Sin(Az + Tau / 6), 0.0);
				const FPetal Standard{ 0.05 * Scale, 0.018 * Scale, 72.0 + Random.FRandRange(-6.0, 6.0), -22.0, 2.0, 0.1, 0.72, 0.16, 0.02, 4, 7 };
				AddPetal(Petals, Top + Between * 0.003 + FVector(0.0, 0.0, 0.004), Between, Standard, [&](double, double S) { return Mix(Violet, Deep, 0.3 * S); });
				// The style arm over the fall's base: pale and petal-like.
				const FPetal Style{ 0.034 * Scale, 0.013 * Scale, 22.0, -14.0, 2.0, 0.25, 0.72, 0.16, 0.0, 4, 5 };
				AddPetal(Petals, Top + Radial * 0.004 + FVector(0.0, 0.0, 0.008), Radial, Style, [&](double, double S) { return Mix(Pale, Violet, 0.3 * S); });
			}
			// The spathe: two green bracts clasping the ovary below the flower.
			for (const double Az : { Az0 + 0.5, Az0 + 0.5 + UE_DOUBLE_PI })
			{
				const FVector R(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				FBlade Bract{ 0.07, 0.014, -12.0, 0.0, 0.0, 0.0, 0.7, 0.6, 2, 5, Colour(Species.LeafColours, 0), Colour(Species.LeafAgeColours, 0) };
				AddBlade(Out.FindOrAdd(Organ::Leaf), Top - Axis * 0.06 + R * 0.004, (Axis + R * 0.18).GetSafeNormal(), R, Bract);
			}
		}

		void Fans(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			const int32 Fans = Draw(Species.ShootsPerPlant, Random);
			const double Spread = Draw(Species.ReachMeters, Random);
			for (int32 F = 0; F < Fans; ++F)
			{
				const double R = Random.FRandRange(0.0, Spread);
				const double A = Random.FRandRange(0.0, Tau);
				const FVector Foot = Site.Crown + FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.0);
				const double Plane = Random.FRandRange(0.0, UE_DOUBLE_PI);
				const FVector Along(FMath::Cos(Plane), FMath::Sin(Plane), 0.0);
				const FVector Across(-Along.Y, Along.X, 0.0);
				const int32 Leaves = Draw(Species.LeavesPerShoot, Random);
				const double Tall = Draw(Species.LeafLengthMeters, Random);
				for (int32 L = 0; L < Leaves; ++L)
				{
					const double X = Leaves > 1 ? -1.0 + 2.0 * L / (Leaves - 1) : 0.0;
					for (int32 Try = 0; Try < LeafTries; ++Try)
					{
						const double Splay = FMath::DegreesToRadians(X * 24.0 + Random.FRandRange(-5.0, 5.0));
						// The fan leans out over the water, more on each try.
						const FVector Dir = (FVector::UpVector * FMath::Cos(Splay) + Along * FMath::Sin(Splay) + FVector(Site.Inward, 0.0) * (0.06 + 0.08 * Try)).GetSafeNormal();
						FBlade Sword;
						Sword.Length = Tall * (1.0 - 0.28 * FMath::Abs(X)) * Random.FRandRange(0.85, 1.05);
						Sword.Width = Draw(Species.LeafWidthMeters, Random);
						Sword.Curl = (Random.FRand() < 0.5 ? -1.0 : 1.0) * Random.FRandRange(8.0, 30.0) * (0.5 + FMath::Abs(X));
						Sword.Twist = Random.FRandRange(-12.0, 12.0);
						Sword.TipShape = 1.4;
						Sword.BaseWidth = 0.9;
						Sword.Along = 12;
						Sword.Cup = 0.02;
						Sword.Root = Mix(FromHex(Species.LeafEdgeColour), Colour(Species.LeafColours, 0), Random.FRand() * 0.5);
						Sword.Tip = Random.FRand() < 0.1 ? Colour(Species.LeafAgeColours, 0) : Vary(Colour(Species.LeafColours, 0), Random, 5.0, 0.08, 0.1);
						FNatureSurface Leaf;
						AddBlade(Leaf, Foot + Along * (X * 0.006), Dir, Across, Sword);
						if (Site.IsClearOf(Leaf, LeafClearance) || Try == LeafTries - 1)
						{
							Out.FindOrAdd(Organ::Leaf).Append(Leaf);
							break;
						}
					}
				}
			}
		}

		void GrowIris(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			Fans(Species, Site, Random, Out);
			const bool bFlowering = Random.FRand() < Species.FloweringShare;
			const int32 Flowers = bFlowering ? Draw(Species.FlowersPerPlant, Random) : 0;
			const FLinearColor StalkFoot = Colour(Species.StemColours, 0), StalkTop = Colour(Species.StemColours, 1);
			for (int32 K = 0; K < Flowers; ++K)
			{
				const double A = Random.FRandRange(0.0, Tau);
				const FVector Foot = Site.Crown + FVector(0.06 * FMath::Cos(A), 0.06 * FMath::Sin(A), 0.0);
				const FVector Top = Site.Crown + FVector(Site.Inward, 0.0) * Random.FRandRange(0.04, 0.1)
					+ FVector(Random.FRandRange(-0.04, 0.04), Random.FRandRange(-0.04, 0.04), Draw(Species.FlowerHeightMeters, Random));
				const FVector Axis = (Top - Foot).GetSafeNormal();
				const FVector Bow = FVector(Random.FRandRange(-1.0, 1.0), Random.FRandRange(-1.0, 1.0), 0.0).GetSafeNormal() * 0.02;
				AddStalk(Out.FindOrAdd(Organ::Stem), Foot, Top - Axis * 0.01, 0.0036, Bow, StalkFoot, StalkTop);
				Flower(Species, Out, Top, Axis, Draw(Species.FlowerSizeMeters, Random) / DrawnFlowerSpan, Random);
				if (Random.FRand() < 0.6)
				{
					// A bud: a furled violet spike in its spathe, on a shorter stalk.
					const FVector BudTop = Top + FVector(Random.FRandRange(-0.06, 0.06), Random.FRandRange(-0.06, 0.06), -Random.FRandRange(0.06, 0.12));
					const FVector BudAxis = (BudTop - Foot).GetSafeNormal();
					AddStalk(Out.FindOrAdd(Organ::Stem), Foot + FVector(0.01, 0.0, 0.0), BudTop, 0.0032, -Bow, StalkFoot, StalkTop);
					AddEllipsoid(Out.FindOrAdd(Organ::Flower), BudTop + BudAxis * 0.03, BudAxis, 0.008, 0.032, 8, 6, Mix(Colour(Species.FlowerColours, 0), Colour(Species.FlowerColours, 1), 0.4), 0.9);
				}
			}
		}

		TArray<FString> IrisFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters"), Out);
			NeedRange(Species.LeafWidthMeters, TEXT("leafWidthMeters"), Out);
			NeedRange(Species.FlowerSizeMeters, TEXT("flowerSizeMeters"), Out);
			NeedRange(Species.FlowerHeightMeters, TEXT("flowerHeightMeters"), Out);
			NeedRange(Species.ShootsPerPlant, TEXT("shootsPerPlant (fans)"), Out);
			NeedRange(Species.LeavesPerShoot, TEXT("leavesPerShoot (swords a fan)"), Out);
			NeedRange(Species.FlowersPerPlant, TEXT("flowersPerPlant"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours({ Species.LeafEdgeColour }, 1, TEXT("leafEdgeColour"), Out);
			NeedColours(Species.LeafAgeColours, 1, TEXT("leafAgeColours"), Out);
			NeedColours(Species.FlowerColours, 3, TEXT("flowerColours"), Out);
			NeedColours(Species.EyeColours, 2, TEXT("eyeColours"), Out);
			NeedColours(Species.StemColours, 2, TEXT("stemColours"), Out);
			return Out;
		}
	}

	const FForm& IrisForm()
	{
		static const FForm Form{ TEXT("iris"), { Organ::Leaf, Organ::Flower, Organ::Stem }, false, &GrowIris, &IrisFaults };
		return Form;
	}
}
