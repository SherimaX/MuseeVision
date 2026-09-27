#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The lotus form (Nelumbo nucifera): from the rhizome in the pond's mud, leaves on long stalks standing clear of the
 * water, each a round peltate shield, its stalk joined under its middle, the blade dished down to that joint and its rim
 * turned up and waved; a few young leaves still floating flat on the water; and, on a flowering plant, flowers held above
 * the leaves on their own stalks: two whorls of broad concave petals, pale at the foot and flushed at the tip, round the
 * yellow top-shaped receptacle and its ring of stamens, with a furled bud or two beside them.
 *
 * Species measures: reachMeters the colony's radius; leafLengthMeters a leaf's radius; leafHeightMeters how high the
 * leaves stand above the water; shootsPerPlant the leaves; flowerSizeMeters a flower's span; flowerHeightMeters how high
 * it stands above the water; flowersPerPlant the flowers of a flowering plant.
 *
 * Species colours: LeafColours the blades' greens; LeafEdgeColour the blade's glaucous middle; StemColours [the stalks];
 * FlowerColours [the petals' pale foot, their flushed tip]; EyeColours [the receptacle, the stamens].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** The share of a plant's leaves still floating on the water in early summer. */
		constexpr double FloatingShare = 0.2;
		/** A blade's dish: its joint lies this share of its radius below its rim, which turns up this much more. */
		constexpr double DishShare = 0.12;
		constexpr double RimLiftShare = 0.06;
		/** The rim's waves round the blade. */
		constexpr int32 RimWaves = 7;
		constexpr double RimWaveShare = 0.025;
		/** The stalks' radius, and a petal's width as a share of its length. */
		constexpr double StalkRadiusMeters = 0.006;
		constexpr double PetalWidthShare = 0.6;
		/** Petals in each of the flower's two whorls. */
		constexpr int32 PetalsPerWhorl = 9;
		/** The receptacle's height and top radius as shares of the flower's span. */
		constexpr double ReceptacleHeightShare = 0.2;
		constexpr double ReceptacleRadiusShare = 0.17;
		constexpr int32 BladeRings = 5;
		constexpr int32 BladeSegments = 24;

		/**
		 * A leaf's blade: a dish of the radius, its joint at the middle, its rim turned up and waved, facing up. Drawn as
		 * rings round the joint, each point's height h(r, theta) over the joint's plane and its normal from that height's
		 * slope.
		 */
		void Blade(const FNatureSpecies& Species, const FVector& Joint, const FVector& Up, double Radius, FRandomStream& Random, FNatureSurface& Out)
		{
			const FLinearColor Green = Vary(AnyColour(Species.LeafColours, Random), Random, 5.0, 0.08, 0.1);
			const FLinearColor Middle = FromHex(Species.LeafEdgeColour);
			const double Phase = Random.FRandRange(0.0, Tau);
			FVector U, W;
			Basis(Up, U, W);
			const auto Height = [&](double S, double Theta)
			{
				return Radius * (DishShare * S + RimLiftShare * FMath::Pow(S, 6.0) + RimWaveShare * FMath::Pow(S, 4.0) * FMath::Sin(RimWaves * Theta + Phase));
			};
			const int32 Middle0 = Out.Vertex(Joint, Up, FVector2D::ZeroVector, Middle);
			TArray<int32> Previous;
			for (int32 R = 1; R <= BladeRings; ++R)
			{
				const double S = double(R) / BladeRings;
				TArray<int32> Current;
				for (int32 K = 0; K < BladeSegments; ++K)
				{
					const double Theta = Tau * K / BladeSegments;
					const FVector Radial = U * FMath::Cos(Theta) + W * FMath::Sin(Theta);
					const double Slope = (Height(FMath::Min(1.0, S + 0.05), Theta) - Height(S - 0.05, Theta)) / (0.1 * Radius);
					const FVector Normal = (Up - Radial * Slope).GetSafeNormal(UE_SMALL_NUMBER, Up);
					const FVector P = Joint + Radial * (Radius * S) + Up * Height(S, Theta);
					Current.Add(Out.Vertex(P, Normal, FVector2D(Radius * S * FMath::Cos(Theta), Radius * S * FMath::Sin(Theta)), Mix(Middle, Green, S)));
				}
				for (int32 K = 0; K < BladeSegments; ++K)
				{
					const int32 Next = (K + 1) % BladeSegments;
					R == 1 ? Out.Triangle(Middle0, Current[K], Current[Next]) : Out.Quad(Previous[K], Previous[Next], Current[Next], Current[K]);
				}
				Previous = MoveTemp(Current);
			}
		}

		void Flower(const FNatureSpecies& Species, const FVector& Top, double Span, FRandomStream& Random, FPlantParts& Out)
		{
			const FLinearColor Foot = Colour(Species.FlowerColours, 0);
			const FLinearColor Tip = Vary(Colour(Species.FlowerColours, 1), Random, 4.0, 0.06, 0.06);
			const double Length = 0.5 * Span;
			for (int32 Whorl = 0; Whorl < 2; ++Whorl)
			{
				const double Az0 = Random.FRandRange(0.0, Tau) + Whorl * UE_DOUBLE_PI / PetalsPerWhorl;
				for (int32 K = 0; K < PetalsPerWhorl; ++K)
				{
					const double Az = Az0 + Tau * K / PetalsPerWhorl + Random.FRandRange(-0.08, 0.08);
					const FVector Radial(FMath::Cos(Az), FMath::Sin(Az), 0.0);
					const FPetal Petal{ Length * (Whorl == 0 ? 1.0 : 0.9), Length * PetalWidthShare, (Whorl == 0 ? 38.0 : 58.0) + Random.FRandRange(-8.0, 8.0), 18.0, 1.5,
						0.22, 0.9, 0.3, 0.0, 5, 7 };
					AddPetal(Out.FindOrAdd(Organ::Flower), Top + Radial * 0.01, Radial, Petal, [&](double, double S) { return Mix(Foot, Tip, FMath::Pow(S, 1.5)); });
				}
			}
			// The receptacle: a top-shaped cone, flat on top, and its ring of stamens.
			const double Radius = ReceptacleRadiusShare * Span;
			const double Height = ReceptacleHeightShare * Span;
			AddLathe(Out.FindOrAdd(Organ::Eye), Top, FVector::UpVector, { FVector2D(0.0, Height), FVector2D(Radius, Height), FVector2D(0.25 * Radius, 0.0) }, 16,
				{ Colour(Species.EyeColours, 0) }, 0.9);
			const FLinearColor Stamen = Colour(Species.EyeColours, 1);
			for (int32 K = 0; K < 3 * PetalsPerWhorl; ++K)
			{
				const double Az = Tau * K / (3 * PetalsPerWhorl);
				const FVector Radial(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				AddEllipsoid(Out.FindOrAdd(Organ::Eye), Top + Radial * (1.15 * Radius) + FVector(0.0, 0.0, 0.5 * Height), (Radial + FVector::UpVector).GetSafeNormal(),
					0.0015, 0.012, 4, 3, Stamen, 0.9);
			}
		}

		void GrowLotus(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			const double Water = Site.WaterLevel;
			const FVector Mud = Site.Crown + FVector(0.0, 0.0, Water - Site.WaterDepth);
			const FLinearColor Stalk = Colour(Species.StemColours, 0);
			const double Reach = Draw(Species.ReachMeters, Random);
			const int32 Leaves = Draw(Species.ShootsPerPlant, Random);
			for (int32 L = 0; L < Leaves; ++L)
			{
				const double R = Reach * FMath::Sqrt(Random.FRand());
				const double Az = Random.FRandRange(0.0, Tau);
				const FVector Plan(R * FMath::Cos(Az), R * FMath::Sin(Az), 0.0);
				const bool bFloating = Random.FRand() < FloatingShare;
				const double Radius = Draw(Species.LeafLengthMeters, Random) * (bFloating ? 0.6 : 1.0);
				const FVector Joint = Site.Crown + Plan + FVector(0.0, 0.0, Water + (bFloating ? 0.005 : Draw(Species.LeafHeightMeters, Random)));
				const FVector Up = bFloating ? FVector::UpVector : (FVector::UpVector + FVector(Random.FRandRange(-0.25, 0.25), Random.FRandRange(-0.25, 0.25), 0.0)).GetSafeNormal();
				AddStalk(Out.FindOrAdd(Organ::Stem), Mud + Plan * 0.3, Joint, StalkRadiusMeters, Plan.GetSafeNormal() * 0.04, Stalk, Stalk);
				Blade(Species, Joint, Up, Radius, Random, Out.FindOrAdd(Organ::Leaf));
			}
			const int32 Flowers = Random.FRand() < Species.FloweringShare ? Draw(Species.FlowersPerPlant, Random) : 0;
			for (int32 F = 0; F < Flowers; ++F)
			{
				const double R = 0.5 * Reach * FMath::Sqrt(Random.FRand());
				const double Az = Random.FRandRange(0.0, Tau);
				const FVector Plan(R * FMath::Cos(Az), R * FMath::Sin(Az), 0.0);
				const FVector Top = Site.Crown + Plan + FVector(0.0, 0.0, Water + Draw(Species.FlowerHeightMeters, Random));
				AddStalk(Out.FindOrAdd(Organ::Stem), Mud + Plan * 0.3, Top, StalkRadiusMeters, FVector::ZeroVector, Stalk, Stalk);
				Flower(Species, Top, Draw(Species.FlowerSizeMeters, Random), Random, Out);
			}
			// A furled bud beside the flowers: pointed, the petals' flush at its tip.
			const FVector BudPlan(0.4 * Reach * FMath::Cos(Random.FRandRange(0.0, Tau)), 0.4 * Reach * FMath::Sin(Random.FRandRange(0.0, Tau)), 0.0);
			const FVector Bud = Site.Crown + BudPlan + FVector(0.0, 0.0, Water + Draw(Species.FlowerHeightMeters, Random));
			AddStalk(Out.FindOrAdd(Organ::Stem), Mud + BudPlan * 0.3, Bud, StalkRadiusMeters, FVector::ZeroVector, Stalk, Stalk);
			const double BudSize = 0.3 * Middle(Species.FlowerSizeMeters);
			AddEllipsoid(Out.FindOrAdd(Organ::Flower), Bud + FVector(0.0, 0.0, BudSize), FVector::UpVector, 0.45 * BudSize, BudSize, 10, 6,
				Mix(Colour(Species.FlowerColours, 0), Colour(Species.FlowerColours, 1), 0.7), 1.0);
		}

		TArray<FString> LotusFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters (a leaf's radius)"), Out);
			NeedRange(Species.LeafHeightMeters, TEXT("leafHeightMeters (above the water)"), Out);
			NeedRange(Species.FlowerHeightMeters, TEXT("flowerHeightMeters (above the water)"), Out);
			NeedRange(Species.FlowerSizeMeters, TEXT("flowerSizeMeters"), Out);
			NeedRange(Species.ShootsPerPlant, TEXT("shootsPerPlant (leaves)"), Out);
			NeedRange(Species.FlowersPerPlant, TEXT("flowersPerPlant"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours({ Species.LeafEdgeColour }, 1, TEXT("leafEdgeColour (the blade's middle)"), Out);
			NeedColours(Species.StemColours, 1, TEXT("stemColours"), Out);
			NeedColours(Species.FlowerColours, 2, TEXT("flowerColours (foot, tip)"), Out);
			NeedColours(Species.EyeColours, 2, TEXT("eyeColours (receptacle, stamens)"), Out);
			return Out;
		}
	}

	const FForm& LotusForm()
	{
		static const FForm Form{ TEXT("lotus"), { Organ::Leaf, Organ::Stem, Organ::Flower, Organ::Eye }, true, &GrowLotus, &LotusFaults };
		return Form;
	}
}
