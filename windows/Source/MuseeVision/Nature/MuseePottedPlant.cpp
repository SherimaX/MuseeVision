#include "Nature/MuseePottedPlant.h"

#include "Nature/NatureMesh.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"

namespace MuseePotGen
{
	namespace MN = MuseeNature;

	constexpr double SoilTop = 0.234;

	struct FOut
	{
		FNatureMesh Pot;
		FNatureMesh Soil;
		FNatureMesh Leaves;
		FNatureMesh Stems;
		FNatureMesh Flowers;
	};

	FLinearColor Opaque(const FLinearColor& C) { FLinearColor Out = C; Out.A = 1.f; return Out; }

	/** The pot (the Swift's lathe [0.12, 0] → [0.16, 0.25] → [0.17, 0.27]) with a rolled lip, soil, moss and pebbles. */
	void AddPot(FOut& Out, const FLinearColor& Glaze, const FRandomStream& Rng)
	{
		const FLinearColor Biscuit = MN::Srgb(0x8A7A68);
		const FLinearColor Deep = Opaque(Glaze * 0.75f);
		const FLinearColor Thin = MN::Mix(Glaze, MN::Srgb(0xB4BEB8), 0.45);
		const TArray<FVector2D> Profile = {
			FVector2D(0.118, 0.0), FVector2D(0.124, 0.012), FVector2D(0.128, 0.03), FVector2D(0.16, 0.238), FVector2D(0.171, 0.253),
			FVector2D(0.176, 0.265), FVector2D(0.169, 0.276), FVector2D(0.158, 0.271), FVector2D(0.153, SoilTop)};
		const TArray<FLinearColor> Colours = {Biscuit, Biscuit, Deep, Glaze, Thin, Thin, Thin, Glaze, Deep};
		FNatureWind Still;
		MN::AddLathe(Out.Pot, FVector::ZeroVector, FVector::UpVector, Profile, 40, Still, Colours, 1.0);
		const TArray<FVector2D> Earth = {FVector2D(0.153, SoilTop), FVector2D(0.11, SoilTop + 0.005), FVector2D(0.05, SoilTop + 0.009), FVector2D(0.0, SoilTop + 0.01)};
		MN::AddLathe(Out.Soil, FVector::ZeroVector, FVector::UpVector, Earth, 24, Still,
					 {MN::Srgb(0x2E241A), MN::Srgb(0x3B2E22), MN::Srgb(0x4A5530), MN::Srgb(0x56642E)}, 0.8);
		const int32 Pebbles = Rng.RandRange(5, 9);
		for (int32 p = 0; p < Pebbles; ++p)
		{
			const double A = Rng.FRandRange(0.0, MN::Tau), Rad = Rng.FRandRange(0.04, 0.13);
			const double R = Rng.FRandRange(0.007, 0.014);
			const FVector At(FMath::Cos(A) * Rad, FMath::Sin(A) * Rad, SoilTop + 0.004 + R * 0.3);
			const FVector Axis = (FVector::UpVector + Rng.GetUnitVector() * 0.3).GetSafeNormal();
			MN::AddEllipsoid(Out.Soil, At, Axis, R, R * 0.6, 8, 5, Still, MN::Vary(MN::Srgb(0x8C8A84), Rng, 8.0, 0.2, 0.2), 0.9);
		}
	}

	/** A Cymbidium: fans of arching strap leaves, and flower spikes at terms 2-5. */
	void AddOrchid(FOut& Out, int32 Term, const FLinearColor& FlowerColour, const FRandomStream& Rng)
	{
		const double Phase = Rng.FRand();
		const int32 Bulbs = 3;
		for (int32 b = 0; b < Bulbs; ++b)
		{
			const double BA = MN::Tau * b / Bulbs + Rng.FRandRange(-0.4, 0.4);
			const FVector Bulb(FMath::Cos(BA) * 0.035, FMath::Sin(BA) * 0.035, SoilTop);
			const double FanAz = Rng.FRandRange(0.0, MN::Tau);
			const int32 Leaves = Rng.RandRange(5, 8);
			for (int32 l = 0; l < Leaves; ++l)
			{
				const double Az = FanAz + (l % 2 == 0 ? 0.0 : UE_DOUBLE_PI) + Rng.FRandRange(-0.5, 0.5);
				const double E = FMath::DegreesToRadians(Rng.FRandRange(58.0, 84.0));
				const FVector Dir(FMath::Cos(Az) * FMath::Cos(E), FMath::Sin(Az) * FMath::Cos(E), FMath::Sin(E));
				const FVector Up = (FVector::UpVector - Dir * FVector::DotProduct(FVector::UpVector, Dir)).GetSafeNormal();
				FNatureBlade Blade;
				Blade.Length = Rng.FRandRange(0.34, 0.58);
				Blade.Width = Rng.FRandRange(0.008, 0.012);
				Blade.Droop = Rng.FRandRange(1.6, 3.4);
				Blade.Twist = Rng.FRandRange(30.0, 90.0) * (Rng.FRand() < 0.5 ? -1.0 : 1.0);
				Blade.Cup = 0.25;
				Blade.TipShape = 0.45;
				Blade.BaseWidth = 0.5;
				Blade.NX = 2;
				Blade.NY = 10;
				Blade.Root = MN::Vary(MN::Srgb(0x2F5A26), Rng, 4.0, 0.1, 0.12);
				Blade.Tip = MN::Vary(MN::Srgb(0x5E8438), Rng, 5.0, 0.1, 0.12);
				Blade.BendGain = 0.35;
				Blade.FlutterGain = 0.3;
				FNatureWind Wind;
				Wind.Phase = Phase;
				Wind.FlutterPhase = Rng.FRand();
				MN::AddBlade(Out.Leaves, Bulb + Dir * 0.005, Dir, Up, Blade, Wind);
			}
		}
		if (Term < 2 || Term > 5) { return; }

		// Flower spikes: slender scapes, three to six flowers on the upper half.
		const int32 Spikes = Rng.RandRange(2, 3);
		for (int32 s = 0; s < Spikes; ++s)
		{
			const double A = Rng.FRandRange(0.0, MN::Tau);
			const FVector Outward(FMath::Cos(A), FMath::Sin(A), 0);
			const FVector From = FVector(0, 0, SoilTop) + Outward * 0.03;
			const double H = Rng.FRandRange(0.28, 0.42);
			auto Scape = [&](double T) { return From + FVector::UpVector * (H * T) + Outward * (0.09 * T * T); };
			FNatureWind Wind;
			Wind.Phase = Phase;
			TArray<FNatureTubeRing> Rings;
			for (int32 r = 0; r <= 6; ++r)
			{
				const double T = r / 6.0;
				FNatureTubeRing& Ring = Rings.AddDefaulted_GetRef();
				Ring.Centre = Scape(T);
				Ring.Dir = (Scape(FMath::Min(1.0, T + 0.05)) - Scape(FMath::Max(0.0, T - 0.05))).GetSafeNormal();
				Ring.Radius = 0.0024 * (1.0 - 0.4 * T);
				Ring.Colour = MN::Mix(MN::Srgb(0x8FA06A), MN::Srgb(0x9A7A8A), T);
				Ring.Wind = Wind;
				Ring.Wind.Bend = 0.3 * T * T;
				Ring.Wind.Height = Ring.Centre.Z;
			}
			MN::AddTube(Out.Stems, Rings, 4, 0.0, true);
			const int32 Flowers = Rng.RandRange(3, 6);
			for (int32 f = 0; f < Flowers; ++f)
			{
				const double T = FMath::Lerp(0.55, 0.96, (f + 0.5) / Flowers);
				const double FA = A + (f % 2 == 0 ? 0.7 : -0.7) + Rng.FRandRange(-0.3, 0.3);
				const FVector Face = (FVector(FMath::Cos(FA), FMath::Sin(FA), 0) + FVector::UpVector * 0.1).GetSafeNormal();
				const FVector Centre = Scape(T) + Face * 0.014 - FVector::UpVector * 0.004;
				const FVector Up = (FVector::UpVector - Face * FVector::DotProduct(FVector::UpVector, Face)).GetSafeNormal();
				FNatureWind FW = Wind;
				FW.Bend = 0.3 * T * T + 0.02;
				FW.FlutterPhase = Rng.FRand();
				auto Part = [&](const FVector& Dir, double Length, double Width, double Curl, const FLinearColor& Root, const FLinearColor& Tip)
				{
					FNatureBlade Blade;
					Blade.Length = Length;
					Blade.Width = Width;
					Blade.Curl = Curl;
					Blade.TipShape = 0.7;
					Blade.BaseWidth = 0.35;
					Blade.Cup = 0.15;
					Blade.NX = 2;
					Blade.NY = 4;
					Blade.Root = Root;
					Blade.Tip = Tip;
					Blade.BendGain = 0.02;
					Blade.FlutterGain = 0.4;
					const FVector D = Dir.GetSafeNormal();
					const FVector N = (Face - D * FVector::DotProduct(Face, D)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
					MN::AddBlade(Out.Flowers, Centre, D, N, Blade, FW);
				};
				const FLinearColor Petal = MN::Vary(FlowerColour, Rng, 4.0, 0.1, 0.05);
				const FLinearColor PetalBase = MN::Mix(Petal, MN::Srgb(0xC8D8A8), 0.35);
				Part(Up * 0.9 + Face * 0.3, 0.032, 0.008, 12.0, PetalBase, Petal);                                    // dorsal sepal
				Part(MN::RotateAbout(Up, Face, 2.0) * 0.9 + Face * 0.3, 0.032, 0.008, 10.0, PetalBase, Petal);         // lateral sepals
				Part(MN::RotateAbout(Up, Face, -2.0) * 0.9 + Face * 0.3, 0.032, 0.008, 10.0, PetalBase, Petal);
				Part(MN::RotateAbout(Up, Face, 0.6) + Face * 0.7, 0.025, 0.008, 25.0, PetalBase, Petal);              // petals over the column
				Part(MN::RotateAbout(Up, Face, -0.6) + Face * 0.7, 0.025, 0.008, 25.0, PetalBase, Petal);
				Part(-Up * 0.7 + Face * 0.7, 0.02, 0.012, -70.0, MN::Srgb(0xF6F2EC), MN::Srgb(0xB04A6A));            // the spotted lip
				MN::AddEllipsoid(Out.Flowers, Centre + Face * 0.006 + Up * 0.003, Face, 0.003, 0.008, 6, 4, FW, MN::Srgb(0xF0ECD8), 0.9);   // column
			}
			// Buds at the tip.
			MN::AddEllipsoid(Out.Flowers, Scape(1.0) + FVector::UpVector * 0.008, FVector::UpVector, 0.005, 0.01, 6, 4, Wind, MN::Srgb(0xB8C898), 0.9);
		}
	}

	/** A mounded chrysanthemum: many stems, lobed leaves, a dome of flower heads at terms 16-19 (buds from 13). */
	void AddChrysanthemum(FOut& Out, int32 Term, const FLinearColor& FlowerColour, const FRandomStream& Rng)
	{
		const bool bFlowering = Term >= 16 && Term <= 19;
		const bool bBuds = Term >= 12 && Term <= 15;
		const double Phase = Rng.FRand();
		const FVector Dome(0, 0, SoilTop + 0.05);
		const FLinearColor StemColour = MN::Srgb(0x5A6A3A);
		TArray<TPair<FVector, FVector>> Tips;
		auto Stem = [&](const FVector& From, const FVector& Dir0, double Length, double Radius, double BaseBend, TArray<FNatureTubeRing>& Rings)
		{
			FVector P = From;
			FVector D = Dir0;
			const int32 Segs = 4;
			for (int32 s = 0; s <= Segs; ++s)
			{
				const double T = double(s) / Segs;
				FNatureTubeRing& Ring = Rings.AddDefaulted_GetRef();
				Ring.Centre = P;
				Ring.Dir = D;
				Ring.Radius = Radius * (1.0 - 0.4 * T);
				Ring.Colour = StemColour;
				Ring.Wind.Bend = BaseBend + 0.25 * T;
				Ring.Wind.Phase = Phase;
				Ring.Wind.Height = P.Z;
				Ring.Wind.Twig = 1.0;
				if (s < Segs)
				{
					const FVector Old = D;
					D = (D + (P - Dome).GetSafeNormal2D() * 0.12 - FVector::UpVector * 0.06 + Rng.GetUnitVector() * 0.08).GetSafeNormal(UE_SMALL_NUMBER, Old);
					P += D * (Length / Segs);
				}
			}
			MN::AddTube(Out.Stems, Rings, 4, 0.0, true);
			// Leaves along it.
			const int32 NumLeaves = FMath::Max(2, FMath::RoundToInt(Length / 0.035));
			for (int32 l = 0; l < NumLeaves; ++l)
			{
				const double T = (l + 0.5) / NumLeaves;
				const int32 I = FMath::Clamp(FMath::FloorToInt(T * Segs), 0, Segs - 1);
				const double F = T * Segs - I;
				const FVector At = FMath::Lerp(Rings[I].Centre, Rings[I + 1].Centre, F);
				const FVector Along = Rings[I].Dir;
				FVector U, W;
				MN::Basis(Along, U, W);
				const double A = l * 2.4 + Rng.FRandRange(-0.3, 0.3);
				const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
				FVector LeafDir = MN::RotateAbout(Along, FVector::CrossProduct(Along, Radial), FMath::DegreesToRadians(Rng.FRandRange(45.0, 70.0)));
				LeafDir = (LeafDir - FVector::UpVector * 0.3).GetSafeNormal();
				FVector N = (FVector::UpVector + Radial * 0.3) - LeafDir * FVector::DotProduct(FVector::UpVector + Radial * 0.3, LeafDir);
				N = N.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
				const double Len = Rng.FRandRange(0.05, 0.075) * (1.0 - 0.3 * T);
				FNatureWind Base;
				Base.Bend = BaseBend + 0.25 * T;
				Base.Phase = Phase;
				Base.FlutterPhase = Rng.FRand();
				Base.Height = At.Z;
				Base.Twig = 1.0;
				FNatureWind Tip = Base;
				Tip.Flutter = 1.0;
				const FLinearColor Colour = MN::Vary(T < 0.25 && Rng.FRand() < 0.4 ? MN::Srgb(0x7A7A3A) : MN::Srgb(0x3E5E2E), Rng, 5.0, 0.1, 0.14);
				MN::AddCard(Out.Leaves, At, LeafDir, N, Len, Len * 0.75, 0.25, 0.08, 1, Base, Tip, Colour, FMath::Clamp(0.55 + 0.45 * T, 0.55, 1.0));
			}
			Tips.Add(TPair<FVector, FVector>(Rings.Last().Centre, Rings.Last().Dir));
		};
		const int32 NumStems = Rng.RandRange(11, 15);
		for (int32 s = 0; s < NumStems; ++s)
		{
			const double A = MN::Tau * s / NumStems + Rng.FRandRange(-0.25, 0.25);
			const double E = FMath::DegreesToRadians(Rng.FRandRange(50.0, 86.0));
			const FVector Dir(FMath::Cos(A) * FMath::Cos(E), FMath::Sin(A) * FMath::Cos(E), FMath::Sin(E));
			const FVector From(FMath::Cos(A) * 0.03, FMath::Sin(A) * 0.03, SoilTop);
			TArray<FNatureTubeRing> Main;
			Stem(From, Dir, Rng.FRandRange(0.2, 0.34), 0.004, 0.0, Main);
			// Two side shoots from the upper part.
			for (int32 k = 0; k < 2; ++k)
			{
				const FNatureTubeRing& Fork = Main[FMath::Clamp(2 + k, 0, Main.Num() - 1)];
				const FVector SideDir = (Fork.Dir + Rng.GetUnitVector() * 0.9 + (Fork.Centre - Dome).GetSafeNormal() * 0.5).GetSafeNormal();
				TArray<FNatureTubeRing> Side;
				Stem(Fork.Centre, SideDir, Rng.FRandRange(0.07, 0.14), 0.0026, Fork.Wind.Bend, Side);
			}
		}
		// The flower heads (or buds) at every tip, facing out of the dome.
		for (const TPair<FVector, FVector>& Tip : Tips)
		{
			const FVector At = Tip.Key;
			const FVector Facing = ((At - Dome).GetSafeNormal() + FVector::UpVector * 0.6 + Rng.GetUnitVector() * 0.2).GetSafeNormal();
			FNatureWind Wind;
			Wind.Bend = 0.28;
			Wind.Phase = Phase;
			Wind.FlutterPhase = Rng.FRand();
			Wind.Height = At.Z;
			Wind.Twig = 1.0;
			if (bFlowering)
			{
				const FLinearColor C = MN::Vary(FlowerColour, Rng, 5.0, 0.08, 0.1);
				const double Size = Rng.FRandRange(0.048, 0.064);
				Wind.Flutter = 0.3;
				MN::AddFlowerCard(Out.Flowers, At + Facing * 0.006, Facing, Rng.FRandRange(0.0, MN::Tau), Size, 0.35, Wind, C, 1.0);
				MN::AddFlowerCard(Out.Flowers, At + Facing * 0.013, Facing, Rng.FRandRange(0.0, MN::Tau), Size * 0.66, 0.5, Wind,
								  MN::Mix(C, MN::Srgb(0xF2D070), 0.2), 1.0);
			}
			else if (bBuds)
			{
				MN::AddEllipsoid(Out.Stems, At + Facing * 0.006, Facing, 0.007, 0.006, 8, 5, Wind, MN::Srgb(0x7A8A3A), 1.0);
			}
		}
	}
}

AMuseePottedPlant::AMuseePottedPlant()
{
	PotCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("PotCollision"));
	PotCollision->SetupAttachment(RootComponent);
	PotCollision->SetMobility(EComponentMobility::Static);
	PotCollision->SetCapsuleSize(19.f, 30.f);
	PotCollision->SetRelativeLocation(FVector(0, 0, 30));
	PotCollision->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	PotCollision->SetHiddenInGame(true);
}

void AMuseePottedPlant::BuildPlant()
{
	MuseePotGen::FOut Out;
	FRandomStream Rng(Seed * 7919 + 5);
	MuseePotGen::AddPot(Out, PotColour, Rng);
	const int32 Term = EffectiveTerm();
	const bool bOrchid = Kind == EMuseePottedPlantKind::Orchid;
	const uint32 Hex = FlowerColour != 0 ? uint32(FlowerColour) : (bOrchid ? 0xE8D8EEu : 0xE6B84Au);
	if (bOrchid) { MuseePotGen::AddOrchid(Out, Term, MuseeNature::Srgb(Hex), Rng); }
	else { MuseePotGen::AddChrysanthemum(Out, Term, MuseeNature::Srgb(Hex), Rng); }
	WriteSection(0, Out.Pot, TEXT("MI_Pot"));
	WriteSection(1, Out.Soil, TEXT("MI_Soil"));
	WriteSection(2, Out.Leaves, bOrchid ? TEXT("MI_Leaf_Orchid") : TEXT("MI_Leaf_Chrysanthemum"));
	WriteSection(3, Out.Stems, TEXT("MI_Stem_Green"));
	WriteSection(4, Out.Flowers, bOrchid ? TEXT("MI_Petal_Orchid") : TEXT("MI_Flower_Chrysanthemum"));
}
