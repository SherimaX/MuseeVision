#include "Nature/MuseeHedge.h"

#include "Math/RandomStream.h"
#include "Nature/NatureMesh.h"

namespace MuseeHedge
{
	namespace MN = MuseeNature;

	const FSpec& Spec(EKind Kind)
	{
		auto Make = [](const TCHAR* Name, EPart Part, bool bYew, int32 Variants, double W, double H, double Batter, double Shoulder, double Inset,
					   double Density, double Bend = 0.0)
		{
			FSpec S;
			S.Name = Name;
			S.Part = Part;
			S.bYew = bYew;
			S.Variants = Variants;
			S.Width = W;
			S.Height = H;
			S.Batter = Batter;
			S.Shoulder = Shoulder;
			S.Inset = Inset;
			S.Density = Density;
			S.Bend = Bend;
			return S;
		};
		// The parterre's box: 0.42 × 0.52 m, clipped square with softened shoulders; its balls 0.92 m across. The exedra's
		// yew: 0.9 × 1.7 m, battered. The Hall of Light's garden hedges: yew, 0.45 × 1.5 m.
		static const FSpec Specs[] = {
			Make(TEXT("BoxRun"), EPart::Run, false, 4, 0.42, 0.52, 0.015, 0.045, 0.055, 32000.0),
			Make(TEXT("BoxCorner"), EPart::Corner, false, 2, 0.42, 0.52, 0.015, 0.045, 0.055, 32000.0),
			Make(TEXT("BoxEnd"), EPart::End, false, 2, 0.42, 0.52, 0.015, 0.045, 0.055, 32000.0),
			Make(TEXT("BoxBall"), EPart::Ball, false, 2, 0.46, 0.92, 0.0, 0.0, 0.055, 32000.0),
			Make(TEXT("YewRun"), EPart::Run, true, 4, 0.9, 1.7, 0.04, 0.08, 0.075, 58000.0, 14.0),
			Make(TEXT("YewEnd"), EPart::End, true, 2, 0.9, 1.7, 0.04, 0.08, 0.075, 58000.0),
			Make(TEXT("NarrowYewRun"), EPart::Run, true, 4, 0.45, 1.5, 0.02, 0.06, 0.065, 58000.0),
			Make(TEXT("NarrowYewEnd"), EPart::End, true, 1, 0.45, 1.5, 0.02, 0.06, 0.065, 58000.0),
		};
		static_assert(UE_ARRAY_COUNT(Specs) == int32(EKind::Num), "a spec per kind");
		return Specs[FMath::Clamp(int32(Kind), 0, int32(EKind::Num) - 1)];
	}

	int32 NumMeshes()
	{
		int32 N = 0;
		for (int32 k = 0; k < int32(EKind::Num); ++k) { N += Spec(EKind(k)).Variants; }
		return N;
	}

	int32 MeshIndex(EKind Kind, int32 Variant)
	{
		int32 N = 0;
		for (int32 k = 0; k < int32(Kind); ++k) { N += Spec(EKind(k)).Variants; }
		return N + FMath::Clamp(Variant, 0, Spec(Kind).Variants - 1);
	}

	void DecodeMeshIndex(int32 Index, EKind& OutKind, int32& OutVariant)
	{
		for (int32 k = 0; k < int32(EKind::Num); ++k)
		{
			const int32 V = Spec(EKind(k)).Variants;
			if (Index < V)
			{
				OutKind = EKind(k);
				OutVariant = Index;
				return;
			}
			Index -= V;
		}
		OutKind = EKind::BoxRun;
		OutVariant = 0;
	}

	FString MeshName(int32 Index)
	{
		EKind Kind;
		int32 V;
		DecodeMeshIndex(Index, Kind, V);
		return FString::Printf(TEXT("SM_Hedge_%s_V%d"), Spec(Kind).Name, V);
	}

	FString MeshPath(int32 Index)
	{
		const FString Name = MeshName(Index);
		return FString::Printf(TEXT("%s/%s.%s"), Folder(), *Name, *Name);
	}

	namespace
	{
		double Smooth(double A, double B, double X) { return FMath::SmoothStep(A, B, X); }

		/** The distance to a rounded box (half extents Q offsets already taken), the usual way. */
		double Rounded(double QX, double QY, double QZ, double R)
		{
			const FVector Out(FMath::Max(QX, 0.0), FMath::Max(QY, 0.0), FMath::Max(QZ, 0.0));
			return Out.Size() + FMath::Min(FMath::Max3(QX, QY, QZ), 0.0) - R;
		}

		/** The clipped surface of one module: a signed distance (m, negative inside) with the shears' wander. */
		struct FShape
		{
			const FSpec& S;
			int32 Seed = 0;
			int32 SharedSeed = 0;
			/** The shallow dips the shears left (centre, radius, depth). */
			struct FDip
			{
				FVector C;
				double R, Depth;
			};
			TArray<FDip> Dips;

			FShape(const FSpec& InSpec, int32 InSeed, int32 InShared) : S(InSpec), Seed(InSeed), SharedSeed(InShared) {}

			double A() const { return 0.5 * S.Width; }
			/** The inward shift of a vertical face at height Z (the batter). */
			double Bat(double Z) const { return S.Batter * FMath::Clamp(Z / S.Height, 0.0, 1.0); }
			double ZCentre() const { return 0.5 * (S.Height - 0.5); }
			double ZHalf() const { return 0.5 * (S.Height + 0.5); }

			/** The hedge running along x (infinite), |y| ≤ A at the foot. */
			double Slab(double Y, double Z) const
			{
				const double R = S.Shoulder;
				return Rounded(-1.0, FMath::Abs(Y) + Bat(Z) - (A() - R), FMath::Abs(Z - ZCentre()) - (ZHalf() - R), R);
			}

			/** The hedge on −x with its end face at x = 0. */
			double EndPiece(double X, double Y, double Z) const
			{
				const double R = S.Shoulder;
				return Rounded(X + Bat(Z) + R, FMath::Abs(Y) + Bat(Z) - (A() - R), FMath::Abs(Z - ZCentre()) - (ZHalf() - R), R);
			}

			double Base(const FVector& P) const
			{
				switch (S.Part)
				{
				case EPart::Run: return Slab(P.Y, P.Z);
				case EPart::End: return EndPiece(P.X, P.Y, P.Z);
				case EPart::Corner:
				{
					// One slab along +x from its end face at x = −A, one along +y from y = −A.
					const double DA = EndPiece(-P.X - A(), P.Y, P.Z);
					const double DB = EndPiece(-P.Y - A(), P.X, P.Z);
					return FMath::Min(DA, DB);
				}
				case EPart::Ball:
				default: return (P - FVector(0.0, 0.0, S.Width - BallSink)).Size() - S.Width;
				}
			}

			/** Noise in about [-1, 1]. */
			static double Signed(const FVector& Q, int32 InSeed) { return 2.0 * (MN::Fbm(Q, 3, InSeed) - 0.5); }

			double Scale() const { return S.bYew ? 1.0 / 0.16 : 1.0 / 0.11; }

			/**
			 * How much of the module's own field shows at P: 0 at its joints, where every module shows the shared field (a
			 * function of the cross-section alone); for a corner, which arm's shared field (OutArmB: 0 the x arm, 1 the y).
			 */
			double OwnWeight(const FVector& P, double& OutArmB) const
			{
				OutArmB = 0.0;
				switch (S.Part)
				{
				case EPart::Run: return FMath::Square(FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(P.X / Length, 0.0, 1.0)));
				case EPart::End: return Smooth(-EndLength, -0.5 * EndLength, P.X);
				case EPart::Corner:
				{
					const double BX = Smooth(0.0, A(), P.X), BY = Smooth(0.0, A(), P.Y);
					OutArmB = BY / FMath::Max(BX + BY, 1e-6);
					return (1.0 - BX) * (1.0 - BY);
				}
				case EPart::Ball:
				default: return 1.0;
				}
			}

			/**
			 * A field that is the same on both sides of every joint: the shared part blended with the module's own at equal
			 * power, so its strength doesn't pulse from module to module along a hedge.
			 */
			double Field(const FVector& P, int32 OwnSeed, int32 CommonSeed, const FVector& Offset) const
			{
				double ArmB;
				const double W = OwnWeight(P, ArmB);
				const double K = Scale();
				auto SharedAt = [&](double Across) { return Signed(FVector(FMath::Abs(Across) * K, P.Z * K, 0.37) + Offset, CommonSeed); };
				double Sh = 0.0;
				if (W < 1.0) { Sh = ArmB > 0.0 ? FMath::Lerp(SharedAt(P.Y), SharedAt(P.X), ArmB) : SharedAt(P.Y); }
				const double Own = W > 0.0 ? Signed(P * K + FVector(3.1, 7.7, 1.3) + Offset, OwnSeed) : 0.0;
				return FMath::Sqrt(FMath::Max(0.0, 1.0 - W)) * Sh + FMath::Sqrt(W) * Own;
			}

			/** Positive bulges the surface out: the shears' wander, and their dips (away from the joints). */
			double Wander(const FVector& P) const
			{
				double D = (S.bYew ? 0.008 : 0.005) * Field(P, Seed, SharedSeed, FVector::ZeroVector);
				for (const FDip& Dip : Dips)
				{
					const double T = 1.0 - (P - Dip.C).SizeSquared() / (Dip.R * Dip.R);
					if (T > 0.0) { D -= Dip.Depth * T * T; }
				}
				return D;
			}

			/** Where the season's new growth comes in tufts (0 to 1), a hand across. */
			double Tufts(const FVector& P) const
			{
				return FMath::Clamp(0.5 + 0.9 * Field(P, Seed + 57, SharedSeed + 57, FVector(5.3, 1.9, 8.1)), 0.0, 1.0);
			}

			double Sdf(const FVector& P) const { return Base(P) - Wander(P); }

			FVector Gradient(const FVector& P) const
			{
				constexpr double H = 0.002;
				const FVector G(Sdf(P + FVector(H, 0, 0)) - Sdf(P - FVector(H, 0, 0)), Sdf(P + FVector(0, H, 0)) - Sdf(P - FVector(0, H, 0)),
								Sdf(P + FVector(0, 0, H)) - Sdf(P - FVector(0, 0, H)));
				return G.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
			}

			/** Where leaves may grow (their centres): the module's own piece of hedge. */
			FBox Footprint() const
			{
				const double Pad = 0.012, W = A() + Pad, H = S.Height + Pad;
				switch (S.Part)
				{
				case EPart::Run: return FBox(FVector(0.0, -W, 0.0), FVector(Length, W, H));
				case EPart::End: return FBox(FVector(-EndLength, -W, 0.0), FVector(Pad, W, H));
				case EPart::Corner: return FBox(FVector(-W, -W, 0.0), FVector(A(), A(), H));
				case EPart::Ball:
				default: return FBox(FVector(-S.Width - Pad, -S.Width - Pad, 0.0), FVector(S.Width + Pad, S.Width + Pad, 2.0 * S.Width - BallSink + Pad));
				}
			}
		};

		/** The project's front-face rule (FNatureMesh): cross(b − a, c − a) points away from the face's viewer. */
		void Triangle(FMuseeLawnPatch& Out, int32 A, int32 B, int32 C, const FVector3f& Facing)
		{
			const FVector3f& PA = Out.Positions[A];
			const FVector3f X = FVector3f::CrossProduct(Out.Positions[B] - PA, Out.Positions[C] - PA);
			if (FVector3f::DotProduct(X, Facing) < 0) { Out.Indices.Append({A, B, C}); }
			else { Out.Indices.Append({A, C, B}); }
		}

		int32 Vertex(FMuseeLawnPatch& Out, const FVector& At, const FVector& Normal, const FVector& Tangent, const FVector2f& UV, const FLinearColor& C)
		{
			const int32 Index = Out.Positions.Num();
			Out.Positions.Add(FVector3f(At * 100.0));
			Out.Normals.Add(FVector3f(Normal.GetSafeNormal()));
			Out.Tangents.Add(FVector3f(Tangent));
			Out.UVs.Add(UV);
			Out.Colours.Add(C);
			return Index;
		}

		/** The leaf's (or needle's) points in its own frame: along A from the stalk (t), across B (w), up N (h); and its UV. */
		struct FLeafPoint
		{
			double T, W, H;
			FVector2f UV;
		};

		void LeafShape(bool bNeedle, double Cup, TArray<FLeafPoint, TInlineAllocator<8>>& Out)
		{
			Out.Reset();
			if (bNeedle)
			{
				// Flat, keeled, pointed: stalk, the two edges half way, the tip; the keel's middle raised.
				Out.Add({0.0, 0.0, 0.0, FVector2f(0.5f, 0.f)});
				Out.Add({0.5, -0.5, 0.0, FVector2f(0.f, 0.5f)});
				Out.Add({1.0, 0.0, 0.0, FVector2f(0.5f, 1.f)});
				Out.Add({0.5, 0.5, 0.0, FVector2f(1.f, 0.5f)});
				Out.Add({0.5, 0.0, Cup, FVector2f(0.5f, 0.5f)});
				return;
			}
			// Box: elliptic-oblong, widest a little below the middle, the tip blunt; the margins rolled under (the midrib
			// stands Cup above them).
			Out.Add({0.0, 0.0, Cup * 0.35, FVector2f(0.5f, 0.f)});
			Out.Add({0.28, -0.46, 0.0, FVector2f(0.04f, 0.28f)});
			Out.Add({0.72, -0.40, 0.0, FVector2f(0.1f, 0.72f)});
			Out.Add({1.0, 0.0, Cup * 0.2, FVector2f(0.5f, 1.f)});
			Out.Add({0.72, 0.40, 0.0, FVector2f(0.9f, 0.72f)});
			Out.Add({0.28, 0.46, 0.0, FVector2f(0.96f, 0.28f)});
			Out.Add({0.45, 0.0, Cup, FVector2f(0.5f, 0.45f)});
		}

		struct FPalette
		{
			FLinearColor Mature, New, Old, Yellowed, Twig;
			double NewShare;
		};

		const FPalette& Palette(bool bYew)
		{
			// Box: glossy dark green, the summer's flush a lighter yellow-green at the skin, the inner leaves duller and a few
			// yellowed; grey-brown twigs. Yew: near-black green, bright new shoots, red-brown twigs.
			// (Box reads olive beside the lawn in photographs of parterres: R/G about 0.55 in the old leaves, 0.75 in the new.)
			static const FPalette Box{MN::Srgb(0x3B5222), MN::Srgb(0x667A2B), MN::Srgb(0x454F2C), MN::Srgb(0x8A7F3A), MN::Srgb(0x5A5139), 0.26};
			static const FPalette Yew{MN::Srgb(0x233A19), MN::Srgb(0x566F28), MN::Srgb(0x2F3C22), MN::Srgb(0x7A6A34), MN::Srgb(0x5A3E2C), 0.3};
			return bYew ? Yew : Box;
		}
	}

	void BuildModule(int32 Index, FMuseeLawnPatch& Out)
	{
		Out = FMuseeLawnPatch();
		EKind Kind;
		int32 Variant;
		DecodeMeshIndex(Index, Kind, Variant);
		const FSpec& S = Spec(Kind);
		const bool bNeedle = S.bYew;
		FRandomStream Rng(7919 + int32(Kind) * 1543 + Variant * 211);
		// The wander shared at the joints: one per hedge (box, the exedra's yew, the narrow yew).
		const int32 Shared = S.bYew ? (S.Width > 0.6 ? 402 : 403) : 401;
		FShape Shape(S, 1000 + int32(Kind) * 37 + Variant * 7, Shared);
		const FBox Box = Shape.Footprint();

		// A dip or two where the shears bit deeper (away from the joints).
		{
			const int32 NumDips = Rng.RandRange(0, 2);
			for (int32 k = 0; k < NumDips; ++k)
			{
				for (int32 Try = 0; Try < 200; ++Try)
				{
					FVector P(Rng.FRandRange(Box.Min.X, Box.Max.X), Rng.FRandRange(Box.Min.Y, Box.Max.Y), Rng.FRandRange(0.25 * S.Height, Box.Max.Z));
					if (S.Part == EPart::Run && (P.X < 0.2 * Length || P.X > 0.8 * Length)) { continue; }
					if (S.Part == EPart::End && P.X < -0.5 * EndLength) { continue; }
					const double D = Shape.Base(P);
					if (FMath::Abs(D) > 0.01) { continue; }
					const double Scale = S.bYew ? 1.6 : 1.0;
					Shape.Dips.Add({P, Rng.FRandRange(0.05, 0.1) * Scale, Rng.FRandRange(0.008, 0.02) * Scale});
					break;
				}
			}
		}

		const double Depth = S.Inset + 0.006;               // leaves down to just inside the core's surface
		const double Lambda = S.bYew ? 0.024 : 0.018;       // most at the skin
		const double PerVolume = S.Density / (Lambda * (1.0 - FMath::Exp(-Depth / Lambda)));
		const double Volume = Box.GetVolume();
		const int64 Candidates = int64(PerVolume * Volume);
		const FPalette& Pal = Palette(S.bYew);
		const FVector Up = FVector::UpVector;
		TArray<FLeafPoint, TInlineAllocator<8>> Pts;

		double SurfaceArea = 0.0;   // an estimate (the shell's candidates over its depth), for the twigs
		for (int64 c = 0; c < Candidates; ++c)
		{
			FVector P(Rng.FRandRange(Box.Min.X, Box.Max.X), Rng.FRandRange(Box.Min.Y, Box.Max.Y), Rng.FRandRange(Box.Min.Z, Box.Max.Z));
			const double D0 = Shape.Sdf(P);
			if (D0 > 0.0 || D0 < -Depth) { continue; }
			SurfaceArea += 1.0 / (PerVolume * Depth);
			// Deeper down, fewer; near the ground the plants are leggy and shaded.
			double Keep = FMath::Exp(D0 / Lambda);
			Keep *= 0.3 + 0.7 * Smooth(0.0, S.bYew ? 0.25 : 0.12, P.Z);
			if (Rng.FRand() > Keep) { continue; }

			const FVector G = Shape.Gradient(P);
			const double DepthFrac = FMath::Clamp(-D0 / Depth, 0.0, 1.0);
			// Turned out to the light (and a little up), more at random deeper in.
			FVector N = (G + Up * (S.bYew ? 0.2 : 0.3) + Rng.GetUnitVector() * (0.5 + 0.7 * DepthFrac)).GetSafeNormal(UE_SMALL_NUMBER, G);
			if (FVector::DotProduct(N, G) < -0.2) { N = -N; }
			FVector Axis = Rng.GetUnitVector();
			// The twigs grow outwards: tips lean out rather than in.
			Axis = (Axis + G * 0.6).GetSafeNormal();
			Axis = (Axis - N * FVector::DotProduct(Axis, N)).GetSafeNormal(UE_SMALL_NUMBER, FVector::CrossProduct(N, FVector::ForwardVector));
			if (Axis.IsNearlyZero()) { continue; }
			const FVector Across = FVector::CrossProduct(N, Axis).GetSafeNormal();

			const bool bSkin = -D0 < (S.bYew ? 0.016 : 0.011);
			// The new growth comes in tufts a hand across (where the shears last missed), not evenly.
			const double Tufts = Smooth(0.3, 0.75, Shape.Tufts(P));
			const double NewShare = Pal.NewShare * (1.0 + 0.9 * FMath::Max(0.0, G.Z)) * FMath::Lerp(0.25, 2.0, Tufts);
			const bool bNew = bSkin && Rng.FRand() < NewShare;
			const bool bOld = !bSkin && Rng.FRand() < 0.35 + 0.4 * DepthFrac;
			const bool bYellow = !bSkin && Rng.FRand() < 0.012;
			double Len, Wid;
			if (bNeedle)
			{
				Len = Rng.FRandRange(0.016, 0.026) * (bNew ? 0.85 : 1.0);
				Wid = Rng.FRandRange(0.0024, 0.0031);
			}
			else
			{
				Len = Rng.FRandRange(0.011, 0.019) * (bNew ? 0.85 : 1.0);
				Wid = Len * Rng.FRandRange(0.46, 0.6);
			}
			const double Cup = bNeedle ? 0.0005 : Wid * Rng.FRandRange(0.1, 0.2);
			LeafShape(bNeedle, Cup, Pts);

			// The leaf about its point P (the middle of its length), then pushed back inside the clipped surface.
			FVector Base = P - Axis * (0.5 * Len);
			auto PointAt = [&](const FLeafPoint& L) { return Base + Axis * (L.T * Len) + Across * (L.W * Wid) + N * L.H; };
			for (int32 Pass = 0; Pass < 3; ++Pass)
			{
				double Worst = -1.0;
				for (const FLeafPoint& L : Pts) { Worst = FMath::Max(Worst, Shape.Sdf(PointAt(L))); }
				const double Tolerance = 0.0015;
				if (Worst <= Tolerance) { break; }
				Base -= G * (Worst - Tolerance + 0.0003);
			}
			if ((Base + Axis * (0.5 * Len)).Z < 0.004) { continue; }
			const double D = -Shape.Sdf(Base + Axis * (0.5 * Len));
			const double Occ = 1.0 - 0.78 * FMath::Pow(Smooth(0.0, Depth, D), 0.8) - 0.12 * (1.0 - Smooth(0.0, 0.15, P.Z));

			const FLinearColor Colour0 = bYellow ? Pal.Yellowed : bNew ? Pal.New : bOld ? Pal.Old : Pal.Mature;
			FLinearColor C = MN::Vary(Colour0, Rng, 5.0, 0.12, 0.16);
			C.A = static_cast<float>(FMath::Clamp(Occ, 0.12, 1.0));

			// Normals: the midrib's up, the margins rolled away from it (a convex, glossy leaf in the light).
			const int32 First = Out.Positions.Num();
			for (int32 i = 0; i < Pts.Num(); ++i)
			{
				const FLeafPoint& L = Pts[i];
				FVector Nv = N;
				if (!bNeedle)
				{
					Nv = N + Across * (L.W * 1.3) + Axis * (L.T < 0.1 ? -0.35 : L.T > 0.9 ? 0.35 : 0.0);
				}
				else
				{
					Nv = N + Across * (L.W * 0.8);
				}
				Vertex(Out, PointAt(L), Nv, Axis, L.UV, C);
			}
			const FVector3f Facing(N);
			const int32 Centre = First + Pts.Num() - 1;
			const int32 Rim = Pts.Num() - 1;
			for (int32 i = 0; i < Rim; ++i) { Triangle(Out, Centre, First + i, First + (i + 1) % Rim, Facing); }
			++Out.Blades;
		}

		// Twigs from the dark core out to the skin: what shows in the gaps.
		{
			const double TwigDensity = S.bYew ? 900.0 : 1300.0;
			const int32 NumTwigs = FMath::RoundToInt32(TwigDensity * SurfaceArea);
			int32 Made = 0;
			for (int32 Try = 0; Try < NumTwigs * 400 && Made < NumTwigs; ++Try)
			{
				const FVector P(Rng.FRandRange(Box.Min.X, Box.Max.X), Rng.FRandRange(Box.Min.Y, Box.Max.Y), Rng.FRandRange(Box.Min.Z, Box.Max.Z));
				const double D0 = Shape.Sdf(P);
				if (D0 > 0.0 || D0 < -0.004 || P.Z < 0.03) { continue; }
				const FVector G = Shape.Gradient(P);
				const FVector Tip = P - G * Rng.FRandRange(0.01, 0.028);
				const FVector Foot = P - G * (S.Inset + 0.004) + Rng.GetUnitVector() * 0.015;
				const FVector Dir = (Tip - Foot).GetSafeNormal();
				FVector U, W;
				MN::Basis(Dir, U, W);
				const double R0 = S.bYew ? 0.0022 : 0.0017, R1 = R0 * 0.5;
				FLinearColor C = MN::Vary(Pal.Twig, Rng, 6.0, 0.15, 0.15);
				int32 Ring[2][3];
				for (int32 e = 0; e < 2; ++e)
				{
					const FVector At = e ? Tip : Foot;
					const double R = e ? R1 : R0;
					C.A = e ? 0.55f : 0.2f;
					for (int32 k = 0; k < 3; ++k)
					{
						const double Ang = MN::Tau * k / 3.0;
						const FVector Radial = U * FMath::Cos(Ang) + W * FMath::Sin(Ang);
						Ring[e][k] = Vertex(Out, At + Radial * R, Radial, Dir, FVector2f(0.f, 0.f), C);
					}
				}
				for (int32 k = 0; k < 3; ++k)
				{
					const int32 K1 = (k + 1) % 3;
					const double Ang = MN::Tau * (k + 0.5) / 3.0;
					const FVector3f Facing(U * FMath::Cos(Ang) + W * FMath::Sin(Ang));
					Triangle(Out, Ring[0][k], Ring[0][K1], Ring[1][K1], Facing);
					Triangle(Out, Ring[0][k], Ring[1][K1], Ring[1][k], Facing);
				}
				++Made;
			}
		}

		// Bent round a centre Bend to the left: x along the midline's arc, y towards the centre; the ends stay radial.
		if (S.Bend > 0.0)
		{
			const double R = S.Bend * 100.0;
			for (int32 i = 0; i < Out.Positions.Num(); ++i)
			{
				const FVector3f P = Out.Positions[i];
				const double Phi = P.X / R, Radius = R - P.Y;
				const float C = static_cast<float>(FMath::Cos(Phi)), Sn = static_cast<float>(FMath::Sin(Phi));
				Out.Positions[i] = FVector3f(static_cast<float>(Radius * Sn), static_cast<float>(R - Radius * C), P.Z);
				auto Turn = [C, Sn](const FVector3f& V) { return FVector3f(V.X * C - V.Y * Sn, V.X * Sn + V.Y * C, V.Z); };
				Out.Normals[i] = Turn(Out.Normals[i]);
				Out.Tangents[i] = Turn(Out.Tangents[i]);
			}
		}
	}
}
