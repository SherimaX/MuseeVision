#include "Nature/MuseeWaterLilies.h"

#include "Nature/NatureMesh.h"
#include "Components/SphereComponent.h"
#include "Engine/CollisionProfile.h"

namespace MuseeLilyGen
{
	namespace MN = MuseeNature;

	struct FOut
	{
		FNatureMesh Pads;
		FNatureMesh Petals;
		FNatureMesh Golden;
		FNatureMesh Centres;
		FNatureMesh Stems;
		FNatureMesh Sepals;
		FNatureMesh Receptacles;
		bool bGolden = false;
		FVector GoldenAt = FVector::ZeroVector;
	};

	/** Where extra pads may float: inside the basin's ellipse and the pond's outline, clear of the edge. */
	struct FArea
	{
		FVector2D Basin = FVector2D::ZeroVector;
		const TArray<FVector2D>* Outline = nullptr;

		bool Contains(const FVector2D& P, double Margin) const
		{
			if (Basin.X > 0 && Basin.Y > 0)
			{
				const double AX = FMath::Max(0.05, Basin.X - Margin), AY = FMath::Max(0.05, Basin.Y - Margin);
				if (FMath::Square(P.X / AX) + FMath::Square(P.Y / AY) > 1.0) { return false; }
			}
			if (Outline && Outline->Num() >= 3)
			{
				const TArray<FVector2D>& Poly = *Outline;
				bool bIn = false;
				double MinDist = TNumericLimits<double>::Max();
				for (int32 i = 0, j = Poly.Num() - 1; i < Poly.Num(); j = i++)
				{
					const FVector2D& A = Poly[i];
					const FVector2D& B = Poly[j];
					if (((A.Y > P.Y) != (B.Y > P.Y)) && (P.X < (B.X - A.X) * (P.Y - A.Y) / (B.Y - A.Y) + A.X)) { bIn = !bIn; }
					const FVector2D AB = B - A;
					const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1e-9), 0.0, 1.0);
					MinDist = FMath::Min(MinDist, (A + AB * T - P).Size());
				}
				if (!bIn || MinDist < Margin) { return false; }
			}
			return true;
		}
	};

	/** A Nymphaea pad: round with the radial slit (the sinus), a little warped, the rim turned up on crowded pads. */
	void AddPad(FNatureMesh& M, const FVector2D& C, double Z, double R, double SlitAz, double Upturn, const FLinearColor& Inner,
				const FLinearColor& Outer, const FLinearColor& Rim, const FRandomStream& Rng)
	{
		const int32 Segs = 44;
		static const double Rho[] = {0.0, 0.08, 0.2, 0.36, 0.52, 0.67, 0.8, 0.9, 0.96, 1.0};
		const int32 NumRings = UE_ARRAY_COUNT(Rho);
		const double Half = FMath::DegreesToRadians(Rng.FRandRange(3.0, 6.5));
		const double WarpPhase = Rng.FRandRange(0.0, MN::Tau);
		const double Warp = Rng.FRandRange(0.3, 1.0);
		const double Phase = Rng.FRand();
		auto Height = [&](double Rh, double Th)
		{
			const double Edge = FMath::Square(FMath::Clamp((Rh - 0.8) / 0.2, 0.0, 1.0));
			double Dist = FMath::Fmod(FMath::Abs(Th - SlitAz), MN::Tau);
			Dist = FMath::Min(Dist, MN::Tau - Dist);
			// The two lobes lift a little either side of the slit.
			return Z + Upturn * R * 0.07 * Edge + Warp * R * 0.012 * FMath::Sin(2.0 * Th + WarpPhase) * Rh * Rh
				+ R * 0.01 * FMath::Exp(-Dist / 0.2) * Rh;
		};
		auto Pos = [&](double Rh, double Th) { return FVector(C.X + FMath::Cos(Th) * R * Rh, C.Y + FMath::Sin(Th) * R * Rh, Height(Rh, Th)); };
		FNatureWind Wind;
		Wind.Bend = 0.02;
		Wind.Phase = Phase;
		Wind.FlutterPhase = FMath::Frac(Phase * 7.3);
		TArray<int32> Grid;
		Grid.SetNum(NumRings * (Segs + 1));
		for (int32 j = 0; j < NumRings; ++j)
		{
			for (int32 i = 0; i <= Segs; ++i)
			{
				const double Th = SlitAz + Half + (MN::Tau - 2.0 * Half) * i / Segs;
				const double Rh = Rho[j];
				const double Step = 0.01;
				const FVector DR = Pos(Rh + Step, Th) - Pos(FMath::Max(0.0, Rh - Step), Th);
				const FVector DT = Pos(FMath::Max(Rh, 0.05), Th + Step) - Pos(FMath::Max(Rh, 0.05), Th - Step);
				FVector N = FVector::CrossProduct(DR, DT).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
				if (N.Z < 0) { N = -N; }
				FLinearColor Colour = MN::Mix(Inner, Outer, FMath::Pow(Rh, 1.5));
				if (Rh > 0.9) { Colour = MN::Mix(Colour, Rim, (Rh - 0.9) / 0.1 * 0.7); }
				Wind.Flutter = 0.15 * Rh * Rh;
				Wind.Height = Pos(Rh, Th).Z;
				Grid[j * (Segs + 1) + i] = M.Vertex(Pos(Rh, Th), N, FVector(-FMath::Sin(Th), FMath::Cos(Th), 0), FVector2D(double(i) / Segs, Rh), Wind, Colour, 1.0);
			}
		}
		for (int32 j = 0; j + 1 < NumRings; ++j)
		{
			for (int32 i = 0; i < Segs; ++i)
			{
				const int32 A = Grid[j * (Segs + 1) + i], B = Grid[j * (Segs + 1) + i + 1];
				const int32 CC = Grid[(j + 1) * (Segs + 1) + i + 1], D = Grid[(j + 1) * (Segs + 1) + i];
				if (j == 0) { M.Triangle(A, CC, D); }
				else { M.Quad(A, B, CC, D); }
			}
		}
		// The pad's edge has a little thickness.
		int32 PrevTop = INDEX_NONE, PrevLow = INDEX_NONE;
		for (int32 i = 0; i <= Segs; ++i)
		{
			const double Th = SlitAz + Half + (MN::Tau - 2.0 * Half) * i / Segs;
			const FVector Out(FMath::Cos(Th), FMath::Sin(Th), 0);
			const FVector Top = Pos(1.0, Th);
			Wind.Flutter = 0.15;
			Wind.Height = Top.Z;
			const int32 T0 = M.Vertex(Top, Out, FVector(-Out.Y, Out.X, 0), FVector2D(double(i) / Segs, 1.0), Wind, Rim, 0.9);
			const int32 L0 = M.Vertex(Top - FVector(0, 0, 0.004), Out, FVector(-Out.Y, Out.X, 0), FVector2D(double(i) / Segs, 1.0), Wind, Rim, 0.7);
			if (i > 0) { M.Quad(PrevTop, T0, L0, PrevLow); }
			PrevTop = T0;
			PrevLow = L0;
		}
	}

	/**
	 * A petal (or sepal, stamen, strap): a curved grid from Base outward along Radial, rising at
	 * Elev degrees and curling by Curl more towards the tip; cupped across; pointed. The normals
	 * face up and inwards (the petal's upper side).
	 */
	void AddPetal(FNatureMesh& M, const FVector& Base, const FVector& Radial, double Length, double Width, double Elev, double Curl, double Cup,
				  double TipShape, const FLinearColor& RootColour, const FLinearColor& TipColour, const FNatureWind& Wind0, int32 NX, int32 NY)
	{
		const FVector Up = FVector::UpVector;
		const FVector Side = FVector::CrossProduct(Up, Radial).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
		TArray<FVector> Line, PlaneN;
		FVector P = Base;
		for (int32 j = 0; j <= NY; ++j)
		{
			const double S = double(j) / NY;
			const double E = FMath::DegreesToRadians(Elev + Curl * S);
			Line.Add(P);
			PlaneN.Add(-Radial * FMath::Sin(E) + Up * FMath::Cos(E));
			P += (Radial * FMath::Cos(E) + Up * FMath::Sin(E)) * (Length / NY);
		}
		const int32 Row = NX + 1;
		TArray<FVector> Pts;
		Pts.SetNum(Row * (NY + 1));
		for (int32 j = 0; j <= NY; ++j)
		{
			const double S = double(j) / NY;
			const double HalfW = 0.5 * Width * (FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Pow(S, TipShape)), 0.6) + 0.25 * FMath::Square(1.0 - S));
			for (int32 i = 0; i <= NX; ++i)
			{
				const double X = NX > 0 ? -1.0 + 2.0 * i / NX : 0.0;
				Pts[j * Row + i] = Line[j] + Side * (X * HalfW) + PlaneN[j] * (Cup * Width * X * X);
			}
		}
		TArray<int32> Index;
		Index.SetNum(Pts.Num());
		for (int32 j = 0; j <= NY; ++j)
		{
			const double S = double(j) / NY;
			for (int32 i = 0; i <= NX; ++i)
			{
				const FVector DX = Pts[j * Row + FMath::Min(i + 1, NX)] - Pts[j * Row + FMath::Max(i - 1, 0)];
				const FVector DY = Pts[FMath::Min(j + 1, NY) * Row + i] - Pts[FMath::Max(j - 1, 0) * Row + i];
				FVector N = FVector::CrossProduct(DX, DY).GetSafeNormal(UE_SMALL_NUMBER, PlaneN[j]);
				if (FVector::DotProduct(N, PlaneN[j]) < 0) { N = -N; }
				FNatureWind Wind = Wind0;
				Wind.Flutter = Wind0.Flutter * (0.2 + 0.8 * S);
				Wind.Height = Pts[j * Row + i].Z;
				const FLinearColor Colour = MN::Mix(RootColour, TipColour, FMath::Pow(S, 1.3));
				Index[j * Row + i] = M.Vertex(Pts[j * Row + i], N, Side, FVector2D(NX > 0 ? double(i) / NX : 0.5, S), Wind, Colour, 0.75 + 0.25 * S);
			}
		}
		for (int32 j = 0; j < NY; ++j)
		{
			for (int32 i = 0; i < NX; ++i)
			{
				M.Quad(Index[j * Row + i], Index[j * Row + i + 1], Index[(j + 1) * Row + i + 1], Index[(j + 1) * Row + i]);
			}
		}
	}

	FVector RadialAt(double Az) { return FVector(FMath::Cos(Az), FMath::Sin(Az), 0); }

	/** A ring of stamens round Centre, curving in over the middle. */
	void AddStamens(FNatureMesh& M, const FVector& Centre, int32 Count, double RingRadius, double Length, double Width, double Elev, double Curl,
					const FLinearColor& Root, const FLinearColor& Tip, const FNatureWind& Wind, const FRandomStream& Rng)
	{
		for (int32 k = 0; k < Count; ++k)
		{
			const double Az = MN::Tau * k / Count + Rng.FRandRange(-0.08, 0.08);
			const FVector Radial = RadialAt(Az);
			const FVector Base = Centre + Radial * (RingRadius * Rng.FRandRange(0.75, 1.1));
			AddPetal(M, Base, Radial, Length * Rng.FRandRange(0.8, 1.15), Width, Elev + Rng.FRandRange(-10.0, 10.0), Curl, 0.0, 1.0, Root, Tip, Wind, 1, 2);
		}
	}

	/** A Nymphaea flower: four sepals, four rings of pointed petals, the stamen crown and the stigma disc. */
	void AddNymphaea(FOut& Out, const FVector& Centre, double Size, int32 Colouring, double Openness, const FRandomStream& Rng, double Phase)
	{
		FNatureMesh& PetalMesh = Colouring == 2 ? Out.Golden : Out.Petals;
		FLinearColor Root, Tip;
		switch (Colouring)
		{
		case 0: Root = MN::Srgb(0xFBEFF3); Tip = MN::Srgb(0xEE94B4); break;   // pink
		case 1: Root = MN::Srgb(0xFFFDF8); Tip = MN::Srgb(0xF8ECEF); break;   // white, a blush at the tips
		default: Root = MN::Srgb(0xFFE17A); Tip = MN::Srgb(0xF2B32A); break; // the golden lily
		}
		const double K = Size / 0.1;
		FNatureWind Wind;
		Wind.Bend = 0.02;
		Wind.Flutter = 0.35;
		Wind.Phase = Phase;
		Wind.FlutterPhase = FMath::Frac(Phase * 3.7);
		const double Az0 = Rng.FRandRange(0.0, MN::Tau);
		const double Lift = 2.0 - Openness;
		// Sepals: green outside (the material's underside colour), pale within.
		const FLinearColor SepalIn = MN::Mix(Root, MN::Srgb(0xDDE6C6), 0.6);
		for (int32 k = 0; k < 4; ++k)
		{
			const FVector Radial = RadialAt(Az0 + MN::Tau * k / 4 + Rng.FRandRange(-0.1, 0.1));
			AddPetal(Out.Sepals, Centre + Radial * (0.01 * K) + FVector(0, 0, -0.004), Radial, Size * 1.02, Size * 0.36,
					 5.0 + Rng.FRandRange(-3.0, 4.0), -6.0, 0.1, 0.9, SepalIn, SepalIn, Wind, 4, 6);
		}
		struct FRing { int32 N; double L, W, E, C; };
		const FRing Rings[4] = {{8, 1.0, 0.33, 17, 6}, {8, 0.93, 0.31, 35, 8}, {7, 0.82, 0.29, 52, 10}, {6, 0.68, 0.27, 68, 10}};
		for (int32 r = 0; r < 4; ++r)
		{
			const FRing& Ring = Rings[r];
			for (int32 k = 0; k < Ring.N; ++k)
			{
				const double Az = Az0 + (k + 0.5 * r) * MN::Tau / Ring.N + Rng.FRandRange(-0.09, 0.09);
				const FVector Radial = RadialAt(Az);
				const FVector Base = Centre + Radial * (0.018 * K * (1.0 - 0.18 * r)) + FVector(0, 0, 0.004 * K * r);
				const double Elev = FMath::Min(80.0, Ring.E * Lift + Rng.FRandRange(-6.0, 6.0));
				AddPetal(PetalMesh, Base, Radial, Size * Ring.L * Rng.FRandRange(0.92, 1.06), Size * Ring.W * Rng.FRandRange(0.9, 1.08), Elev,
						 Ring.C + Rng.FRandRange(-4.0, 4.0), 0.12, 0.85, Root, Tip, Wind, 4, 7);
			}
		}
		// The stamens, golden yellow, curving in over the stigma.
		const FLinearColor StamenRoot = MN::Srgb(Colouring == 2 ? 0xF0A820 : 0xF2BE2C);
		const FLinearColor StamenTip = MN::Srgb(Colouring == 2 ? 0xF8D050 : 0xF8DA64);
		FNatureWind Still = Wind;
		Still.Flutter = 0.1;
		AddStamens(Out.Centres, Centre + FVector(0, 0, 0.006 * K), 46, 0.017 * K, 0.034 * K, 0.0045 * K, 55.0, 30.0, StamenRoot, StamenTip, Still, Rng);
		const TArray<FVector2D> Disc = {FVector2D(0.0, 0.012 * K), FVector2D(0.009 * K, 0.009 * K), FVector2D(0.016 * K, 0.014 * K), FVector2D(0.017 * K, 0.004 * K)};
		// The profile runs inward over the top; turn it so its normals face up.
		TArray<FVector2D> Profile = {Disc[3], Disc[2], Disc[1], Disc[0]};
		MN::AddLathe(Out.Centres, Centre, FVector::UpVector, Profile, 16, Still, {MN::Srgb(0xE0A826), MN::Srgb(0xEFC23A), MN::Srgb(0xE6B030), MN::Srgb(0xD89A20)}, 0.9);
	}

	/** A closed bud (Nymphaea or lotus): a pointed ovoid about Axis. */
	void AddBud(FNatureMesh& M, const FVector& Origin, const FVector& Axis, double Height, double Radius, const FLinearColor& BaseColour,
				const FLinearColor& TipColour, const FNatureWind& Wind)
	{
		const TArray<FVector2D> Profile = {
			FVector2D(0.0, 0.0), FVector2D(0.55 * Radius, 0.08 * Height), FVector2D(0.92 * Radius, 0.28 * Height), FVector2D(Radius, 0.45 * Height),
			FVector2D(0.8 * Radius, 0.68 * Height), FVector2D(0.42 * Radius, 0.87 * Height), FVector2D(0.08 * Radius, 0.98 * Height), FVector2D(0.0, Height)};
		TArray<FLinearColor> Colours;
		for (int32 i = 0; i < Profile.Num(); ++i) { Colours.Add(MN::Mix(BaseColour, TipColour, FMath::Pow(double(i) / (Profile.Num() - 1), 1.6))); }
		MN::AddLathe(M, Origin, Axis, Profile, 12, Wind, Colours, 1.0);
	}

	/** A stalk from under the water up to To, curving gently. */
	void AddStalk(FNatureMesh& M, const FVector& From, const FVector& To, double Radius, double Sway, const FLinearColor& Colour, const FNatureWind& TopWind,
				  const FRandomStream& Rng)
	{
		const FVector Bow = FVector(Rng.FRandRange(-1.0, 1.0), Rng.FRandRange(-1.0, 1.0), 0).GetSafeNormal() * Sway;
		TArray<FNatureTubeRing> Rings;
		const int32 Segs = 6;
		for (int32 s = 0; s <= Segs; ++s)
		{
			const double T = double(s) / Segs;
			FNatureTubeRing& Ring = Rings.AddDefaulted_GetRef();
			Ring.Centre = FMath::Lerp(From, To, T) + Bow * FMath::Sin(UE_DOUBLE_PI * T);
			Ring.Radius = Radius * (1.0 - 0.2 * T);
			Ring.Colour = Colour;
			Ring.Wind = TopWind;
			Ring.Wind.Bend = TopWind.Bend * T * T;
			Ring.Wind.Flutter = 0;
			Ring.Wind.Height = Ring.Centre.Z;
		}
		for (int32 s = 0; s <= Segs; ++s)
		{
			const FVector A = Rings[FMath::Max(0, s - 1)].Centre, B = Rings[FMath::Min(Segs, s + 1)].Centre;
			Rings[s].Dir = (B - A).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		}
		MN::AddTube(M, Rings, 5, 0.0, false);
	}

	/** A lotus leaf: round and peltate, cupped, the rim waved; floating or held up. */
	void AddLotusLeaf(FNatureMesh& M, const FVector& Centre, const FVector& Normal, double R, double Cup, double Wave, const FLinearColor& Middle,
					  const FLinearColor& Edge, const FLinearColor& Spot, const FNatureWind& Wind0, const FRandomStream& Rng)
	{
		const int32 Segs = 48;
		static const double Rho[] = {0.0, 0.06, 0.14, 0.26, 0.4, 0.54, 0.68, 0.8, 0.9, 0.97, 1.0};
		const int32 NumRings = UE_ARRAY_COUNT(Rho);
		FVector U, W;
		MN::Basis(Normal, U, W);
		const double WavePhase = Rng.FRandRange(0.0, MN::Tau);
		const int32 Waves = Rng.RandRange(6, 9);
		auto Pos = [&](double Rh, double Th)
		{
			const double H = Cup * R * Rh * Rh + Wave * R * 0.05 * FMath::Sin(Waves * Th + WavePhase) * FMath::Pow(Rh, 4.0);
			return Centre + (U * FMath::Cos(Th) + W * FMath::Sin(Th)) * (R * Rh) + Normal * H;
		};
		TArray<int32> Grid;
		Grid.SetNum(NumRings * (Segs + 1));
		for (int32 j = 0; j < NumRings; ++j)
		{
			for (int32 i = 0; i <= Segs; ++i)
			{
				const double Th = MN::Tau * i / Segs;
				const double Rh = Rho[j];
				const double Step = 0.01;
				const FVector DR = Pos(Rh + Step, Th) - Pos(FMath::Max(0.0, Rh - Step), Th);
				const FVector DT = Pos(FMath::Max(Rh, 0.05), Th + Step) - Pos(FMath::Max(Rh, 0.05), Th - Step);
				FVector N = FVector::CrossProduct(DR, DT).GetSafeNormal(UE_SMALL_NUMBER, Normal);
				if (FVector::DotProduct(N, Normal) < 0) { N = -N; }
				FLinearColor Colour = Rh < 0.1 ? MN::Mix(Spot, Middle, Rh / 0.1) : MN::Mix(Middle, Edge, Rh * Rh);
				FNatureWind Wind = Wind0;
				Wind.Bend = Wind0.Bend + 0.08 * Rh;
				Wind.Flutter = 0.35 * Rh * Rh;
				Wind.Height = Pos(Rh, Th).Z;
				Grid[j * (Segs + 1) + i] = M.Vertex(Pos(Rh, Th), N, U * -FMath::Sin(Th) + W * FMath::Cos(Th), FVector2D(double(i) / Segs, Rh), Wind, Colour,
													0.85 + 0.15 * Rh);
			}
		}
		for (int32 j = 0; j + 1 < NumRings; ++j)
		{
			for (int32 i = 0; i < Segs; ++i)
			{
				const int32 A = Grid[j * (Segs + 1) + i], B = Grid[j * (Segs + 1) + i + 1];
				const int32 CC = Grid[(j + 1) * (Segs + 1) + i + 1], D = Grid[(j + 1) * (Segs + 1) + i];
				if (j == 0) { M.Triangle(A, CC, D); }
				else { M.Quad(A, B, CC, D); }
			}
		}
	}

	/** A lotus flower on its stalk: three rings of broad cupped petals round the seed-head receptacle and stamens. */
	void AddLotusFlower(FOut& Out, const FVector& Top, double Size, const FRandomStream& Rng, const FNatureWind& Wind)
	{
		const FLinearColor Root = MN::Srgb(0xF6EEE6), Tip = MN::Srgb(0xE095B0);
		const double Az0 = Rng.FRandRange(0.0, MN::Tau);
		struct FRing { int32 N; double L, W, E, C; };
		const FRing Rings[3] = {{5, 0.56, 0.62, 30, 22}, {6, 0.52, 0.6, 46, 18}, {6, 0.46, 0.56, 62, 12}};
		FNatureWind PetalWind = Wind;
		PetalWind.Flutter = 0.3;
		for (int32 r = 0; r < 3; ++r)
		{
			const FRing& Ring = Rings[r];
			for (int32 k = 0; k < Ring.N; ++k)
			{
				const FVector Radial = RadialAt(Az0 + (k + 0.5 * r) * MN::Tau / Ring.N + Rng.FRandRange(-0.1, 0.1));
				AddPetal(Out.Petals, Top + Radial * (0.05 * Size) + FVector(0, 0, 0.01 * Size * r), Radial, Size * Ring.L * Rng.FRandRange(0.92, 1.06),
						 Size * Ring.L * Ring.W, Ring.E + Rng.FRandRange(-6.0, 6.0), Ring.C, 0.2, 1.5, Root, Tip, PetalWind, 4, 7);
			}
		}
		const double S = Size;
		const TArray<FVector2D> Pod = {FVector2D(0.03 * S, -0.02 * S), FVector2D(0.085 * S, 0.055 * S), FVector2D(0.09 * S, 0.066 * S), FVector2D(0.0, 0.07 * S)};
		MN::AddLathe(Out.Receptacles, Top, FVector::UpVector, Pod, 18, Wind, {MN::Srgb(0xB8B048), MN::Srgb(0xD2CA5C), MN::Srgb(0xD8D060), MN::Srgb(0xC8C050)}, 1.0);
		AddStamens(Out.Centres, Top + FVector(0, 0, 0.01 * S), 64, 0.09 * S, 0.12 * S, 0.004, 48.0, 22.0, MN::Srgb(0xF2C84A), MN::Srgb(0xFFE890), Wind, Rng);
	}

	void BuildWaterLilies(const TArray<FMuseeLilySpec>& Plants, const FArea& Area, double Fullness, int32 Seed, FOut& Out)
	{
		FRandomStream Rng(Seed * 7919 + 31);
		double Layer = 0.002;   // pads float a millimetre or so apart where they overlap
		int32 PadCount = 0;
		int32 Flowering = 0;
		const TArray<FLinearColor> Greens = {MN::Srgb(0x3F6B2E), MN::Srgb(0x4A7632), MN::Srgb(0x557F36), MN::Srgb(0x44702C), MN::Srgb(0x5C8238)};
		const FLinearColor Bronze = MN::Srgb(0x6E5A2E), Yellowing = MN::Srgb(0x9A9440), RimRed = MN::Srgb(0x6A3A2A);
		auto PadColours = [&](FLinearColor& Inner, FLinearColor& Outer, FLinearColor& Rim, double Radius)
		{
			const FLinearColor Base = MN::Vary(Greens[Rng.RandRange(0, Greens.Num() - 1)], Rng, 4.0, 0.08, 0.1);
			Inner = MN::Mix(Base, MN::Srgb(0x6E9448), 0.25);
			Outer = Base;
			Rim = MN::Mix(Base, RimRed, Rng.FRandRange(0.1, 0.5));
			if (Radius < 0.12 && Rng.FRand() < 0.5) { Inner = MN::Mix(Inner, Bronze, 0.5); Outer = MN::Mix(Outer, Bronze, 0.6); }   // young pads
			else if (Rng.FRand() < 0.15) { Outer = MN::Mix(Outer, Yellowing, 0.5); }                                               // an old one
		};
		for (const FMuseeLilySpec& Spec : Plants)
		{
			// The plan's radius is the colony's reach: real Nymphaea pads are 20–45 cm across.
			const FVector2D At = Spec.Position;
			const double Reach = FMath::Max(0.1, double(Spec.Radius));
			const double R = FMath::Clamp(Reach * 0.45, 0.1, 0.22);
			const bool bFlower = Spec.bFlower || Spec.bGolden;
			const double SlitAz = Rng.FRandRange(0.0, MN::Tau);
			// With a flower, the pad sits beside it, its slit towards the flower.
			const FVector2D PadCentre = bFlower ? At - FVector2D(FMath::Cos(SlitAz), FMath::Sin(SlitAz)) * (0.45 * R) : At;
			FLinearColor Inner, Outer, Rim;
			PadColours(Inner, Outer, Rim, R);
			AddPad(Out.Pads, PadCentre, Layer, R, SlitAz, Rng.FRand() < 0.3 ? 1.0 : 0.2, Inner, Outer, Rim, Rng);
			Layer = 0.002 + 0.0009 * (++PadCount % 5);
			// The colony: more pads, overlapping, out to the plan's radius.
			const int32 Extra = FMath::RoundToInt((2.0 + 8.0 * Reach) * Fullness);
			for (int32 e = 0; e < Extra; ++e)
			{
				for (int32 Try = 0; Try < 8; ++Try)
				{
					const double SR = R * Rng.FRandRange(0.55, 1.0);
					const double A = Rng.FRandRange(0.0, MN::Tau);
					const FVector2D C = At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * (Reach * Rng.FRandRange(0.35, 1.0) + SR * 0.5);
					if (!Area.Contains(C, SR + 0.04)) { continue; }
					PadColours(Inner, Outer, Rim, SR);
					AddPad(Out.Pads, C, Layer, SR, Rng.FRandRange(0.0, MN::Tau), Rng.FRand() < 0.35 ? 1.0 : 0.15, Inner, Outer, Rim, Rng);
					Layer = 0.002 + 0.0009 * (++PadCount % 5);
					break;
				}
			}
			if (bFlower)
			{
				const int32 Colouring = Spec.bGolden ? 2 : (Flowering++ % 2);
				const double Size = Spec.bGolden ? 0.12 : FMath::Clamp(0.25 * Reach + 0.02, 0.085, 0.115);
				const FVector Centre(At.X, At.Y, 0.012);
				AddNymphaea(Out, Centre, Size, Colouring, Rng.FRandRange(0.85, 1.0), Rng, Rng.FRand());
				if (Spec.bGolden)
				{
					Out.bGolden = true;
					Out.GoldenAt = Centre + FVector(0, 0, 0.04);
				}
				// Sometimes a bud nearby, closed on the water.
				if (Rng.FRand() < 0.6)
				{
					const double A = Rng.FRandRange(0.0, MN::Tau);
					const FVector2D B2 = At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rng.FRandRange(0.22, 0.4);
					if (Area.Contains(B2, 0.08))
					{
						const FVector Axis = (FVector(FMath::Cos(A), FMath::Sin(A), 0) * 0.8 + FVector::UpVector * Rng.FRandRange(0.5, 1.2)).GetSafeNormal();
						FNatureWind Wind;
						Wind.Bend = 0.02;
						Wind.Phase = Rng.FRand();
						const FLinearColor Tip = Colouring == 0 ? MN::Srgb(0xD9869E) : (Colouring == 2 ? MN::Srgb(0xE8C040) : MN::Srgb(0xE6E8D0));
						AddBud(Out.Sepals, FVector(B2.X, B2.Y, -0.01), Axis, 0.075, 0.022, MN::Srgb(0x5E7A34), Tip, Wind);
					}
				}
			}
		}
	}

	void BuildLotus(const TArray<FMuseeLilySpec>& Plants, const FArea& Area, double Fullness, int32 Seed, int32 Term, FOut& Out)
	{
		// Leaves from Lixia to Lidong, withered from Shuangjiang; flowers from Mangzhong to Chushu.
		if (Term < 6 || Term > 19) { return; }
		const bool bYoung = Term <= 7;
		const bool bWithered = Term >= 16;
		const bool bFlowers = Term >= 8 && Term <= 13;
		const bool bSeedHeads = Term == 14 || Term == 15;
		FRandomStream Rng(Seed * 7919 + 47);
		const FLinearColor Middle = MN::Srgb(bWithered ? 0x8A7A5A : (bYoung ? 0x7EA060 : 0x6E9460));
		const FLinearColor Edge = MN::Srgb(bWithered ? 0x6E5A3A : (bYoung ? 0x6A9050 : 0x5A8250));
		const FLinearColor Spot = MN::Srgb(bWithered ? 0xA89A70 : 0xB8C890);
		const FLinearColor StalkColour = MN::Srgb(bWithered ? 0x7A6A48 : 0x74864A);
		double Layer = 0.003;
		int32 Floating = 0;
		auto Leaf = [&](const FVector2D& At, double R, bool bRaised)
		{
			FNatureWind Wind;
			Wind.Phase = Rng.FRand();
			Wind.FlutterPhase = Rng.FRand();
			const FLinearColor M1 = MN::Vary(Middle, Rng, 4.0, 0.08, 0.1), E1 = MN::Vary(Edge, Rng, 4.0, 0.08, 0.1);
			if (bRaised)
			{
				const double H = Rng.FRandRange(0.22, bWithered ? 0.45 : 0.7);
				const double Tilt = FMath::DegreesToRadians(Rng.FRandRange(5.0, 22.0));
				const double A = Rng.FRandRange(0.0, MN::Tau);
				const FVector Normal(FMath::Sin(Tilt) * FMath::Cos(A), FMath::Sin(Tilt) * FMath::Sin(A), FMath::Cos(Tilt));
				const FVector Centre(At.X, At.Y, H);
				Wind.Bend = 0.25 + 0.4 * H;
				AddStalk(Out.Stems, FVector(At.X + Rng.FRandRange(-0.1, 0.1), At.Y + Rng.FRandRange(-0.1, 0.1), -0.3), Centre - Normal * 0.01, 0.007, 0.06,
						 StalkColour, Wind, Rng);
				AddLotusLeaf(Out.Pads, Centre, Normal, R, bWithered ? 0.3 : Rng.FRandRange(0.12, 0.22), bWithered ? 2.0 : Rng.FRandRange(0.4, 1.0),
							 M1, E1, Spot, Wind, Rng);
			}
			else
			{
				Wind.Bend = 0.02;
				AddLotusLeaf(Out.Pads, FVector(At.X, At.Y, Layer), FVector::UpVector, R, bWithered ? 0.1 : 0.03, bWithered ? 1.2 : 0.2, M1, E1, Spot, Wind, Rng);
				Layer = 0.003 + 0.001 * (++Floating % 5);
			}
		};
		int32 Index = 0;
		for (const FMuseeLilySpec& Spec : Plants)
		{
			const FVector2D At = Spec.Position;
			const double R = FMath::Max(0.08, double(Spec.Radius)) * (bYoung ? 0.7 : 1.0);
			const bool bHasFlower = Spec.FlowerSize > 0 || Spec.bFlower;
			Leaf(At, R, !bYoung && (bHasFlower || Index % 2 == 0));
			const int32 Extra = FMath::RoundToInt((2.0 + 3.0 * R) * Fullness);
			for (int32 e = 0; e < Extra; ++e)
			{
				for (int32 Try = 0; Try < 8; ++Try)
				{
					const double SR = R * Rng.FRandRange(0.5, 0.9);
					const double A = Rng.FRandRange(0.0, MN::Tau);
					const FVector2D C = At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rng.FRandRange(0.3, 0.8);
					const bool bRaised = !bYoung && Rng.FRand() < 0.5;
					if (!Area.Contains(C, bRaised ? 0.05 : SR + 0.03)) { continue; }
					Leaf(C, SR, bRaised);
					break;
				}
			}
			const double Size = Spec.FlowerSize > 0 ? double(Spec.FlowerSize) : 0.26;
			FNatureWind Wind;
			Wind.Phase = Rng.FRand();
			Wind.FlutterPhase = Rng.FRand();
			if (bFlowers && bHasFlower)
			{
				const FVector Top(At.X + Rng.FRandRange(-0.08, 0.08), At.Y + Rng.FRandRange(-0.08, 0.08), Rng.FRandRange(0.5, 0.62));
				Wind.Bend = 0.5;
				AddStalk(Out.Stems, FVector(At.X, At.Y, -0.3), Top, 0.0065, 0.05, StalkColour, Wind, Rng);
				AddLotusFlower(Out, Top, Size, Rng, Wind);
			}
			if ((bFlowers || Term == 7) && Rng.FRand() < 0.5)
			{
				// A bud on its own stalk.
				const double A = Rng.FRandRange(0.0, MN::Tau);
				const FVector2D B2 = At + FVector2D(FMath::Cos(A), FMath::Sin(A)) * Rng.FRandRange(0.2, 0.45);
				if (Area.Contains(B2, 0.05))
				{
					const FVector Top(B2.X, B2.Y, Rng.FRandRange(0.35, 0.55));
					Wind.Bend = 0.45;
					AddStalk(Out.Stems, FVector(B2.X, B2.Y, -0.3), Top, 0.006, 0.04, StalkColour, Wind, Rng);
					AddBud(Out.Petals, Top - FVector(0, 0, 0.01), FVector::UpVector, Size * 0.5, Size * 0.17, MN::Srgb(0xB8C890), MN::Srgb(0xD8708E), Wind);
				}
			}
			if (bSeedHeads && bHasFlower)
			{
				// The seed head, nodding a little, the petals fallen.
				const FVector Top(At.X, At.Y, Rng.FRandRange(0.45, 0.6));
				Wind.Bend = 0.5;
				AddStalk(Out.Stems, FVector(At.X, At.Y, -0.3), Top, 0.0065, 0.06, StalkColour, Wind, Rng);
				const double S = Size * 1.3;
				const FVector Axis = (FVector::UpVector + FVector(Rng.FRandRange(-0.3, 0.3), Rng.FRandRange(-0.3, 0.3), 0)).GetSafeNormal();
				const TArray<FVector2D> Pod = {FVector2D(0.02 * S, -0.03 * S), FVector2D(0.09 * S, 0.06 * S), FVector2D(0.095 * S, 0.07 * S), FVector2D(0.0, 0.075 * S)};
				MN::AddLathe(Out.Receptacles, Top, Axis, Pod, 18, Wind, {MN::Srgb(0x6E7A38), MN::Srgb(0x8A9A48), MN::Srgb(0x8A9448), MN::Srgb(0x7A8440)}, 1.0);
			}
			++Index;
		}
	}
}

AMuseeWaterLilies::AMuseeWaterLilies()
{
	GetRootComponent()->SetMobility(EComponentMobility::Movable);
	GoldenTarget = CreateDefaultSubobject<USphereComponent>(TEXT("GoldenTarget"));
	GoldenTarget->SetupAttachment(RootComponent);
	GoldenTarget->SetMobility(EComponentMobility::Movable);
	GoldenTarget->InitSphereRadius(25.f);
	GoldenTarget->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	GoldenTarget->SetHiddenInGame(true);
	GoldenTarget->SetCanEverAffectNavigation(false);
}

void AMuseeWaterLilies::BuildPlant()
{
	MuseeLilyGen::FOut Out;
	MuseeLilyGen::FArea Area;
	Area.Basin = BasinRadii;
	Area.Outline = &PondOutline;
	const bool bLotus = Kind == EMuseeWaterPlant::Lotus;
	if (bLotus) { MuseeLilyGen::BuildLotus(Plants, Area, Fullness, Seed, EffectiveTerm(), Out); }
	else { MuseeLilyGen::BuildWaterLilies(Plants, Area, Fullness, Seed, Out); }

	WriteSection(0, Out.Pads, bLotus ? TEXT("MI_Leaf_Lotus") : TEXT("MI_Pad_Lily"));
	WriteSection(1, Out.Petals, bLotus ? TEXT("MI_Petal_Lotus") : TEXT("MI_Petal_Lily"));
	WriteSection(2, Out.Golden, TEXT("MI_Petal_GoldenLily"));
	WriteSection(3, Out.Centres, TEXT("MI_Stamen"));
	WriteSection(4, Out.Stems, TEXT("MI_Stem_Green"));
	WriteSection(5, Out.Sepals, TEXT("MI_Sepal_Lily"));
	WriteSection(6, Out.Receptacles, TEXT("MI_Receptacle"));

	// The golden lily's target for the look trace: set in the editor (it is saved with the map).
	UWorld* PlantWorld = GetWorld();
	if (PlantWorld && PlantWorld->IsGameWorld()) { return; }
	if (Out.bGolden)
	{
		GoldenTarget->SetRelativeLocation(Out.GoldenAt * 100.0);
		GoldenTarget->SetSphereRadius(25.f);
		GoldenTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GoldenTarget->SetCollisionResponseToAllChannels(ECR_Ignore);
		GoldenTarget->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
	else
	{
		GoldenTarget->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	}
}
