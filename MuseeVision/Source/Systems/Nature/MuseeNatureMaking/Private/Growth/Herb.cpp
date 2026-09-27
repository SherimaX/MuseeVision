#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The herb form: a meadow flower or a bulb, one plant at each of its bed's points. A rosette of basal leaves (a
 * scabious's or a buttercup's lanceolate blades, a narcissus's or a tulip's straps) and a few upright stems, each ending
 * in a flower: a ring of petals standing as the species' shape says (`cup` rising, `flat` spread, `reflexed` swept back,
 * `head` a flat pincushion of florets), round its eye (a buttercup's boss, a sneezeweed's dome), with a narcissus's
 * corona where the species has one. Out of flower (a bulb in June) it is its leaves alone.
 *
 * Species: leavesPerShoot the basal leaves; leafLengthMeters, leafWidthMeters; shootsPerPlant the stems;
 * flowerHeightMeters their height; flowersPerPlant the flowers of a flowering plant; flowerSizeMeters a flower's span;
 * petalsPerFlower; flowerShape; coronaShare. Colours: LeafColours; StemColours [the stems]; FlowerColours [the petals'
 * foot, their tip]; EyeColours [the eye, the corona].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** How a flower's petals stand, by its shape: their rise above the level at the base, and their curl by the tip. */
		struct FStance
		{
			double Elevation;
			double Curl;
		};

		FStance StanceOf(FName Shape)
		{
			if (Shape == TEXT("cup")) { return { 55.0, 15.0 }; }
			if (Shape == TEXT("reflexed")) { return { -10.0, -35.0 }; }
			return { 12.0, 0.0 };
		}

		/** The stem's radius, a petal's width as a share of its length, and the eye's radius as a share of the flower's span. */
		constexpr double StemRadiusMeters = 0.0018;
		constexpr double PetalWidthShare = 0.55;
		constexpr double EyeShare = 0.14;
		/** A head of florets (a scabious): a shallow dome of this share of its span in height, drawn in rings. */
		constexpr double HeadDomeShare = 0.18;

		void Flower(const FNatureSpecies& Species, const FVector& Top, const FVector& Axis, FRandomStream& Random, FPlantParts& Out)
		{
			const double Span = Draw(Species.FlowerSizeMeters, Random);
			const FLinearColor Foot = Colour(Species.FlowerColours, 0);
			const FLinearColor Tip = Vary(Colour(Species.FlowerColours, 1), Random, 4.0, 0.06, 0.08);
			if (Species.FlowerShape == TEXT("head"))
			{
				const double R = 0.5 * Span;
				AddLathe(Out.FindOrAdd(Organ::Flower), Top, Axis, { FVector2D(R, 0.0), FVector2D(0.7 * R, 0.6 * HeadDomeShare * Span), FVector2D(0.0, HeadDomeShare * Span) }, 14,
					{ Tip, Mix(Tip, Foot, 0.5), Foot }, 1.0);
				return;
			}
			const FStance Stance = StanceOf(Species.FlowerShape);
			const int32 Petals = FMath::Max(3, Draw(Species.PetalsPerFlower, Random));
			const double Length = 0.5 * Span;
			const double Az0 = Random.FRandRange(0.0, Tau);
			for (int32 K = 0; K < Petals; ++K)
			{
				const double Az = Az0 + Tau * K / Petals;
				const FVector Radial(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				const double Width = FMath::Min(Length * PetalWidthShare, 1.8 * Tau * Length / Petals);
				const FPetal Petal{ Length, Width, Stance.Elevation + Random.FRandRange(-6.0, 6.0), Stance.Curl, 1.5, 0.15, 0.9, 0.3, 0.0, 3, 5 };
				AddPetal(Out.FindOrAdd(Organ::Flower), Top + Radial * 0.002, Radial, Petal, [&](double, double S) { return Mix(Foot, Tip, S); });
			}
			const double Eye = EyeShare * Span;
			AddEllipsoid(Out.FindOrAdd(Organ::Eye), Top + FVector(0.0, 0.0, 0.3 * Eye), Axis, Eye, 0.6 * Eye, 8, 5, Colour(Species.EyeColours, 0), 0.9);
			if (Species.CoronaShare > 0.0f)
			{
				const double Corona = Species.CoronaShare * Length;
				AddLathe(Out.FindOrAdd(Organ::Eye), Top, Axis, { FVector2D(1.3 * Eye, Corona), FVector2D(Eye, 0.3 * Corona), FVector2D(0.8 * Eye, 0.0) }, 12,
					{ Colour(Species.EyeColours, 1) }, 0.95);
			}
		}

		void GrowHerb(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			const int32 Leaves = Draw(Species.LeavesPerShoot, Random);
			for (int32 L = 0; L < Leaves; ++L)
			{
				const double Az = Random.FRandRange(0.0, Tau);
				const FVector Out2(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				const FVector Dir = (FVector::UpVector + Out2 * Random.FRandRange(0.4, 1.2)).GetSafeNormal();
				FBlade Blade;
				Blade.Length = Draw(Species.LeafLengthMeters, Random);
				Blade.Width = Draw(Species.LeafWidthMeters, Random);
				Blade.Droop = Random.FRandRange(1.0, 3.0);
				Blade.Twist = Random.FRandRange(-20.0, 20.0);
				Blade.TipShape = 0.9;
				Blade.BaseWidth = 0.3;
				Blade.Cup = 0.1;
				Blade.Across = 2;
				Blade.Along = 5;
				Blade.Root = Colour(Species.LeafColours, 0);
				Blade.Tip = Vary(AnyColour(Species.LeafColours, Random), Random, 4.0, 0.08, 0.1);
				AddBlade(Out.FindOrAdd(Organ::Leaf), Site.Crown, Dir, FVector(-Dir.X, -Dir.Y, 1.0 - Dir.Z).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector), Blade);
			}
			const int32 Flowers = Random.FRand() < Species.FloweringShare ? Draw(Species.FlowersPerPlant, Random) : 0;
			const int32 Stems = FMath::Min(Draw(Species.ShootsPerPlant, Random), Flowers);
			for (int32 S = 0; S < Stems; ++S)
			{
				const FVector Lean(Random.FRandRange(-0.12, 0.12), Random.FRandRange(-0.12, 0.12), 0.0);
				const double Height = Draw(Species.FlowerHeightMeters, Random);
				const FVector Top = Site.Crown + Lean * Height + FVector(0.0, 0.0, Height);
				AddStalk(Out.FindOrAdd(Organ::Stem), Site.Crown, Top, StemRadiusMeters, Lean * 0.05, Colour(Species.StemColours, 0), Colour(Species.StemColours, 0));
				Flower(Species, Top, (FVector::UpVector + Lean).GetSafeNormal(), Random, Out);
			}
		}

		TArray<FString> HerbFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.LeavesPerShoot, TEXT("leavesPerShoot (basal leaves)"), Out);
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters"), Out);
			NeedRange(Species.LeafWidthMeters, TEXT("leafWidthMeters"), Out);
			NeedRange(Species.ShootsPerPlant, TEXT("shootsPerPlant (stems)"), Out);
			NeedRange(Species.FlowerHeightMeters, TEXT("flowerHeightMeters"), Out);
			NeedRange(Species.FlowersPerPlant, TEXT("flowersPerPlant"), Out);
			NeedRange(Species.FlowerSizeMeters, TEXT("flowerSizeMeters"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours(Species.StemColours, 1, TEXT("stemColours"), Out);
			NeedColours(Species.FlowerColours, 2, TEXT("flowerColours (foot, tip)"), Out);
			if (Species.FlowerShape != TEXT("head"))
			{
				NeedRange(Species.PetalsPerFlower, TEXT("petalsPerFlower"), Out);
				NeedColours(Species.EyeColours, Species.CoronaShare > 0.0f ? 2 : 1, TEXT("eyeColours (the eye, and the corona where there is one)"), Out);
			}
			if (Species.FlowerShape != TEXT("cup") && Species.FlowerShape != TEXT("flat") && Species.FlowerShape != TEXT("reflexed") && Species.FlowerShape != TEXT("head"))
			{
				Out.Add(TEXT("its flowerShape is none of cup, flat, reflexed and head"));
			}
			return Out;
		}
	}

	const FForm& HerbForm()
	{
		static const FForm Form{ TEXT("herb"), { Organ::Leaf, Organ::Stem, Organ::Flower, Organ::Eye }, false, &GrowHerb, &HerbFaults };
		return Form;
	}
}
