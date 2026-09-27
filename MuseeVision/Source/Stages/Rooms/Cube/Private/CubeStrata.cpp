#include "CubeStrata.h"

#include "CubePlan.h"
#include "Misc/Crc.h"

namespace Musee::Cube::Strata
{
	using namespace Plan;

	namespace
	{
		/** The key the ground's noise is seeded from (§7). */
		constexpr const TCHAR* GroundKey = TEXT("cube.ground");

		/** Where a stone lies in a cell: 15 % to 85 % across it, so neighbours rarely merge. */
		constexpr double CellMargin = 0.15;
		constexpr double CellSpan = 0.70;
		/** A stone's height across its depth: flatter 0.55, rounder 0.95 of its length. */
		constexpr double StoneFlat = 0.55;
		constexpr double StoneRound = 0.95;
		/** A stone stands proud by half to all of its rule's most. */
		constexpr double StoneLeast = 0.5;

		/** The seeds of the rule's terms, so each wanders independently of the others. */
		enum class ESeed : uint32
		{
			HardcoreWave = 1, TopsoilWave, TopsoilRipple, SubsoilWave, SubsoilRipple, ChalkWave, ChalkRipple, CrushedStones, CrushedGrain, SoilCrumbCoarse,
			SoilCrumbFine, SoilStones, ClaySmooth, SpadeWander, ChalkBlocks, ChalkGrain, PartingWave,
		};

		/** An integer hash (murmur's finaliser) of a cell and a seed: the same on every machine. */
		uint32 Mix(int64 A, int64 B, uint32 Seed)
		{
			uint32 H = uint32(A) * 0x8da6b343u ^ uint32(B) * 0xd8163841u ^ Seed * 0xcb1ab31fu;
			H ^= H >> 16;
			H *= 0x85ebca6bu;
			H ^= H >> 13;
			H *= 0xc2b2ae35u;
			H ^= H >> 16;
			return H;
		}

		/** A number 0 to 1 drawn from a hash for one of a stone's properties. */
		double Unit(uint32 Hash, uint32 Property)
		{
			return double(Mix(Hash, Property, 0x9e3779b9u) & 0xFFFFFF) / double(0x1000000);
		}

		uint32 KeySeed()
		{
			static const uint32 Seed = FCrc::StrCrc32(GroundKey);
			return Seed;
		}

		/** N(θ, z, f): Perlin noise on the nominal cylinder at f cycles per metre, from -1 to 1, periodic round the axis. */
		double Noise(double Theta, double Z, double CyclesPerMeter, ESeed Seed)
		{
			const uint32 Hash = Mix(KeySeed(), int64(Seed), 0u);
			const FVector Shift(256.0 * Unit(Hash, 1), 256.0 * Unit(Hash, 2), 256.0 * Unit(Hash, 3));
			const double R = GroundFace * CyclesPerMeter;
			return FMath::PerlinNoise3D(FVector(R * FMath::Cos(Theta), R * FMath::Sin(Theta), Z * CyclesPerMeter) + Shift);
		}

		double Term(const FNoiseTerm& T, double Theta, double Z, ESeed Seed)
		{
			return T.Amplitude * Noise(Theta, Z, T.CyclesPerMeter, Seed);
		}

		double Contact(const FContactRule& Rule, double Theta, ESeed Wave, ESeed Ripple)
		{
			return Rule.Mean + Term(Rule.Waves[0], Theta, 0.0, Wave) + Term(Rule.Waves[1], Theta, 0.0, Ripple);
		}

		/** The arc length round the nominal face at a plan angle. */
		double ArcOf(double Theta)
		{
			return GroundFace * FMath::Fmod(Theta + 2.0 * UE_DOUBLE_PI * 4.0, 2.0 * UE_DOUBLE_PI);
		}

		/** How many of a length fit round the face, whole, so a pattern of that pitch closes on itself. */
		double RoundPitch(double Pitch)
		{
			const double Round = 2.0 * UE_DOUBLE_PI * GroundFace;
			return Round / FMath::RoundToDouble(Round / Pitch);
		}

		/**
		 * The stones of a matrix at a point: jittered cells, a stone in a share of them, each a dome over an ellipse of a
		 * random size and turn, steep at its edge (a stone set in soil). The cells wrap round the face. The tallest wins.
		 */
		double Stones(double Theta, double Z, const FStoneRule& Rule, ESeed Seed)
		{
			const double CellU = RoundPitch(Rule.Cell);
			const int64 Round = FMath::RoundToInt64(2.0 * UE_DOUBLE_PI * GroundFace / CellU);
			const double U = ArcOf(Theta);
			const int64 CU = FMath::FloorToInt64(U / CellU);
			const int64 CZ = FMath::FloorToInt64(Z / Rule.Cell);
			double Tallest = 0.0;
			for (int64 DU = -1; DU <= 1; ++DU)
			{
				for (int64 DZ = -1; DZ <= 1; ++DZ)
				{
					const uint32 Hash = Mix(((CU + DU) % Round + Round) % Round, CZ + DZ, KeySeed() ^ uint32(Seed));
					if (Unit(Hash, 0) > Rule.Share)
					{
						continue;
					}
					const double Du = U - (CU + DU + CellMargin + CellSpan * Unit(Hash, 1)) * CellU;
					const double Dz = Z - (CZ + DZ + CellMargin + CellSpan * Unit(Hash, 2)) * Rule.Cell;
					const double Long = FMath::Lerp(Rule.HalfSizeMin, Rule.HalfSizeMax, Unit(Hash, 3));
					const double Short = Long * FMath::Lerp(StoneFlat, StoneRound, Unit(Hash, 4));
					const double Turn = UE_DOUBLE_PI * Unit(Hash, 5);
					const double Pu = Du * FMath::Cos(Turn) + Dz * FMath::Sin(Turn);
					const double Pz = -Du * FMath::Sin(Turn) + Dz * FMath::Cos(Turn);
					const double Q = FMath::Square(Pu / Long) + FMath::Square(Pz / Short);
					if (Q < 1.0)
					{
						Tallest = FMath::Max(Tallest, Rule.Proud * FMath::Lerp(StoneLeast, 1.0, Unit(Hash, 6)) * FMath::Sqrt(1.0 - Q));
					}
				}
			}
			return Tallest;
		}

		/** 1 well above a contact, 0 well below it, blending linearly over its width. */
		double Above(double Z, double Edge, double Width)
		{
			return FMath::Clamp((Z - Edge) / Width + 0.5, 0.0, 1.0);
		}

		/** The faces of the layers (§7's Face column). */
		double CrushedFace(double Theta, double Z)
		{
			return Stones(Theta, Z, CrushedStones, ESeed::CrushedStones) + Term(CrushedGrain, Theta, Z, ESeed::CrushedGrain);
		}

		double SoilFace(double Theta, double Z)
		{
			return Term(SoilCrumb[0], Theta, Z, ESeed::SoilCrumbCoarse) + Term(SoilCrumb[1], Theta, Z, ESeed::SoilCrumbFine)
				+ Stones(Theta, Z, SoilStones, ESeed::SoilStones);
		}

		double ClayFace(double Theta, double Z)
		{
			const double Wander = Term(SpadeWander, Theta, Z, ESeed::SpadeWander);
			const double Marks = SpadeMark * FMath::Sin(2.0 * UE_DOUBLE_PI * ArcOf(Theta) / RoundPitch(SpadePitch) + Wander);
			return Term(ClaySmooth, Theta, Z, ESeed::ClaySmooth) + Marks;
		}

		double ChalkFace(double Theta, double Z)
		{
			const double Blocks = ChalkBlocks.Amplitude * FMath::Abs(Noise(Theta, Z, ChalkBlocks.CyclesPerMeter, ESeed::ChalkBlocks));
			const double Into = FMath::Fmod(FMath::Abs(Z - Term(PartingWave, Theta, 0.0, ESeed::PartingWave)), PartingSpacing);
			const double Parting = -PartingDepth * FMath::Exp(-FMath::Square((Into - PartingWidth) / PartingWidth));
			return Blocks + Term(ChalkGrain, Theta, Z, ESeed::ChalkGrain) + Parting;
		}
	}

	const TCHAR* LayerName(ELayer Layer)
	{
		switch (Layer)
		{
		case ELayer::CrushedStone: return TEXT("crushed-stone");
		case ELayer::Topsoil: return TEXT("topsoil");
		case ELayer::Subsoil: return TEXT("subsoil");
		case ELayer::ClayWithFlints: return TEXT("clay-with-flints");
		default: return TEXT("chalk");
		}
	}

	FContacts ContactsAt(double Theta)
	{
		// The pipe at bearing 205: its arc s from the pipe's line, the shorter way round.
		const double PipeTheta = FMath::DegreesToRadians(PipeBearing - 90.0);
		const double S = GroundFace * FMath::UnwindRadians(Theta - PipeTheta);
		FContacts Out;
		Out.Z[0] = Contact(Hardcore, Theta, ESeed::HardcoreWave, ESeed::HardcoreWave);
		Out.Z[1] = Contact(TopsoilSubsoil, Theta, ESeed::TopsoilWave, ESeed::TopsoilRipple);
		Out.Z[2] = Contact(SubsoilClay, Theta, ESeed::SubsoilWave, ESeed::SubsoilRipple);
		Out.Z[3] = Contact(ClayChalk, Theta, ESeed::ChalkWave, ESeed::ChalkRipple) - PipeDepth * FMath::Exp(-FMath::Square(S / PipeHalfWidth));
		return Out;
	}

	ELayer LayerAt(double Theta, double Z)
	{
		const FContacts Here = ContactsAt(Theta);
		int32 Layer = 0;
		while (Layer < 4 && Z < Here.Z[Layer])
		{
			++Layer;
		}
		return ELayer(Layer);
	}

	double ProudAt(double Theta, double Z)
	{
		// Each face holds above its contact and blends across it: hardcore, the soils, the clay, and the chalk below all.
		const FContacts Here = ContactsAt(Theta);
		const double Hard = Above(Z, Here.Z[0], Hardcore.Blend);
		const double Soil = (1.0 - Hard) * Above(Z, Here.Z[2], SubsoilClay.Blend);
		const double Clay = (1.0 - Hard - Soil) * Above(Z, Here.Z[3], ClayChalk.Blend);
		const double Chalk = FMath::Max(0.0, 1.0 - Hard - Soil - Clay);
		double Proud = 0.0;
		Proud += Hard > 0.0 ? Hard * CrushedFace(Theta, Z) : 0.0;
		Proud += Soil > 0.0 ? Soil * SoilFace(Theta, Z) : 0.0;
		Proud += Clay > 0.0 ? Clay * ClayFace(Theta, Z) : 0.0;
		Proud += Chalk > 0.0 ? Chalk * ChalkFace(Theta, Z) : 0.0;
		return Proud;
	}

	double DeepestRecess()
	{
		const double Crushed = CrushedGrain.Amplitude;
		const double Soil = SoilCrumb[0].Amplitude + SoilCrumb[1].Amplitude;
		const double Clay = ClaySmooth.Amplitude + SpadeMark;
		const double Chalk = ChalkGrain.Amplitude + PartingDepth;
		return FMath::Max(FMath::Max(Crushed, Soil), FMath::Max(Clay, Chalk));
	}

	double HighestProud()
	{
		const double Crushed = CrushedStones.Proud + CrushedGrain.Amplitude;
		const double Soil = SoilCrumb[0].Amplitude + SoilCrumb[1].Amplitude + SoilStones.Proud;
		const double Clay = ClaySmooth.Amplitude + SpadeMark;
		const double Chalk = ChalkBlocks.Amplitude + ChalkGrain.Amplitude;
		return FMath::Max(FMath::Max(Crushed, Soil), FMath::Max(Clay, Chalk));
	}

	double AmmoniteAt(double X, double Y, double Behind)
	{
		// The whorl's centreline is a logarithmic spiral, rho(phi) = rho1 exp(b (phi - phi1)), expanding 2.1 times a turn,
		// its outer whorl's section reaching the shell's half diameter. A point may lie over several whorls (the shell is
		// involute): the highest wins.
		const double Turn = 2.0 * UE_DOUBLE_PI;
		const double Last = AmmoniteWhorls * Turn;
		const double Growth = FMath::Loge(AmmoniteExpansion) / Turn;
		const double Outer = 0.5 * AmmoniteDiameter / (1.0 + WhorlWidth);
		const double Rho = FMath::Sqrt(X * X + Y * Y);
		const double First = FMath::Fmod(FMath::Atan2(Y, X) + Turn, Turn);
		double Highest = 0.0;
		for (double Phi = First; Phi <= Last; Phi += Turn)
		{
			const double Centre = Outer * FMath::Exp(Growth * (Phi - Last));
			const double Rib = 1.0 + RibHeight * FMath::Pow(FMath::Max(0.0, FMath::Cos(RibsPerTurn * Phi)), 3.0) * FMath::Clamp(Phi / Turn, 0.0, 1.0);
			const double Half = WhorlWidth * Centre * Rib;
			const double Off = FMath::Abs(Rho - Centre);
			if (Off < Half)
			{
				Highest = FMath::Max(Highest, Behind + WhorlHeight * Centre * Rib * FMath::Sqrt(1.0 - FMath::Square(Off / Half)));
			}
		}
		return Highest;
	}
}
