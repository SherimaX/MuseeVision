#include "Nature/MuseeMeadow.h"

#include "Nature/NatureMesh.h"

namespace MuseeMeadowGen
{
	namespace MN = MuseeNature;

	enum ESection : int32
	{
		Grass = 0,
		Stems,
		Leaves,
		Scabious,
		Mallow,
		Helenium,
		Buttercup,
		Narcissus,
		Daffodil,
		Tulip,
		SeedHead,
		NumSections
	};

	const TCHAR* const Materials[NumSections] = {
		TEXT("MI_Grass"), TEXT("MI_Stem_Green"), TEXT("MI_Leaf_Meadow"), TEXT("MI_Flower_Scabious"), TEXT("MI_Flower_Mallow"),
		TEXT("MI_Flower_Helenium"), TEXT("MI_Flower_Buttercup"), TEXT("MI_Flower_Narcissus"), TEXT("MI_Flower_Daffodil"),
		TEXT("MI_Petal_Meadow"), TEXT("MI_Flower_SeedHead")};

	/** The flower a bed's colour stands for, and its real colour. */
	struct FKind
	{
		int32 Section = Scabious;
		FLinearColor Colour = FLinearColor::White;
		double HeadSize = 0.04;
		double Cup = 0.25;
		int32 StemsMin = 1;
		int32 StemsMax = 2;
		bool bBranched = false;
	};

	FKind KindFor(uint32 Hex, bool bMeadow, bool bSeedHeads)
	{
		FKind K;
		if (bSeedHeads)
		{
			K.Section = SeedHead; K.Colour = MN::Srgb(0x6E5838); K.HeadSize = 0.034; K.Cup = 0.4; K.StemsMin = 1; K.StemsMax = 3;
			return K;
		}
		switch (Hex)
		{
		case 0xA08AB0: K.Section = Scabious; K.Colour = MN::Srgb(0x9C88C4); K.HeadSize = 0.038; K.Cup = 0.35; break;
		case 0xD9A5A0:
			if (bMeadow) { K.Section = Mallow; K.Colour = MN::Srgb(0xE6A2B4); K.HeadSize = 0.05; K.Cup = 0.3; }
			else { K.Section = Tulip; K.Colour = MN::Srgb(0xE58AA0); K.HeadSize = 0.06; K.StemsMax = 1; }
			break;
		case 0xC9826A: K.Section = Helenium; K.Colour = MN::Srgb(0xC8643A); K.HeadSize = 0.042; K.Cup = 0.15; K.bBranched = true; break;
		case 0xE6C77A:
			if (bMeadow) { K.Section = Buttercup; K.Colour = MN::Srgb(0xF2C830); K.HeadSize = 0.024; K.Cup = 0.45; K.StemsMin = 2; K.StemsMax = 3; K.bBranched = true; }
			else { K.Section = Daffodil; K.Colour = MN::Srgb(0xF2CA3C); K.HeadSize = 0.06; K.Cup = 0.2; K.StemsMax = 1; }
			break;
		case 0xF3EEE5: K.Section = Narcissus; K.Colour = MN::Srgb(0xF8F6EE); K.HeadSize = 0.05; K.Cup = 0.15; K.StemsMax = 1; break;
		default:
			K.Section = bMeadow ? Mallow : Narcissus;
			K.Colour = MN::Srgb(Hex);
			break;
		}
		return K;
	}

	struct FOut
	{
		FNatureMesh Sections[NumSections];
	};

	/** A leaf card from At along Dir, turned to the sky. */
	void Leaf(FOut& Out, const FVector& At, const FVector& Dir, double Length, double Aspect, const FNatureWind& Base, const FLinearColor& Colour,
			  const FRandomStream& Rng)
	{
		FVector N = FVector::UpVector - Dir * FVector::DotProduct(FVector::UpVector, Dir);
		N = (N.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector) + Rng.GetUnitVector() * 0.3).GetSafeNormal();
		N = (N - Dir * FVector::DotProduct(N, Dir)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		FNatureWind Tip = Base;
		Tip.Flutter = 1.0;
		Tip.Bend = Base.Bend + 0.05;
		MN::AddCard(Out.Sections[Leaves], At, Dir, N, Length, Length * Aspect, 0.3, 0.12, 2, Base, Tip, Colour, 0.8);
	}

	/** One plant at a plotted point: stems, leaves and flower heads (or seed heads). */
	void Plant(FOut& Out, const FVector2D& At, const FKind& Kind, double HMin, double HMax, const FRandomStream& Rng)
	{
		const FLinearColor StemColour = MN::Srgb(Kind.Section == SeedHead ? 0x7A6A4A : 0x5E7A3A);
		const FLinearColor LeafColour = MN::Srgb(Kind.Section == SeedHead ? 0x8A7A58 : 0x4E7432);
		const double Phase = Rng.FRand();
		// A rosette of basal leaves (not on the bulbs, whose leaves are straps).
		const bool bBulb = Kind.Section == Narcissus || Kind.Section == Daffodil || Kind.Section == Tulip;
		const int32 Basal = Rng.RandRange(3, 5);
		for (int32 b = 0; b < Basal; ++b)
		{
			const double A = MN::Tau * b / Basal + Rng.FRandRange(-0.4, 0.4);
			const FVector Out2(FMath::Cos(A), FMath::Sin(A), 0);
			FNatureWind Base;
			Base.Phase = Phase;
			Base.FlutterPhase = Rng.FRand();
			const FVector From(At.X, At.Y, 0.01);
			if (bBulb)
			{
				FNatureBlade Blade;
				Blade.Length = Rng.FRandRange(0.18, 0.3);
				Blade.Width = Rng.FRandRange(0.012, 0.02);
				Blade.Droop = Rng.FRandRange(1.0, 3.0);
				Blade.Cup = 0.2;
				Blade.TipShape = 0.5;
				Blade.BaseWidth = 0.6;
				Blade.NX = 2;
				Blade.NY = 5;
				Blade.Root = MN::Srgb(0x4E7A3A);
				Blade.Tip = MN::Srgb(0x6A8E4A);
				Blade.BendGain = 0.3;
				Blade.FlutterGain = 0.3;
				const FVector Dir = (Out2 * 0.35 + FVector::UpVector).GetSafeNormal();
				MN::AddBlade(Out.Sections[Stems], From, Dir, (FVector::UpVector - Dir * FVector::DotProduct(FVector::UpVector, Dir)).GetSafeNormal(), Blade, Base);
			}
			else
			{
				Leaf(Out, From, (Out2 + FVector::UpVector * 0.35).GetSafeNormal(), Rng.FRandRange(0.07, 0.13), 0.32, Base,
					 MN::Vary(LeafColour, Rng, 5.0, 0.1, 0.12), Rng);
			}
		}

		const int32 NumStems = Rng.RandRange(Kind.StemsMin, Kind.StemsMax);
		for (int32 s = 0; s < NumStems; ++s)
		{
			const double H = Rng.FRandRange(HMin, HMax);
			const double LeanA = Rng.FRandRange(0.0, MN::Tau);
			const FVector Lean = FVector(FMath::Cos(LeanA), FMath::Sin(LeanA), 0) * Rng.FRandRange(0.02, 0.2);
			const FVector From(At.X + Rng.FRandRange(-0.03, 0.03), At.Y + Rng.FRandRange(-0.03, 0.03), 0.0);
			auto StemAt = [&](double T) { return From + FVector::UpVector * (H * T) + Lean * (H * T * T); };
			TArray<FNatureTubeRing> Rings;
			for (int32 r = 0; r <= 4; ++r)
			{
				const double T = r / 4.0;
				FNatureTubeRing& Ring = Rings.AddDefaulted_GetRef();
				Ring.Centre = StemAt(T);
				Ring.Dir = (StemAt(FMath::Min(1.0, T + 0.05)) - StemAt(FMath::Max(0.0, T - 0.05))).GetSafeNormal();
				Ring.Radius = (bBulb ? 0.0035 : 0.0022) * (1.0 - 0.35 * T);
				Ring.Colour = StemColour;
				Ring.Wind.Bend = 0.8 * T * T;
				Ring.Wind.Phase = Phase;
				Ring.Wind.Height = Ring.Centre.Z;
				Ring.Wind.Twig = 1.0;
			}
			MN::AddTube(Out.Sections[Stems], Rings, 3, 0.0, true);
			// Stem leaves on the lower part.
			if (!bBulb)
			{
				const int32 NumLeaves = Rng.RandRange(2, 4);
				for (int32 l = 0; l < NumLeaves; ++l)
				{
					const double T = Rng.FRandRange(0.15, 0.6);
					const double A = l * 2.4 + Rng.FRandRange(-0.4, 0.4);
					const FVector Side(FMath::Cos(A), FMath::Sin(A), 0);
					FNatureWind Base;
					Base.Bend = 0.8 * T * T;
					Base.Phase = Phase;
					Base.FlutterPhase = Rng.FRand();
					Base.Height = StemAt(T).Z;
					Leaf(Out, StemAt(T), (Side + FVector::UpVector * 0.5).GetSafeNormal(), Rng.FRandRange(0.05, 0.09), 0.28, Base,
						 MN::Vary(LeafColour, Rng, 5.0, 0.1, 0.12), Rng);
				}
			}
			// Heads: at the tip, and on side shoots for the branched kinds.
			TArray<FVector> Heads = {StemAt(1.0)};
			if (Kind.bBranched)
			{
				const int32 Extra = Rng.RandRange(1, 3);
				for (int32 e = 0; e < Extra; ++e)
				{
					const double T = Rng.FRandRange(0.65, 0.9);
					const double A = Rng.FRandRange(0.0, MN::Tau);
					const FVector Tip = StemAt(T) + FVector(FMath::Cos(A), FMath::Sin(A), 0) * Rng.FRandRange(0.05, 0.1) + FVector::UpVector * Rng.FRandRange(0.04, 0.1);
					TArray<FNatureTubeRing> Shoot;
					for (int32 r = 0; r < 2; ++r)
					{
						FNatureTubeRing& Ring = Shoot.AddDefaulted_GetRef();
						Ring.Centre = r == 0 ? StemAt(T) : Tip;
						Ring.Dir = (Tip - StemAt(T)).GetSafeNormal();
						Ring.Radius = 0.0015;
						Ring.Colour = StemColour;
						Ring.Wind.Bend = 0.8 * T * T + 0.1 * r;
						Ring.Wind.Phase = Phase;
						Ring.Wind.Height = Ring.Centre.Z;
						Ring.Wind.Twig = 1.0;
					}
					MN::AddTube(Out.Sections[Stems], Shoot, 3, 0.0, true);
					Heads.Add(Tip);
				}
			}
			for (const FVector& Head : Heads)
			{
				FNatureWind Wind;
				Wind.Bend = 0.9;
				Wind.Flutter = 0.3;
				Wind.Phase = Phase;
				Wind.FlutterPhase = Rng.FRand();
				Wind.Height = Head.Z;
				Wind.Twig = 1.0;
				const FLinearColor Colour = MN::Vary(Kind.Colour, Rng, 4.0, 0.08, 0.08);
				if (Kind.Section == Tulip)
				{
					// A tulip: six petals standing in a cup.
					const double Az0 = Rng.FRandRange(0.0, MN::Tau);
					for (int32 p = 0; p < 6; ++p)
					{
						const double A = Az0 + MN::Tau * p / 6 + (p % 2) * 0.15;
						const FVector Radial(FMath::Cos(A), FMath::Sin(A), 0);
						const FVector Dir = (FVector::UpVector + Radial * (p % 2 == 0 ? 0.28 : 0.18)).GetSafeNormal();
						FNatureBlade Blade;
						Blade.Length = Kind.HeadSize * Rng.FRandRange(0.95, 1.05);
						Blade.Width = Kind.HeadSize * 0.62;
						Blade.Curl = 18.0;
						Blade.Cup = 0.3;
						Blade.TipShape = 1.4;
						Blade.BaseWidth = 0.3;
						Blade.NX = 3;
						Blade.NY = 5;
						Blade.Root = MN::Mix(Colour, MN::Srgb(0xF2E070), 0.35);
						Blade.Tip = Colour;
						Blade.BendGain = 0.0;
						Blade.FlutterGain = 0.2;
						MN::AddBlade(Out.Sections[Tulip], Head - Radial * 0.006, Dir, -Radial, Blade, Wind);
					}
					continue;
				}
				FVector Facing = (FVector::UpVector * 0.8 + Lean * 2.0 + Rng.GetUnitVector() * 0.3).GetSafeNormal();
				if (Kind.Section == Narcissus || Kind.Section == Daffodil)
				{
					// Narcissi nod sideways.
					Facing = (Lean.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector) + FVector::UpVector * 0.2 - FVector::UpVector * 0.1).GetSafeNormal();
				}
				MN::AddFlowerCard(Out.Sections[Kind.Section], Head + Facing * 0.004, Facing, Rng.FRandRange(0.0, MN::Tau),
								  Kind.HeadSize * Rng.FRandRange(0.85, 1.15), Kind.Cup, Wind, Colour, 1.0);
			}
		}
	}

	/** A tuft of grass blades, and in summer a few flowering stems. */
	void Tuft(FOut& Out, const FVector2D& At, EMuseeNatureSeason Season, const FRandomStream& Rng)
	{
		double LMin = 0.25, LMax = 0.6, DroopMin = 1.2, DroopMax = 3.0;
		FLinearColor Root = MN::Srgb(0x3E5A26), Tip = MN::Srgb(0x86A04C), Dry = MN::Srgb(0xB8AA70);
		double DryShare = 0.2;
		switch (Season)
		{
		case EMuseeNatureSeason::Spring: LMin = 0.12; LMax = 0.35; Tip = MN::Srgb(0x8CB050); DryShare = 0.03; break;
		case EMuseeNatureSeason::Autumn: LMin = 0.2; LMax = 0.45; Tip = MN::Srgb(0x9A9A58); DryShare = 0.45; break;
		case EMuseeNatureSeason::Winter: LMin = 0.12; LMax = 0.32; DroopMin = 2.5; DroopMax = 5.0; Root = MN::Srgb(0x5A6038); Tip = MN::Srgb(0xA0946A); DryShare = 0.7; break;
		default: break;
		}
		const double Phase = Rng.FRand();
		const int32 Blades = Rng.RandRange(10, 22);
		for (int32 b = 0; b < Blades; ++b)
		{
			const double A = Rng.FRandRange(0.0, MN::Tau);
			const double Rad = Rng.FRandRange(0.0, 0.06);
			const FVector From(At.X + FMath::Cos(A) * Rad, At.Y + FMath::Sin(A) * Rad, -0.01);
			const double E = FMath::DegreesToRadians(Rng.FRandRange(58.0, 88.0));
			const double Az = A + Rng.FRandRange(-0.6, 0.6);
			const FVector Dir(FMath::Cos(Az) * FMath::Cos(E), FMath::Sin(Az) * FMath::Cos(E), FMath::Sin(E));
			FNatureBlade Blade;
			Blade.Length = Rng.FRandRange(LMin, LMax);
			Blade.Width = Rng.FRandRange(0.004, 0.007);
			Blade.Droop = Rng.FRandRange(DroopMin, DroopMax);
			Blade.Twist = Rng.FRandRange(-40.0, 40.0);
			Blade.TipShape = 0.35;
			Blade.BaseWidth = 0.8;
			Blade.NX = 1;
			Blade.NY = 3;
			Blade.Root = MN::Vary(Root, Rng, 4.0, 0.1, 0.15);
			Blade.Tip = Rng.FRand() < DryShare ? MN::Vary(Dry, Rng, 4.0, 0.1, 0.1) : MN::Vary(Tip, Rng, 5.0, 0.1, 0.15);
			Blade.BendGain = 0.9 * Blade.Length / 0.5;
			Blade.FlutterGain = 0.3;
			FNatureWind Wind;
			Wind.Phase = Phase;
			Wind.FlutterPhase = Rng.FRand();
			const FVector Across(-Dir.Y, Dir.X, 0);
			MN::AddBlade(Out.Sections[Grass], From, Dir, FVector::CrossProduct(Across.GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector), Dir).GetSafeNormal(),
						 Blade, Wind);
		}
		if (Season == EMuseeNatureSeason::Summer)
		{
			// Flowering grass: a thin stem and a loose, nodding panicle.
			const int32 Stalks = Rng.RandRange(0, 2);
			for (int32 s = 0; s < Stalks; ++s)
			{
				const double H = Rng.FRandRange(0.5, 0.85);
				const FVector From(At.X + Rng.FRandRange(-0.03, 0.03), At.Y + Rng.FRandRange(-0.03, 0.03), 0.0);
				const FVector Lean = FVector(Rng.FRandRange(-1.0, 1.0), Rng.FRandRange(-1.0, 1.0), 0).GetSafeNormal() * Rng.FRandRange(0.03, 0.12);
				FNatureWind Wind;
				Wind.Phase = Phase;
				TArray<FNatureTubeRing> Rings;
				for (int32 r = 0; r <= 3; ++r)
				{
					const double T = r / 3.0;
					FNatureTubeRing& Ring = Rings.AddDefaulted_GetRef();
					Ring.Centre = From + FVector::UpVector * (H * T) + Lean * (H * T * T);
					Ring.Dir = (FVector::UpVector + Lean * 2.0 * T).GetSafeNormal();
					Ring.Radius = 0.0012;
					Ring.Colour = MN::Srgb(0x8A9A50);
					Ring.Wind = Wind;
					Ring.Wind.Bend = 1.0 * T * T;
					Ring.Wind.Height = Ring.Centre.Z;
				}
				MN::AddTube(Out.Sections[Grass], Rings, 3, 0.0, false);
				const FVector Top = Rings.Last().Centre;
				Wind.Bend = 1.0;
				for (int32 k = 0; k < 5; ++k)
				{
					FNatureBlade Spike;
					Spike.Length = Rng.FRandRange(0.03, 0.06);
					Spike.Width = 0.006;
					Spike.Droop = 6.0;
					Spike.TipShape = 0.8;
					Spike.NX = 1;
					Spike.NY = 2;
					Spike.Root = MN::Srgb(0xA8A060);
					Spike.Tip = MN::Srgb(0xC8B880);
					Spike.BendGain = 0.1;
					Spike.FlutterGain = 0.6;
					const FVector Dir = (Rng.GetUnitVector() + FVector::UpVector * 0.4).GetSafeNormal();
					MN::AddBlade(Out.Sections[Grass], Top - FVector::UpVector * (0.015 * k), Dir, FVector::UpVector, Spike, Wind);
				}
			}
		}
	}
}

AMuseeMeadow::AMuseeMeadow()
{
}

void AMuseeMeadow::BuildPlant()
{
	namespace G = MuseeMeadowGen;
	G::FOut Out;
	FRandomStream Rng(Seed * 7919 + 211);
	const EMuseeNatureSeason Now = EffectiveSeason();
	for (const FMuseeFlowerBed& Bed : Beds)
	{
		// The Swift's season rules (buildHallGardens).
		const bool bMeadow = Bed.bMeadow;
		int32 Count = Bed.Points.Num();
		double HMin = bMeadow ? 0.4 : 0.2, HMax = bMeadow ? 0.9 : 0.35;
		bool bSeed = false;
		if (Now == EMuseeNatureSeason::Winter && bMeadow) { bSeed = true; HMin = 0.5; HMax = 0.9; }
		else if (!bMeadow && Now != EMuseeNatureSeason::Spring) { continue; }   // the bulbs are over
		else if (Now == EMuseeNatureSeason::Autumn) { Count = Bed.Points.Num() / 2; HMin = 0.4; HMax = 0.8; }
		const G::FKind Kind = G::KindFor(uint32(Bed.Colour) & 0xFFFFFFu, bMeadow, bSeed);
		for (int32 i = 0; i < Count; ++i) { G::Plant(Out, Bed.Points[i], Kind, HMin, HMax, Rng); }
	}
	if (GrassMax.X > GrassMin.X && GrassMax.Y > GrassMin.Y && GrassDensity > 0)
	{
		const double Area = (GrassMax.X - GrassMin.X) * (GrassMax.Y - GrassMin.Y);
		const int32 Tufts = FMath::RoundToInt(Area * GrassDensity);
		for (int32 t = 0; t < Tufts; ++t)
		{
			const FVector2D P(Rng.FRandRange(GrassMin.X, GrassMax.X), Rng.FRandRange(GrassMin.Y, GrassMax.Y));
			bool bHole = false;
			for (const FVector4& Hole : GrassHoles)
			{
				if (P.X > Hole.X && P.X < Hole.Z && P.Y > Hole.Y && P.Y < Hole.W) { bHole = true; break; }
			}
			if (!bHole) { G::Tuft(Out, P, Now, Rng); }
		}
	}
	for (int32 s = 0; s < G::NumSections; ++s) { WriteSection(s, Out.Sections[s], G::Materials[s]); }
}
