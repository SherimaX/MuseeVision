#include "Growth/Organs.h"
#include "Growth/Plant.h"

/**
 * The water-lily form (Nymphaea), re-expressed from the as-built museum's lily colonies
 * (windows/Source/MuseeVision/Nature/MuseeWaterLilies.cpp, BuildWaterLilies): each plant is a colony of floating pads
 * round its crown, round with the radial slit (the sinus), a little warped, crowded ones with their rims turned up;
 * a flowering plant holds one flower of four rings of pointed petals over four sepals, a crown of stamens round the
 * stigma disc, sitting on the water; sometimes a closed bud beside it.
 *
 * Species colours: LeafColours the pads' greens; LeafAgeColours [young bronze, old yellowing]; LeafEdgeColour the
 * rim's red; LeafMiddleColour the pale centre; FlowerColours [petal base, petal tip]; EyeColours [stamen base, stamen
 * tip, stigma disc]; StemColours [the sepals' and buds' green, the sepals' pale inner face].
 */
namespace Musee::NatureMaking
{
	namespace
	{
		/** The colony's first pad spans about half its reach, as the as-built colonies were drawn. */
		constexpr double FirstPadShare = 0.45;
		/** A colony holds round(2 + 8 x reach in metres) pads (the as-built rule, the planting plan's note). */
		constexpr double PadsAtCrown = 2.0;
		constexpr double PadsPerMeterOfReach = 8.0;
		/** Pads float a millimetre or so apart where they overlap: a first layer, and a step for each of five layers. */
		constexpr double PadFloat = 0.002;
		constexpr double PadLayerStep = 0.0009;
		constexpr int32 PadLayers = 5;
		/** A pad keeps this far off the basin's lining, and a bud this far. */
		constexpr double PadClearance = 0.04;
		constexpr double BudClearance = 0.08;
		/** A pad's edge is about 4 mm thick. */
		constexpr double PadThickness = 0.004;
		/** A pad smaller than this radius may still be young and bronze. */
		constexpr double YoungPadRadius = 0.12;
		/** The flower sits 12 mm proud of the water on its receptacle. */
		constexpr double FlowerRise = 0.012;

		struct FPadTints
		{
			FLinearColor Inner, Outer, Rim;
		};

		FPadTints PadTints(const FNatureSpecies& Species, FRandomStream& Random, double Radius)
		{
			const FLinearColor Base = Vary(Colour(Species.LeafColours, Random.RandRange(0, Species.LeafColours.Num() - 1)), Random, 4.0, 0.08, 0.1);
			FPadTints Tints{ Mix(Base, FromHex(Species.LeafMiddleColour), 0.25), Base, Mix(Base, FromHex(Species.LeafEdgeColour), Random.FRandRange(0.1, 0.5)) };
			if (Radius < YoungPadRadius && Random.FRand() < 0.5)
			{
				Tints.Inner = Mix(Tints.Inner, Colour(Species.LeafAgeColours, 0), 0.5);
				Tints.Outer = Mix(Tints.Outer, Colour(Species.LeafAgeColours, 0), 0.6);
			}
			else if (Random.FRand() < 0.15)
			{
				Tints.Outer = Mix(Tints.Outer, Colour(Species.LeafAgeColours, 1), 0.5);
			}
			return Tints;
		}

		/** A pad: round with the radial slit, a little warped, its rim turned up by Upturn, 4 mm thick at its edge. */
		void AddPad(FNatureSurface& Mesh, const FVector2D& Centre, double Z, double R, double SlitAz, double Upturn, const FPadTints& Tints, FRandomStream& Random)
		{
			constexpr int32 Segments = 44;
			static const double Rings[] = { 0.0, 0.08, 0.2, 0.36, 0.52, 0.67, 0.8, 0.9, 0.96, 1.0 };
			constexpr int32 NumRings = UE_ARRAY_COUNT(Rings);
			const double Half = FMath::DegreesToRadians(Random.FRandRange(3.0, 6.5));
			const double WarpPhase = Random.FRandRange(0.0, Tau);
			const double Warp = Random.FRandRange(0.3, 1.0);
			auto Height = [&](double Rh, double Th)
			{
				const double Edge = FMath::Square(FMath::Clamp((Rh - 0.8) / 0.2, 0.0, 1.0));
				double Gap = FMath::Fmod(FMath::Abs(Th - SlitAz), Tau);
				Gap = FMath::Min(Gap, Tau - Gap);
				// The two lobes lift a little either side of the slit.
				return Z + Upturn * R * 0.07 * Edge + Warp * R * 0.012 * FMath::Sin(2.0 * Th + WarpPhase) * Rh * Rh + R * 0.01 * FMath::Exp(-Gap / 0.2) * Rh;
			};
			auto At = [&](double Rh, double Th) { return FVector(Centre.X + FMath::Cos(Th) * R * Rh, Centre.Y + FMath::Sin(Th) * R * Rh, Height(Rh, Th)); };
			TArray<int32> Grid;
			Grid.SetNum(NumRings * (Segments + 1));
			for (int32 J = 0; J < NumRings; ++J)
			{
				for (int32 I = 0; I <= Segments; ++I)
				{
					const double Th = SlitAz + Half + (Tau - 2.0 * Half) * I / Segments;
					const double Rh = Rings[J];
					const FVector DR = At(Rh + 0.01, Th) - At(FMath::Max(0.0, Rh - 0.01), Th);
					const FVector DT = At(FMath::Max(Rh, 0.05), Th + 0.01) - At(FMath::Max(Rh, 0.05), Th - 0.01);
					FVector N = FVector::CrossProduct(DR, DT).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
					N = N.Z < 0.0 ? -N : N;
					FLinearColor Tint = Mix(Tints.Inner, Tints.Outer, FMath::Pow(Rh, 1.5));
					Tint = Rh > 0.9 ? Mix(Tint, Tints.Rim, (Rh - 0.9) / 0.1 * 0.7) : Tint;
					const FVector P = At(Rh, Th);
					Grid[J * (Segments + 1) + I] = Mesh.Vertex(P, N, FVector2D(P.X - Centre.X, P.Y - Centre.Y), Tint);
				}
			}
			for (int32 J = 0; J + 1 < NumRings; ++J)
			{
				for (int32 I = 0; I < Segments; ++I)
				{
					const int32 A = Grid[J * (Segments + 1) + I], B = Grid[J * (Segments + 1) + I + 1];
					const int32 C = Grid[(J + 1) * (Segments + 1) + I + 1], D = Grid[(J + 1) * (Segments + 1) + I];
					J == 0 ? Mesh.Triangle(A, C, D) : Mesh.Quad(A, B, C, D);
				}
			}
			int32 PreviousTop = INDEX_NONE, PreviousLow = INDEX_NONE;
			for (int32 I = 0; I <= Segments; ++I)
			{
				const double Th = SlitAz + Half + (Tau - 2.0 * Half) * I / Segments;
				const FVector Out(FMath::Cos(Th), FMath::Sin(Th), 0.0);
				const FVector Top = At(1.0, Th);
				const double Round = R * (Tau - 2.0 * Half) * I / Segments;
				const int32 T0 = Mesh.Vertex(Top, Out, FVector2D(Round, 0.0), Tints.Rim, 0.9);
				const int32 L0 = Mesh.Vertex(Top - FVector(0.0, 0.0, PadThickness), Out, FVector2D(Round, PadThickness), Tints.Rim, 0.7);
				if (I > 0)
				{
					Mesh.Quad(PreviousTop, T0, L0, PreviousLow);
				}
				PreviousTop = T0;
				PreviousLow = L0;
			}
		}

		/** A flower: four sepals, four rings of pointed petals opening outwards, the stamen crown and the stigma disc. */
		void AddFlower(const FNatureSpecies& Species, FPlantParts& Out, const FVector& Centre, double Size, FRandomStream& Random)
		{
			const double K = Size / 0.1;
			const double Lift = 2.0 - Random.FRandRange(0.85, 1.0);
			const double Az0 = Random.FRandRange(0.0, Tau);
			const FLinearColor Root = Colour(Species.FlowerColours, 0), Tip = Colour(Species.FlowerColours, 1);
			const FLinearColor SepalInner = Mix(Root, Colour(Species.StemColours, 1), 0.6);
			for (int32 S = 0; S < 4; ++S)
			{
				const double Az = Az0 + Tau * S / 4 + Random.FRandRange(-0.1, 0.1);
				const FVector Radial(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				FPetal Sepal{ Size * 1.02, Size * 0.36, 5.0 + Random.FRandRange(-3.0, 4.0), -6.0, 1.0, 0.1, 0.9, 0.25, 0.0, 4, 6 };
				AddPetal(Out.FindOrAdd(Organ::Sepal), Centre + Radial * (0.01 * K) - FVector(0.0, 0.0, 0.004), Radial, Sepal,
					[&](double, double) { return SepalInner; });
			}
			struct FRing { int32 Count; double Length, Width, Elevation, Curl; };
			static const FRing Rings[4] = { { 8, 1.0, 0.33, 17, 6 }, { 8, 0.93, 0.31, 35, 8 }, { 7, 0.82, 0.29, 52, 10 }, { 6, 0.68, 0.27, 68, 10 } };
			for (int32 R = 0; R < 4; ++R)
			{
				for (int32 P = 0; P < Rings[R].Count; ++P)
				{
					const double Az = Az0 + (P + 0.5 * R) * Tau / Rings[R].Count + Random.FRandRange(-0.09, 0.09);
					const FVector Radial(FMath::Cos(Az), FMath::Sin(Az), 0.0);
					FPetal Petal{ Size * Rings[R].Length * Random.FRandRange(0.92, 1.06), Size * Rings[R].Width * Random.FRandRange(0.9, 1.08),
						FMath::Min(80.0, Rings[R].Elevation * Lift + Random.FRandRange(-6.0, 6.0)), Rings[R].Curl + Random.FRandRange(-4.0, 4.0), 1.0, 0.12, 0.85, 0.25, 0.0, 4, 7 };
					AddPetal(Out.FindOrAdd(Organ::Flower), Centre + Radial * (0.018 * K * (1.0 - 0.18 * R)) + FVector(0.0, 0.0, 0.004 * K * R), Radial, Petal,
						[&](double, double S) { return Mix(Root, Tip, FMath::Pow(S, 1.3)); });
				}
			}
			// The stamens, golden, curving in over the stigma: 46 round a ring (the as-built count).
			const FLinearColor StamenRoot = Colour(Species.EyeColours, 0), StamenTip = Colour(Species.EyeColours, 1);
			for (int32 S = 0; S < 46; ++S)
			{
				const double Az = Tau * S / 46 + Random.FRandRange(-0.08, 0.08);
				const FVector Radial(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				FPetal Stamen{ 0.034 * K * Random.FRandRange(0.8, 1.15), 0.0045 * K, 55.0 + Random.FRandRange(-10.0, 10.0), 30.0, 1.0, 0.0, 1.0, 0.25, 0.0, 1, 2 };
				AddPetal(Out.FindOrAdd(Organ::Eye), Centre + FVector(0.0, 0.0, 0.006 * K) + Radial * (0.017 * K * Random.FRandRange(0.75, 1.1)), Radial, Stamen,
					[&](double, double S) { return Mix(StamenRoot, StamenTip, S); });
			}
			const FLinearColor Disc = Colour(Species.EyeColours, 2);
			const TArray<FVector2D> Profile = { FVector2D(0.017 * K, 0.004 * K), FVector2D(0.016 * K, 0.014 * K), FVector2D(0.009 * K, 0.009 * K), FVector2D(0.0, 0.012 * K) };
			AddLathe(Out.FindOrAdd(Organ::Eye), Centre, FVector::UpVector, Profile, 16, { Disc, Mix(Disc, StamenTip, 0.4), Mix(Disc, StamenTip, 0.2), Disc }, 0.9);
		}

		/** A closed bud on the water: a pointed ovoid, green below and the flower's tip colour above. */
		void AddBud(const FNatureSpecies& Species, FNatureSurface& Mesh, const FVector& Origin, const FVector& Axis)
		{
			constexpr double Height = 0.075, Radius = 0.022;
			const TArray<FVector2D> Profile = { FVector2D(0.0, 0.0), FVector2D(0.55 * Radius, 0.08 * Height), FVector2D(0.92 * Radius, 0.28 * Height),
				FVector2D(Radius, 0.45 * Height), FVector2D(0.8 * Radius, 0.68 * Height), FVector2D(0.42 * Radius, 0.87 * Height), FVector2D(0.08 * Radius, 0.98 * Height),
				FVector2D(0.0, Height) };
			TArray<FLinearColor> Tints;
			for (int32 I = 0; I < Profile.Num(); ++I)
			{
				Tints.Add(Mix(Colour(Species.StemColours, 0), Colour(Species.FlowerColours, 1), FMath::Pow(double(I) / (Profile.Num() - 1), 1.6)));
			}
			AddLathe(Mesh, Origin, Axis, Profile, 12, Tints, 1.0);
		}

		void GrowWaterLily(const FNatureSpecies& Species, const FPlantSite& Site, FRandomStream& Random, FPlantParts& Out)
		{
			const FVector2D At(Site.Crown.X, Site.Crown.Y);
			const double Water = Site.Crown.Z + Site.WaterLevel;
			const double Reach = Draw(Species.ReachMeters, Random);
			const double R = FMath::Clamp(Reach * FirstPadShare, double(Species.LeafLengthMeters[0]), double(Species.LeafLengthMeters[1]));
			const bool bFlower = Random.FRand() < Species.FloweringShare;
			const double SlitAz = Random.FRandRange(0.0, Tau);
			// With a flower, the first pad sits beside it, its slit towards the flower.
			const FVector2D First = bFlower ? At - FVector2D(FMath::Cos(SlitAz), FMath::Sin(SlitAz)) * (0.45 * R) : At;
			int32 Pads = 0;
			AddPad(Out.FindOrAdd(Organ::Leaf), First, Water + PadFloat, R, SlitAz, Random.FRand() < 0.3 ? 1.0 : 0.2, PadTints(Species, Random, R), Random);
			const int32 More = FMath::RoundToInt(PadsAtCrown + PadsPerMeterOfReach * Reach);
			for (int32 E = 0; E < More; ++E)
			{
				for (int32 Try = 0; Try < 8; ++Try)
				{
					const double SR = R * Random.FRandRange(0.55, 1.0);
					const double A = Random.FRandRange(0.0, Tau);
					const FVector2D C = At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * (Reach * Random.FRandRange(0.35, 1.0) + SR * 0.5);
					if (Site.IsClear && !Site.IsClear(FVector(C.X, C.Y, Water), SR + PadClearance))
					{
						continue;
					}
					const double Layer = PadFloat + PadLayerStep * (++Pads % PadLayers);
					AddPad(Out.FindOrAdd(Organ::Leaf), C, Water + Layer, SR, Random.FRandRange(0.0, Tau), Random.FRand() < 0.35 ? 1.0 : 0.15, PadTints(Species, Random, SR), Random);
					break;
				}
			}
			if (!bFlower)
			{
				return;
			}
			AddFlower(Species, Out, FVector(At.X, At.Y, Water + FlowerRise), Draw(Species.FlowerSizeMeters, Random), Random);
			if (Random.FRand() < 0.6)
			{
				const double A = Random.FRandRange(0.0, Tau);
				const FVector2D Bud = At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Random.FRandRange(0.22, 0.4);
				if (!Site.IsClear || Site.IsClear(FVector(Bud.X, Bud.Y, Water), BudClearance))
				{
					const FVector Axis = (FVector(FMath::Cos(A), FMath::Sin(A), 0.0) * 0.8 + FVector::UpVector * Random.FRandRange(0.5, 1.2)).GetSafeNormal();
					AddBud(Species, Out.FindOrAdd(Organ::Sepal), FVector(Bud.X, Bud.Y, Water - 0.01), Axis);
				}
			}
		}

		TArray<FString> WaterLilyFaults(const FNatureSpecies& Species)
		{
			TArray<FString> Out;
			NeedRange(Species.LeafLengthMeters, TEXT("leafLengthMeters (the pads' radii)"), Out);
			NeedRange(Species.FlowerSizeMeters, TEXT("flowerSizeMeters"), Out);
			NeedColours(Species.LeafColours, 1, TEXT("leafColours"), Out);
			NeedColours(Species.LeafAgeColours, 2, TEXT("leafAgeColours"), Out);
			NeedColours({ Species.LeafEdgeColour, Species.LeafMiddleColour }, 2, TEXT("leafEdgeColour and leafMiddleColour"), Out);
			NeedColours(Species.FlowerColours, 2, TEXT("flowerColours"), Out);
			NeedColours(Species.EyeColours, 3, TEXT("eyeColours"), Out);
			NeedColours(Species.StemColours, 2, TEXT("stemColours"), Out);
			return Out;
		}
	}

	const FForm& WaterLilyForm()
	{
		static const FForm Form{ TEXT("water-lily"), { Organ::Leaf, Organ::Sepal, Organ::Flower, Organ::Eye }, true, &GrowWaterLily, &WaterLilyFaults, EPlanting::EachPlant, true };
		return Form;
	}
}
