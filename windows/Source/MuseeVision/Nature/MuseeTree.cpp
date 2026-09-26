#include "Nature/MuseeTree.h"

#include "Nature/NatureMesh.h"
#include "Algo/BinarySearch.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"

namespace MuseeTreeGen
{
	namespace MN = MuseeNature;

	/** One order of branching: how many children, where, at what angle, how long and how they bend. */
	struct FLevel
	{
		int32 Count = 0;          // level 1: children of the trunk
		double Density = 0;       // deeper levels: children per metre of the parent
		double Start = 0.1;       // where along the parent the children begin and end (0 … 1)
		double End = 1.0;
		double Angle = 45;        // degrees from the parent's axis
		double AngleLow = 45;     // level 1: the angle at the bottom of the crown (Angle at the top)
		double AngleVar = 10;
		double Phyllo = 137.5;    // degrees round the parent between successive children
		double Length = 0.5;      // level 1: fraction of the way to the crown's edge; deeper: of the parent's length
		double LengthVar = 0.2;
		double LengthTaper = 0.4; // children shorter towards the parent's tip
		double Radius = 0.5;      // of the parent's radius where they start
		double Curve = 0;         // degrees over the length, + up
		double CurveVar = 5;      // random wander per segment, degrees
		double Gravity = 0;       // droop, degrees
		double Up = 0;            // upward tropism, degrees
		double Zigzag = 0;        // a kink at each segment, alternating, degrees
		int32 Segments = 4;
		int32 Sides = 4;
		double BendGain = 0.3;    // how much more the wind bends it from base to tip
	};

	struct FSpec
	{
		double Height = 8;
		double TrunkRadius = 0.12;
		double TrunkTip = 0.08;     // the trunk's radius at its top (fraction)
		double Flare = 0.3;
		int32 FlareLobes = 4;
		double Gnarl = 0;
		double Lean = 2;
		double Wander = 3;
		double TrunkZigzag = 0;
		double TrunkFrac = 0.95;    // trunk length (fraction of the height)
		double Follow = 0.5;        // the crown follows the trunk's top this much
		int32 TrunkSegments = 10;
		int32 TrunkSides = 14;
		int32 Stems = 1;
		double StemSpread = 0;
		bool bEven = false;         // level 1 spaced evenly round the trunk (pruned scaffolds)
		double MinRadius = 0.002;
		double CrownCentre = 6;
		double CrownH = 2;
		double CrownV = 2;
		TArray<FLevel> Levels;
		// Leaves
		int32 LeafLevel = 2;
		double LeafStart = 0.15;
		int32 LeafCount = 10000;
		double LeafLength = 0.07;
		double LeafAspect = 0.6;
		double LeafAngle = 50;
		double LeafDroop = 0.3;
		double LeafFold = 0.25;
		double LeafCurl = 0.06;
		int32 LeafRows = 1;
		const TCHAR* Bark = TEXT("MI_Bark_Birch");
		const TCHAR* LeafMaterial = TEXT("MI_Leaf_Birch");
	};

	/** What the season shows. */
	struct FLook
	{
		double LeafAmount = 1;           // of the full count (0 = bare)
		double LeafScale = 1;
		TArray<FLinearColor> LeafColours;
		double FlowerCount = 0;
		double FlowerSize = 0.03;
		int32 ClusterMin = 1;
		int32 ClusterMax = 3;
		double Pedicel = 0.012;
		double FacingUp = 0.3;
		double Cup = 0.2;
		int32 FlowerLevel = 2;
		FLinearColor FlowerA = FLinearColor::White;
		FLinearColor FlowerB = FLinearColor::White;
		double FlowerBShare = 0;
		const TCHAR* FlowerMaterial = TEXT("MI_Blossom_Cherry");
		int32 FruitCount = 0;
		double FruitRadius = 0.02;
		double FruitElong = 0.95;
		double Stalk = 0.03;
		bool bPairs = false;
		TArray<FLinearColor> FruitColours;
	};

	FLevel& AddLevel(FSpec& S) { return S.Levels.AddDefaulted_GetRef(); }

	/** How far P can go along D (plan) before it leaves Box: the nearest side ahead (0 if already beyond it; 1e9 if none). */
	double RayExitBox2D(const FVector2D& P, const FVector2D& D, const FBox2D& Box)
	{
		double T = 1e9;
		if (D.X > 1e-6) { T = FMath::Min(T, (Box.Max.X - P.X) / D.X); }
		else if (D.X < -1e-6) { T = FMath::Min(T, (Box.Min.X - P.X) / D.X); }
		if (D.Y > 1e-6) { T = FMath::Min(T, (Box.Max.Y - P.Y) / D.Y); }
		else if (D.Y < -1e-6) { T = FMath::Min(T, (Box.Min.Y - P.Y) / D.Y); }
		return FMath::Max(0.0, T);
	}

	/** The box less a margin on every side (never inverted). */
	FBox2D Shrunk(const FBox2D& Box, double Margin)
	{
		if (!Box.bIsValid) { return Box; }
		FBox2D Out(Box.Min + FVector2D(Margin, Margin), Box.Max - FVector2D(Margin, Margin));
		if (Out.Min.X > Out.Max.X) { Out.Min.X = Out.Max.X = Box.GetCenter().X; }
		if (Out.Min.Y > Out.Max.Y) { Out.Min.Y = Out.Max.Y = Box.GetCenter().Y; }
		return Out;
	}

	bool InPlan(const FBox2D& Box, const FVector& P) { return !Box.bIsValid || Box.IsInside(FVector2D(P.X, P.Y)); }

	FSpec BirchSpec()
	{
		FSpec S;
		S.Height = 9.3; S.TrunkRadius = 0.13; S.TrunkTip = 0.07; S.Flare = 0.28; S.FlareLobes = 4;
		S.Lean = 1.5; S.Wander = 2.0; S.TrunkFrac = 0.97; S.TrunkSegments = 14; S.TrunkSides = 16; S.MinRadius = 0.0013;
		S.CrownCentre = 6.8; S.CrownH = 2.0; S.CrownV = 2.6;
		FLevel& L1 = AddLevel(S);
		L1.Count = 32; L1.Start = 0.40; L1.End = 0.97; L1.Angle = 32; L1.AngleLow = 70; L1.AngleVar = 8; L1.Phyllo = 137.5;
		L1.Length = 0.9; L1.LengthVar = 0.15; L1.Radius = 0.40; L1.Curve = 16; L1.CurveVar = 4; L1.Gravity = 10;
		L1.Segments = 8; L1.Sides = 7; L1.BendGain = 0.30;
		FLevel& L2 = AddLevel(S);
		L2.Density = 5.0; L2.Start = 0.15; L2.Angle = 50; L2.AngleVar = 12; L2.Phyllo = 150; L2.Length = 0.45; L2.LengthVar = 0.3;
		L2.LengthTaper = 0.5; L2.Radius = 0.55; L2.Curve = -8; L2.CurveVar = 7; L2.Gravity = 50; L2.Segments = 5; L2.Sides = 5; L2.BendGain = 0.35;
		FLevel& L3 = AddLevel(S);
		L3.Density = 8.0; L3.Start = 0.1; L3.Angle = 35; L3.AngleVar = 12; L3.Phyllo = 170; L3.Length = 0.6; L3.LengthVar = 0.3;
		L3.LengthTaper = 0.3; L3.Radius = 0.6; L3.Curve = 0; L3.CurveVar = 8; L3.Gravity = 140; L3.Segments = 4; L3.Sides = 3; L3.BendGain = 0.35;
		S.LeafLevel = 2; S.LeafStart = 0.25; S.LeafCount = 15000; S.LeafLength = 0.062; S.LeafAspect = 0.74; S.LeafAngle = 55;
		S.LeafDroop = 0.45; S.LeafFold = 0.2; S.LeafCurl = 0.04; S.LeafRows = 1;
		S.Bark = TEXT("MI_Bark_Birch"); S.LeafMaterial = TEXT("MI_Leaf_Birch");
		return S;
	}

	FSpec OrchardSpec(bool bApple)
	{
		FSpec S;
		S.Height = 6.4; S.TrunkRadius = bApple ? 0.15 : 0.14; S.TrunkTip = 0.55; S.Flare = 0.3; S.FlareLobes = 5; S.Gnarl = bApple ? 0.05 : 0.02;
		S.Lean = 3; S.Wander = 4; S.TrunkFrac = bApple ? 0.42 : 0.47; S.TrunkSegments = 8; S.TrunkSides = 16; S.bEven = true; S.MinRadius = 0.002;
		S.CrownCentre = 4.7; S.CrownH = 2.3; S.CrownV = 1.9;
		FLevel& L1 = AddLevel(S);
		L1.Count = bApple ? 5 : 6; L1.Start = 0.6; L1.End = 1.0; L1.Angle = bApple ? 40 : 32; L1.AngleLow = bApple ? 58 : 50; L1.AngleVar = 7;
		L1.Length = 0.95; L1.LengthVar = 0.1; L1.Radius = 0.66; L1.Curve = -6; L1.CurveVar = 5; L1.Zigzag = bApple ? 7 : 4;
		L1.Segments = 8; L1.Sides = 10; L1.BendGain = 0.22;
		FLevel& L2 = AddLevel(S);
		L2.Density = 5.5; L2.Start = 0.1; L2.Angle = 45; L2.AngleVar = 12; L2.Length = 0.55; L2.LengthVar = 0.3; L2.LengthTaper = 0.55;
		L2.Radius = 0.55; L2.Curve = 8; L2.CurveVar = 9; L2.Gravity = 12; L2.Up = 12; L2.Zigzag = 6; L2.Segments = 5; L2.Sides = 6; L2.BendGain = 0.3;
		FLevel& L3 = AddLevel(S);
		L3.Density = 9; L3.Start = 0.08; L3.Angle = 45; L3.AngleVar = 15; L3.Length = 0.55; L3.LengthVar = 0.35; L3.LengthTaper = 0.4;
		L3.Radius = 0.55; L3.Curve = 6; L3.CurveVar = 12; L3.Gravity = 22; L3.Zigzag = 7; L3.Segments = 4; L3.Sides = 4; L3.BendGain = 0.3;
		FLevel& L4 = AddLevel(S);
		L4.Density = 10; L4.Start = 0.1; L4.Angle = 50; L4.AngleVar = 15; L4.Length = 0.4; L4.LengthVar = 0.4; L4.LengthTaper = 0.3;
		L4.Radius = 0.6; L4.Curve = 5; L4.CurveVar = 12; L4.Gravity = 20; L4.Up = 4; L4.Zigzag = 5; L4.Segments = 2; L4.Sides = 3; L4.BendGain = 0.2;
		S.LeafLevel = 3; S.LeafStart = 0.1; S.LeafCount = 12000; S.LeafLength = bApple ? 0.08 : 0.1; S.LeafAspect = bApple ? 0.62 : 0.48;
		S.LeafAngle = 50; S.LeafDroop = bApple ? 0.22 : 0.35; S.LeafFold = 0.3; S.LeafCurl = 0.08; S.LeafRows = 1;
		S.Bark = bApple ? TEXT("MI_Bark_Apple") : TEXT("MI_Bark_Cherry");
		S.LeafMaterial = bApple ? TEXT("MI_Leaf_Apple") : TEXT("MI_Leaf_Cherry");
		return S;
	}

	FSpec PlumSpec()
	{
		FSpec S;
		S.Height = 3.6; S.TrunkRadius = 0.12; S.TrunkTip = 0.45; S.Flare = 0.35; S.FlareLobes = 3; S.Gnarl = 0.16;
		S.Lean = 11; S.Wander = 8; S.TrunkZigzag = 9; S.TrunkFrac = 0.62; S.Follow = 0.85; S.TrunkSegments = 9; S.TrunkSides = 14;
		S.MinRadius = 0.002; S.CrownCentre = 2.75; S.CrownH = 1.35; S.CrownV = 0.95;
		FLevel& L1 = AddLevel(S);
		L1.Count = 7; L1.Start = 0.45; L1.End = 1.0; L1.Angle = 40; L1.AngleLow = 62; L1.AngleVar = 12; L1.Length = 0.95; L1.LengthVar = 0.2;
		L1.Radius = 0.6; L1.Curve = 4; L1.CurveVar = 8; L1.Gravity = 4; L1.Up = 10; L1.Zigzag = 18; L1.Segments = 6; L1.Sides = 9; L1.BendGain = 0.25;
		FLevel& L2 = AddLevel(S);
		L2.Density = 4.5; L2.Start = 0.12; L2.Angle = 48; L2.AngleVar = 15; L2.Phyllo = 160; L2.Length = 0.55; L2.LengthVar = 0.3;
		L2.LengthTaper = 0.45; L2.Radius = 0.5; L2.Curve = 6; L2.CurveVar = 8; L2.Gravity = 6; L2.Up = 8; L2.Zigzag = 20; L2.Segments = 4;
		L2.Sides = 5; L2.BendGain = 0.3;
		// The long straight one-year shoots of the mume, which carry the flowers.
		FLevel& L3 = AddLevel(S);
		L3.Density = 7.0; L3.Start = 0.1; L3.Angle = 38; L3.AngleVar = 10; L3.Phyllo = 180; L3.Length = 0.6; L3.LengthVar = 0.3;
		L3.LengthTaper = 0.3; L3.Radius = 0.5; L3.Curve = 8; L3.CurveVar = 4; L3.Up = 22; L3.Zigzag = 3; L3.Segments = 3; L3.Sides = 4; L3.BendGain = 0.3;
		FLevel& L4 = AddLevel(S);
		L4.Density = 8.0; L4.Start = 0.1; L4.Angle = 45; L4.AngleVar = 15; L4.Phyllo = 140; L4.Length = 0.35; L4.LengthVar = 0.4;
		L4.LengthTaper = 0.3; L4.Radius = 0.6; L4.Curve = 4; L4.CurveVar = 10; L4.Gravity = 5; L4.Up = 10; L4.Zigzag = 10; L4.Segments = 2;
		L4.Sides = 3; L4.BendGain = 0.2;
		S.LeafLevel = 3; S.LeafStart = 0.1; S.LeafCount = 7000; S.LeafLength = 0.07; S.LeafAspect = 0.55; S.LeafAngle = 50;
		S.LeafDroop = 0.25; S.LeafFold = 0.25; S.LeafCurl = 0.06;
		S.Bark = TEXT("MI_Bark_Plum"); S.LeafMaterial = TEXT("MI_Leaf_Plum");
		return S;
	}

	FSpec OsmanthusSpec()
	{
		FSpec S;
		S.Height = 4.1; S.TrunkRadius = 0.075; S.TrunkTip = 0.4; S.Flare = 0.2; S.FlareLobes = 0; S.Gnarl = 0.04;
		S.Lean = 4; S.Wander = 5; S.TrunkFrac = 0.55; S.TrunkSegments = 8; S.TrunkSides = 10; S.Stems = 3; S.StemSpread = 13;
		S.MinRadius = 0.002; S.CrownCentre = 2.7; S.CrownH = 1.5; S.CrownV = 1.45;
		FLevel& L1 = AddLevel(S);
		L1.Count = 8; L1.Start = 0.3; L1.End = 1.0; L1.Angle = 35; L1.AngleLow = 55; L1.AngleVar = 10; L1.Length = 0.9; L1.LengthVar = 0.15;
		L1.Radius = 0.62; L1.Curve = 6; L1.CurveVar = 7; L1.Gravity = 6; L1.Zigzag = 4; L1.Segments = 6; L1.Sides = 6; L1.BendGain = 0.25;
		FLevel& L2 = AddLevel(S);
		L2.Density = 6; L2.Start = 0.1; L2.Angle = 45; L2.AngleVar = 15; L2.Length = 0.5; L2.LengthVar = 0.3; L2.LengthTaper = 0.5;
		L2.Radius = 0.6; L2.Curve = 6; L2.CurveVar = 10; L2.Gravity = 10; L2.Zigzag = 4; L2.Segments = 4; L2.Sides = 4; L2.BendGain = 0.3;
		FLevel& L3 = AddLevel(S);
		L3.Density = 9; L3.Start = 0.1; L3.Angle = 40; L3.AngleVar = 15; L3.Phyllo = 180; L3.Length = 0.5; L3.LengthVar = 0.3;
		L3.LengthTaper = 0.4; L3.Radius = 0.6; L3.Curve = 6; L3.CurveVar = 10; L3.Gravity = 8; L3.Up = 4; L3.Segments = 3; L3.Sides = 3; L3.BendGain = 0.3;
		S.LeafLevel = 2; S.LeafStart = 0.05; S.LeafCount = 11000; S.LeafLength = 0.085; S.LeafAspect = 0.36; S.LeafAngle = 45;
		S.LeafDroop = 0.1; S.LeafFold = 0.35; S.LeafCurl = 0.05;
		S.Bark = TEXT("MI_Bark_Osmanthus"); S.LeafMaterial = TEXT("MI_Leaf_Osmanthus");
		return S;
	}

	TArray<FLinearColor> Palette(std::initializer_list<uint32> Hexes)
	{
		TArray<FLinearColor> Out;
		for (uint32 H : Hexes) { Out.Add(MN::Srgb(H)); }
		return Out;
	}

	/** The season's look, after the Swift's colours (Shared/Wings/HallOfLight.swift, ChineseWing.swift). */
	FLook LookFor(EMuseeTreeSpecies Kind, bool bApple, EMuseeNatureSeason Now, int32 Term)
	{
		FLook L;
		switch (Kind)
		{
		case EMuseeTreeSpecies::Birch:
			switch (Now)
			{
			case EMuseeNatureSeason::Spring:
				L.LeafAmount = 0.85; L.LeafScale = 0.8; L.LeafColours = Palette({0x9DBF55, 0x8DB24A, 0xA9C766, 0x93B84E});
				break;
			case EMuseeNatureSeason::Summer:
				L.LeafColours = Palette({0x557F2E, 0x4E7A2A, 0x648A36, 0x5A8433, 0x4A7228});
				break;
			case EMuseeNatureSeason::Autumn:
				L.LeafAmount = 0.8; L.LeafColours = Palette({0xD9B337, 0xE0C045, 0xC9A030, 0xB8A83E, 0xC08A28, 0xE3C552});
				break;
			default:
				L.LeafAmount = 0;
				break;
			}
			break;

		case EMuseeTreeSpecies::OrchardFruit:
			switch (Now)
			{
			case EMuseeNatureSeason::Spring:
				L.LeafAmount = bApple ? 0.35 : 0.25; L.LeafScale = 0.55;
				L.LeafColours = bApple ? Palette({0x86A04A, 0x7E9A44, 0x94AC52}) : Palette({0x8A8A45, 0x7C8A3E, 0x9A8C48});
				L.FlowerCount = bApple ? 5200 : 7000; L.FlowerSize = bApple ? 0.038 : 0.032;
				L.ClusterMin = bApple ? 4 : 2; L.ClusterMax = bApple ? 6 : 5;
				L.Pedicel = bApple ? 0.018 : 0.028; L.FacingUp = bApple ? 0.45 : -0.15; L.Cup = 0.22;
				L.FlowerA = MN::Srgb(bApple ? 0xFBF2F2 : 0xFBF5F4);
				L.FlowerB = MN::Srgb(bApple ? 0xF2C4D0 : 0xF7E0E6);
				L.FlowerBShare = bApple ? 0.3 : 0.4;
				L.FlowerMaterial = bApple ? TEXT("MI_Blossom_Apple") : TEXT("MI_Blossom_Cherry");
				break;
			case EMuseeNatureSeason::Summer:
				L.LeafColours = bApple ? Palette({0x557F3A, 0x4C7634, 0x5E8840, 0x51793A}) : Palette({0x4A7A2E, 0x3F6E28, 0x55833A, 0x467529});
				if (bApple)
				{
					L.FruitCount = 90; L.FruitRadius = 0.022; L.FruitElong = 0.9; L.Stalk = 0.018;
					L.FruitColours = Palette({0x9DB84A, 0xAFC25A, 0x93AE44});
				}
				else
				{
					// Late June: the cherries are ripe.
					L.FruitCount = 150; L.FruitRadius = 0.011; L.FruitElong = 0.95; L.Stalk = 0.035; L.bPairs = true;
					L.FruitColours = Palette({0x7A0C18, 0x9E1422, 0x8A1020, 0x6A0A14});
				}
				break;
			case EMuseeNatureSeason::Autumn:
				L.LeafAmount = bApple ? 0.85 : 0.75;
				L.LeafColours = bApple ? Palette({0x8A9A3E, 0xB9A13E, 0xC98A34, 0x6E8238, 0xA89A40})
									   : Palette({0xC4502A, 0xD9782E, 0xB83A22, 0xD9A03A, 0x9A8A3A, 0xE08A30});
				if (bApple)
				{
					L.FruitCount = 110; L.FruitRadius = 0.034; L.FruitElong = 0.88; L.Stalk = 0.016;
					L.FruitColours = Palette({0xB8322A, 0xC9502E, 0xD4AA3A, 0xA82A24});
				}
				break;
			default:
				L.LeafAmount = 0;
				break;
			}
			break;

		case EMuseeTreeSpecies::ChinesePlum:
			if (Term <= 2 || Term >= 22)
			{
				// Blossom on the bare wood: pink at Lichun, red in the deep cold.
				L.LeafAmount = 0;
				const bool bRed = Term >= 22;
				L.FlowerCount = 2600; L.FlowerSize = 0.024; L.ClusterMin = 1; L.ClusterMax = 2; L.Pedicel = 0.004;
				L.FacingUp = 0.2; L.Cup = 0.28; L.FlowerLevel = 1;
				L.FlowerA = MN::Srgb(bRed ? 0xB0343A : 0xF0B8C8);
				L.FlowerB = MN::Srgb(bRed ? 0xC24048 : 0xF7D5DD);
				L.FlowerBShare = 0.4;
				L.FlowerMaterial = TEXT("MI_Blossom_Plum");
			}
			else if (Term <= 5)
			{
				L.LeafAmount = 0.7; L.LeafScale = 0.8; L.LeafColours = Palette({0x86A84E, 0x7EA048, 0x92B055});
			}
			else if (Term <= 14)
			{
				L.LeafColours = Palette({0x55803A, 0x4D7834, 0x5E8A40, 0x52793A});
				if (Term >= 7 && Term <= 10)
				{
					// The plum rain (meiyu): the fruit ripens yellow-green.
					L.FruitCount = 45; L.FruitRadius = 0.015; L.FruitElong = 1.0; L.Stalk = 0.006;
					L.FruitColours = Palette({0xB8C050, 0xC8C458, 0xA8B848});
				}
			}
			else if (Term <= 17)
			{
				L.LeafAmount = 0.7; L.LeafColours = Palette({0xC9A45C, 0xB89A48, 0xD2B060, 0xA89250});
			}
			else
			{
				L.LeafAmount = 0;
			}
			break;

		case EMuseeTreeSpecies::Osmanthus:
			L.LeafColours = Palette({0x34502A, 0x2E4A26, 0x3E5C30, 0x46633A, 0x3A5A2E});
			if (Term >= 3 && Term <= 5) { L.LeafColours.Append(Palette({0x6E8A3E, 0x78924A})); }
			if (Term >= 14 && Term <= 16)
			{
				// Around Qiufen: tiny four-lobed flowers in the leaf axils.
				L.FlowerCount = 3600; L.FlowerSize = 0.0095; L.ClusterMin = 5; L.ClusterMax = 10; L.Pedicel = 0.008;
				L.FacingUp = 0.1; L.Cup = 0.3; L.FlowerLevel = 2;
				L.FlowerA = MN::Srgb(0xE8A44A); L.FlowerB = MN::Srgb(0xF0C070); L.FlowerBShare = 0.4;
				L.FlowerMaterial = TEXT("MI_Flower_Osmanthus");
			}
			break;

		default:
			break;
		}
		return L;
	}

	struct FBranch
	{
		TArray<FVector> P;
		TArray<FVector> D;
		TArray<double> R;
		TArray<double> Bend;
		TArray<double> Phase;
		int32 Level = 0;
		double Length = 0;

		void Sample(double T, FVector& OutP, FVector& OutD, double& OutR, double& OutBend, double& OutPhase) const
		{
			const int32 Segs = P.Num() - 1;
			const double X = FMath::Clamp(T, 0.0, 1.0) * Segs;
			const int32 I = FMath::Clamp(FMath::FloorToInt(X), 0, Segs - 1);
			const double F = X - I;
			OutP = FMath::Lerp(P[I], P[I + 1], F);
			OutD = FMath::Lerp(D[I], D[I + 1], F).GetSafeNormal(UE_SMALL_NUMBER, D[I + 1]);
			OutR = FMath::Lerp(R[I], R[I + 1], F);
			OutBend = FMath::Lerp(Bend[I], Bend[I + 1], F);
			OutPhase = FMath::Lerp(Phase[I], Phase[I + 1], F);
		}
	};

	class FGrower
	{
	public:
		FGrower(const FSpec& InSpec, int32 InSeed, double InHeight, const FVector& InCrownCentre, const FVector& InCrownRadii,
				const FBox2D& InKeep = FBox2D(ForceInit), double InKeepFrom = 0.0)
			: Spec(InSpec), Rng(InSeed), TreeH(InHeight), CrownCentre(InCrownCentre), CrownRadii(InCrownRadii), Keep(InKeep), KeepFrom(InKeepFrom)
		{
			// The wood stays a leaf's length inside the box.
			KeepWood = Shrunk(Keep, 0.12);
			BasePhase = FMath::Frac(InSeed * 0.6180339887);
		}

		void Run()
		{
			for (int32 s = 0; s < Spec.Stems; ++s)
			{
				const double LeanRad = FMath::DegreesToRadians(Spec.Lean + (Spec.Stems > 1 ? Spec.StemSpread : 0.0));
				const double Az = Spec.Stems == 1 ? Rng.FRandRange(0.0, MN::Tau) : MN::Tau * s / Spec.Stems + Rng.FRandRange(-0.4, 0.4);
				const FVector StemDir(FMath::Sin(LeanRad) * FMath::Cos(Az), FMath::Sin(LeanRad) * FMath::Sin(Az), FMath::Cos(LeanRad));
				const FVector Offset = Spec.Stems > 1 ? FVector(FMath::Cos(Az), FMath::Sin(Az), 0.0) * 0.07 : FVector::ZeroVector;
				Grow(0, Offset + FVector(0, 0, -0.08), StemDir, TreeH * Spec.TrunkFrac, Spec.TrunkRadius / FMath::Sqrt(double(Spec.Stems)), 0.0, BasePhase);
			}
		}

		const FSpec& Spec;
		FRandomStream Rng;
		double TreeH;
		FVector CrownCentre;
		FVector CrownRadii;
		double BasePhase = 0;
		TArray<FBranch> Branches;
		FBox2D Keep;          // plan box the plant stays inside above KeepFrom (invalid: no limit)
		double KeepFrom = 0;
		FBox2D KeepWood;      // Keep less a leaf's length

		/** Whether a point of the plant is where Keep allows it. */
		bool Allowed(const FVector& P, const FBox2D& Box) const { return P.Z <= KeepFrom || InPlan(Box, P); }

	private:
		bool bCrownPlaced = false;

		void Grow(int32 Level, const FVector& Start, const FVector& Dir, double Length, double Radius, double Bend0, double Phase0)
		{
			const bool bTrunk = Level == 0;
			const FLevel* Own = bTrunk ? nullptr : &Spec.Levels[Level - 1];
			int32 Segs = FMath::Max(1, bTrunk ? Spec.TrunkSegments : Own->Segments);
			const double Curve = bTrunk ? 0.0 : Own->Curve;
			const double Wander = bTrunk ? Spec.Wander : Own->CurveVar;
			const double Gravity = bTrunk ? 0.0 : Own->Gravity;
			const double Tropism = bTrunk ? 0.0 : Own->Up;
			const double Zig = bTrunk ? Spec.TrunkZigzag : Own->Zigzag;
			const double Gain = bTrunk ? 0.0 : Own->BendGain;
			const double TipFraction = bTrunk ? Spec.TrunkTip : 0.08;
			const double Drift = bTrunk ? 0.0 : Rng.FRandRange(-0.05, 0.05);

			FBranch B;
			B.Level = Level;
			B.Length = Length;
			B.P.Add(Start);
			B.D.Add(Dir);
			FVector Cur = Dir;
			double ZigSign = Rng.FRand() < 0.5 ? 1.0 : -1.0;
			double ZigAz = Rng.FRandRange(0.0, MN::Tau);
			for (int32 i = 1; i <= Segs; ++i)
			{
				const double Along = double(i) / Segs;
				FVector U, W;
				MN::Basis(Cur, U, W);
				FVector Axis = FVector::CrossProduct(Cur, FVector::UpVector);
				if (Axis.SizeSquared() < 1e-8) { Axis = U; }
				Cur = MN::RotateAbout(Cur, Axis, FMath::DegreesToRadians(Curve / Segs));
				const double WanderAz = Rng.FRandRange(0.0, MN::Tau);
				Cur = MN::RotateAbout(Cur, U * FMath::Cos(WanderAz) + W * FMath::Sin(WanderAz), FMath::DegreesToRadians(Rng.FRandRange(-Wander, Wander)));
				if (Gravity != 0) { Cur = (Cur - FVector::UpVector * (FMath::DegreesToRadians(Gravity) / Segs * (0.5 + Along))).GetSafeNormal(); }
				if (Tropism != 0) { Cur = (Cur + FVector::UpVector * (FMath::DegreesToRadians(Tropism) / Segs)).GetSafeNormal(); }
				if (Zig != 0)
				{
					ZigAz += 2.2;
					MN::Basis(Cur, U, W);
					Cur = MN::RotateAbout(Cur, U * FMath::Cos(ZigAz) + W * FMath::Sin(ZigAz), FMath::DegreesToRadians(Zig) * ZigSign);
					ZigSign = -ZigSign;
				}
				const FVector Next = B.P[i - 1] + Cur * (Length / Segs);
				B.P.Add(Next);
				B.D.Add(Cur);
			}
			if (Keep.bIsValid)
			{
				// A limb that bends out of the keep box ends where it would leave it.
				int32 Kept = Segs;
				for (int32 i = 1; i <= Segs; ++i) { if (!Allowed(B.P[i], KeepWood)) { Kept = i - 1; break; } }
				if (Kept < Segs)
				{
					if (Kept == 0 && !bTrunk) { return; }
					Kept = FMath::Max(1, Kept);
					B.P.SetNum(Kept + 1);
					B.D.SetNum(Kept + 1);
					Length *= double(Kept) / Segs;
					B.Length = Length;
					Segs = Kept;
				}
			}
			for (int32 i = 0; i <= Segs; ++i)
			{
				const double Along = double(i) / Segs;
				B.R.Add(Radius * (1.0 - (1.0 - TipFraction) * FMath::Pow(Along, 0.85)));
				if (bTrunk)
				{
					const double Z = FMath::Max(0.0, B.P[i].Z) / FMath::Max(TreeH, 1.0);
					B.Bend.Add(0.3 * Z * Z);
				}
				else
				{
					B.Bend.Add(FMath::Min(1.6, Bend0 + Gain * Along * FMath::Sqrt(Length)));
				}
				B.Phase.Add(Phase0 + Drift * Along);
			}
			if (bTrunk && !bCrownPlaced)
			{
				// The crown sits over where the trunk has leant to.
				CrownCentre.X = B.P.Last().X * Spec.Follow;
				CrownCentre.Y = B.P.Last().Y * Spec.Follow;
				bCrownPlaced = true;
			}
			Branches.Add(B);

			if (Level >= Spec.Levels.Num()) { return; }
			const FLevel& Kids = Spec.Levels[Level];
			const int32 Count = bTrunk ? Kids.Count : FMath::Max(0, FMath::RoundToInt(Kids.Density * Length + Rng.FRandRange(-0.5, 0.5)));
			const double Az0 = Rng.FRandRange(0.0, MN::Tau);
			double Az = Az0;
			for (int32 k = 0; k < Count; ++k)
			{
				const double T = FMath::Clamp(Kids.Start + (Kids.End - Kids.Start) * (k + 0.5 + Rng.FRandRange(-0.35, 0.35)) / FMath::Max(1, Count), 0.0, 0.999);
				FVector P, D;
				double PR, PB, PP;
				B.Sample(T, P, D, PR, PB, PP);
				const double Rel = (T - Kids.Start) / FMath::Max(1e-3, Kids.End - Kids.Start);
				double Angle = bTrunk ? FMath::Lerp(Kids.AngleLow, Kids.Angle, Rel) : Kids.Angle;
				Angle += Rng.FRandRange(-Kids.AngleVar, Kids.AngleVar);
				if (bTrunk && Spec.bEven) { Az = Az0 + MN::Tau * k / FMath::Max(1, Count) + FMath::DegreesToRadians(Rng.FRandRange(-14.0, 14.0)); }
				else { Az += FMath::DegreesToRadians(Kids.Phyllo + Rng.FRandRange(-12.0, 12.0)); }
				FVector U, W;
				MN::Basis(D, U, W);
				const FVector ChildDir = MN::RotateAbout(D, U * FMath::Cos(Az) + W * FMath::Sin(Az), FMath::DegreesToRadians(Angle)).GetSafeNormal();
				const double Reach = MN::RayToEllipsoid(P, ChildDir, CrownCentre, CrownRadii);
				double ChildLength;
				if (bTrunk)
				{
					ChildLength = (Reach > 0 ? Reach : 0.3) * Kids.Length * (1.0 + Rng.FRandRange(-Kids.LengthVar, Kids.LengthVar));
				}
				else
				{
					ChildLength = Length * Kids.Length * (1.0 - Kids.LengthTaper * T) * (1.0 + Rng.FRandRange(-Kids.LengthVar, Kids.LengthVar));
					const double Limit = FMath::Max((Reach > 0 ? Reach : 0.0) * 1.1 + 0.15 * (4 - Level), 0.08);
					ChildLength = FMath::Min(ChildLength, Limit);
				}
				if (Keep.bIsValid && P.Z + ChildDir.Z * ChildLength > KeepFrom)
				{
					// No further than the keep box's side in plan (the tip lies where the box allows it).
					const FVector2D Flat(ChildDir.X, ChildDir.Y);
					const double Across = Flat.Size();
					if (Across > 1e-3)
					{
						const double Exit = RayExitBox2D(FVector2D(P.X, P.Y), Flat / Across, KeepWood) / Across;
						ChildLength = FMath::Min(ChildLength, Exit - 0.05);
					}
				}
				if (ChildLength < 0.04) { continue; }
				double ChildRadius = PR * Kids.Radius * (0.55 + 0.45 * FMath::Sqrt(FMath::Min(1.0, ChildLength / FMath::Max(Length, 1e-3))));
				ChildRadius = FMath::Max(FMath::Min(ChildRadius, PR * 0.85), Spec.MinRadius);
				Grow(Level + 1, P, ChildDir, ChildLength, ChildRadius, PB, PP);
			}
		}
	};

	/** Darker inside the crown, where little sky reaches. */
	double CrownOcclusion(const FVector& P, const FVector& Centre, const FVector& Radii, double Floor)
	{
		const double E = ((P - Centre) / Radii).Size();
		return FMath::Clamp(Floor + (1.0 - Floor) * E * E, Floor, 1.0);
	}

	/** Ring direction: the mean of the segments either side. */
	FVector RingDir(const FBranch& B, int32 i)
	{
		const int32 Last = B.D.Num() - 1;
		const FVector A = B.D[FMath::Clamp(i, 0, Last)];
		const FVector C = B.D[FMath::Clamp(i + 1, 0, Last)];
		return (A + C).GetSafeNormal(UE_SMALL_NUMBER, A);
	}

	struct FOutput
	{
		FNatureMesh Bark;
		FNatureMesh Leaves;
		FNatureMesh Flowers;
		FNatureMesh Fruit;
		const TCHAR* BarkMaterial = TEXT("MI_Bark_Birch");
		const TCHAR* LeafMaterial = TEXT("MI_Leaf_Birch");
		const TCHAR* FlowerMaterial = TEXT("MI_Blossom_Cherry");
		double CollisionRadius = 0.2;
		FBox2D Footprint = FBox2D(ForceInit);
	};

	void GrowTree(EMuseeTreeSpecies Kind, int32 Variant, EMuseeNatureSeason Now, int32 Term, int32 Seed, double Height, double CrownZ,
				  const FVector2D& Radii, double Density, const FBox2D& Keep, double KeepFrom, FOutput& Out)
	{
		const bool bApple = Variant == 1;
		FSpec Spec;
		switch (Kind)
		{
		case EMuseeTreeSpecies::OrchardFruit: Spec = OrchardSpec(bApple); break;
		case EMuseeTreeSpecies::ChinesePlum: Spec = PlumSpec(); break;
		case EMuseeTreeSpecies::Osmanthus: Spec = OsmanthusSpec(); break;
		default: Spec = BirchSpec(); break;
		}
		const double Scale = Height > 0 ? Height / Spec.Height : 1.0;
		const double TreeH = Spec.Height * Scale;
		const FVector CrownCentre(0, 0, CrownZ > 0 ? CrownZ : Spec.CrownCentre * Scale);
		const FVector CrownRadii(Radii.X > 0 ? Radii.X : Spec.CrownH * Scale, Radii.X > 0 ? Radii.X : Spec.CrownH * Scale,
								 Radii.Y > 0 ? Radii.Y : Spec.CrownV * Scale);
		Spec.TrunkRadius *= FMath::Sqrt(Scale);

		FGrower G(Spec, Seed * 7919 + 101, TreeH, CrownCentre, CrownRadii, Keep, KeepFrom);
		G.Run();
		const FLook Look = LookFor(Kind, bApple, Now, Term);
		FRandomStream& Rng = G.Rng;
		const int32 MaxLevel = FMath::Max(1, Spec.Levels.Num());

		Out.BarkMaterial = Spec.Bark;
		Out.LeafMaterial = Spec.LeafMaterial;
		Out.FlowerMaterial = Look.FlowerMaterial;
		Out.CollisionRadius = Spec.Stems > 1 ? 0.28 : Spec.TrunkRadius * 1.15 + 0.08;

		// Bark.
		FNatureTubeShape TrunkShape;
		TrunkShape.Flare = Spec.Flare;
		TrunkShape.Lobes = Spec.FlareLobes;
		TrunkShape.Gnarl = Spec.Gnarl;
		TrunkShape.Seed = Seed;
		FNatureTubeShape LimbShape;
		LimbShape.Gnarl = Spec.Gnarl * 0.6;
		LimbShape.Seed = Seed + 7;
		for (const FBranch& B : G.Branches)
		{
			const int32 Sides = B.Level == 0 ? Spec.TrunkSides : Spec.Levels[B.Level - 1].Sides;
			TArray<FNatureTubeRing> Rings;
			Rings.Reserve(B.P.Num());
			for (int32 i = 0; i < B.P.Num(); ++i)
			{
				FNatureTubeRing& Ring = Rings.AddDefaulted_GetRef();
				Ring.Centre = B.P[i];
				Ring.Dir = RingDir(B, i);
				// A collar where a branch leaves its parent.
				Ring.Radius = B.R[i] * (i == 0 && B.Level > 0 ? 1.15 : 1.0);
				Ring.Wind.Bend = B.Bend[i];
				Ring.Wind.Phase = B.Phase[i];
				Ring.Wind.Height = B.P[i].Z;
				Ring.Wind.Twig = double(B.Level) / MaxLevel;
				Ring.Occlusion = B.Level == 0 ? CrownOcclusion(B.P[i], CrownCentre, CrownRadii, 0.7) : CrownOcclusion(B.P[i], CrownCentre, CrownRadii, 0.55);
			}
			const int32 UseSides = B.R[0] < 0.0045 ? 3 : Sides;
			const FNatureTubeShape& Shape = B.Level == 0 ? TrunkShape : (B.Level == 1 ? LimbShape : FNatureTubeShape());
			MN::AddTube(Out.Bark, Rings, UseSides, 0.0, true, Shape);
		}

		// Leaves along the twigs, turned to the light.
		if (Look.LeafAmount > 0 && Look.LeafColours.Num() > 0)
		{
			double Total = 0;
			for (const FBranch& B : G.Branches) { if (B.Level >= Spec.LeafLevel) { Total += B.Length * (1.0 - Spec.LeafStart); } }
			const double Target = Spec.LeafCount * Look.LeafAmount * Density;
			const double PerMetre = Target / FMath::Max(Total, 1e-3);
			Out.Leaves.Reserve(int32(Target * 4.4), int32(Target * 2.2));
			for (const FBranch& B : G.Branches)
			{
				if (B.Level < Spec.LeafLevel) { continue; }
				const int32 N = FMath::FloorToInt(PerMetre * B.Length * (1.0 - Spec.LeafStart) + Rng.FRand());
				for (int32 k = 0; k < N; ++k)
				{
					const double T = Spec.LeafStart + (1.0 - Spec.LeafStart) * (k + Rng.FRand()) / N;
					FVector P, D;
					double PR, PB, PP;
					B.Sample(T, P, D, PR, PB, PP);
					FVector U, W;
					MN::Basis(D, U, W);
					const double A = k * 2.39996 + Rng.FRandRange(-0.3, 0.3);
					const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
					FVector LeafDir = MN::RotateAbout(D, FVector::CrossProduct(D, Radial), FMath::DegreesToRadians(Spec.LeafAngle + Rng.FRandRange(-15.0, 15.0)));
					LeafDir = (LeafDir - FVector::UpVector * (Spec.LeafDroop * Rng.FRandRange(0.6, 1.4))).GetSafeNormal(UE_SMALL_NUMBER, Radial);
					const FVector Outward = ((P - CrownCentre) / CrownRadii).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
					const FVector Want = (Outward * 0.55 + FVector::UpVector * 0.75 + Rng.GetUnitVector() * 0.45).GetSafeNormal();
					FVector Normal = Want - LeafDir * FVector::DotProduct(Want, LeafDir);
					if (Normal.SizeSquared() < 1e-6) { Normal = FVector::CrossProduct(LeafDir, U); }
					Normal.Normalize();
					const double Len = Spec.LeafLength * Look.LeafScale * Rng.FRandRange(0.75, 1.25);
					const double Wid = Len * Spec.LeafAspect * Rng.FRandRange(0.9, 1.1);
					FNatureWind Base;
					Base.Bend = PB;
					Base.Phase = PP;
					Base.FlutterPhase = Rng.FRand();
					Base.Height = P.Z;
					Base.Twig = 1.0;
					FNatureWind Tip = Base;
					Tip.Flutter = 1.0;
					Tip.Bend = PB + 0.04;
					const FLinearColor Colour = MN::Vary(Look.LeafColours[Rng.RandRange(0, Look.LeafColours.Num() - 1)], Rng, 5.0, 0.1, 0.14);
					if (!G.Allowed(P + Radial * PR + LeafDir * Len, G.Keep)) { continue; }
					MN::AddCard(Out.Leaves, P + Radial * PR, LeafDir, Normal, Len, Wid, Spec.LeafFold, Spec.LeafCurl, Spec.LeafRows, Base, Tip,
								Colour, CrownOcclusion(P, CrownCentre, CrownRadii, 0.45), Outward * 0.5);
				}
			}
		}

		// Blossom (on the wood, in clusters).
		if (Look.FlowerCount > 0)
		{
			double Total = 0;
			for (const FBranch& B : G.Branches) { if (B.Level >= Look.FlowerLevel) { Total += B.Length; } }
			const double Clusters = Look.FlowerCount * Density / (0.5 * (Look.ClusterMin + Look.ClusterMax));
			const double PerMetre = Clusters / FMath::Max(Total, 1e-3);
			for (const FBranch& B : G.Branches)
			{
				if (B.Level < Look.FlowerLevel) { continue; }
				const int32 N = FMath::FloorToInt(PerMetre * B.Length + Rng.FRand());
				for (int32 k = 0; k < N; ++k)
				{
					const double T = 0.08 + 0.92 * (k + Rng.FRand()) / N;
					FVector P, D;
					double PR, PB, PP;
					B.Sample(T, P, D, PR, PB, PP);
					FVector U, W;
					MN::Basis(D, U, W);
					const FVector Outward = ((P - CrownCentre) / CrownRadii).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
					const int32 Cluster = Rng.RandRange(Look.ClusterMin, Look.ClusterMax);
					for (int32 j = 0; j < Cluster; ++j)
					{
						const double A = Rng.FRandRange(0.0, MN::Tau);
						const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
						const double Stalk = Look.Pedicel * Rng.FRandRange(0.5, 1.3);
						const FVector Centre = P + Radial * (PR + Stalk) + D * Rng.FRandRange(-0.01, 0.01)
							- FVector::UpVector * (Look.FacingUp < 0 ? Stalk * 0.6 : 0.0);
						const FVector Facing = (Radial * 0.8 + FVector::UpVector * Look.FacingUp + Outward * 0.4 + Rng.GetUnitVector() * 0.35)
							.GetSafeNormal(UE_SMALL_NUMBER, Radial);
						FNatureWind Wind;
						Wind.Bend = PB + 0.02;
						Wind.Flutter = 0.5;
						Wind.Phase = PP;
						Wind.FlutterPhase = Rng.FRand();
						Wind.Height = P.Z;
						Wind.Twig = 1.0;
						const FLinearColor Colour = MN::Vary(Rng.FRand() < Look.FlowerBShare ? Look.FlowerB : Look.FlowerA, Rng, 3.0, 0.08, 0.04);
						if (!G.Allowed(Centre, G.Keep)) { continue; }
						MN::AddFlowerCard(Out.Flowers, Centre, Facing, Rng.FRandRange(0.0, MN::Tau), Look.FlowerSize * Rng.FRandRange(0.8, 1.15),
										  Look.Cup, Wind, Colour, CrownOcclusion(P, CrownCentre, CrownRadii, 0.6));
					}
				}
			}
		}

		// Fruit, hanging from the twigs on short stalks.
		if (Look.FruitCount > 0 && Look.FruitColours.Num() > 0)
		{
			TArray<int32> Bearers;
			TArray<double> Cumulative;
			double Total = 0;
			for (int32 i = 0; i < G.Branches.Num(); ++i)
			{
				if (G.Branches[i].Level >= FMath::Max(2, MaxLevel - 1))
				{
					Total += G.Branches[i].Length;
					Bearers.Add(i);
					Cumulative.Add(Total);
				}
			}
			const FLinearColor StalkColour = MN::Srgb(0x5E6A32);
			const int32 Count = Bearers.Num() > 0 ? FMath::RoundToInt(Look.FruitCount * Density) : 0;
			for (int32 f = 0; f < Count; ++f)
			{
				const double Pick = Rng.FRandRange(0.0, Total);
				int32 Found = Algo::LowerBound(Cumulative, Pick);
				Found = FMath::Clamp(Found, 0, Bearers.Num() - 1);
				const FBranch& B = G.Branches[Bearers[Found]];
				FVector P, D;
				double PR, PB, PP;
				B.Sample(Rng.FRandRange(0.2, 0.95), P, D, PR, PB, PP);
				FVector U, W;
				MN::Basis(D, U, W);
				const double A = Rng.FRandRange(0.0, MN::Tau);
				const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
				const FLinearColor Colour = MN::Vary(Look.FruitColours[Rng.RandRange(0, Look.FruitColours.Num() - 1)], Rng, 4.0, 0.08, 0.12);
				const int32 Hanging = Look.bPairs ? 2 : 1;
				for (int32 h = 0; h < Hanging; ++h)
				{
					const FVector Splay = Look.bPairs ? U * (h == 0 ? 0.35 : -0.35) : FVector::ZeroVector;
					const FVector StalkDir = (-FVector::UpVector + Radial * 0.3 + Splay + Rng.GetUnitVector() * 0.12).GetSafeNormal();
					const FVector From = P + Radial * PR;
					const FVector To = From + StalkDir * Look.Stalk;
					FNatureWind Wind;
					Wind.Bend = PB + 0.05;
					Wind.Phase = PP;
					Wind.Height = P.Z;
					Wind.Twig = 1.0;
					TArray<FNatureTubeRing> Rings;
					for (int32 r = 0; r < 2; ++r)
					{
						FNatureTubeRing& Ring = Rings.AddDefaulted_GetRef();
						Ring.Centre = r == 0 ? From : To;
						Ring.Dir = StalkDir;
						Ring.Radius = Look.bPairs ? 0.0008 : 0.0016;
						Ring.Wind = Wind;
						Ring.Colour = StalkColour;
					}
					MN::AddTube(Out.Fruit, Rings, 3, 0.0, false);
					const double FR = Look.FruitRadius * Rng.FRandRange(0.85, 1.15);
					const bool bSmall = FR < 0.016;
					MN::AddEllipsoid(Out.Fruit, To + StalkDir * (FR * Look.FruitElong * 0.85), -StalkDir, FR, FR * Look.FruitElong, bSmall ? 7 : 10,
									 bSmall ? 5 : 7, Wind, Colour, CrownOcclusion(P, CrownCentre, CrownRadii, 0.6));
				}
			}
		}
	}

	/** A clump of bamboo: segmented culms with node rings, paired branches from 2 m, and leaf sprays. */
	void GrowBamboo(const TArray<FVector2D>& InCulms, EMuseeNatureSeason Now, int32 Term, int32 Seed, double MeanHeight, double Density,
					const FBox2D& Keep, double KeepFrom, FOutput& Out)
	{
		// Keep: the leaves stay inside the box, the wood a leaf's length (and the wind's sway) inside it.
		const FBox2D KeepWood = Shrunk(Keep, 0.2);
		auto Allowed = [&](const FVector& P, const FBox2D& Box) { return P.Z <= KeepFrom || InPlan(Box, P); };
		Out.BarkMaterial = TEXT("MI_Bark_Bamboo");
		Out.LeafMaterial = TEXT("MI_Leaf_Bamboo");
		FRandomStream Rng(Seed * 7919 + 88);
		TArray<FVector2D> Points = InCulms;
		if (Points.Num() == 0)
		{
			for (int32 i = 0; i < 9; ++i)
			{
				const double A = MN::Tau * i / 9 + Rng.FRandRange(-0.3, 0.3);
				const double Rad = Rng.FRandRange(0.05, 0.45);
				Points.Add(FVector2D(FMath::Cos(A) * Rad, FMath::Sin(A) * Rad));
			}
		}
		FVector2D Centroid = FVector2D::ZeroVector;
		for (const FVector2D& Pt : Points) { Centroid += Pt; Out.Footprint += Pt; }
		Centroid /= Points.Num();

		const bool bSnow = Term >= 19 && Term <= 23;
		const TArray<FLinearColor> LeafColours = bSnow ? Palette({0xB9C4AE, 0x9FB08E, 0x7E9A62, 0xC8D0C0})
													   : Palette({0x5E8A3E, 0x6B9444, 0x507A36, 0x5A8640, 0x648C40});
		const FLinearColor Yellowed = MN::Srgb(0xA8A450);
		const FLinearColor Powder = MN::Srgb(0xF4F4EA);
		const FLinearColor NodeTint = MN::Srgb(0xC8C8A8);
		const double HeightScale = MeanHeight > 0 ? MeanHeight / 5.4 : 1.0;

		for (int32 c = 0; c < Points.Num(); ++c)
		{
			const FVector2D Base2 = Points[c];
			const double H = Rng.FRandRange(4.7, 6.1) * HeightScale;
			const double R0 = Rng.FRandRange(0.022, 0.032);
			const double Age = Rng.FRand();
			const FLinearColor Culm = MN::Mix(FLinearColor::White, MN::Srgb(0xF0E6B8), Age * 0.6);
			FVector2D Away = Base2 - Centroid;
			if (Away.SizeSquared() < 1e-4) { const double A = Rng.FRandRange(0.0, MN::Tau); Away = FVector2D(FMath::Cos(A), FMath::Sin(A)); }
			Away.Normalize();
			Away = (Away + FVector2D(Rng.FRandRange(-0.4, 0.4), Rng.FRandRange(-0.4, 0.4))).GetSafeNormal();
			double Lean = FMath::Tan(FMath::DegreesToRadians(Rng.FRandRange(3.0, 9.0) + 6.0 * FMath::Min(1.0, (Base2 - Centroid).Size())));
			double Droop = Rng.FRandRange(0.25, 0.7);
			if (KeepWood.bIsValid)
			{
				// In plan the culm runs straight from its foot to its tip: lean away from a side it would cross, and lean
				// less if the tip would still pass it.
				const double Reach = H * Lean + Droop;
				const FVector2D Tip = Base2 + Away * Reach;
				if ((Tip.X > KeepWood.Max.X && Away.X > 0) || (Tip.X < KeepWood.Min.X && Away.X < 0)) { Away.X = -Away.X; }
				if ((Tip.Y > KeepWood.Max.Y && Away.Y > 0) || (Tip.Y < KeepWood.Min.Y && Away.Y < 0)) { Away.Y = -Away.Y; }
				const double Room = RayExitBox2D(Base2, Away, KeepWood);
				if (Reach > Room)
				{
					const double S = FMath::Clamp(Room / Reach, 0.0, 1.0);
					Lean *= S;
					Droop *= S;
				}
			}
			const double CulmPhase = Rng.FRand();
			auto CulmAt = [&](double Z) -> FVector
			{
				const double S = Z / H;
				const double Out2 = H * Lean * S * S + Droop * FMath::Pow(S, 6.0);
				return FVector(Base2.X + Away.X * Out2, Base2.Y + Away.Y * Out2, Z - 0.15 * Droop * FMath::Pow(S, 8.0));
			};
			auto CulmDir = [&](double Z) -> FVector { return (CulmAt(FMath::Min(H, Z + 0.02)) - CulmAt(FMath::Max(0.0, Z - 0.02))).GetSafeNormal(); };
			auto CulmRadius = [&](double Z) { return FMath::Max(0.0045, R0 * (1.0 - 0.62 * FMath::Pow(FMath::Clamp(Z / H, 0.0, 1.0), 1.3))); };
			auto CulmWind = [&](double Z)
			{
				FNatureWind Wind;
				Wind.Bend = 1.1 * FMath::Square(Z / H);
				Wind.Phase = CulmPhase + 0.05 * Z / H;
				Wind.Height = Z;
				return Wind;
			};

			// Nodes: short internodes at the base, longest in the middle.
			TArray<double> Nodes;
			double NodeZ = -0.05;
			while (NodeZ < H - 0.12)
			{
				Nodes.Add(NodeZ);
				NodeZ += (0.1 + 0.24 * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(NodeZ / H * 1.15, 0.0, 1.0)), 0.7)) * Rng.FRandRange(0.9, 1.1);
			}
			TArray<FNatureTubeRing> Rings;
			auto AddRing = [&](double At, double RadiusScale, const FLinearColor& Colour)
			{
				FNatureTubeRing& Ring = Rings.AddDefaulted_GetRef();
				Ring.Centre = CulmAt(At);
				Ring.Dir = CulmDir(At);
				Ring.Radius = CulmRadius(At) * RadiusScale;
				Ring.Wind = CulmWind(At);
				Ring.Colour = Colour;
				Ring.Occlusion = FMath::Clamp(0.55 + 0.45 * At / H, 0.55, 1.0);
			};
			for (int32 n = 0; n < Nodes.Num(); ++n)
			{
				const double NZ = Nodes[n];
				if (n > 0)
				{
					AddRing(NZ - 0.045, 1.0, Culm);
					AddRing(NZ - 0.012, 1.0, MN::Mix(Culm, Powder, 0.55));   // the waxy white ring below the node
					AddRing(NZ, 1.07, MN::Mix(Culm, NodeTint, 0.5));        // the node's ridge
				}
				AddRing(NZ + 0.012, 1.0, Culm);
			}
			AddRing(H, 0.35, Culm);
			MN::AddTube(Out.Bark, Rings, 7, 0.0, true);

			// Branches: two at each node from 2 m, alternating sides; leaf sprays at their tips.
			double SideAz = Rng.FRandRange(0.0, MN::Tau);
			auto AddSpray = [&](const FVector& At, const FVector& Dir, const FNatureWind& Wind, int32 Leaves)
			{
				FVector U, W;
				MN::Basis(Dir, U, W);
				for (int32 l = 0; l < Leaves; ++l)
				{
					const double Fan = (Leaves > 1 ? double(l) / (Leaves - 1) - 0.5 : 0.0) * 2.2 + Rng.FRandRange(-0.2, 0.2);
					const FVector Side = U * FMath::Cos(Fan) + W * FMath::Sin(Fan);
					FVector LeafDir = (Dir * 0.5 + Side * 0.8 - FVector::UpVector * Rng.FRandRange(0.4, 0.9)).GetSafeNormal();
					FVector Normal = FVector::UpVector - LeafDir * FVector::DotProduct(FVector::UpVector, LeafDir);
					if (Normal.SizeSquared() < 1e-6) { Normal = Side; }
					Normal = (Normal.GetSafeNormal() + Rng.GetUnitVector() * 0.25).GetSafeNormal();
					Normal = (Normal - LeafDir * FVector::DotProduct(Normal, LeafDir)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
					const double Len = Rng.FRandRange(0.10, 0.17);
					if (!Allowed(At, Keep) || !Allowed(At + LeafDir * Len, Keep)) { continue; }
					FNatureWind Tip = Wind;
					Tip.Flutter = 1.0;
					Tip.FlutterPhase = Rng.FRand();
					FNatureWind Stem = Tip;
					Stem.Flutter = 0.0;
					const FLinearColor Colour = MN::Vary(Rng.FRand() < 0.05 ? Yellowed : LeafColours[Rng.RandRange(0, LeafColours.Num() - 1)], Rng, 4.0, 0.1, 0.12);
					MN::AddCard(Out.Leaves, At, LeafDir, Normal, Len, Len * Rng.FRandRange(0.12, 0.15), 0.3, 0.22, 2, Stem, Tip, Colour,
								FMath::Clamp(0.6 + 0.4 * Wind.Height / H, 0.6, 1.0));
				}
			};
			// Where a branch's rings would lie (AddBranch's path), to keep it inside the keep box.
			auto BranchFits = [&](const FVector& From, const FVector& Dir0, double Length)
			{
				if (!KeepWood.bIsValid) { return true; }
				FVector P = From;
				FVector Dir = Dir0;
				for (int32 s = 0; s < 3; ++s)
				{
					Dir = (Dir - FVector::UpVector * 0.12).GetSafeNormal();
					P += Dir * (Length / 3);
					if (!Allowed(P, KeepWood)) { return false; }
				}
				return true;
			};
			// Fit a branch: as grown, else turned the other way round the culm (in plan), else shorter; false: none.
			auto FitBranch = [&](const FVector& From, FVector& Dir, double& Length)
			{
				if (BranchFits(From, Dir, Length)) { return true; }
				const FVector Turned(-Dir.X, -Dir.Y, Dir.Z);
				if (BranchFits(From, Turned, Length)) { Dir = Turned; return true; }
				for (double L = Length * 0.7; L >= 0.12; L *= 0.7)
				{
					if (BranchFits(From, Dir, L)) { Length = L; return true; }
					if (BranchFits(From, Turned, L)) { Dir = Turned; Length = L; return true; }
				}
				return false;
			};

			auto AddBranch = [&](const FVector& From, const FVector& Dir0, double Length, double Radius, double BaseBend, int32 Depth)
			{
				TArray<FNatureTubeRing> BranchRings;
				FVector P = From;
				FVector Dir = Dir0;
				const int32 Segs = 3;
				for (int32 s = 0; s <= Segs; ++s)
				{
					const double T = double(s) / Segs;
					FNatureTubeRing& Ring = BranchRings.AddDefaulted_GetRef();
					Ring.Centre = P;
					Ring.Dir = Dir;
					Ring.Radius = FMath::Max(0.0012, Radius * (1.0 - 0.7 * T));
					Ring.Wind.Bend = BaseBend + 0.25 * T * Length;
					Ring.Wind.Phase = CulmPhase;
					Ring.Wind.Height = P.Z;
					Ring.Wind.Twig = 1.0;
					Ring.Colour = Culm;
					if (s < Segs)
					{
						Dir = (Dir - FVector::UpVector * 0.12).GetSafeNormal();
						P += Dir * (Length / Segs);
					}
				}
				MN::AddTube(Out.Bark, BranchRings, 3, 0.0, true);
				const FNatureWind TipWind = BranchRings.Last().Wind;
				AddSpray(P, Dir, TipWind, FMath::RoundToInt(Rng.FRandRange(3.0, 6.0) * Density));
				// A leaf or two along the way.
				if (Depth == 0 && Rng.FRand() < 0.7 * Density)
				{
					const FNatureTubeRing& Mid = BranchRings[1];
					AddSpray(Mid.Centre, (Mid.Dir + Rng.GetUnitVector() * 0.5).GetSafeNormal(), Mid.Wind, Rng.RandRange(1, 2));
				}
				return BranchRings;
			};
			const double BranchFrom = FMath::Max(2.0, 0.35 * H);
			for (int32 n = 1; n < Nodes.Num(); ++n)
			{
				const double NZ = Nodes[n];
				if (NZ < BranchFrom || NZ > H - 0.1) { continue; }
				SideAz += UE_DOUBLE_PI + Rng.FRandRange(-0.35, 0.35);
				const double Rel = (NZ - BranchFrom) / FMath::Max(0.1, H - BranchFrom);
				const double Length = (0.3 + 0.55 * FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(0.25 + 0.75 * Rel, 0.0, 1.0))) * Rng.FRandRange(0.8, 1.2) * (1.0 - 0.5 * Rel);
				const FVector Axis = CulmDir(NZ);
				FVector U, W;
				MN::Basis(Axis, U, W);
				for (int32 b = 0; b < 2; ++b)
				{
					const double A = SideAz + (b == 0 ? 0.0 : Rng.FRandRange(0.5, 0.9));
					const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
					FVector Dir = MN::RotateAbout(Axis, FVector::CrossProduct(Axis, Radial), FMath::DegreesToRadians(Rng.FRandRange(45.0, 65.0)));
					double BLength = Length * (b == 0 ? 1.0 : 0.6);
					const FNatureWind AtNode = CulmWind(NZ);
					const FVector BranchFoot = CulmAt(NZ) + Radial * CulmRadius(NZ) * 0.8;
					if (!FitBranch(BranchFoot, Dir, BLength)) { continue; }
					const TArray<FNatureTubeRing> Main = AddBranch(BranchFoot, Dir, BLength, 0.0045, AtNode.Bend, 0);
					// Branchlets.
					const int32 Subs = Rng.RandRange(1, 2);
					for (int32 s = 0; s < Subs; ++s)
					{
						const FNatureTubeRing& From = Main[FMath::Clamp(1 + s, 1, Main.Num() - 1)];
						FVector SubDir = (From.Dir + Rng.GetUnitVector() * 0.8 + FVector::UpVector * 0.2).GetSafeNormal();
						double SubLength = BLength * Rng.FRandRange(0.3, 0.5);
						if (!FitBranch(From.Centre, SubDir, SubLength)) { continue; }
						AddBranch(From.Centre, SubDir, SubLength, 0.0025, From.Wind.Bend, 1);
					}
				}
			}
			// The culm's own tip carries a spray.
			AddSpray(CulmAt(H), CulmDir(H), CulmWind(H), FMath::RoundToInt(6 * Density));
		}
		Out.Footprint = Out.Footprint.ExpandBy(0.15);
	}
}

AMuseeTree::AMuseeTree()
{
	TrunkCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TrunkCollision"));
	TrunkCollision->SetupAttachment(RootComponent);
	TrunkCollision->SetMobility(EComponentMobility::Static);
	TrunkCollision->SetCapsuleSize(20.f, 120.f);
	TrunkCollision->SetRelativeLocation(FVector(0, 0, 120));
	TrunkCollision->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	TrunkCollision->SetHiddenInGame(true);

	ClumpCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ClumpCollision"));
	ClumpCollision->SetupAttachment(RootComponent);
	ClumpCollision->SetMobility(EComponentMobility::Static);
	ClumpCollision->SetBoxExtent(FVector(50, 50, 150));
	ClumpCollision->SetRelativeLocation(FVector(0, 0, 150));
	ClumpCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClumpCollision->SetHiddenInGame(true);
}

void AMuseeTree::BuildPlant()
{
	MuseeTreeGen::FOutput Out;
	const bool bBamboo = Species == EMuseeTreeSpecies::Bamboo;
	if (bBamboo)
	{
		MuseeTreeGen::GrowBamboo(Culms, EffectiveSeason(), EffectiveTerm(), Seed, TreeHeight, LeafDensity, Keep, KeepFromHeight, Out);
	}
	else
	{
		MuseeTreeGen::GrowTree(Species, FruitVariant, EffectiveSeason(), EffectiveTerm(), Seed, TreeHeight, CrownHeight, CrownRadii, LeafDensity, Keep,
								 KeepFromHeight, Out);
	}
	WriteSection(0, Out.Bark, Out.BarkMaterial);
	WriteSection(1, Out.Leaves, Out.LeafMaterial);
	WriteSection(2, Out.Flowers, Out.FlowerMaterial);
	WriteSection(3, Out.Fruit, TEXT("MI_Fruit"));

	// Collision: set in the editor (it is saved with the map); in play it is already in place.
	UWorld* PlantWorld = GetWorld();
	if (PlantWorld && PlantWorld->IsGameWorld()) { return; }
	if (bBamboo)
	{
		const FVector2D C = Out.Footprint.bIsValid ? Out.Footprint.GetCenter() : FVector2D::ZeroVector;
		const FVector2D E = Out.Footprint.bIsValid ? Out.Footprint.GetExtent() : FVector2D(0.5, 0.5);
		ClumpCollision->SetRelativeLocation(FVector(C.X * 100.0, C.Y * 100.0, 150.0));
		ClumpCollision->SetBoxExtent(FVector(FMath::Max(10.0, E.X * 100.0), FMath::Max(10.0, E.Y * 100.0), 150.0));
		ClumpCollision->SetCollisionProfileName(bBlockVisitors ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		TrunkCollision->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	}
	else
	{
		TrunkCollision->SetCapsuleSize(static_cast<float>(Out.CollisionRadius * 100.0), 120.f);
		TrunkCollision->SetRelativeLocation(FVector(0, 0, 120));
		TrunkCollision->SetCollisionProfileName(bBlockVisitors ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		ClumpCollision->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	}
}
