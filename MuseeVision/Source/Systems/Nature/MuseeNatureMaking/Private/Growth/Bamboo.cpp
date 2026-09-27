#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The bamboo form (Phyllostachys), re-expressed from the as-built museum's grove (windows/Source/MuseeVision/Nature/
 * MuseeTree.cpp, GrowBamboo): each plant is one culm from the running rhizome, standing at its point and leaning out of
 * the grove, its internodes short at the foot and longest at mid-height, a waxy white ring under each node and the node's
 * ridge; from about two metres up two branches at each node on alternating sides (Phyllostachys branches in pairs), each
 * with a branchlet or two, and at every tip a spray of long narrow leaves hanging from their stalks. The culm's own tip
 * carries a spray.
 *
 * Species colours: StemColours [the culm, the powder under a node, the node's ridge]; LeafColours the leaves';
 * LeafAgeColours [a yellowed leaf].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** Branches begin at two metres, or a third of the culm up, whichever is higher (the as-built grove's rule). */
		constexpr double LowestBranchMeters = 2.0;
		constexpr double LowestBranchShare = 0.35;
		/** A culm leans 3 to 9 degrees, and 6 more a metre out from the grove's middle, and droops at its tip. */
		constexpr double LeastLeanDegrees = 3.0;
		constexpr double MostLeanDegrees = 9.0;
		constexpr double LeanPerMeterDegrees = 6.0;
		/** A culm narrows by this share of its foot's radius to its tip, where it closes to a point a third as wide. */
		constexpr double TipNarrowing = 0.62;
		constexpr double TipCloseShare = 0.35;
		/** An internode: 0.10 m at the foot, 0.34 at mid-height. */
		constexpr double ShortestInternodeMeters = 0.10;
		constexpr double InternodeRiseMeters = 0.24;
		/** The powdery ring under a node and the node's ridge, as the as-built culm drew them. */
		constexpr double RingBelowMeters = 0.045;
		constexpr double PowderBelowMeters = 0.012;
		constexpr double RidgeShare = 1.07;
		/** A branch's foot radius and its branchlets'. */
		constexpr double BranchRadiusMeters = 0.0045;
		constexpr double BranchletRadiusMeters = 0.0025;
		/** The share of yellowed leaves in a grove in summer. */
		constexpr double YellowedShare = 0.05;

		struct FCulm
		{
			FVector Foot;
			FVector2D Away;
			double Height = 0.0;
			double Radius = 0.0;
			double Lean = 0.0;
			double Droop = 0.0;

			FVector At(double Z) const
			{
				const double S = Z / Height;
				const double Out = Height * Lean * S * S + Droop * FMath::Pow(S, 6.0);
				return Foot + FVector(Away.X * Out, Away.Y * Out, Z - 0.15 * Droop * FMath::Pow(S, 8.0));
			}

			FVector Dir(double Z) const
			{
				return (At(FMath::Min(Height, Z + 0.02)) - At(FMath::Max(0.0, Z - 0.02))).GetSafeNormal();
			}

			double RadiusAt(double Z) const
			{
				return FMath::Max(BranchRadiusMeters, Radius * (1.0 - TipNarrowing * FMath::Pow(FMath::Clamp(Z / Height, 0.0, 1.0), 1.3)));
			}
		};

		void Spray(const FNatureSpecies& Species, const FVector& At, const FVector& Dir, int32 Leaves, FRandomStream& Random, FNatureSurface& Out)
		{
			FVector U, W;
			Basis(Dir, U, W);
			for (int32 L = 0; L < Leaves; ++L)
			{
				const double Fan = (Leaves > 1 ? double(L) / (Leaves - 1) - 0.5 : 0.0) * 2.2 + Random.FRandRange(-0.2, 0.2);
				const FVector Side = U * FMath::Cos(Fan) + W * FMath::Sin(Fan);
				const FVector LeafDir = (Dir * 0.5 + Side * 0.8 - FVector::UpVector * Random.FRandRange(0.4, 0.9)).GetSafeNormal();
				FVector Normal = (FVector::UpVector - LeafDir * FVector::DotProduct(FVector::UpVector, LeafDir)).GetSafeNormal(UE_SMALL_NUMBER, Side);
				Normal = (Normal + Random.GetUnitVector() * 0.25).GetSafeNormal();
				Normal = (Normal - LeafDir * FVector::DotProduct(Normal, LeafDir)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
				const FLinearColor Tint = Vary(Random.FRand() < YellowedShare ? Colour(Species.LeafAgeColours, 0) : AnyColour(Species.LeafColours, Random), Random, 4.0, 0.1, 0.12);
				AddCard(Out, At, LeafDir, Normal, Draw(Species.LeafLengthMeters, Random), Draw(Species.LeafWidthMeters, Random), 0.3, 0.22, 2, Tint, 1.0);
			}
		}

		/** A branch of three segments sagging from the culm, a spray at its tip; the rings it was drawn through. */
		TArray<FTubeRing> Branch(const FNatureSpecies& Species, const FVector& From, const FVector& Dir0, double Length, double Radius, const FLinearColor& Tint,
			FRandomStream& Random, FPlantParts& Out)
		{
			constexpr int32 Segments = 3;
			TArray<FTubeRing> Rings;
			FVector P = From;
			FVector Dir = Dir0;
			for (int32 S = 0; S <= Segments; ++S)
			{
				Rings.Add({ P, Dir, FMath::Max(0.0012, Radius * (1.0 - 0.7 * S / Segments)), Tint });
				Dir = (Dir - FVector::UpVector * 0.12).GetSafeNormal();
				P += Dir * (Length / Segments);
			}
			AddTube(Out.FindOrAdd(Organ::Stem), Rings, 3, true);
			Spray(Species, Rings.Last().Centre, Rings.Last().Dir, Random.RandRange(3, 6), Random, Out.FindOrAdd(Organ::Leaf));
			return Rings;
		}

		/** The nodes' heights, from the soil: short internodes at the foot, longest at mid-height. */
		TArray<double> Nodes(const FCulm& Culm, FRandomStream& Random)
		{
			TArray<double> Out;
			double Z = -0.05;
			while (Z < Culm.Height - 0.12)
			{
				Out.Add(Z);
				Z += (ShortestInternodeMeters + InternodeRiseMeters * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(Z / Culm.Height * 1.15, 0.0, 1.0)), 0.7))
					* Random.FRandRange(0.9, 1.1);
			}
			return Out;
		}

		void CulmTube(const FNatureSpecies& Species, const FCulm& Culm, const TArray<double>& At, FPlantParts& Out)
		{
			const FLinearColor Green = Colour(Species.StemColours, 0);
			const FLinearColor Powder = Mix(Green, Colour(Species.StemColours, 1), 0.55);
			const FLinearColor Ridge = Mix(Green, Colour(Species.StemColours, 2), 0.5);
			TArray<FTubeRing> Rings;
			const auto Ring = [&](double Z, double Share, const FLinearColor& Tint) { Rings.Add({ Culm.At(Z), Culm.Dir(Z), Culm.RadiusAt(Z) * Share, Tint }); };
			for (int32 N = 0; N < At.Num(); ++N)
			{
				if (N > 0)
				{
					Ring(At[N] - RingBelowMeters, 1.0, Green);
					Ring(At[N] - PowderBelowMeters, 1.0, Powder);
					Ring(At[N], RidgeShare, Ridge);
				}
				Ring(At[N] + PowderBelowMeters, 1.0, Green);
			}
			Ring(Culm.Height, TipCloseShare, Green);
			AddTube(Out.FindOrAdd(Organ::Stem), Rings, 7, true);
		}

		void GrowBamboo(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			FCulm Culm;
			Culm.Foot = Site.Crown;
			Culm.Height = Draw(Species.HeightMeters, Random) * (Site.Height > 0.0 ? Site.Height / Middle(Species.HeightMeters) : 1.0);
			Culm.Radius = Draw(Species.StemRadiusMeters, Random);
			// Out from the grove's middle, the bed's anchor.
			const FVector2D FromMiddle(Site.Crown.X, Site.Crown.Y);
			const double AzOut = Random.FRandRange(0.0, Tau);
			const FVector2D Out2 = FromMiddle.SizeSquared() > 1e-4 ? FromMiddle.GetSafeNormal() : FVector2D(FMath::Cos(AzOut), FMath::Sin(AzOut));
			Culm.Away = (Out2 + FVector2D(Random.FRandRange(-0.4, 0.4), Random.FRandRange(-0.4, 0.4))).GetSafeNormal();
			Culm.Lean = FMath::Tan(FMath::DegreesToRadians(Random.FRandRange(LeastLeanDegrees, MostLeanDegrees) + LeanPerMeterDegrees * FMath::Min(1.0, FromMiddle.Size())));
			Culm.Droop = Random.FRandRange(0.25, 0.7);
			const TArray<double> At = Nodes(Culm, Random);
			CulmTube(Species, Culm, At, Out);
			const FLinearColor Tint = Colour(Species.StemColours, 0);
			const double From = FMath::Max(LowestBranchMeters, LowestBranchShare * Culm.Height);
			double SideAz = Random.FRandRange(0.0, Tau);
			for (int32 N = 1; N < At.Num(); ++N)
			{
				if (At[N] < From || At[N] > Culm.Height - 0.1)
				{
					continue;
				}
				SideAz += UE_DOUBLE_PI + Random.FRandRange(-0.35, 0.35);
				const double Rel = (At[N] - From) / FMath::Max(0.1, Culm.Height - From);
				const double Length = (0.3 + 0.55 * FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(0.25 + 0.75 * Rel, 0.0, 1.0))) * Random.FRandRange(0.8, 1.2) * (1.0 - 0.5 * Rel);
				const FVector Axis = Culm.Dir(At[N]);
				FVector U, W;
				Basis(Axis, U, W);
				for (int32 B = 0; B < 2; ++B)
				{
					const double A = SideAz + (B == 0 ? 0.0 : Random.FRandRange(0.5, 0.9));
					const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
					const FVector Dir = RotateAbout(Axis, FVector::CrossProduct(Axis, Radial), FMath::DegreesToRadians(Random.FRandRange(45.0, 65.0)));
					const double BranchLength = Length * (B == 0 ? 1.0 : 0.6);
					const TArray<FTubeRing> Main = Branch(Species, Culm.At(At[N]) + Radial * Culm.RadiusAt(At[N]) * 0.8, Dir, BranchLength, BranchRadiusMeters, Tint, Random, Out);
					for (int32 S = 0, Subs = Random.RandRange(1, 2); S < Subs; ++S)
					{
						const FTubeRing& Fork = Main[FMath::Clamp(1 + S, 1, Main.Num() - 1)];
						const FVector SubDir = (Fork.Dir + Random.GetUnitVector() * 0.8 + FVector::UpVector * 0.2).GetSafeNormal();
						Branch(Species, Fork.Centre, SubDir, BranchLength * Random.FRandRange(0.3, 0.5), BranchletRadiusMeters, Tint, Random, Out);
					}
				}
			}
			Spray(Species, Culm.At(Culm.Height), Culm.Dir(Culm.Height), 6, Random, Out.FindOrAdd(Organ::Leaf));
		}

		TArray<FString> BambooFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.HeightMeters, TEXT("heightMeters (a culm's)"), Out);
			NeedRange(Species.StemRadiusMeters, TEXT("stemRadiusMeters (a culm's foot)"), Out);
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters"), Out);
			NeedRange(Species.LeafWidthMeters, TEXT("leafWidthMeters"), Out);
			NeedColours(Species.StemColours, 3, TEXT("stemColours (the culm, the powder, the node)"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours(Species.LeafAgeColours, 1, TEXT("leafAgeColours (a yellowed leaf)"), Out);
			return Out;
		}
	}

	const FForm& BambooForm()
	{
		static const FForm Form{ TEXT("bamboo"), { Organ::Stem, Organ::Leaf }, false, &GrowBamboo, &BambooFaults };
		return Form;
	}
}
