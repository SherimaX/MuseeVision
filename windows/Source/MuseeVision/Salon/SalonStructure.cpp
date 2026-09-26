#include "Salon/SalonStructure.h"

#include "Salon/SalonKit.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Geometry/MuseeBake.h"
#include "Nature/NatureMesh.h"

/**
 * Everything is built in plan metres (x east, plan y south, height up; the Rotunda's centre is the origin), from
 * Plan.Salon and Plan.Oval (Shared/Plan/MuseumPlan.swift), Shared/Wings/Salon.swift, PondPlan (Shared/Wings/Reserve.swift)
 * and the Salon boards. The names below are prefixed k (constants) so no local can hide them.
 */
namespace SalonBuild
{
	using SalonKit::Cross2;
	using SalonKit::FFrame;
	using SalonKit::FMeshData;
	using SalonKit::FProfile;

	constexpr double kPi = UE_DOUBLE_PI;
	constexpr double kSink = 0.002;   // how far a face that stops runs on into the solid it meets

	// ---------------------------------------------------------------- The Salon
	constexpr double kEndX = -14.0, kFarX = -74.0;           // inner faces of Bay 1's end wall and Bay 5's far wall
	constexpr double kEndOutX = -13.4, kFarOutX = -74.6;     // their outer faces (walls 0.6 m)
	constexpr double kHalf = 7.0, kOutHalf = 7.6;            // long walls: inner and outer faces
	constexpr double kSpring = 6.5, kVaultR = 7.0;           // the vault springs at 6.5 m; crown 13.5 m
	constexpr double kIntrados = 4.5, kPierHalf = 0.6;       // transverse arches: 9 m clear; piers 1.2 m thick, 2.5 m deep
	constexpr double kPierX[4] = {-26.0, -38.0, -50.0, -62.0};

	struct FBaySpan
	{
		double West;
		double East;
	};
	constexpr FBaySpan kBays[5] = {{-25.4, -14.0}, {-37.4, -26.6}, {-49.4, -38.6}, {-61.4, -50.6}, {-74.0, -62.6}};

	constexpr double kDoorHalf = 1.5, kDoorSpring = 3.3;     // the doors: 3 m, round-arched, springing 3.3 m
	constexpr int32 kDoorSegs = 64;                          // per semicircle
	constexpr double kSkirtEnd = kDoorHalf + 0.18;           // a skirting stops inside the door's architrave
	constexpr double kArchitraveOut = kDoorHalf + 0.22;      // the architrave's outer edge
	constexpr double kCabinetDoorX = -20.0;                  // salon data 6 (opening 4.5–7.5)
	constexpr double kSilkBottom = 0.15, kSilkTop = 6.25;    // the silk runs from the skirting's slot up behind the cornice

	// Light slots (m from the wall, height).
	constexpr double kCoveP = 0.255, kCoveZ = 6.27, kCoveWidth = 0.51;
	constexpr double kTopSlotP = 0.02, kTopSlotZ = 5.62, kTopSlotWidth = 0.04;
	constexpr double kBaseSlotP = 0.015, kBaseSlotZ = 0.155, kBaseSlotWidth = 0.03;
	constexpr double kCabinetTopSlotZ = 5.12;

	// ---------------------------------------------------------------- The vault and its lanterns
	constexpr int32 kRows = 19, kRowSub = 6, kVaultSegs = kRows * kRowSub;   // 114 stations per semicircle
	constexpr int32 kCols = 8, kLanternRow = 9, kLanternCol = 3;             // the lantern: crown row, columns 3–4
	constexpr double kRoofBottom = 13.85, kRoofTop = 14.05;
	constexpr double kGlassZ = 13.58, kBarDepth = 0.07, kFrameW = 0.04, kBarHalf = 0.015;
	constexpr double kCofferInset[7] = {0.0, 0.10, 0.115, 0.175, 0.19, 0.24, 0.28};
	constexpr double kCofferDepth[7] = {0.0, 0.0, 0.08, 0.08, 0.16, 0.16, 0.24};

	// ---------------------------------------------------------------- The Manet cabinet (Plan.Salon.Cabinet)
	constexpr double kCabE = -16.0, kCabW = -24.0, kCabN = -15.6, kCabS = -7.6;   // inner faces (S: the Salon's outer face)
	constexpr double kCabH = 5.5, kCabWall = 0.4, kCabRoof = 6.3, kCabCornice = 0.33;
	constexpr double kLayX = -20.0, kLayY = -11.6, kLayHalf = 2.5, kLayTop = 5.9;

	// ---------------------------------------------------------------- The Nymphéas oval (Plan.Oval, PondPlan)
	constexpr double kOvalX = -86.5, kOvalA = 11.0, kOvalB = 7.5, kOvalWall = 0.6;
	constexpr double kOvalBand = 5.0, kOvalRoof = 7.2;   // the plaster walls are 5 m; the cove rises above
	constexpr double kBlockHalf = 2.1, kBlockTop = 5.2;   // the masonry round the passage from Bay 5
	constexpr int32 kOvalSegs = 720;
	constexpr double kPondX0 = -90.4, kPondX1 = -82.9, kPondY = 1.2;   // PondPlan.openingRect: left open

	// ---------------------------------------------------------------- The interior update (proposals/salon-interior)
	// Silk on the piers: each bay's silk wraps onto the faces of its piers, corner to corner, a 0.14 m stone edge left at
	// each jamb; an upholstered panel 12 mm proud (silk over felt on battens) from the base's top to the impost's underside.
	constexpr double kSilkJambEdge = 0.14, kSilkProud = 0.012, kPierSilkZ0 = 0.30, kPierSilkZ1 = 6.06;
	// The transverse arches: fifteen voussoirs (V-jointed: each arris chamfered 8 mm at 45°) out to the extrados, a
	// keystone proud by 20 mm with a flat top, an archivolt moulding round the extrados. Each stone carries its own tone in
	// its vertex colour (R tone, G polish; B 1 on the joints' chamfers: dirt and shadow in the joint) for M_Salon_Ashlar.
	constexpr int32 kVoussoirs = 15, kKeystone = 7;
	constexpr double kRingOut = 5.30, kKeyTop = 5.80, kKeyProud = 0.020, kVJoint = 0.012;
	// The oak/stone strips: brushed bronze, 12 mm wide, flush (0.5 mm proud, as the thresholds).
	constexpr double kStripHalf = 0.006;
	// Sealed junctions: where the vault meets the arches, the lunettes and the lantern shafts, one surface runs on behind
	// the other (a baked Nanite mesh can open a hairline where two edges only touch, and the sky showed through it).
	constexpr double kSeal = 0.03, kSealIn = 0.004;
	// Bay 4's muslin: stretched on a frame at the shaft's mouth, sagging 3 cm; the sky panels in the shafts over the glass.
	constexpr double kMuslinZ = 13.490, kMuslinSag = 0.030, kVeilZ = 13.95;
	// Bay 3's pergola on the roof: 6 × 5.2 m, its trellis 2.4 m over the roof.
	constexpr double kPergolaHalfX = 3.0, kPergolaHalfY = 2.6, kPergolaTop = kRoofTop + 2.42;

	// ================================================================ Small helpers

	TArray<double> SortedUnique(TArray<double> Values, double Tolerance = 1e-7)
	{
		Values.Sort();
		TArray<double> Result;
		for (double V : Values)
		{
			if (Result.Num() == 0 || V - Result.Last() > Tolerance) { Result.Add(V); }
		}
		return Result;
	}

	inline double VA(int32 J) { return kPi * J / kVaultSegs; }

	/** A point on the barrel at angle A (0 at the north springing, pi at the south) and radius Rad. */
	inline FVector VP(double X, double A, double Rad) { return FVector(X, -Rad * FMath::Cos(A), kSpring + Rad * FMath::Sin(A)); }

	inline FVector2D VaultUV(const FVector& P) { return FVector2D(P.X, kVaultR * FMath::Atan2(P.Z - kSpring, -P.Y)); }

	inline FVector TowardAxis(const FVector& P) { return FVector(0.0, -P.Y, kSpring - P.Z); }

	inline double CofferPitch() { return kPi * kVaultR / kRows; }

	/** The plain band of vault next to each arch (so bays 2–4 get square coffers). */
	inline double StripWidth() { return 0.5 * (10.8 - kCols * CofferPitch()); }

	/** The x of coffer column line C (0…kCols) in a bay. */
	inline double ColumnX(const FBaySpan& Bay, int32 C)
	{
		const double Strip = StripWidth();
		return Bay.West + Strip + (Bay.East - Bay.West - 2.0 * Strip) * C / kCols;
	}

	inline void DoorStation(int32 K, double& L, double& Z)
	{
		const double Phi = kPi * K / kDoorSegs;
		L = kDoorHalf * FMath::Cos(Phi);
		Z = kDoorSpring + kDoorHalf * FMath::Sin(Phi);
	}

	inline double DoorHead(double L) { return kDoorSpring + FMath::Sqrt(FMath::Max(0.0, kDoorHalf * kDoorHalf - L * L)); }

	/** A vertical wall plane: At(u, z) = Origin + U u + z up; N faces the room it bounds. */
	struct FWall
	{
		FVector Origin = FVector::ZeroVector;
		FVector U = FVector::ForwardVector;
		FVector N = FVector::RightVector;

		FVector At(double Along, double Z) const { return Origin + U * Along + FVector(0.0, 0.0, Z); }
	};

	FWall MakeWall(double OX, double OY, const FVector& Along, const FVector& Facing)
	{
		FWall W;
		W.Origin = FVector(OX, OY, 0.0);
		W.U = Along;
		W.N = Facing;
		return W;
	}

	struct FHole
	{
		double U0, U1, Z0, Z1;
	};

	/**
	 * The face of a wall over [U0, U1] × [Z0, Z1], less round-arched doors (centred at Doors, from the floor) and
	 * rectangular holes. Columns stand at every arch station, so the door's reveal meets the face vertex for vertex.
	 */
	void WallFace(FMeshData& M, const FWall& W, double U0, double U1, double Z0, double Z1, const TArray<double>& Doors,
				  const TArray<FHole>& Holes = TArray<FHole>())
	{
		TArray<double> Raw = {U0, U1};
		for (double C : Doors)
		{
			for (int32 K = 0; K <= kDoorSegs; ++K)
			{
				double L = 0.0, Z = 0.0;
				DoorStation(K, L, Z);
				Raw.Add(C + L);
			}
		}
		for (const FHole& H : Holes)
		{
			Raw.Add(H.U0);
			Raw.Add(H.U1);
		}
		TArray<double> Us;
		for (double V : SortedUnique(Raw))
		{
			if (V >= U0 - 1e-9 && V <= U1 + 1e-9) { Us.Add(V); }
		}
		for (int32 i = 0; i + 1 < Us.Num(); ++i)
		{
			const double A = Us[i], B = Us[i + 1], Mid = 0.5 * (A + B);
			double BotA = Z0, BotB = Z0;
			for (double C : Doors)
			{
				if (FMath::Abs(Mid - C) < kDoorHalf)
				{
					BotA = FMath::Max(Z0, DoorHead(A - C));
					BotB = FMath::Max(Z0, DoorHead(B - C));
				}
			}
			const FHole* Hit = nullptr;
			for (const FHole& H : Holes)
			{
				if (Mid > H.U0 && Mid < H.U1) { Hit = &H; }
			}
			auto Span = [&](double LoA, double LoB, double Hi)
			{
				if (Hi - FMath::Max(LoA, LoB) < 1e-7) { return; }
				M.Rect(W.At(A, LoA), W.At(B, LoB), W.At(B, Hi), W.At(A, Hi), W.N);
			};
			if (Hit)
			{
				if (Hit->Z0 > Z0 + 1e-7) { Span(Z0, Z0, FMath::Min(Hit->Z0, Z1)); }
				if (Hit->Z1 < Z1 - 1e-7)
				{
					const double Lo = FMath::Max(Hit->Z1, Z0);
					Span(Lo, Lo, Z1);
				}
			}
			else
			{
				Span(BotA, BotB, Z1);
			}
		}
	}

	/**
	 * The reveal of a round-arched door: jambs and soffit from Inner(l, z) to Outer(l, z), l across the door along
	 * Lat. Its stations are the wall faces' door columns, so they meet vertex for vertex.
	 */
	void Tunnel(FMeshData& M, TFunctionRef<FVector(double, double)> Inner, TFunctionRef<FVector(double, double)> Outer, const FVector& Lat)
	{
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double L = Side * kDoorHalf;
			M.Rect(Inner(L, -kSink), Outer(L, -kSink), Outer(L, kDoorSpring), Inner(L, kDoorSpring), -Lat * Side);
		}
		const int32 Base = M.Positions.Num();
		for (int32 K = 0; K <= kDoorSegs; ++K)
		{
			double L = 0.0, Z = 0.0;
			DoorStation(K, L, Z);
			const double Phi = kPi * K / kDoorSegs;
			const FVector Nrm = -(Lat * FMath::Cos(Phi) + FVector::UpVector * FMath::Sin(Phi));
			const FVector PA = Inner(L, Z), PB = Outer(L, Z);
			M.Vertex(PA, Nrm, FVector2D(0.0, kDoorHalf * Phi));
			M.Vertex(PB, Nrm, FVector2D(FVector::Dist(PA, PB), kDoorHalf * Phi));
		}
		for (int32 K = 0; K < kDoorSegs; ++K)
		{
			const int32 A = Base + 2 * K;
			M.Quad(A, A + 1, A + 3, A + 2);
		}
	}

	/**
	 * Frames round a door's opening for its architrave: up the left jamb (l = -1.5), over the arch, down the right.
	 * A runs out from the opening in the wall's surface, B out of the wall (SurfaceN).
	 */
	TArray<FFrame> DoorFrames(TFunctionRef<FVector(double, double)> Edge, TFunctionRef<FVector(double)> SurfaceN, TFunctionRef<FVector(double)> Tangent)
	{
		TArray<FFrame> Run;
		double S = 0.0;
		auto Push = [&](double L, double Z, const FVector& D)
		{
			FFrame F;
			F.Origin = Edge(L, Z);
			if (Run.Num() > 0) { S += FVector::Dist(Run.Last().Origin, F.Origin); }
			F.AxisA = F.NormA = D;
			F.AxisB = F.NormB = SurfaceN(L);
			F.S = S;
			Run.Add(F);
		};
		Push(-kDoorHalf, -kSink, -Tangent(-kDoorHalf));
		for (int32 K = kDoorSegs; K >= 0; --K)
		{
			double L = 0.0, Z = 0.0;
			DoorStation(K, L, Z);
			const double Phi = kPi * K / kDoorSegs;
			Push(L, Z, Tangent(L) * FMath::Cos(Phi) + FVector::UpVector * FMath::Sin(Phi));
		}
		Push(kDoorHalf, -kSink, Tangent(kDoorHalf));
		return Run;
	}

	TArray<FVector2D> CatmullRom(const TArray<FVector2D>& Pts, int32 Sub)
	{
		TArray<FVector2D> Result;
		const int32 N = Pts.Num();
		auto P = [&](int32 i) -> FVector2D
		{
			if (i < 0) { return Pts[0] * 2.0 - Pts[1]; }
			if (i >= N) { return Pts[N - 1] * 2.0 - Pts[N - 2]; }
			return Pts[i];
		};
		for (int32 i = 0; i + 1 < N; ++i)
		{
			const FVector2D A = P(i - 1), B = P(i), C = P(i + 1), D = P(i + 2);
			for (int32 s = 0; s < Sub; ++s)
			{
				const double T = double(s) / Sub, T2 = T * T, T3 = T2 * T;
				Result.Add((B * 2.0 + (C - A) * T + (A * 2.0 - B * 5.0 + C * 4.0 - D) * T2 + (B * 3.0 - A - C * 3.0 + D) * T3) * 0.5);
			}
		}
		Result.Add(Pts.Last());
		return Result;
	}

	double PathLength(const TArray<FVector2D>& Pts)
	{
		double L = 0.0;
		for (int32 i = 0; i + 1 < Pts.Num(); ++i) { L += FVector2D::Distance(Pts[i], Pts[i + 1]); }
		return L;
	}

	/** The part of a polyline between arc lengths S0 and S1. */
	TArray<FVector2D> SubPath(const TArray<FVector2D>& Pts, double S0, double S1)
	{
		TArray<FVector2D> Result;
		double Acc = 0.0;
		for (int32 i = 0; i + 1 < Pts.Num(); ++i)
		{
			const double L = FVector2D::Distance(Pts[i], Pts[i + 1]);
			const double A = Acc, B = Acc + L;
			Acc = B;
			if (L < 1e-12 || B <= S0 || A >= S1) { continue; }
			if (Result.Num() == 0) { Result.Add(FMath::Lerp(Pts[i], Pts[i + 1], (FMath::Max(S0, A) - A) / L)); }
			if (B < S1 - 1e-9) { Result.Add(Pts[i + 1]); }
			else
			{
				Result.Add(FMath::Lerp(Pts[i], Pts[i + 1], (S1 - A) / L));
				break;
			}
		}
		return Result;
	}

	/** Andrew's monotone chain: the convex hull, counter-clockwise (x right, y up). */
	TArray<FVector2D> ConvexHull(TArray<FVector2D> Pts)
	{
		Pts.Sort([](const FVector2D& A, const FVector2D& B) { return A.X < B.X || (A.X == B.X && A.Y < B.Y); });
		TArray<FVector2D> H;
		for (const FVector2D& Q : Pts)
		{
			while (H.Num() >= 2 && Cross2(H[H.Num() - 1] - H[H.Num() - 2], Q - H[H.Num() - 2]) <= 0.0) { H.RemoveAt(H.Num() - 1); }
			H.Add(Q);
		}
		const int32 LowerCount = H.Num() + 1;
		for (int32 i = Pts.Num() - 2; i >= 0; --i)
		{
			const FVector2D& Q = Pts[i];
			while (H.Num() >= LowerCount && Cross2(H[H.Num() - 1] - H[H.Num() - 2], Q - H[H.Num() - 2]) <= 0.0) { H.RemoveAt(H.Num() - 1); }
			H.Add(Q);
		}
		H.RemoveAt(H.Num() - 1);
		return H;
	}

	/** Sutherland–Hodgman: a polygon clipped by a convex, counter-clockwise one. */
	TArray<FVector2D> ClipConvex(TArray<FVector2D> Poly, const TArray<FVector2D>& Clip)
	{
		for (int32 e = 0; e < Clip.Num() && Poly.Num() > 0; ++e)
		{
			const FVector2D A = Clip[e], B = Clip[(e + 1) % Clip.Num()];
			const TArray<FVector2D> In = Poly;
			Poly.Reset();
			for (int32 i = 0; i < In.Num(); ++i)
			{
				const FVector2D P = In[i], Q = In[(i + 1) % In.Num()];
				const double DP = Cross2(B - A, P - A), DQ = Cross2(B - A, Q - A);
				if (DP >= 0.0) { Poly.Add(P); }
				if ((DP >= 0.0) != (DQ >= 0.0)) { Poly.Add(P + (Q - P) * (DP / (DP - DQ))); }
			}
		}
		return Poly;
	}

	// ================================================================ Profiles (metres)

	/** The Salon's cornice at the springing (A out of the wall, B up): a slot for the silk's light, an ovolo, the corona,
	 * a cyma and the lip that hides the cove light; behind it the trough, open to the vault. Ends on the springing. */
	FProfile SalonCornice()
	{
		FProfile P;
		P.Add(0.000, 5.625).Add(0.040, 5.625).Add(0.040, 5.600).Add(0.075, 5.600).Add(0.075, 5.800).Add(0.095, 5.800).Add(0.095, 5.825)
			.Arc(0.095, 5.905, 0.080, 0.080, -90.0, 0.0, 8)
			.Add(0.175, 5.925).Add(0.195, 5.925).Add(0.195, 5.945).Add(0.470, 5.945).Add(0.470, 6.150).Add(0.490, 6.150).Add(0.490, 6.180)
			.Arc(0.490, 6.280, 0.040, 0.100, -90.0, 0.0, 8).SmoothLast()
			.Arc(0.570, 6.280, 0.040, 0.100, 180.0, 90.0, 8)
			.Add(0.585, 6.380).Add(0.585, 6.470).Add(0.510, 6.470).Add(0.510, 6.250).Add(0.000, 6.250).Add(0.000, kSpring);
		return P;
	}

	/** The Salon's skirting, with a slot along the silk for a strip of light. */
	FProfile SalonSkirting()
	{
		FProfile P;
		P.Add(0.060, -kSink).Add(0.060, 0.160).Add(0.040, 0.180).Add(0.030, 0.180).Add(0.030, 0.150).Add(0.000, 0.150);
		return P;
	}

	/** A pier's base: plinth, torus, fillet (A out of the pier face). */
	FProfile PierBase()
	{
		FProfile P;
		P.Add(0.100, -kSink).Add(0.100, 0.160).Add(0.060, 0.160)
			.Arc(0.060, 0.200, 0.040, 0.040, -90.0, 90.0, 12)
			.Add(0.045, 0.240).Add(0.030, 0.270).Add(0.030, 0.300).Add(0.000, 0.300);
		return P;
	}

	/** A pier's impost at the springing: fillet, ovolo, abacus. */
	FProfile PierImpost()
	{
		FProfile P;
		P.Add(0.000, 6.060).Add(0.020, 6.060).Add(0.020, 6.085)
			.Arc(0.020, 6.140, 0.055, 0.055, -90.0, 0.0, 8)
			.Add(0.075, 6.160).Add(0.085, 6.160).Add(0.085, 6.400).Add(0.100, 6.400).Add(0.100, kSpring).Add(0.000, kSpring);
		return P;
	}

	/** An arch's archivolt (A out from the intrados in the arch's face, B out of the face): a fascia and a cavetto. */
	FProfile Archivolt()
	{
		FProfile P;
		P.Add(0.420, -kSink).Add(0.420, 0.012).Add(0.400, 0.012)
			.Arc(0.400, 0.045, 0.040, 0.033, -90.0, -180.0, 8)
			.Add(0.020, 0.045).Add(0.000, 0.025).Add(0.000, 0.000);
		return P;
	}

	/** A door's architrave (A out from the opening, B out of the wall): two fasciae and a rounded back band. */
	FProfile Architrave()
	{
		FProfile P;
		P.Add(0.220, -kSink).Add(0.220, 0.090).Add(0.205, 0.090)
			.Arc(0.205, 0.070, 0.020, 0.020, 90.0, 180.0, 6)
			.Add(0.160, 0.070).Add(0.160, 0.064).Add(0.100, 0.058).Add(0.100, 0.052).Add(0.020, 0.046).Add(0.000, 0.030).Add(0.000, 0.000);
		return P;
	}

	/** The oval side's architrave: slimmer, so its top (4.96 m) stays under the cove. */
	FProfile SlimArchitrave()
	{
		FProfile P;
		P.Add(0.160, -kSink).Add(0.160, 0.075).Add(0.145, 0.075)
			.Arc(0.145, 0.058, 0.017, 0.017, 90.0, 180.0, 6)
			.Add(0.100, 0.058).Add(0.100, 0.052).Add(0.020, 0.046).Add(0.000, 0.030).Add(0.000, 0.000);
		return P;
	}

	/** The cabinet's cornice (clear of the door's architrave, top 5.02 m): a light slot, a fascia and a cove up to the
	 * ceiling, which starts kCabCornice out from the wall. */
	FProfile CabinetCornice()
	{
		FProfile P;
		P.Add(0.000, 5.125).Add(0.040, 5.125).Add(0.040, 5.100).Add(0.070, 5.100).Add(0.070, 5.180).Add(0.090, 5.180).Add(0.090, 5.200)
			.Arc(0.270, 5.200, 0.180, 0.180, 180.0, 90.0, 12)
			.Add(0.290, 5.380).Add(0.290, 5.420).Add(kCabCornice, 5.420).Add(kCabCornice, kCabH);
		return P;
	}

	/** The oval's cove: a fillet, then a deep quarter-ellipse up to a soffit ring and the velarium's reveal. */
	FProfile OvalCove()
	{
		FProfile P;
		P.Add(0.000, kOvalBand).Add(0.030, kOvalBand).Add(0.030, 5.050).Add(0.015, 5.070)
			.Arc(1.300, 5.070, 1.285, 1.130, 180.0, 90.0, 32)
			.Add(1.450, 6.200).Add(1.450, 6.350);
		return P;
	}

	FProfile OvalSkirting()
	{
		FProfile P;
		P.Add(0.018, -kSink).Add(0.018, 0.090).Add(0.008, 0.100).Add(-kSink, 0.100);
		return P;
	}

	/** A bench's slab (A across the bench, B up): 0.55 m wide, 0.10 m thick, chamfered top edges. Closed. */
	FProfile BenchSlab()
	{
		const double W = 0.275, C = 0.015;
		FProfile P;
		P.bClosed = true;
		P.Add(-W, 0.35).Add(W, 0.35).Add(W, 0.45 - C).Add(W - C, 0.45).Add(-W + C, 0.45).Add(-W, 0.45 - C);
		return P;
	}

	// ================================================================ The oval (inner face a 11 × b 7.5 m)

	inline FVector2D OvalIn(double T) { return FVector2D(kOvalX + kOvalA * FMath::Cos(T), kOvalB * FMath::Sin(T)); }

	/** The inner face's point at plan y = L on the east side (the door's side). */
	inline FVector2D OvalInAtY(double L)
	{
		const double Q = L / kOvalB;
		return FVector2D(kOvalX + kOvalA * FMath::Sqrt(FMath::Max(0.0, 1.0 - Q * Q)), L);
	}

	/** The outward unit normal of the inner face at P. */
	inline FVector2D OvalGrad(const FVector2D& P)
	{
		return FVector2D((P.X - kOvalX) / (kOvalA * kOvalA), P.Y / (kOvalB * kOvalB)).GetSafeNormal();
	}

	inline FVector2D OvalOut(double T)
	{
		const FVector2D P = OvalIn(T);
		return P + OvalGrad(P) * kOvalWall;
	}

	/** The parameter at which the outer face reaches y = kBlockHalf (east side). */
	double BlockT()
	{
		double Lo = 0.0, Hi = 0.5 * kPi;
		for (int32 i = 0; i < 80; ++i)
		{
			const double Mid = 0.5 * (Lo + Hi);
			if (OvalOut(Mid).Y < kBlockHalf) { Lo = Mid; }
			else { Hi = Mid; }
		}
		return 0.5 * (Lo + Hi);
	}

	/** Stations round the outer face: uniform, plus the passage block's edges. From -pi. */
	TArray<double> OuterStations()
	{
		const double TB = BlockT();
		TArray<double> Ts = {TB, -TB};
		for (int32 i = 0; i < kOvalSegs; ++i) { Ts.Add(-kPi + 2.0 * kPi * i / kOvalSegs); }
		return SortedUnique(Ts, 1e-9);
	}

	struct FOvalColumn
	{
		FVector2D P = FVector2D::ZeroVector;
		double Head = 0.0;
		bool bDoor = false;
	};

	/**
	 * The inner face's columns, a closed loop: from the door's south jamb round the room to its north jamb, then the
	 * door's arch stations back across (these carry the arch's head).
	 */
	TArray<FOvalColumn> OvalColumns()
	{
		TArray<FOvalColumn> Result;
		const double TJ = FMath::Asin(kDoorHalf / kOvalB);
		for (int32 i = 0; i <= kOvalSegs; ++i)
		{
			FOvalColumn C;
			if (i == 0 || i == kOvalSegs)
			{
				C.P = OvalInAtY(i == 0 ? kDoorHalf : -kDoorHalf);
				C.Head = kDoorSpring;
				C.bDoor = true;
			}
			else
			{
				C.P = OvalIn(TJ + (2.0 * kPi - 2.0 * TJ) * i / kOvalSegs);
			}
			Result.Add(C);
		}
		for (int32 K = kDoorSegs - 1; K >= 1; --K)
		{
			double L = 0.0, Z = 0.0;
			DoorStation(K, L, Z);
			FOvalColumn C;
			C.P = OvalInAtY(L);
			C.Head = Z;
			C.bDoor = true;
			Result.Add(C);
		}
		return Result;
	}

	const TArray<TArray<FVector2D>>& BenchLines()
	{
		// Plan.Oval.benches: the centrelines from the plan.
		static const TArray<TArray<FVector2D>> Lines = {
			{FVector2D(-91.58, 1.16), FVector2D(-91.40, 1.44), FVector2D(-91.18, 1.70), FVector2D(-90.92, 1.95), FVector2D(-90.64, 2.19), FVector2D(-90.32, 2.40),
			 FVector2D(-89.97, 2.60), FVector2D(-89.60, 2.79), FVector2D(-89.20, 2.94), FVector2D(-88.78, 3.08), FVector2D(-88.35, 3.20)},
			{FVector2D(-84.65, 3.20), FVector2D(-84.22, 3.08), FVector2D(-83.80, 2.94), FVector2D(-83.40, 2.79), FVector2D(-83.03, 2.60), FVector2D(-82.68, 2.40),
			 FVector2D(-82.36, 2.19), FVector2D(-82.08, 1.95), FVector2D(-81.82, 1.70), FVector2D(-81.60, 1.44), FVector2D(-81.42, 1.16)},
			{FVector2D(-81.42, -1.16), FVector2D(-81.60, -1.44), FVector2D(-81.82, -1.70), FVector2D(-82.08, -1.95), FVector2D(-82.36, -2.19), FVector2D(-82.68, -2.40),
			 FVector2D(-83.03, -2.60), FVector2D(-83.40, -2.79), FVector2D(-83.80, -2.94), FVector2D(-84.22, -3.08), FVector2D(-84.65, -3.20)},
			{FVector2D(-88.35, -3.20), FVector2D(-88.78, -3.08), FVector2D(-89.20, -2.94), FVector2D(-89.60, -2.79), FVector2D(-89.97, -2.60), FVector2D(-90.32, -2.40),
			 FVector2D(-90.64, -2.19), FVector2D(-90.92, -1.95), FVector2D(-91.18, -1.70), FVector2D(-91.40, -1.44), FVector2D(-91.58, -1.16)},
		};
		return Lines;
	}

	// ================================================================ The parts

	struct FParts
	{
		FMeshData Silk[5];
		FMeshData CabinetSilk, Stone, OvalWall;
		FMeshData Trim;
		FMeshData Cornice, Cove;
		FMeshData Vault;
		FMeshData Roof;
		FMeshData Floor, StoneDiscs, Rings;
		FMeshData Benches;
		FMeshData Glass, Steel;
		FMeshData Velarium, Laylight;
		FMeshData SteleFoot;
		// The interior update.
		FMeshData Voussoirs, Parquet, Rosettes;
		FMeshData Opal, Muslin, Veil, VeilLeaf;
		FMeshData PergolaBronze, Planters;
		FNatureMesh PergolaStems, PergolaLeaves;
	};

	const FVector kAxisX(1.0, 0.0, 0.0);
	const FVector kAxisY(0.0, 1.0, 0.0);
	const FVector kUp(0.0, 0.0, 1.0);

	void ArchitraveOnWall(FMeshData& M, const FWall& W, double C)
	{
		SalonKit::Sweep(M, DoorFrames([&](double L, double Z) { return W.At(C + L, Z); }, [&](double) { return W.N; }, [&](double) { return W.U; }),
						Architrave());
	}

	// ================================================================ The interior update: piers and arches

	/**
	 * A silk panel on a pier's side face (the face at x = X, facing F = ±1 along x): from the wall's corner (y = YWall)
	 * to the jamb's stone edge (y = YJamb), from the base's top to the impost's underside; 12 mm proud, its edge to the
	 * jamb returned (the upholsterer's tacked edge), top and bottom returned under the impost and onto the base.
	 */
	void PierSilk(FMeshData& M, double X, double F, double YWall, double YJamb)
	{
		const double XF = X + F * kSilkProud, XB = X - F * kSink;
		const FVector N(F, 0.0, 0.0);
		M.Rect(FVector(XF, YWall, kPierSilkZ0), FVector(XF, YJamb, kPierSilkZ0), FVector(XF, YJamb, kPierSilkZ1), FVector(XF, YWall, kPierSilkZ1), N);
		const double Toward = YJamb > YWall ? 1.0 : -1.0;   // the jamb's side, along y
		M.Rect(FVector(XB, YJamb, kPierSilkZ0), FVector(XF, YJamb, kPierSilkZ0), FVector(XF, YJamb, kPierSilkZ1), FVector(XB, YJamb, kPierSilkZ1), FVector(0.0, Toward, 0.0));
		M.Rect(FVector(XB, YWall, kPierSilkZ1), FVector(XF, YWall, kPierSilkZ1), FVector(XF, YJamb, kPierSilkZ1), FVector(XB, YJamb, kPierSilkZ1), kUp);
		M.Rect(FVector(XB, YWall, kPierSilkZ0), FVector(XF, YWall, kPierSilkZ0), FVector(XF, YJamb, kPierSilkZ0), FVector(XB, YJamb, kPierSilkZ0), -kUp);
	}

	/** A small deterministic random number in 0…1 from integers (a stone's own slice of the photographed travertine). */
	inline double Hash01(int32 A, int32 B, int32 C)
	{
		uint32 H = uint32(A) * 73856093u ^ uint32(B) * 19349663u ^ uint32(C) * 83492791u;
		H ^= H >> 13; H *= 0x5bd1e995u; H ^= H >> 15;
		return double(H & 0xFFFFFF) / double(0xFFFFFF);
	}

	/** The world direction of increasing arch angle at angle A (in the arch's plane). */
	inline FVector ArchTangent(double A) { return FVector(0.0, FMath::Sin(A), FMath::Cos(A)); }

	/** The outward radial direction at arch angle A. */
	inline FVector ArchRadial(double A) { return FVector(0.0, -FMath::Cos(A), FMath::Sin(A)); }

	/** The archivolt round the voussoirs' extrados (A out from the extrados in the face, B out of it): a bead, a fascia, a cavetto. */
	FProfile ArchivoltOuter()
	{
		FProfile P;
		// Traced with the solid on the left: from its outer edge back in to the voussoirs.
		P.Add(0.175, -kSink).Add(0.175, 0.000)
			.Arc(0.175, 0.012, 0.022, 0.012, -90.0, -180.0, 6)          // a cavetto up to the fascia
			.Add(0.030, 0.012).Add(0.026, 0.004)                          // the fascia, 12 mm proud, and a quirk
			.Arc(0.013, 0.004, 0.013, 0.010, 0.0, 180.0, 8)                // the bead against the voussoirs, 14 mm proud
			.Add(0.000, -kSink);
		return P;
	}

	/**
	 * One transverse arch's masonry (pier PierIndex, faces at x = PW and PE): fifteen voussoirs from the intrados
	 * (R 4.5) to the extrados (R 5.3), each its own stone (its own slice of the travertine on UV0), their joints
	 * V-chamfered on both faces and across the soffit; the keystone proud of the faces by 20 mm, its sides radial and
	 * its top flat, crossing the archivolt; the archivolt round the extrados, stopping against the keystone.
	 */
	void Voussoirs(FParts& Out, int32 PierIndex, double PW, double PE)
	{
		FMeshData& M = Out.Voussoirs;
		const double Ri = kIntrados;
		constexpr int32 NR = 3, NA = 8;
		for (int32 S = 0; S < kVoussoirs; ++S)
		{
			const double A0 = kPi * S / kVoussoirs, A1 = kPi * (S + 1) / kVoussoirs;
			const bool bKey = S == kKeystone;
			const bool bLow = S > 0, bHigh = S < kVoussoirs - 1;   // a joint on this side (the springers sit on the imposts)
			const FVector2D Slice(1.7 * Hash01(PierIndex, S, 1), 1.7 * Hash01(PierIndex, S, 2));
			const uint8 Tone = uint8(FMath::RoundToInt(255.0 * Hash01(PierIndex, S, 3))), Polish = uint8(FMath::RoundToInt(255.0 * Hash01(PierIndex, S, 4)));
			int32 Mark = M.Positions.Num();
			auto Paint = [&](bool bJoint) { M.Paint(Mark, FColor(Tone, Polish, bJoint ? 255 : 0, 255)); Mark = M.Positions.Num(); };
			// UV0 in metres: along the radius and round the arch, from the stone's own place in the quarry.
			auto StoneUV = [Slice](const FVector& P)
			{
				const double DY = -P.Y, DZ = P.Z - kSpring;
				const double R = FMath::Sqrt(DY * DY + DZ * DZ), A = FMath::Atan2(DZ, DY);
				return FVector2D(R + Slice.X, R * A + Slice.Y);
			};
			// … and on the soffit, across the arch and round it.
			auto SoffitUV = [Slice](const FVector& P)
			{
				const double DY = -P.Y, DZ = P.Z - kSpring;
				return FVector2D(P.X + Slice.X, kIntrados * FMath::Atan2(DZ, DY) + Slice.Y);
			};
			// The soffit: flush across the arch (the keystone's underside reaches its proud faces), its joints chamfered.
			const double XS0 = PW - (bKey ? kKeyProud : 0.0), XS1 = PE + (bKey ? kKeyProud : 0.0);
			const double SA0 = A0 + (bLow && !bKey ? kVJoint / Ri : 0.0), SA1 = A1 - (bHigh && !bKey ? kVJoint / Ri : 0.0);
			M.Patch(1, NA, [&](int32 I, int32 J) { return VP(I == 0 ? XS0 : XS1, FMath::Lerp(SA0, SA1, double(J) / NA), Ri); },
					SoffitUV, [](const FVector& P) { return TowardAxis(P); });
			Paint(false);
			for (int32 Side = 0; Side < 2; ++Side)
			{
				if (bKey || (Side == 0 ? !bLow : !bHigh)) { continue; }
				const double AJ = Side == 0 ? A0 : A1, AE = Side == 0 ? SA0 : SA1;
				const FVector Hint = ArchRadial(AJ) * -1.0 + ArchTangent(AJ) * (Side == 0 ? -1.0 : 1.0);
				M.Patch(1, 1, [&](int32 I, int32 J) { return J == 0 ? VP(I == 0 ? PW : PE, AJ, Ri + kVJoint) : VP(I == 0 ? PW : PE, AE, Ri); },
						SoffitUV, [Hint](const FVector&) { return Hint; });
				Paint(true);
			}
			// The two faces.
			for (int32 FaceSide = -1; FaceSide <= 1; FaceSide += 2)
			{
				const double X = FaceSide < 0 ? PW : PE;
				const FVector Facing = kAxisX * FaceSide;
				if (bKey)
				{
					// The keystone: its face 20 mm proud, from the intrados up to its flat top; its radial sides from the
					// joints' depth to its face; its top.
					const double XF = X + FaceSide * kKeyProud, XB = X - FaceSide * kVJoint;
					auto TopR = [](double A) { return kKeyTop / FMath::Max(0.2, FMath::Sin(A)); };
					M.Patch(NR + 2, NA, [&](int32 I, int32 J)
						{
							const double A = FMath::Lerp(A0, A1, double(J) / NA);
							return VP(XF, A, FMath::Lerp(Ri, TopR(A), double(I) / (NR + 2)));
						}, StoneUV, [Facing](const FVector&) { return Facing; });
					Paint(false);
					for (int32 Side = 0; Side < 2; ++Side)
					{
						const double A = Side == 0 ? A0 : A1;
						const FVector Hint = ArchTangent(A) * (Side == 0 ? -1.0 : 1.0);
						M.Patch(1, NR + 2, [&](int32 I, int32 J) { return VP(I == 0 ? XB : XF, A, FMath::Lerp(Ri, TopR(A), double(J) / (NR + 2))); },
								StoneUV, [Hint](const FVector&) { return Hint; });
						Paint(false);
					}
					const FVector T0 = VP(0.0, A0, TopR(A0)), T1 = VP(0.0, A1, TopR(A1));
					M.Rect(FVector(XB, T0.Y, T0.Z), FVector(XF, T0.Y, T0.Z), FVector(XF, T1.Y, T1.Z), FVector(XB, T1.Y, T1.Z), kUp);
					Paint(false);
					continue;
				}
				auto FaceAngle = [&](double R, double T)
				{
					const double D = kVJoint / R;
					return FMath::Lerp(A0 + (bLow ? D : 0.0), A1 - (bHigh ? D : 0.0), T);
				};
				M.Patch(NR, NA, [&](int32 I, int32 J)
					{
						const double R = FMath::Lerp(Ri, kRingOut, double(I) / NR);
						return VP(X, FaceAngle(R, double(J) / NA), R);
					}, StoneUV, [Facing](const FVector&) { return Facing; });
				Paint(false);
				// The V: from the face's chamfered edge back to the joint line, 8 mm into the stone.
				for (int32 Side = 0; Side < 2; ++Side)
				{
					if (Side == 0 ? !bLow : !bHigh) { continue; }
					const double AJ = Side == 0 ? A0 : A1;
					const double AngleT = Side == 0 ? 0.0 : 1.0;
					const FVector Hint = Facing + ArchTangent(AJ) * (Side == 0 ? -1.0 : 1.0);
					M.Patch(1, NR, [&](int32 I, int32 J)
						{
							const double R = FMath::Lerp(Ri, kRingOut, double(J) / NR);
							return I == 0 ? VP(X - FaceSide * kVJoint, AJ, R) : VP(X, FaceAngle(R, AngleT), R);
						}, StoneUV, [Hint](const FVector&) { return Hint; });
					Paint(true);
				}
			}
		}
		// The archivolt, in two runs that stop against the keystone's sides.
		const FProfile Volt = ArchivoltOuter();
		const double KA0 = kPi * kKeystone / kVoussoirs, KA1 = kPi * (kKeystone + 1) / kVoussoirs;
		for (int32 FaceSide = -1; FaceSide <= 1; FaceSide += 2)
		{
			const double X = FaceSide < 0 ? PW : PE;
			const FVector Facing = kAxisX * FaceSide;
			for (int32 RunIndex = 0; RunIndex < 2; ++RunIndex)
			{
				const double From = RunIndex == 0 ? 0.0 : KA1, To = RunIndex == 0 ? KA0 : kPi;
				TArray<FFrame> Run;
				constexpr int32 Steps = 48;
				for (int32 K = 0; K <= Steps; ++K)
				{
					const double A = FMath::Lerp(From, To, double(K) / Steps);
					FFrame F;
					F.Origin = VP(X, A, kRingOut - 0.004);
					F.AxisA = F.NormA = ArchRadial(A);
					F.AxisB = F.NormB = Facing;
					F.S = kRingOut * A;
					Run.Add(F);
				}
				SalonKit::Sweep(Out.Trim, Run, Volt);
			}
		}
	}

	/** The bays' silk, the piers and transverse arches, their bases, imposts and archivolts. */
	void BuildBays(FParts& Out)
	{
		for (int32 b = 0; b < 5; ++b)
		{
			const FBaySpan& Bay = kBays[b];
			const TArray<double> NorthDoors = (b == 0) ? TArray<double>({kCabinetDoorX}) : TArray<double>();
			WallFace(Out.Silk[b], MakeWall(0.0, -kHalf, kAxisX, kAxisY), Bay.West - kSink, Bay.East + kSink, kSilkBottom, kSilkTop, NorthDoors);
			WallFace(Out.Silk[b], MakeWall(0.0, kHalf, kAxisX, -kAxisY), Bay.West - kSink, Bay.East + kSink, kSilkBottom, kSilkTop, TArray<double>());
			if (b == 0)
			{
				WallFace(Out.Silk[b], MakeWall(kEndX, 0.0, kAxisY, -kAxisX), -kHalf - kSink, kHalf + kSink, kSilkBottom, kSilkTop, TArray<double>({0.0}));
			}
			if (b == 4)
			{
				WallFace(Out.Silk[b], MakeWall(kFarX, 0.0, kAxisY, kAxisX), -kHalf - kSink, kHalf + kSink, kSilkBottom, kSilkTop, TArray<double>({0.0}));
			}
		}

		const FProfile Base = PierBase(), Impost = PierImpost();
		for (int32 PierIndex = 0; PierIndex < 4; ++PierIndex)
		{
			const double PX = kPierX[PierIndex];
			const double PW = PX - kPierHalf, PE = PX + kPierHalf;
			for (int32 Sgn = -1; Sgn <= 1; Sgn += 2)
			{
				// The pier: its two side faces and its face to the axis, from the wall to the springing.
				const double YW = Sgn * (kHalf + kSink), YF = Sgn * kIntrados;
				Out.Stone.Rect(FVector(PW, YW, -kSink), FVector(PW, YF, -kSink), FVector(PW, YF, kSpring), FVector(PW, YW, kSpring), -kAxisX);
				Out.Stone.Rect(FVector(PE, YW, -kSink), FVector(PE, YF, -kSink), FVector(PE, YF, kSpring), FVector(PE, YW, kSpring), kAxisX);
				Out.Stone.Rect(FVector(PW, YF, -kSink), FVector(PE, YF, -kSink), FVector(PE, YF, kSpring), FVector(PW, YF, kSpring), FVector(0.0, -Sgn, 0.0));
				// Base and impost round its three faces, from the wall back to the wall (mitred at its corners).
				TArray<FVector2D> Path;
				if (Sgn < 0) { Path = {FVector2D(PW, YW), FVector2D(PW, YF), FVector2D(PE, YF), FVector2D(PE, YW)}; }
				else { Path = {FVector2D(PE, YW), FVector2D(PE, YF), FVector2D(PW, YF), FVector2D(PW, YW)}; }
				SalonKit::SweepPlan(Out.Trim, Path, false, Base);
				SalonKit::SweepPlan(Out.Trim, Path, false, Impost);
				// The silk on its two side faces, each in the colour of the bay it looks into (the east face into the bay
				// east of it), from the wall's corner to 0.14 m short of the jamb.
				PierSilk(Out.Silk[PierIndex], PE, 1.0, Sgn * kHalf, Sgn * (kIntrados + kSilkJambEdge));
				PierSilk(Out.Silk[PierIndex + 1], PW, -1.0, Sgn * kHalf, Sgn * (kIntrados + kSilkJambEdge));
			}
			// The arch: fifteen voussoirs and a keystone, V-jointed on both faces and across the soffit; the spandrels
			// above them (coursed stone, on the vault's own stations); an archivolt moulding round the extrados.
			Voussoirs(Out, PierIndex, PW, PE);
			for (int32 FaceSide = -1; FaceSide <= 1; FaceSide += 2)
			{
				const double X = FaceSide < 0 ? PW : PE;
				const FVector Facing = kAxisX * FaceSide;
				for (int32 J = 0; J < kVaultSegs; ++J)
				{
					// (on up 30 mm behind the vault's shell, so no gap can open where the two meet: no pinhole of light)
					Out.Stone.Rect(VP(X, VA(J), kRingOut), VP(X, VA(J), kVaultR + kSeal), VP(X, VA(J + 1), kVaultR + kSeal), VP(X, VA(J + 1), kRingOut), Facing);
				}
			}
		}
	}

	/** One triple-stepped coffer: ring 0 is the cell's edge (the middle of the rib), the rest step up into the vault. */
	void AddCoffer(FMeshData& M, double U0, double U1, int32 Row)
	{
		const int32 J0 = Row * kRowSub;
		const double A0 = VA(J0), A1 = VA(J0 + kRowSub);
		const FVector Centre = VP(0.5 * (U0 + U1), 0.5 * (A0 + A1), kVaultR - 1.0);
		auto Toward = [&Centre](const FVector& P) { return Centre - P; };
		auto Uv = [](const FVector& P) { return VaultUV(P); };
		auto Angle = [&](int32 K, int32 T) -> double
		{
			if (K == 0) { return VA(J0 + T); }
			const double B0 = A0 + kCofferInset[K] / kVaultR, B1 = A1 - kCofferInset[K] / kVaultR;
			return B0 + (B1 - B0) * T / kRowSub;
		};
		auto Ring = [&](int32 K, bool bFar, int32 T) { return VP(bFar ? U1 - kCofferInset[K] : U0 + kCofferInset[K], Angle(K, T), kVaultR + kCofferDepth[K]); };
		for (int32 K = 0; K < 6; ++K)
		{
			M.Patch(1, kRowSub, [&](int32 I, int32 T) { return Ring(K + I, false, T); }, Uv, Toward);
			M.Patch(1, kRowSub, [&](int32 I, int32 T) { return Ring(K + I, true, T); }, Uv, Toward);
			M.Patch(1, 1, [&](int32 I, int32 J) { return Ring(K + J, I == 1, 0); }, Uv, Toward);
			M.Patch(1, 1, [&](int32 I, int32 J) { return Ring(K + J, I == 1, kRowSub); }, Uv, Toward);
		}
		M.Patch(1, kRowSub, [&](int32 I, int32 T) { return Ring(6, I == 1, T); }, Uv, Toward);
	}

	/**
	 * A gilt rosette, Ø 0.22 m, on a coffer's deepest panel (centre C, N out of the panel into the room, T along the
	 * vault): twelve broad outer petals, twelve inner petals between them, each a lobe with a sunk midrib,
	 * a domed boss in a bead. Cast (as staff is) and gilded; each petal's edge runs 2 mm into the panel.
	 */
	void AddRosette(FMeshData& M, const FVector& C, const FVector& N, const FVector& T)
	{
		const FVector B = FVector::CrossProduct(N, T).GetSafeNormal();
		auto Uv = [](const FVector& P) { return FVector2D(P.X, P.Y + P.Z); };
		auto Petal = [&](double Phi, double R0, double R1, double W, double H0, double H1, double Groove, double Curl)
		{
			const FVector D = T * FMath::Cos(Phi) + B * FMath::Sin(Phi);
			const FVector S = B * FMath::Cos(Phi) - T * FMath::Sin(Phi);
			constexpr int32 NU = 6, NV = 10;
			M.Patch(NU, NV, [&](int32 I, int32 J)
				{
					const double U = -1.0 + 2.0 * I / NU, V = double(J) / NV;
					const double R = FMath::Lerp(R0, R1, V);
					// Widest a third of the way out, a rounded base, a point at the tip.
					const double Width = W * FMath::Pow(FMath::Sin(kPi * FMath::Lerp(0.12, 1.0, V)), 0.7) * (1.0 - 0.25 * V);
					const double Crown = FMath::Lerp(H0, H1, V) * FMath::Sqrt(FMath::Max(0.0, 1.0 - U * U));
					const double Rib = Groove * FMath::Exp(-FMath::Square(U / 0.22)) * FMath::Sin(kPi * FMath::Min(1.0, V * 1.15));
					const double Lift = Curl * FMath::Square(FMath::Max(0.0, V - 0.6) / 0.4) * FMath::Sqrt(FMath::Max(0.0, 1.0 - U * U));
					return C + D * R + S * (U * Width) + N * (Crown - Rib + Lift - 0.002);
				}, Uv, [N](const FVector&) { return N; });
		};
		// A full flower (as the gilt rosettes of the Louvre's and the Petit Palais's coffers): twelve broad outer petals
		// overlapping, twelve inner ones between them, cupped up round the boss.
		for (int32 K = 0; K < 12; ++K)
		{
			const double Phi = kPi * K / 6.0;
			Petal(Phi, 0.022, 0.112, 0.044, 0.016, 0.004, 0.004, 0.004);                 // outer
			Petal(Phi + kPi / 12.0, 0.020, 0.078, 0.034, 0.026, 0.012, 0.005, 0.003);     // inner
		}
		// The boss: a low dome in a bead.
		constexpr int32 NLat = 6, NLon = 24;
		M.Patch(NLat, NLon, [&](int32 I, int32 J)
			{
				const double Lat = 0.5 * kPi * I / NLat * 1.05, Lon = 2.0 * kPi * J / NLon;
				return C + N * (0.030 * FMath::Cos(Lat) - 0.002) + (T * FMath::Cos(Lon) + B * FMath::Sin(Lon)) * (0.021 * FMath::Sin(Lat));
			}, Uv, [C](const FVector& P) { return P - C; });
		M.Patch(8, NLon, [&](int32 I, int32 J)
			{
				const double Th = kPi * I / 8, Lon = 2.0 * kPi * J / NLon;
				const FVector Rad = T * FMath::Cos(Lon) + B * FMath::Sin(Lon);
				return C + Rad * (0.025 + 0.004 * FMath::Cos(Th)) + N * (0.012 + 0.004 * FMath::Sin(Th) - 0.002);
			}, Uv, [C, N](const FVector& P)
			{
				// Out of the bead's tube: from its centre ring.
				const FVector V = P - C;
				const FVector Rad = (V - N * FVector::DotProduct(V, N)).GetSafeNormal();
				return P - (C + Rad * 0.025 + N * 0.010);
			});
	}

	/** The lantern's opening in a bay's vault. */
	struct FLantern
	{
		double U0, UMid, U1, YN, YS;
		int32 J0, J1;
	};

	FLantern LanternOf(const FBaySpan& Bay)
	{
		FLantern L;
		L.U0 = ColumnX(Bay, kLanternCol);
		L.UMid = ColumnX(Bay, kLanternCol + 1);
		L.U1 = ColumnX(Bay, kLanternCol + 2);
		L.J0 = kLanternRow * kRowSub;
		L.J1 = L.J0 + kRowSub;
		L.YN = -kVaultR * FMath::Cos(VA(L.J0));
		L.YS = -kVaultR * FMath::Cos(VA(L.J1));
		return L;
	}

	/** The coffered vault, its plain bands, the lunettes over the end walls, the lanterns (shaft, glass, steel). */
	void BuildVault(FParts& Out)
	{
		auto Uv = [](const FVector& P) { return VaultUV(P); };
		auto Toward = [](const FVector& P) { return TowardAxis(P); };
		for (int32 b = 0; b < 5; ++b)
		{
			const FBaySpan& Bay = kBays[b];
			for (int32 End = 0; End < 2; ++End)
			{
				// The bands run 4 mm on behind the arches' and the end walls' faces (sealed joints: see kSeal).
				const double X0 = End == 0 ? Bay.West - kSealIn : ColumnX(Bay, kCols);
				const double X1 = End == 0 ? ColumnX(Bay, 0) : Bay.East + kSealIn;
				Out.Vault.Patch(1, kVaultSegs, [&](int32 I, int32 J) { return VP(I == 0 ? X0 : X1, VA(J), kVaultR); }, Uv, Toward);
			}
			for (int32 C = 0; C < kCols; ++C)
			{
				for (int32 Row = 0; Row < kRows; ++Row)
				{
					if (Row == kLanternRow && (C == kLanternCol || C == kLanternCol + 1)) { continue; }
					AddCoffer(Out.Vault, ColumnX(Bay, C), ColumnX(Bay, C + 1), Row);
					// Its gilt rosette, on the deepest panel's centre.
					const double AMid = 0.5 * (VA(Row * kRowSub) + VA(Row * kRowSub + kRowSub));
					const FVector Centre = VP(0.5 * (ColumnX(Bay, C) + ColumnX(Bay, C + 1)), AMid, kVaultR + kCofferDepth[6]);
					AddRosette(Out.Rosettes, Centre, TowardAxis(Centre).GetSafeNormal(), kAxisX);
				}
			}

			// The lantern: a plaster shaft from the vault up through the roof, clear glass, a thin steel grid under it.
			const FLantern L = LanternOf(Bay);
			const double AN = VA(L.J0), AS = VA(L.J1);
			for (int32 Half = 0; Half < 2; ++Half)
			{
				const double XA = Half == 0 ? L.U0 : L.UMid, XB = Half == 0 ? L.UMid : L.U1;
				// (the shaft's walls start 4 mm inside the vault's face: no gap where they meet the coffers)
				Out.Vault.Rect(VP(XA, AN, kVaultR - kSealIn), VP(XB, AN, kVaultR - kSealIn), FVector(XB, L.YN, kRoofTop), FVector(XA, L.YN, kRoofTop), kAxisY);
				Out.Vault.Rect(VP(XA, AS, kVaultR - kSealIn), VP(XB, AS, kVaultR - kSealIn), FVector(XB, L.YS, kRoofTop), FVector(XA, L.YS, kRoofTop), -kAxisY);
			}
			for (int32 J = L.J0; J < L.J1; ++J)
			{
				for (int32 Side = 0; Side < 2; ++Side)
				{
					const double X = Side == 0 ? L.U0 : L.U1;
					const FVector P0 = VP(X, VA(J), kVaultR - kSealIn), P1 = VP(X, VA(J + 1), kVaultR - kSealIn);
					Out.Vault.Rect(P0, P1, FVector(X, P1.Y, kRoofTop), FVector(X, P0.Y, kRoofTop), Side == 0 ? kAxisX : -kAxisX);
				}
			}
			// Five lanterns, five lights: bay 2's panes are opal glass (a diffuser: it stops the sun and glows with it);
			// the others clear, with the sky over Giverny in the shaft above them (VeilMesh); bay 4's light comes
			// through a muslin blind stretched under the grid.
			const FVector G0(L.U0, L.YN, kGlassZ), G1(L.U1, L.YN, kGlassZ), G2(L.U1, L.YS, kGlassZ), G3(L.U0, L.YS, kGlassZ);
			if (b == 1) { Out.Opal.Rect(G0, G1, G2, G3, -kUp); }
			else { Out.Glass.Rect(G0, G1, G2, G3, -kUp); }
			// The sky over every lantern: the cloud you see through the clear glass (and that dims the opal and the
			// muslin); over bay 3 it lets the leaves above it show through even when overcast.
			const FVector V(0.0, 0.0, kVeilZ - kGlassZ);
			(b == 2 ? Out.VeilLeaf : Out.Veil).Rect(G0 + V, G1 + V, G2 + V, G3 + V, -kUp);
			if (b == 3)
			{
				// The muslin: a cloth on a frame at the shaft's mouth, sagging under its own weight.
				constexpr int32 NX = 24, NY = 12;
				Out.Muslin.Patch(NX, NY, [&](int32 I, int32 J)
					{
						const double S = double(I) / NX, T = double(J) / NY;
						const double Sag = kMuslinSag * (1.0 - FMath::Square(2.0 * S - 1.0)) * (1.0 - FMath::Square(2.0 * T - 1.0));
						return FVector(FMath::Lerp(L.U0, L.U1, S), FMath::Lerp(L.YN, L.YS, T), kMuslinZ - Sag);
					}, [](const FVector& P) { return FVector2D(P.X, P.Y); }, [](const FVector&) { return FVector(0.0, 0.0, -1.0); });
				// Its frame: a slim steel angle round the mouth (25 × 25 mm).
				const double FZ0 = kMuslinZ - 0.025, FZ1 = kMuslinZ + 0.004, W = 0.025;
				Out.Steel.Box(FVector(L.U0, L.YN, FZ0), FVector(L.U1, L.YN + W, FZ1), FMeshData::NegZ | FMeshData::PosY);
				Out.Steel.Box(FVector(L.U0, L.YS - W, FZ0), FVector(L.U1, L.YS, FZ1), FMeshData::NegZ | FMeshData::NegY);
				Out.Steel.Box(FVector(L.U0, L.YN + W, FZ0), FVector(L.U0 + W, L.YS - W, FZ1), FMeshData::NegZ | FMeshData::PosX);
				Out.Steel.Box(FVector(L.U1 - W, L.YN + W, FZ0), FVector(L.U1, L.YS - W, FZ1), FMeshData::NegZ | FMeshData::NegX);
			}
			const double Z0 = kGlassZ - kBarDepth, Z1 = kGlassZ;
			Out.Steel.Box(FVector(L.U0, L.YN, Z0), FVector(L.U1, L.YN + kFrameW, Z1), FMeshData::NegZ | FMeshData::PosY);
			Out.Steel.Box(FVector(L.U0, L.YS - kFrameW, Z0), FVector(L.U1, L.YS, Z1), FMeshData::NegZ | FMeshData::NegY);
			Out.Steel.Box(FVector(L.U0, L.YN + kFrameW, Z0), FVector(L.U0 + kFrameW, L.YS - kFrameW, Z1), FMeshData::NegZ | FMeshData::PosX);
			Out.Steel.Box(FVector(L.U1 - kFrameW, L.YN + kFrameW, Z0), FVector(L.U1, L.YS - kFrameW, Z1), FMeshData::NegZ | FMeshData::NegX);
			Out.Steel.Box(FVector(L.U0 + kFrameW, -kBarHalf, Z0), FVector(L.U1 - kFrameW, kBarHalf, Z1), FMeshData::NegZ | FMeshData::NegY | FMeshData::PosY);
			for (int32 K = 1; K < 6; ++K)
			{
				const double X = L.U0 + (L.U1 - L.U0) * K / 6.0;
				Out.Steel.Box(FVector(X - kBarHalf, L.YN + kFrameW, Z0), FVector(X + kBarHalf, -kBarHalf, Z1), FMeshData::NegZ | FMeshData::NegX | FMeshData::PosX);
				Out.Steel.Box(FVector(X - kBarHalf, kBarHalf, Z0), FVector(X + kBarHalf, L.YS - kFrameW, Z1), FMeshData::NegZ | FMeshData::NegX | FMeshData::PosX);
			}
		}
		// The lunettes: the end walls above the springing, up to the vault on its own stations.
		for (int32 Which = 0; Which < 2; ++Which)
		{
			const double X = Which == 0 ? kEndX : kFarX;
			const FVector Facing = Which == 0 ? -kAxisX : kAxisX;
			for (int32 J = 0; J < kVaultSegs; ++J)
			{
				const FVector T0 = VP(X, VA(J), kVaultR + kSeal), T1 = VP(X, VA(J + 1), kVaultR + kSeal);   // behind the vault
				Out.Vault.Rect(FVector(X, T0.Y, kSpring), FVector(X, T1.Y, kSpring), T1, T0, Facing);
			}
		}
	}

	/**
	 * Bay 3's leaf canopy (the Light board: "a canopy planted over the lantern"): on the roof, a pergola of patinated bronze
	 * (four posts, two beams, cross bars every half metre) 2.4 m over the lantern, and four vines (Vitis) from travertine
	 * planters at its posts, trained up the posts and out along the bars, their leaves in clusters with gaps between: the
	 * sun comes down through them onto the dancers as it does at the Moulin de la Galette. The leaves sway in the museum's
	 * wind (Nature/NatureMesh.h: UV1 bend and flutter, UV2 phases, UV3 height and twig; colour sRGB, alpha occlusion).
	 */
	void BuildPergola(FParts& Out)
	{
		const FLantern L = LanternOf(kBays[2]);
		const double XC = 0.5 * (L.U0 + L.U1), Z0 = kRoofTop;
		const double XW = XC - kPergolaHalfX, XE = XC + kPergolaHalfX, YN = -kPergolaHalfY, YS = kPergolaHalfY;
		const double Post = 0.035, BeamW = 0.025, BeamD = 0.08, BarW = 0.014, BarD = 0.03;
		const double BeamTop = kPergolaTop, BarTop = kPergolaTop + BarD;
		FMeshData& Bz = Out.PergolaBronze;
		// Posts (70 mm square), beams along x on them, bars across on the beams.
		for (const double X : {XW, XE})
		{
			for (const double Y : {YN, YS})
			{
				Bz.Box(FVector(X - Post, Y - Post, Z0 - kSink), FVector(X + Post, Y + Post, BeamTop - BeamD), FMeshData::AllFaces & ~FMeshData::NegZ);
			}
		}
		for (const double Y : {YN, YS})
		{
			Bz.Box(FVector(XW - 0.15, Y - BeamW, BeamTop - BeamD), FVector(XE + 0.15, Y + BeamW, BeamTop), FMeshData::AllFaces);
		}
		TArray<double> BarXs;
		for (double X = XW; X <= XE + 1e-6; X += 0.5) { BarXs.Add(X); }
		for (const double X : BarXs)
		{
			Bz.Box(FVector(X - BarW, YN - 0.12, BeamTop), FVector(X + BarW, YS + 0.12, BarTop), FMeshData::AllFaces);
		}
		// The planters: travertine boxes 0.7 m square, 0.55 m high, inside the posts; their soil 60 mm under the rim.
		const double PH = 0.55, PW2 = 0.35, Wall = 0.06;
		TArray<FVector2D> PlanterCentres;
		for (const double X : {XW + PW2 + 0.05, XE - PW2 - 0.05})
		{
			for (const double Y : {YN + PW2 + 0.05, YS - PW2 - 0.05}) { PlanterCentres.Add(FVector2D(X, Y)); }
		}
		for (const FVector2D& C : PlanterCentres)
		{
			Out.Planters.Box(FVector(C.X - PW2, C.Y - PW2, Z0 - kSink), FVector(C.X + PW2, C.Y + PW2, Z0 + PH),
							 FMeshData::NegX | FMeshData::PosX | FMeshData::NegY | FMeshData::PosY);
			// The rim: four sides' tops, and the soil inside (in the planter's stone, darkened by nothing: it is seen only from above).
			const double I = PW2 - Wall;
			Out.Planters.Rect(FVector(C.X - PW2, C.Y - PW2, Z0 + PH), FVector(C.X + PW2, C.Y - PW2, Z0 + PH), FVector(C.X + I, C.Y - I, Z0 + PH), FVector(C.X - I, C.Y - I, Z0 + PH), kUp);
			Out.Planters.Rect(FVector(C.X + PW2, C.Y - PW2, Z0 + PH), FVector(C.X + PW2, C.Y + PW2, Z0 + PH), FVector(C.X + I, C.Y + I, Z0 + PH), FVector(C.X + I, C.Y - I, Z0 + PH), kUp);
			Out.Planters.Rect(FVector(C.X + PW2, C.Y + PW2, Z0 + PH), FVector(C.X - PW2, C.Y + PW2, Z0 + PH), FVector(C.X - I, C.Y + I, Z0 + PH), FVector(C.X + I, C.Y + I, Z0 + PH), kUp);
			Out.Planters.Rect(FVector(C.X - PW2, C.Y + PW2, Z0 + PH), FVector(C.X - PW2, C.Y - PW2, Z0 + PH), FVector(C.X - I, C.Y - I, Z0 + PH), FVector(C.X - I, C.Y + I, Z0 + PH), kUp);
			Out.Planters.Box(FVector(C.X - I, C.Y - I, Z0 + PH - 0.06), FVector(C.X + I, C.Y + I, Z0 + PH), FMeshData::PosZ);
		}

		// The vines: a trunk from each planter's soil up the nearest post to the bars, then out along the bars.
		FRandomStream Rng(1874);
		const FLinearColor BarkTint = MuseeNature::Srgb(0x6E6254);
		auto Wind = [](double Bend, double Height, double Phase, double Twig)
		{
			FNatureWind W;
			W.Bend = Bend;
			W.Phase = Phase;
			W.Height = Height;
			W.Twig = Twig;
			return W;
		};
		auto Tube = [&](const TArray<FVector>& Pts, double R0, double R1, double Bend0, double Bend1, double Phase)
		{
			TArray<FNatureTubeRing> Rings;
			for (int32 i = 0; i < Pts.Num(); ++i)
			{
				const double T = Pts.Num() > 1 ? double(i) / (Pts.Num() - 1) : 0.0;
				FNatureTubeRing R;
				R.Centre = Pts[i];
				R.Dir = (i + 1 < Pts.Num() ? Pts[i + 1] - Pts[i] : Pts[i] - Pts[i - 1]).GetSafeNormal();
				R.Radius = FMath::Lerp(R0, R1, T);
				R.Wind = Wind(FMath::Lerp(Bend0, Bend1, T), Pts[i].Z - Z0, Phase, T);
				R.Colour = BarkTint;
				R.Occlusion = 0.8;
				Rings.Add(R);
			}
			MuseeNature::AddTube(Out.PergolaStems, Rings, 6, 0.0, true);
		};
		const double VineTop = BarTop + 0.02;
		for (const FVector2D& C : PlanterCentres)
		{
			// Up the post (wound loosely round it), then along the beam towards the middle.
			const double PX = C.X < XC ? XW : XE, PY = C.Y < 0.0 ? YN : YS;
			TArray<FVector> Trunk;
			for (int32 K = 0; K <= 10; ++K)
			{
				const double T = double(K) / 10;
				const double Wrap = 2.5 * kPi * T;
				const FVector2D Around = FMath::Lerp(C, FVector2D(PX, PY), FMath::Min(1.0, T * 3.0)) + FVector2D(FMath::Cos(Wrap), FMath::Sin(Wrap)) * 0.06 * FMath::Min(1.0, T * 3.0);
				Trunk.Add(FVector(Around.X, Around.Y, FMath::Lerp(Z0 + PH - 0.04, VineTop, T)));
			}
			Tube(Trunk, 0.028, 0.016, 0.0, 0.02, Rng.FRand());
		}
		// Along each bar a cane, with spurs to the leaf clusters; the clusters are where a slow noise says, so the canopy
		// has its holes (the dapple) and its dense parts.
		struct FCluster { FVector Centre; double Radius; };
		TArray<FCluster> Clusters;
		for (const double X : BarXs)
		{
			TArray<FVector> Cane;
			const double Phase = Rng.FRand();
			for (double Y = YN; Y <= YS + 1e-6; Y += 0.2)
			{
				Cane.Add(FVector(X + 0.03 * FMath::Sin(Y * 3.1 + X), Y, VineTop + 0.01 * FMath::Sin(Y * 5.0)));
			}
			Tube(Cane, 0.012, 0.007, 0.02, 0.06, Phase);
			for (double Y = YN + 0.1; Y <= YS - 0.1; Y += 0.16)
			{
				const double Dense = MuseeNature::Fbm(FVector(X * 0.9, Y * 0.9, 3.7), 3, 11);   // about −1 … 1
				if (Dense < -0.15 + 0.2 * Rng.FRand()) { continue; }
				const FVector Root(X, Y, VineTop);
				const FVector Centre = Root + FVector(Rng.FRandRange(-0.22, 0.22), Rng.FRandRange(-0.06, 0.06), Rng.FRandRange(-0.10, 0.05));
				Tube({Root, FMath::Lerp(Root, Centre, 0.5) + FVector(0, 0, 0.03), Centre}, 0.005, 0.003, 0.06, 0.12, Phase + Rng.FRand() * 0.2);
				Clusters.Add({Centre, 0.14 + 0.08 * FMath::Max(0.0, Dense)});
			}
		}
		// The leaves: palmate, 9–16 cm, on petioles from the cluster's centre, turned up towards the sky, some hanging.
		for (const FCluster& Cl : Clusters)
		{
			const int32 N = 7 + Rng.RandRange(0, 6);
			for (int32 k = 0; k < N; ++k)
			{
				const double Az = 2.0 * kPi * Rng.FRand();
				const FVector Out2(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				const FVector Base = Cl.Centre + Out2 * Rng.FRandRange(0.02, Cl.Radius) + FVector(0, 0, Rng.FRandRange(-0.08, 0.04));
				const double Size = Rng.FRandRange(0.09, 0.16);
				const FVector Dir = (Out2 + FVector(0, 0, Rng.FRandRange(-0.6, 0.15))).GetSafeNormal();
				FVector Normal = (FVector(0, 0, 1) + FVector(Rng.FRandRange(-0.45, 0.45), Rng.FRandRange(-0.45, 0.45), 0.0)).GetSafeNormal();
				Normal = (Normal - Dir * FVector::DotProduct(Normal, Dir)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
				FNatureWind WB = Wind(0.12, Base.Z - Z0, Rng.FRand(), 1.0), WT = WB;
				WB.Flutter = 0.2;
				WT.Flutter = 1.0;
				WB.FlutterPhase = WT.FlutterPhase = Rng.FRand();
				const FLinearColor Tint = MuseeNature::Vary(MuseeNature::Srgb(0x5E7A3A), Rng, 6.0, 0.12, 0.14);
				MuseeNature::AddCard(Out.PergolaLeaves, Base, Dir, Normal, Size, Size, 0.25, 0.15, 2, WB, WT, Tint,
									 FMath::Clamp(0.65 + 0.35 * (Base.Z - (VineTop - 0.15)) / 0.2, 0.55, 1.0));
			}
		}
	}

	/** The cornices at the springing (on the vault's stations, so the vault meets them vertex for vertex). */
	void BuildCornices(FParts& Out)
	{
		const FProfile Profile = SalonCornice();
		for (int32 b = 0; b < 5; ++b)
		{
			const FBaySpan& Bay = kBays[b];
			TArray<double> Xs = {Bay.West, Bay.East};
			for (int32 C = 0; C <= kCols; ++C) { Xs.Add(ColumnX(Bay, C)); }
			Xs = SortedUnique(Xs);
			TArray<FVector2D> Path;
			if (b == 0)
			{
				// North wall west to east, round Bay 1's end wall, south wall back west.
				Path.Add(FVector2D(Bay.West - kSink, -kHalf));
				for (double X : Xs) { Path.Add(FVector2D(X, -kHalf)); }
				for (int32 J = 1; J <= kVaultSegs; ++J) { Path.Add(FVector2D(kEndX, -kVaultR * FMath::Cos(VA(J)))); }
				for (int32 i = Xs.Num() - 2; i >= 0; --i) { Path.Add(FVector2D(Xs[i], kHalf)); }
				Path.Add(FVector2D(Bay.West - kSink, kHalf));
				SalonKit::SweepPlan(Out.Cornice, Path, false, Profile);
			}
			else if (b == 4)
			{
				// South wall east to west, round Bay 5's far wall, north wall back east.
				Path.Add(FVector2D(Bay.East + kSink, kHalf));
				for (int32 i = Xs.Num() - 1; i >= 0; --i) { Path.Add(FVector2D(Xs[i], kHalf)); }
				for (int32 J = kVaultSegs - 1; J >= 0; --J) { Path.Add(FVector2D(kFarX, -kVaultR * FMath::Cos(VA(J)))); }
				for (int32 i = 1; i < Xs.Num(); ++i) { Path.Add(FVector2D(Xs[i], -kHalf)); }
				Path.Add(FVector2D(Bay.East + kSink, -kHalf));
				SalonKit::SweepPlan(Out.Cornice, Path, false, Profile);
			}
			else
			{
				Path.Add(FVector2D(Bay.West - kSink, -kHalf));
				for (double X : Xs) { Path.Add(FVector2D(X, -kHalf)); }
				Path.Add(FVector2D(Bay.East + kSink, -kHalf));
				SalonKit::SweepPlan(Out.Cornice, Path, false, Profile);
				Path.Reset();
				Path.Add(FVector2D(Bay.East + kSink, kHalf));
				for (int32 i = Xs.Num() - 1; i >= 0; --i) { Path.Add(FVector2D(Xs[i], kHalf)); }
				Path.Add(FVector2D(Bay.West - kSink, kHalf));
				SalonKit::SweepPlan(Out.Cornice, Path, false, Profile);
			}
		}
	}

	/** Skirtings along the bays' walls; they stop inside the doors' architraves and die into the piers' bases. */
	void BuildSkirtings(FParts& Out)
	{
		const FProfile Profile = SalonSkirting();
		auto Run = [&](const TArray<FVector2D>& Path) { SalonKit::SweepPlan(Out.Trim, Path, false, Profile); };
		const FBaySpan& B1 = kBays[0];
		const FBaySpan& B5 = kBays[4];
		Run({FVector2D(B1.West - kSink, -kHalf), FVector2D(kCabinetDoorX - kSkirtEnd, -kHalf)});
		Run({FVector2D(kCabinetDoorX + kSkirtEnd, -kHalf), FVector2D(kEndX, -kHalf), FVector2D(kEndX, -kSkirtEnd)});
		Run({FVector2D(kEndX, kSkirtEnd), FVector2D(kEndX, kHalf), FVector2D(B1.West - kSink, kHalf)});
		for (int32 b = 1; b < 4; ++b)
		{
			Run({FVector2D(kBays[b].West - kSink, -kHalf), FVector2D(kBays[b].East + kSink, -kHalf)});
			Run({FVector2D(kBays[b].East + kSink, kHalf), FVector2D(kBays[b].West - kSink, kHalf)});
		}
		Run({FVector2D(B5.East + kSink, kHalf), FVector2D(kFarX, kHalf), FVector2D(kFarX, kSkirtEnd)});
		Run({FVector2D(kFarX, -kSkirtEnd), FVector2D(kFarX, -kHalf), FVector2D(B5.East + kSink, -kHalf)});
	}

	/** The outer faces, the doors' reveals and architraves, and the roof (open only over the lanterns' glass). */
	void BuildEnvelope(FParts& Out)
	{
		// Outer faces, up to the roof's underside.
		WallFace(Out.Stone, MakeWall(0.0, -kOutHalf, kAxisX, -kAxisY), kFarOutX, kEndOutX, -kSink, kRoofBottom, TArray<double>(),
				 TArray<FHole>({FHole{kCabW - kCabWall, kCabE + kCabWall, -1.0, kCabH}}));
		WallFace(Out.Stone, MakeWall(0.0, kOutHalf, kAxisX, kAxisY), kFarOutX, kEndOutX, -kSink, kRoofBottom, TArray<double>());
		WallFace(Out.Stone, MakeWall(kEndOutX, 0.0, kAxisY, kAxisX), -kOutHalf, kOutHalf, -kSink, kRoofBottom, TArray<double>({0.0}));
		WallFace(Out.Stone, MakeWall(kFarOutX, 0.0, kAxisY, -kAxisX), -kOutHalf, kOutHalf, -kSink, kRoofBottom, TArray<double>(),
				 TArray<FHole>({FHole{-kBlockHalf, kBlockHalf, -1.0, kBlockTop}}));

		// The door from the Rotunda passage, through Bay 1's end wall.
		const FWall EndIn = MakeWall(kEndX, 0.0, kAxisY, -kAxisX), EndOut = MakeWall(kEndOutX, 0.0, kAxisY, kAxisX);
		Tunnel(Out.Stone, [&](double L, double Z) { return EndIn.At(0.0 + L, Z); }, [&](double L, double Z) { return EndOut.At(0.0 + L, Z); }, kAxisY);
		ArchitraveOnWall(Out.Trim, EndIn, 0.0);

		// The door to the Manet cabinet, through Bay 1's north wall.
		const FWall NorthIn = MakeWall(0.0, -kHalf, kAxisX, kAxisY), CabinetSide = MakeWall(0.0, kCabS, kAxisX, -kAxisY);
		Tunnel(Out.Stone, [&](double L, double Z) { return NorthIn.At(kCabinetDoorX + L, Z); }, [&](double L, double Z) { return CabinetSide.At(kCabinetDoorX + L, Z); }, kAxisX);
		ArchitraveOnWall(Out.Trim, NorthIn, kCabinetDoorX);
		ArchitraveOnWall(Out.Trim, CabinetSide, kCabinetDoorX);

		// The door to the oval: one vaulted passage from Bay 5's far wall to the oval's inner face.
		const FWall FarIn = MakeWall(kFarX, 0.0, kAxisY, kAxisX);
		auto OvalEdge = [](double L, double Z) { const FVector2D P = OvalInAtY(L); return FVector(P.X, P.Y, Z); };
		Tunnel(Out.Stone, [&](double L, double Z) { return FarIn.At(0.0 + L, Z); }, OvalEdge, kAxisY);
		ArchitraveOnWall(Out.Trim, FarIn, 0.0);
		SalonKit::Sweep(Out.Trim, DoorFrames(OvalEdge,
			[](double L) { const FVector2D G = OvalGrad(OvalInAtY(L)); return FVector(-G.X, -G.Y, 0.0); },
			[](double L) { const FVector2D G = OvalGrad(OvalInAtY(L)); return FVector(-G.Y, G.X, 0.0); }), SlimArchitrave());

		// The Salon's roof: a slab over the whole Salon, open only over the five lanterns (their shafts close its holes).
		const double X0 = kFarOutX - 0.1, X1 = kEndOutX + 0.1, Y0 = -kOutHalf - 0.1, Y1 = kOutHalf + 0.1;
		TArray<double> Xs = {X0, X1}, Ys = {Y0, Y1};
		TArray<FLantern> Lanterns;
		for (const FBaySpan& Bay : kBays)
		{
			const FLantern L = LanternOf(Bay);
			Lanterns.Add(L);
			Xs.Add(L.U0);
			Xs.Add(L.UMid);
			Xs.Add(L.U1);
			Ys.Add(L.YN);
			Ys.Add(L.YS);
		}
		Xs = SortedUnique(Xs);
		Ys = SortedUnique(Ys);
		for (int32 i = 0; i + 1 < Xs.Num(); ++i)
		{
			for (int32 j = 0; j + 1 < Ys.Num(); ++j)
			{
				const double CX = 0.5 * (Xs[i] + Xs[i + 1]), CY = 0.5 * (Ys[j] + Ys[j + 1]);
				bool bOpen = false;
				for (const FLantern& L : Lanterns)
				{
					if (CX > L.U0 && CX < L.U1 && CY > L.YN && CY < L.YS) { bOpen = true; }
				}
				if (bOpen) { continue; }
				Out.Roof.Rect(FVector(Xs[i], Ys[j], kRoofTop), FVector(Xs[i + 1], Ys[j], kRoofTop), FVector(Xs[i + 1], Ys[j + 1], kRoofTop), FVector(Xs[i], Ys[j + 1], kRoofTop), kUp);
				Out.Roof.Rect(FVector(Xs[i], Ys[j], kRoofBottom), FVector(Xs[i + 1], Ys[j], kRoofBottom), FVector(Xs[i + 1], Ys[j + 1], kRoofBottom), FVector(Xs[i], Ys[j + 1], kRoofBottom), -kUp);
			}
		}
		Out.Roof.Box(FVector(X0, Y0, kRoofBottom), FVector(X1, Y1, kRoofTop), FMeshData::NegX | FMeshData::PosX | FMeshData::NegY | FMeshData::PosY);
		// The void over the vault, closed from inside: the outer walls' faces were one-sided (seen only from outside), so
		// from within the roof space the sky showed through them, and any pinhole in the vault read as a bright dot. Its
		// walls now face in too, from below the springing to the roof: the void is dark.
		{
			const double VZ0 = kSpring - 0.3, VZ1 = kRoofBottom + kSink, In = 0.01;
			const double XA = kFarOutX + In, XB = kEndOutX - In, YA = -kOutHalf + In, YB = kOutHalf - In;
			Out.Roof.Rect(FVector(XA, YA, VZ0), FVector(XB, YA, VZ0), FVector(XB, YA, VZ1), FVector(XA, YA, VZ1), kAxisY);
			Out.Roof.Rect(FVector(XA, YB, VZ0), FVector(XB, YB, VZ0), FVector(XB, YB, VZ1), FVector(XA, YB, VZ1), -kAxisY);
			Out.Roof.Rect(FVector(XA, YA, VZ0), FVector(XA, YB, VZ0), FVector(XA, YB, VZ1), FVector(XA, YA, VZ1), kAxisX);
			Out.Roof.Rect(FVector(XB, YA, VZ0), FVector(XB, YB, VZ0), FVector(XB, YB, VZ1), FVector(XB, YA, VZ1), -kAxisX);
		}
	}

	/** The Manet cabinet: silk walls, skirting, a coved cornice, a laylit ceiling, its outer faces and roof. */
	void BuildCabinet(FParts& Out)
	{
		WallFace(Out.CabinetSilk, MakeWall(kCabE, 0.0, kAxisY, -kAxisX), kCabN - kSink, kCabS + kSink, kSilkBottom, kCabH, TArray<double>());
		WallFace(Out.CabinetSilk, MakeWall(kCabW, 0.0, kAxisY, kAxisX), kCabN - kSink, kCabS + kSink, kSilkBottom, kCabH, TArray<double>());
		WallFace(Out.CabinetSilk, MakeWall(0.0, kCabN, kAxisX, kAxisY), kCabW - kSink, kCabE + kSink, kSilkBottom, kCabH, TArray<double>());
		WallFace(Out.CabinetSilk, MakeWall(0.0, kCabS, kAxisX, -kAxisY), kCabW - kSink, kCabE + kSink, kSilkBottom, kCabH, TArray<double>({kCabinetDoorX}));

		// Cornice round the room (on the ceiling's grid lines, so the ceiling meets it vertex for vertex).
		const TArray<FVector2D> Loop = {
			FVector2D(kCabE, kCabS), FVector2D(kLayX + kLayHalf, kCabS), FVector2D(kLayX - kLayHalf, kCabS), FVector2D(kCabW, kCabS),
			FVector2D(kCabW, kLayY + kLayHalf), FVector2D(kCabW, kLayY - kLayHalf), FVector2D(kCabW, kCabN),
			FVector2D(kLayX - kLayHalf, kCabN), FVector2D(kLayX + kLayHalf, kCabN), FVector2D(kCabE, kCabN),
			FVector2D(kCabE, kLayY - kLayHalf), FVector2D(kCabE, kLayY + kLayHalf)};
		SalonKit::SweepPlan(Out.Cornice, Loop, true, CabinetCornice());
		SalonKit::SweepPlan(Out.Trim,
			{FVector2D(kCabinetDoorX - kSkirtEnd, kCabS), FVector2D(kCabW, kCabS), FVector2D(kCabW, kCabN), FVector2D(kCabE, kCabN),
			 FVector2D(kCabE, kCabS), FVector2D(kCabinetDoorX + kSkirtEnd, kCabS)}, false, SalonSkirting());

		// The ceiling, round the laylight's opening; the laylight's well and its glowing panel.
		const double CornicePlan = kCabCornice;
		const double Xs[4] = {kCabW + CornicePlan, kLayX - kLayHalf, kLayX + kLayHalf, kCabE - CornicePlan};
		const double Ys[4] = {kCabN + CornicePlan, kLayY - kLayHalf, kLayY + kLayHalf, kCabS - CornicePlan};
		for (int32 i = 0; i < 3; ++i)
		{
			for (int32 j = 0; j < 3; ++j)
			{
				if (i == 1 && j == 1) { continue; }
				Out.Vault.Rect(FVector(Xs[i], Ys[j], kCabH), FVector(Xs[i + 1], Ys[j], kCabH), FVector(Xs[i + 1], Ys[j + 1], kCabH), FVector(Xs[i], Ys[j + 1], kCabH), -kUp);
			}
		}
		const double LX0 = Xs[1], LX1 = Xs[2], LY0 = Ys[1], LY1 = Ys[2];
		Out.Vault.Rect(FVector(LX0, LY0, kCabH), FVector(LX0, LY1, kCabH), FVector(LX0, LY1, kLayTop), FVector(LX0, LY0, kLayTop), kAxisX);
		Out.Vault.Rect(FVector(LX1, LY0, kCabH), FVector(LX1, LY1, kCabH), FVector(LX1, LY1, kLayTop), FVector(LX1, LY0, kLayTop), -kAxisX);
		Out.Vault.Rect(FVector(LX0, LY0, kCabH), FVector(LX1, LY0, kCabH), FVector(LX1, LY0, kLayTop), FVector(LX0, LY0, kLayTop), kAxisY);
		Out.Vault.Rect(FVector(LX0, LY1, kCabH), FVector(LX1, LY1, kCabH), FVector(LX1, LY1, kLayTop), FVector(LX0, LY1, kLayTop), -kAxisY);
		Out.Laylight.Rect(FVector(LX0, LY0, kLayTop), FVector(LX1, LY0, kLayTop), FVector(LX1, LY1, kLayTop), FVector(LX0, LY1, kLayTop), -kUp);

		// Outside: three outer faces and the roof (the Salon's north wall rises above it).
		const double OE = kCabE + kCabWall, OW = kCabW - kCabWall, ON = kCabN - kCabWall;
		WallFace(Out.Stone, MakeWall(OE, 0.0, kAxisY, kAxisX), ON, kCabS + kSink, -kSink, kCabRoof, TArray<double>());
		WallFace(Out.Stone, MakeWall(OW, 0.0, kAxisY, -kAxisX), ON, kCabS + kSink, -kSink, kCabRoof, TArray<double>());
		WallFace(Out.Stone, MakeWall(0.0, ON, kAxisX, -kAxisY), OW, OE, -kSink, kCabRoof, TArray<double>());
		Out.Roof.Rect(FVector(OW, ON, kCabRoof), FVector(OE, ON, kCabRoof), FVector(OE, kCabS + kSink, kCabRoof), FVector(OW, kCabS + kSink, kCabRoof), kUp);
		// Its roof space closed from inside (as the Salon's: no sky through a pinhole).
		{
			const double Z0 = kCabH - 0.2, Z1 = kCabRoof, In = 0.01;
			Out.Roof.Rect(FVector(OW + In, ON + In, Z0), FVector(OE - In, ON + In, Z0), FVector(OE - In, ON + In, Z1), FVector(OW + In, ON + In, Z1), kAxisY);
			Out.Roof.Rect(FVector(OW + In, ON + In, Z0), FVector(OW + In, kCabS, Z0), FVector(OW + In, kCabS, Z1), FVector(OW + In, ON + In, Z1), kAxisX);
			Out.Roof.Rect(FVector(OE - In, ON + In, Z0), FVector(OE - In, kCabS, Z0), FVector(OE - In, kCabS, Z1), FVector(OE - In, ON + In, Z1), -kAxisX);
			Out.Roof.Rect(FVector(OW, ON, Z1 - kSink), FVector(OE, ON, Z1 - kSink), FVector(OE, kCabS, Z1 - kSink), FVector(OW, kCabS, Z1 - kSink), -kUp);
		}
	}

	/** The oval: plaster walls, skirting, cove and velarium; outer face, roof and the masonry round the passage. */
	void BuildOval(FParts& Out)
	{
		const TArray<FOvalColumn> Cols = OvalColumns();
		const int32 N = Cols.Num();

		// The inner face, from the floor (or the door's arch) to the cove.
		double S = 0.0;
		for (int32 i = 0; i < N; ++i)
		{
			const FOvalColumn& CA = Cols[i];
			const FOvalColumn& CB = Cols[(i + 1) % N];
			const double Len = FVector2D::Distance(CA.P, CB.P);
			const bool bDoor = CA.bDoor && CB.bDoor;
			const double BotA = bDoor ? CA.Head : -kSink, BotB = bDoor ? CB.Head : -kSink;
			const FVector2D GA = OvalGrad(CA.P), GB = OvalGrad(CB.P);
			const FVector NA(-GA.X, -GA.Y, 0.0), NB(-GB.X, -GB.Y, 0.0);
			const int32 V0 = Out.OvalWall.Vertex(FVector(CA.P.X, CA.P.Y, BotA), NA, FVector2D(S, -BotA));
			const int32 V1 = Out.OvalWall.Vertex(FVector(CB.P.X, CB.P.Y, BotB), NB, FVector2D(S + Len, -BotB));
			const int32 V2 = Out.OvalWall.Vertex(FVector(CB.P.X, CB.P.Y, kOvalBand), NB, FVector2D(S + Len, -kOvalBand));
			const int32 V3 = Out.OvalWall.Vertex(FVector(CA.P.X, CA.P.Y, kOvalBand), NA, FVector2D(S, -kOvalBand));
			Out.OvalWall.Quad(V0, V1, V2, V3);
			S += Len;
		}

		// The cove, on the wall's own columns, and the velarium on the cove's rim.
		TArray<FVector2D> Path;
		for (const FOvalColumn& C : Cols) { Path.Add(C.P); }
		const FProfile Cove = OvalCove();
		const TArray<TArray<FFrame>> Runs = SalonKit::PlanRuns(Path, true);
		for (const TArray<FFrame>& Run : Runs) { SalonKit::Sweep(Out.Cove, Run, Cove); }
		const FVector2D RimPoint = Cove.Points.Last().P;
		TArray<FVector> Rim;
		for (const TArray<FFrame>& Run : Runs)
		{
			for (int32 k = 0; k + 1 < Run.Num(); ++k) { Rim.Add(Run[k].At(RimPoint)); }
		}
		const FVector Centre(kOvalX, 0.0, RimPoint.Y);
		const int32 Rings = 24;
		const double Rise = 0.5;
		Out.Velarium.Patch(Rings, Rim.Num(), [&](int32 I, int32 J) -> FVector
			{
				const FVector& R = Rim[J % Rim.Num()];
				if (I == 0) { return R; }
				const double Rho = 1.0 - double(I) / Rings;
				const FVector Q = Centre + (R - Centre) * Rho;
				return FVector(Q.X, Q.Y, RimPoint.Y + Rise * (1.0 - Rho * Rho));
			},
			[](const FVector& P) { return FVector2D(P.X, P.Y); }, [](const FVector&) { return FVector(0.0, 0.0, -1.0); });

		// Skirting, from the door's architrave round to the other side.
		TArray<FVector2D> Skirt;
		const double SkirtEnd = kDoorHalf + 0.13;   // inside the slim architrave
		const double TS = FMath::Asin(SkirtEnd / kOvalB);
		Skirt.Add(OvalInAtY(SkirtEnd));
		for (int32 i = 1; i < kOvalSegs; ++i) { Skirt.Add(OvalIn(TS + (2.0 * kPi - 2.0 * TS) * i / kOvalSegs)); }
		Skirt.Add(OvalInAtY(-SkirtEnd));
		SalonKit::SweepPlan(Out.Trim, Skirt, false, OvalSkirting());

		// Outside: the outer face (open where the passage's masonry joins it), the roof, the passage's masonry.
		const TArray<double> Ts = OuterStations();
		const double TB = BlockT();
		const int32 NT = Ts.Num();
		double SO = 0.0;
		for (int32 i = 0; i < NT; ++i)
		{
			const double TA = Ts[i], TBn = (i + 1 < NT) ? Ts[i + 1] : Ts[0] + 2.0 * kPi;
			const FVector2D PA = OvalOut(TA), PB = OvalOut(TBn);
			const double Len = FVector2D::Distance(PA, PB);
			const double Bot = FMath::Abs(0.5 * (TA + TBn)) < TB ? kBlockTop : -kSink;
			const FVector2D GA = OvalGrad(OvalIn(TA)), GB = OvalGrad(OvalIn(TBn));
			const FVector NA(GA.X, GA.Y, 0.0), NB(GB.X, GB.Y, 0.0);
			const int32 V0 = Out.Stone.Vertex(FVector(PA.X, PA.Y, Bot), NA, FVector2D(SO, -Bot));
			const int32 V1 = Out.Stone.Vertex(FVector(PB.X, PB.Y, Bot), NB, FVector2D(SO + Len, -Bot));
			const int32 V2 = Out.Stone.Vertex(FVector(PB.X, PB.Y, kOvalRoof), NB, FVector2D(SO + Len, -kOvalRoof));
			const int32 V3 = Out.Stone.Vertex(FVector(PA.X, PA.Y, kOvalRoof), NA, FVector2D(SO, -kOvalRoof));
			Out.Stone.Quad(V0, V1, V2, V3);
			SO += Len;
		}
		// The roof space over the cove, closed from inside: the outer face's ring facing in, and the roof's underside.
		for (int32 i = 0; i < NT; ++i)
		{
			const double TA = Ts[i], TBn = (i + 1 < NT) ? Ts[i + 1] : Ts[0] + 2.0 * kPi;
			const FVector2D GA = OvalGrad(OvalIn(TA)), GB = OvalGrad(OvalIn(TBn));
			const FVector2D PA = OvalOut(TA) - GA * 0.01, PB = OvalOut(TBn) - GB * 0.01;
			Out.Roof.Rect(FVector(PA.X, PA.Y, kOvalBand - 0.1), FVector(PB.X, PB.Y, kOvalBand - 0.1), FVector(PB.X, PB.Y, kOvalRoof - kSink),
						  FVector(PA.X, PA.Y, kOvalRoof - kSink), FVector(-(GA.X + GB.X), -(GA.Y + GB.Y), 0.0).GetSafeNormal());
		}
		{
			const int32 Under = Out.Roof.Vertex(FVector(kOvalX, 0.0, kOvalRoof - kSink), -kUp, FVector2D(kOvalX, 0.0));
			for (int32 i = 0; i < NT; ++i) { const FVector2D P = OvalOut(Ts[i]); Out.Roof.Vertex(FVector(P.X, P.Y, kOvalRoof - kSink), -kUp, FVector2D(P.X, P.Y)); }
			for (int32 i = 0; i < NT; ++i) { Out.Roof.Tri(Under, Under + 1 + i, Under + 1 + (i + 1) % NT); }
		}
		const int32 Hub = Out.Roof.Vertex(FVector(kOvalX, 0.0, kOvalRoof), kUp, FVector2D(kOvalX, 0.0));
		for (int32 i = 0; i < NT; ++i) { const FVector2D P = OvalOut(Ts[i]); Out.Roof.Vertex(FVector(P.X, P.Y, kOvalRoof), kUp, FVector2D(P.X, P.Y)); }
		for (int32 i = 0; i < NT; ++i) { Out.Roof.Tri(Hub, Hub + 1 + i, Hub + 1 + (i + 1) % NT); }

		// The passage's masonry between Bay 5's far wall and the oval: two side faces and a top.
		const FVector2D PS = OvalOut(TB), PN = OvalOut(-TB);
		Out.Stone.Rect(FVector(kFarOutX, PS.Y, -kSink), FVector(PS.X, PS.Y, -kSink), FVector(PS.X, PS.Y, kBlockTop), FVector(kFarOutX, PS.Y, kBlockTop), kAxisY);
		Out.Stone.Rect(FVector(kFarOutX, PN.Y, -kSink), FVector(PN.X, PN.Y, -kSink), FVector(PN.X, PN.Y, kBlockTop), FVector(kFarOutX, PN.Y, kBlockTop), -kAxisY);
		for (int32 i = 0; i + 1 < NT; ++i)
		{
			if (Ts[i] < -TB - 1e-12 || Ts[i + 1] > TB + 1e-12) { continue; }
			const FVector2D WA = OvalOut(Ts[i]), WB = OvalOut(Ts[i + 1]);
			Out.Roof.Rect(FVector(kFarOutX, WA.Y, kBlockTop), FVector(kFarOutX, WB.Y, kBlockTop), FVector(WB.X, WB.Y, kBlockTop), FVector(WA.X, WA.Y, kBlockTop), kUp);
		}

		// The four pond benches are AMuseeFurniture's now (Furniture/MuseeFurniture.cpp: oak on travertine plinths).
	}

	/** The floor's outline west of Bay 5's far wall: the oval's outer face and the passage's masonry. */
	TArray<FVector2D> OvalFloorHull()
	{
		TArray<FVector2D> Pts;
		for (double T : OuterStations()) { Pts.Add(OvalOut(T)); }
		Pts.Add(FVector2D(kFarOutX, -kBlockHalf));
		Pts.Add(FVector2D(kFarOutX, kBlockHalf));
		return ConvexHull(Pts);
	}

	/** Oak here (a bay, between its arches; the Manet cabinet), stone elsewhere. */
	bool IsParquet(double X, double Y)
	{
		for (const FBaySpan& Bay : kBays)
		{
			if (X > Bay.West && X < Bay.East && FMath::Abs(Y) < kHalf) { return true; }
		}
		return X > kCabW && X < kCabE && Y > kCabN && Y < kCabS;
	}

	/**
	 * Where the oak meets the stone, a flush strip of brushed bronze, 12 mm wide (the Details board: oak 22 mm, brass,
	 * travertine): under each arch on both sides, and at the inner faces of the three doors' reveals.
	 */
	void BuildFloorStrips(FParts& Out)
	{
		auto Strip = [&Out](double X0, double Y0, double X1, double Y1)
		{
			Out.Rings.Box(FVector(X0, Y0, -0.012), FVector(X1, Y1, 0.0005),
						  FMeshData::PosZ | FMeshData::NegX | FMeshData::PosX | FMeshData::NegY | FMeshData::PosY);
		};
		for (const double PX : kPierX)
		{
			for (const double X : {PX - kPierHalf, PX + kPierHalf})
			{
				Strip(X - kStripHalf, -kIntrados, X + kStripHalf, kIntrados);
			}
		}
		Strip(kEndX - kStripHalf, -kDoorHalf, kEndX + kStripHalf, kDoorHalf);                                   // the Rotunda's door
		Strip(kFarX - kStripHalf, -kDoorHalf, kFarX + kStripHalf, kDoorHalf);                                   // the oval's
		Strip(kCabinetDoorX - kDoorHalf, -kHalf - kStripHalf, kCabinetDoorX + kDoorHalf, -kHalf + kStripHalf);  // the cabinet's, both sides
		Strip(kCabinetDoorX - kDoorHalf, kCabS - kStripHalf, kCabinetDoorX + kDoorHalf, kCabS + kStripHalf);
	}

	/**
	 * The floors (Salon and its door reveals, cabinet, oval and its passage) as one conforming grid, so no two pieces
	 * meet off a vertex; the pond's opening left open; round each viewing stone, a ring of floor from its square to its
	 * bronze ring, then the stone.
	 */
	void BuildFloor(FParts& Out, const TArray<FVector2D>& Stones)
	{
		const double CellHalf = 0.6, RDisc = 0.40, RRing = 0.45, GridStep = 2.4;
		const int32 NS = 96;
		const TArray<FVector2D> Hull = OvalFloorHull();
		double HX0 = 1e9, HY0 = 1e9, HY1 = -1e9;
		for (const FVector2D& P : Hull)
		{
			HX0 = FMath::Min(HX0, P.X);
			HY0 = FMath::Min(HY0, P.Y);
			HY1 = FMath::Max(HY1, P.Y);
		}
		const double CabX0 = kCabW - kCabWall, CabX1 = kCabE + kCabWall, CabY0 = kCabN - kCabWall;
		TArray<double> Xs = {HX0, kFarOutX, kEndOutX, CabX0, CabX1, kPondX0, kPondX1, kEndX, kFarX, kCabW, kCabE};
		TArray<double> Ys = {HY0, HY1, -kOutHalf, kOutHalf, CabY0, -kPondY, kPondY, -kHalf, kHalf, kCabN, kCabS};
		for (const FBaySpan& Bay : kBays)
		{
			Xs.Add(Bay.West);
			Xs.Add(Bay.East);
		}
		for (const FVector2D& St : Stones)
		{
			Xs.Add(St.X - CellHalf);
			Xs.Add(St.X + CellHalf);
			Ys.Add(St.Y - CellHalf);
			Ys.Add(St.Y + CellHalf);
		}
		for (double X = FMath::CeilToDouble(HX0 / GridStep) * GridStep; X < kEndOutX; X += GridStep) { Xs.Add(X); }
		const double YLo = FMath::Min(HY0, CabY0), YHi = FMath::Max(HY1, kOutHalf);
		for (double Y = FMath::CeilToDouble(YLo / GridStep) * GridStep; Y < YHi; Y += GridStep) { Ys.Add(Y); }
		Xs = SortedUnique(Xs);
		Ys = SortedUnique(Ys);

		auto Inside = [](double X, double Y, double X0, double X1, double Y0, double Y1) { return X > X0 && X < X1 && Y > Y0 && Y < Y1; };
		for (int32 i = 0; i + 1 < Xs.Num(); ++i)
		{
			for (int32 j = 0; j + 1 < Ys.Num(); ++j)
			{
				const double X0 = Xs[i], X1 = Xs[i + 1], Y0 = Ys[j], Y1 = Ys[j + 1];
				const double CX = 0.5 * (X0 + X1), CY = 0.5 * (Y0 + Y1);
				if (Inside(CX, CY, kPondX0, kPondX1, -kPondY, kPondY)) { continue; }
				bool bStone = false;
				for (const FVector2D& St : Stones)
				{
					if (FMath::Abs(CX - St.X) < CellHalf && FMath::Abs(CY - St.Y) < CellHalf) { bStone = true; }
				}
				if (bStone) { continue; }
				if (X1 <= kFarOutX + 1e-9)
				{
					const TArray<FVector2D> Cell = ClipConvex({FVector2D(X0, Y0), FVector2D(X1, Y0), FVector2D(X1, Y1), FVector2D(X0, Y1)}, Hull);
					if (Cell.Num() < 3) { continue; }
					double Area = 0.0;
					for (int32 k = 0; k < Cell.Num(); ++k) { Area += Cross2(Cell[k], Cell[(k + 1) % Cell.Num()]); }
					if (FMath::Abs(Area) < 1e-8) { continue; }
					TArray<FVector> Pts;
					for (const FVector2D& Q : Cell) { Pts.Add(FVector(Q.X, Q.Y, 0.0)); }
					Out.Floor.Poly(Pts, kUp);
				}
				else if (Inside(CX, CY, kFarOutX, kEndOutX, -kOutHalf, kOutHalf) || Inside(CX, CY, CabX0, CabX1, CabY0, kCabS))
				{
					// Oak to stand on (the bays, the cabinet), stone to pass (under the arches, in the reveals).
					FMeshData& Target = IsParquet(CX, CY) ? Out.Parquet : Out.Floor;
					Target.Rect(FVector(X0, Y0, 0.0), FVector(X1, Y0, 0.0), FVector(X1, Y1, 0.0), FVector(X0, Y1, 0.0), kUp);
				}
			}
		}

		// The pond's corners: the opening is a rectangle, the basin (PondPlan.basin: a 4.2 × b 2.3 about (−86.5, 0)) an
		// ellipse that leaves its four corners open (west 0.32 m over the stair's top landing, east a 2 cm sliver down the
		// shaft). Floor them up to 3 cm inside the basin's foot, 1 mm proud of the landing, with a soffit for the stair.
		{
			constexpr double Cx = -86.5, A = 4.2 - 0.03, B = 2.3 - 0.03, Z = 0.001;
			constexpr int32 N = 24;
			for (const double SX : {-1.0, 1.0})
			{
				for (const double SY : {-1.0, 1.0})
				{
					const double XE = SX < 0 ? kPondX0 : kPondX1, DX = FMath::Abs(XE - Cx);
					if (FMath::Square(DX / A) + FMath::Square(kPondY / B) <= 1.0) { continue; }   // already under the basin
					const double T0 = FMath::Acos(FMath::Min(1.0, DX / A));                     // the arc meets x = XE
					const double T1 = FMath::Asin(FMath::Min(1.0, kPondY / B));                 // … and y = ±kPondY
					TArray<FVector> Pts = {FVector(XE, SY * kPondY, Z)};                        // the corner first (Poly fans from it)
					for (int32 K = 0; K <= N; ++K)
					{
						const double T = FMath::Lerp(T1, T0, double(K) / N);
						Pts.Add(FVector(Cx + SX * A * FMath::Cos(T), SY * B * FMath::Sin(T), Z));
					}
					Out.Floor.Poly(Pts, kUp);
					for (FVector& P : Pts) { P.Z = 0.0; }
					Out.Floor.Poly(Pts, -kUp);
				}
			}
		}

		for (const FVector2D& St : Stones)
		{
			auto Circle = [&St, NS](double R, int32 K)
			{
				const double T = 2.0 * kPi * (K % NS) / NS;
				return FVector(St.X + R * FMath::Cos(T), St.Y + R * FMath::Sin(T), 0.0);
			};
			auto UvOf = [](const FVector& P) { return FVector2D(P.X, P.Y); };
			// The stone: honed travertine.
			const int32 Hub = Out.StoneDiscs.Vertex(FVector(St.X, St.Y, 0.0), kUp, FVector2D(St.X, St.Y));
			for (int32 K = 0; K < NS; ++K) { const FVector P = Circle(RDisc, K); Out.StoneDiscs.Vertex(P, kUp, UvOf(P)); }
			for (int32 K = 0; K < NS; ++K) { Out.StoneDiscs.Tri(Hub, Hub + 1 + K, Hub + 1 + (K + 1) % NS); }
			// Its bronze ring.
			const int32 RingBase = Out.Rings.Positions.Num();
			for (int32 K = 0; K < NS; ++K)
			{
				const FVector P = Circle(RDisc, K), Q = Circle(RRing, K);
				Out.Rings.Vertex(P, kUp, UvOf(P));
				Out.Rings.Vertex(Q, kUp, UvOf(Q));
			}
			for (int32 K = 0; K < NS; ++K)
			{
				const int32 A = RingBase + 2 * K, B = RingBase + 2 * ((K + 1) % NS);
				Out.Rings.Quad(A, B, B + 1, A + 1);
			}
			// The floor between the ring and the stone's square in the grid (a zipper round both loops by angle). The
			// square carries every grid line that crosses it; its extra points lie exactly on its axis-aligned sides.
			const double BX0 = St.X - CellHalf, BX1 = St.X + CellHalf, BY0 = St.Y - CellHalf, BY1 = St.Y + CellHalf;
			TArray<FVector2D> Bound = {FVector2D(BX0, BY0), FVector2D(BX1, BY0), FVector2D(BX1, BY1), FVector2D(BX0, BY1)};
			for (double X : Xs)
			{
				if (X > BX0 + 1e-6 && X < BX1 - 1e-6) { Bound.Add(FVector2D(X, BY0)); Bound.Add(FVector2D(X, BY1)); }
			}
			for (double Y : Ys)
			{
				if (Y > BY0 + 1e-6 && Y < BY1 - 1e-6) { Bound.Add(FVector2D(BX0, Y)); Bound.Add(FVector2D(BX1, Y)); }
			}
			// And a point on the square on each of the ring's rays, so every spoke of the zipper is nearly radial (a fan
			// from a corner across a quarter of the ring would fold over itself).
			for (int32 K = 0; K < NS; ++K)
			{
				const double T = 2.0 * kPi * K / NS, C = FMath::Cos(T), Sn = FMath::Sin(T);
				if (FMath::Abs(C) >= FMath::Abs(Sn)) { Bound.Add(FVector2D(C > 0.0 ? BX1 : BX0, St.Y + CellHalf * Sn / FMath::Abs(C))); }
				else { Bound.Add(FVector2D(St.X + CellHalf * C / FMath::Abs(Sn), Sn > 0.0 ? BY1 : BY0)); }
			}
			auto Ang = [&St](const FVector2D& P)
			{
				double A = FMath::Atan2(P.Y - St.Y, P.X - St.X);
				if (A < 0.0) { A += 2.0 * kPi; }
				return A;
			};
			Bound.Sort([&Ang](const FVector2D& A, const FVector2D& B) { return Ang(A) < Ang(B); });
			{
				TArray<FVector2D> Distinct;
				for (const FVector2D& Q : Bound)
				{
					if (Distinct.Num() == 0 || FVector2D::Distance(Q, Distinct.Last()) > 1e-9) { Distinct.Add(Q); }
				}
				if (Distinct.Num() > 1 && FVector2D::Distance(Distinct[0], Distinct.Last()) <= 1e-9) { Distinct.RemoveAt(Distinct.Num() - 1); }
				Bound = Distinct;
			}
			const int32 NB = Bound.Num();
			FMeshData& Zip = IsParquet(St.X, St.Y) ? Out.Parquet : Out.Floor;   // a stone let into the oak, or into the stone
			const int32 Base = Zip.Positions.Num();
			for (int32 K = 0; K < NS; ++K) { const FVector P = Circle(RRing, K); Zip.Vertex(P, kUp, UvOf(P)); }
			for (int32 K = 0; K < NB; ++K) { const FVector P(Bound[K].X, Bound[K].Y, 0.0); Zip.Vertex(P, kUp, UvOf(P)); }
			auto AI = [NS](int32 K) { return 2.0 * kPi * K / NS; };
			auto AO = [&](int32 K) { return Ang(Bound[K % NB]) + 2.0 * kPi * (K / NB); };
			int32 CI = 0, CO = 0;
			while (CI < NS || CO < NB)
			{
				const bool bInner = CO >= NB || (CI < NS && AI(CI + 1) <= AO(CO + 1));
				if (bInner)
				{
					Zip.Tri(Base + CI % NS, Base + (CI + 1) % NS, Base + NS + CO % NB);
					++CI;
				}
				else
				{
					Zip.Tri(Base + CI % NS, Base + NS + (CO + 1) % NB, Base + NS + CO % NB);
					++CO;
				}
			}
		}
	}

	/** The glass stele's foot (Plan.Salon.Stele): 0.4 × 1.4 × 0.08 m, its top edges eased. */
	void BuildSteleFoot(FParts& Out)
	{
		const double X0 = -20.35, X1 = -19.95, Y0 = 0.7, Y1 = 2.1, H = 0.08, E = 0.006;
		FProfile Edge;
		Edge.Add(E, H).Add(0.0, H - E).Add(0.0, -kSink);
		const TArray<TArray<FFrame>> Runs = SalonKit::PlanRuns({FVector2D(X0, Y0), FVector2D(X1, Y0), FVector2D(X1, Y1), FVector2D(X0, Y1)}, true);
		TArray<FVector> Top;
		for (const TArray<FFrame>& Run : Runs)
		{
			SalonKit::Sweep(Out.SteleFoot, Run, Edge);
			Top.Add(Run[0].At(FVector2D(E, H)));
		}
		Out.SteleFoot.Poly(Top, kUp);
	}

	// ================================================================ Light lines

	struct FPlanLine
	{
		FVector A = FVector::ZeroVector;
		FVector B = FVector::ZeroVector;
		FVector Facing = FVector::UpVector;
		FName Kind;
		int32 Room = 0;
		double Width = 0.0;
	};

	TArray<FPlanLine> PlanLightLines()
	{
		TArray<FPlanLine> Lines;
		const FName Cove(TEXT("VaultCove")), SilkTop(TEXT("SilkTop")), SilkBase(TEXT("SilkBase"));
		// A line along a wall from U0 to U1 (wall coordinates), Inset into the room, at height Z.
		auto Add = [&Lines](const FWall& W, double U0, double U1, double Inset, double Z, const FName& Kind, int32 Room, double Width)
		{
			FPlanLine L;
			L.A = W.At(U0, Z) + W.N * Inset;
			L.B = W.At(U1, Z) + W.N * Inset;
			if (Kind == FName(TEXT("VaultCove"))) { L.Facing = (W.N * 0.34 + FVector::UpVector).GetSafeNormal(); }
			else if (Kind == FName(TEXT("SilkTop"))) { L.Facing = (-W.N * 0.3 - FVector::UpVector).GetSafeNormal(); }
			else { L.Facing = (-W.N * 0.3 + FVector::UpVector).GetSafeNormal(); }
			L.Kind = Kind;
			L.Room = Room;
			L.Width = Width;
			Lines.Add(L);
		};
		const FWall North = MakeWall(0.0, -kHalf, kAxisX, kAxisY), South = MakeWall(0.0, kHalf, kAxisX, -kAxisY);
		const FWall EndWall = MakeWall(kEndX, 0.0, kAxisY, -kAxisX), FarWall = MakeWall(kFarX, 0.0, kAxisY, kAxisX);
		const double PierClear = 0.12;   // the piers' imposts and bases close the slots' last 0.1 m
		for (int32 b = 0; b < 5; ++b)
		{
			const FBaySpan& Bay = kBays[b];
			const int32 Room = b + 1;
			// Where each slot's run ends at the bay's east and west: a pier, or the end wall's corner.
			auto Ends = [&](double Inset, double& W0, double& E0)
			{
				W0 = (b == 4) ? kFarX + Inset : Bay.West + PierClear;
				E0 = (b == 0) ? kEndX - Inset : Bay.East - PierClear;
			};
			double W0 = 0.0, E0 = 0.0;
			Ends(kCoveP, W0, E0);
			Add(North, W0, E0, kCoveP, kCoveZ, Cove, Room, kCoveWidth);
			Add(South, W0, E0, kCoveP, kCoveZ, Cove, Room, kCoveWidth);
			Ends(kTopSlotP, W0, E0);
			Add(North, W0, E0, kTopSlotP, kTopSlotZ, SilkTop, Room, kTopSlotWidth);
			Add(South, W0, E0, kTopSlotP, kTopSlotZ, SilkTop, Room, kTopSlotWidth);
			Ends(kBaseSlotP, W0, E0);
			if (b == 0)
			{
				Add(North, W0, kCabinetDoorX - kArchitraveOut, kBaseSlotP, kBaseSlotZ, SilkBase, Room, kBaseSlotWidth);
				Add(North, kCabinetDoorX + kArchitraveOut, E0, kBaseSlotP, kBaseSlotZ, SilkBase, Room, kBaseSlotWidth);
			}
			else
			{
				Add(North, W0, E0, kBaseSlotP, kBaseSlotZ, SilkBase, Room, kBaseSlotWidth);
			}
			Add(South, W0, E0, kBaseSlotP, kBaseSlotZ, SilkBase, Room, kBaseSlotWidth);
			if (b == 0 || b == 4)
			{
				const FWall& W = (b == 0) ? EndWall : FarWall;
				Add(W, -kHalf + kCoveP, kHalf - kCoveP, kCoveP, kCoveZ, Cove, Room, kCoveWidth);
				Add(W, -kHalf + kTopSlotP, kHalf - kTopSlotP, kTopSlotP, kTopSlotZ, SilkTop, Room, kTopSlotWidth);
				Add(W, -kHalf + kBaseSlotP, -kArchitraveOut, kBaseSlotP, kBaseSlotZ, SilkBase, Room, kBaseSlotWidth);
				Add(W, kArchitraveOut, kHalf - kBaseSlotP, kBaseSlotP, kBaseSlotZ, SilkBase, Room, kBaseSlotWidth);
			}
		}
		// The cabinet: the slot under its cornice and the slot in its skirting.
		const FWall CE = MakeWall(kCabE, 0.0, kAxisY, -kAxisX), CW = MakeWall(kCabW, 0.0, kAxisY, kAxisX);
		const FWall CN = MakeWall(0.0, kCabN, kAxisX, kAxisY), CS = MakeWall(0.0, kCabS, kAxisX, -kAxisY);
		for (const FWall* W : {&CE, &CW})
		{
			Add(*W, kCabN + kTopSlotP, kCabS - kTopSlotP, kTopSlotP, kCabinetTopSlotZ, SilkTop, 0, kTopSlotWidth);
			Add(*W, kCabN + kBaseSlotP, kCabS - kBaseSlotP, kBaseSlotP, kBaseSlotZ, SilkBase, 0, kBaseSlotWidth);
		}
		for (const FWall* W : {&CN, &CS})
		{
			Add(*W, kCabW + kTopSlotP, kCabE - kTopSlotP, kTopSlotP, kCabinetTopSlotZ, SilkTop, 0, kTopSlotWidth);
		}
		Add(CN, kCabW + kBaseSlotP, kCabE - kBaseSlotP, kBaseSlotP, kBaseSlotZ, SilkBase, 0, kBaseSlotWidth);
		Add(CS, kCabW + kBaseSlotP, kCabinetDoorX - kArchitraveOut, kBaseSlotP, kBaseSlotZ, SilkBase, 0, kBaseSlotWidth);
		Add(CS, kCabinetDoorX + kArchitraveOut, kCabE - kBaseSlotP, kBaseSlotP, kBaseSlotZ, SilkBase, 0, kBaseSlotWidth);
		return Lines;
	}
}

// ==================================================================== ASalonStructure

ASalonStructure::ASalonStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* ComponentName, bool bCollide, bool bShadow)
	{
		UProceduralMeshComponent* M = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		M->SetupAttachment(RootComponent);
		M->bUseAsyncCooking = true;
		M->bUseComplexAsSimpleCollision = true;
		if (bCollide)
		{
			M->SetCollisionProfileName(TEXT("BlockAll"));
			M->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		M->SetCastShadow(bShadow);
		return M;
	};
	WallsMesh = Make(TEXT("Walls"), true, true);
	TrimMesh = Make(TEXT("Trim"), true, true);
	CorniceMesh = Make(TEXT("Cornices"), false, true);
	VaultMesh = Make(TEXT("Vault"), false, true);
	RoofMesh = Make(TEXT("Roof"), false, true);
	FloorMesh = Make(TEXT("Floor"), true, true);
	BenchMesh = Make(TEXT("Benches"), true, true);
	GlassMesh = Make(TEXT("LanternGlass"), false, false);
	SteelMesh = Make(TEXT("LanternSteel"), false, true);
	DaylitMesh = Make(TEXT("Daylit"), false, false);
	SteleFootMesh = Make(TEXT("SteleFoot"), true, true);
	DiffuserMesh = Make(TEXT("LanternDiffusers"), false, true);
	VeilMesh = Make(TEXT("LanternSky"), false, true);
	PergolaMesh = Make(TEXT("Pergola"), false, true);

	auto Soft = [](const TCHAR* Path) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path)); };
	// The interior update's silks, one step deeper (Scripts/salon_interior.py makes them; the imported ones are the fallback).
	for (int32 i = 1; i <= 5; ++i)
	{
		BaySilkMaterials.Add(Soft(*FString::Printf(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Silk_%d.MI_Salon_Silk_%d"), i, i)));
	}
	VoussoirMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Ashlar.MI_Salon_Ashlar"));
	ParquetMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Parquet.MI_Salon_Parquet"));
	GiltMaterial = Soft(TEXT("/Game/Museum/Materials/M_Gilt_Aged.M_Gilt_Aged"));
	LanternGlassMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_LanternGlass.MI_Salon_LanternGlass"));
	OpalMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Opal.MI_Salon_Opal"));
	MuslinMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Muslin.MI_Salon_Muslin"));
	SkyVeilMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_SkyVeil.MI_Salon_SkyVeil"));
	SkyVeilLeafMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_SkyVeilLeaves.MI_Salon_SkyVeilLeaves"));
	PergolaMaterials = {
		Soft(TEXT("/Game/Museum/Materials/M_Bronze_Patina.M_Bronze_Patina")),
		Soft(TEXT("/Game/Museum/Nature/MI_Bark_Apple.MI_Bark_Apple")),
		Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_VineLeaf.MI_Salon_VineLeaf")),
		Soft(TEXT("/Game/Museum/Materials/M_Travertine_Solid.M_Travertine_Solid"))};
	FallbackStoneMaterial = Soft(TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed"));
	CabinetSilkMaterial = Soft(TEXT("/Game/Museum/Materials/USD/MI_wall_cabinet.MI_wall_cabinet"));
	StoneMaterial = Soft(TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed"));
	TrimMaterial = Soft(TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed"));
	CorniceMaterial = Soft(TEXT("/Game/Museum/Materials/M_Plaster_Moulding.M_Plaster_Moulding"));
	VaultMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Vault.MI_Salon_Vault"));   // the vault in half-light
	OvalPlasterMaterial = Soft(TEXT("/Game/Museum/Materials/USD/MI_plaster_oval_r92.MI_plaster_oval_r92"));
	RoofMaterial = Soft(TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed"));
	FloorMaterial = Soft(TEXT("/Game/Museum/Materials/M_Travertine_Polished.M_Travertine_Polished"));
	ViewingStoneMaterial = Soft(TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed"));
	BronzeMaterial = Soft(TEXT("/Game/Museum/Materials/USD/MI_bronze.MI_bronze"));
	BenchMaterial = Soft(TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed"));
	GlassMaterial = Soft(TEXT("/Game/Museum/Materials/M_Glass.M_Glass"));
	SteelMaterial = Soft(TEXT("/Game/Museum/Materials/M_RibSteel.M_RibSteel"));
	VelariumMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Velarium.MI_Salon_Velarium"));   // the weather over Giverny
	LaylightMaterial = Soft(TEXT("/Game/Museum/Materials/MI_light_grid_F2EEE6.MI_light_grid_F2EEE6"));
	BrassMaterial = Soft(TEXT("/Game/Museum/Materials/M_Brass_Brushed.M_Brass_Brushed"));

	// The SalonPlan board's viewing stones (salon data → plan metres).
	ViewingStones = {
		FVector2D(-20.0, -4.0), FVector2D(-19.7, 3.6), FVector2D(-20.0, -11.6),
		FVector2D(-32.0, 0.0), FVector2D(-32.0, -3.6), FVector2D(-30.0, 3.8), FVector2D(-35.2, 4.6),
		FVector2D(-44.0, 0.0), FVector2D(-44.0, -3.6), FVector2D(-44.0, 3.6),
		FVector2D(-56.0, 0.0), FVector2D(-56.0, -3.8), FVector2D(-56.0, 3.8),
		FVector2D(-68.3, 0.0), FVector2D(-68.3, -3.8), FVector2D(-68.3, 3.8),
		FVector2D(-79.3, 0.0), FVector2D(-93.7, 0.0)};
}

void ASalonStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
}

void ASalonStructure::BeginPlay()
{
	Super::BeginPlay();
	ApplyMaterials();
}

void ASalonStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	SalonBuild::FParts Parts;
	SalonBuild::BuildBays(Parts);
	SalonBuild::BuildVault(Parts);
	SalonBuild::BuildCornices(Parts);
	SalonBuild::BuildSkirtings(Parts);
	SalonBuild::BuildEnvelope(Parts);
	SalonBuild::BuildCabinet(Parts);
	SalonBuild::BuildOval(Parts);
	SalonBuild::BuildFloor(Parts, ViewingStones);
	SalonBuild::BuildFloorStrips(Parts);
	SalonBuild::BuildSteleFoot(Parts);
	SalonBuild::BuildPergola(Parts);

	for (int32 b = 0; b < 5; ++b) { Parts.Silk[b].Write(WallsMesh, b, true); }
	Parts.CabinetSilk.Write(WallsMesh, 5, true);
	Parts.Stone.Write(WallsMesh, 6, true);
	Parts.OvalWall.Write(WallsMesh, 7, true);
	Parts.Trim.Write(TrimMesh, 0, true);
	Parts.Cornice.Write(CorniceMesh, 0, false);
	Parts.Cove.Write(CorniceMesh, 1, false);
	Parts.Vault.Write(VaultMesh, 0, false);
	Parts.Roof.Write(RoofMesh, 0, false);
	Parts.Floor.Write(FloorMesh, 0, true);
	Parts.StoneDiscs.Write(FloorMesh, 1, true);
	Parts.Rings.Write(FloorMesh, 2, true);
	Parts.Benches.Write(BenchMesh, 0, true);
	Parts.Glass.Write(GlassMesh, 0, false);
	Parts.Steel.Write(SteelMesh, 0, false);
	Parts.Velarium.Write(DaylitMesh, 0, false);
	Parts.Laylight.Write(DaylitMesh, 1, false);
	Parts.SteleFoot.Write(SteleFootMesh, 0, true);
	// The interior update.
	Parts.Voussoirs.Write(WallsMesh, 8, true);
	Parts.Rosettes.Write(VaultMesh, 1, false);
	Parts.Parquet.Write(FloorMesh, 3, true);
	Parts.Opal.Write(DiffuserMesh, 0, false);
	Parts.Muslin.Write(DiffuserMesh, 1, false);
	Parts.Veil.Write(VeilMesh, 0, false);
	Parts.VeilLeaf.Write(VeilMesh, 1, false);
	Parts.PergolaBronze.Write(PergolaMesh, 0, false);
	Parts.Planters.Write(PergolaMesh, 3, false);
	int32 NatureSection = 1;
	for (const FNatureMesh* Mesh : {&Parts.PergolaStems, &Parts.PergolaLeaves})
	{
		if (Mesh->IsEmpty()) { PergolaMesh->ClearMeshSection(NatureSection++); continue; }
		PergolaMesh->CreateMeshSection(NatureSection++, Mesh->Vertices, Mesh->Triangles, Mesh->Normals, Mesh->UV0, Mesh->UV1, Mesh->UV2, Mesh->UV3,
									   Mesh->Colors, Mesh->Tangents, false);
	}
}

void ASalonStructure::ApplyMaterials()
{
	auto Apply = [](UProceduralMeshComponent* Target, int32 Section, const TSoftObjectPtr<UMaterialInterface>& Ref)
	{
		if (!Target || Ref.IsNull()) { return; }
		if (UMaterialInterface* Mat = Ref.LoadSynchronous()) { Target->SetMaterial(Section, Mat); }
	};
	// The interior update's materials, each with a fallback so nothing ever shows the default material (a map saved before
	// Scripts/salon_interior.py has run keeps the as-built look for that part).
	auto Load = [](const TSoftObjectPtr<UMaterialInterface>& Ref) -> UMaterialInterface* { return Ref.IsNull() ? nullptr : Ref.LoadSynchronous(); };
	auto LoadPath = [](const TCHAR* Path) -> UMaterialInterface* { return Cast<UMaterialInterface>(FSoftObjectPath(Path).TryLoad()); };
	auto ApplyOr = [&](UProceduralMeshComponent* Target, int32 Section, const TSoftObjectPtr<UMaterialInterface>& Ref, const TCHAR* Fallback)
	{
		if (!Target) { return false; }
		UMaterialInterface* M = Load(Ref);
		if (!M && Fallback) { M = LoadPath(Fallback); }
		if (M) { Target->SetMaterial(Section, M); }
		return M != nullptr;
	};
	for (int32 b = 0; b < 5; ++b)
	{
		const FString Imported = FString::Printf(TEXT("/Game/Museum/Materials/USD/MI_wall_bay_%d.MI_wall_bay_%d"), b + 1, b + 1);
		if (BaySilkMaterials.IsValidIndex(b)) { ApplyOr(WallsMesh, b, BaySilkMaterials[b], *Imported); }
	}
	ApplyOr(WallsMesh, 8, VoussoirMaterial, TEXT("/Game/Museum/Materials/M_Travertine_Solid.M_Travertine_Solid"));
	ApplyOr(VaultMesh, 1, GiltMaterial, TEXT("/Game/Museum/Materials/M_Gilt.M_Gilt"));
	ApplyOr(FloorMesh, 3, ParquetMaterial, TEXT("/Game/Museum/Materials/M_Oak_Fumed.M_Oak_Fumed"));
	ApplyOr(GlassMesh, 0, LanternGlassMaterial, TEXT("/Game/Museum/Materials/M_Glass.M_Glass"));
	ApplyOr(DiffuserMesh, 0, OpalMaterial, TEXT("/Game/Museum/Materials/MI_light_grid_FFFFFF.MI_light_grid_FFFFFF"));
	ApplyOr(DiffuserMesh, 1, MuslinMaterial, TEXT("/Game/Museum/Materials/MI_light_grid_F2EEE6.MI_light_grid_F2EEE6"));
	// The sky panels: without their material they would be grey slabs over the glass, so they are hidden instead.
	if (VeilMesh)
	{
		const bool bSky = ApplyOr(VeilMesh, 0, SkyVeilMaterial, nullptr) && ApplyOr(VeilMesh, 1, SkyVeilLeafMaterial, nullptr);
		VeilMesh->SetVisibility(bSky);
	}
	static const TCHAR* PergolaFallbacks[4] = {TEXT("/Game/Museum/Materials/USD/MI_bronze_dark.MI_bronze_dark"),
		TEXT("/Game/Museum/Nature/MI_Bark_Apple.MI_Bark_Apple"), TEXT("/Game/Museum/Nature/MI_Leaf_Apple.MI_Leaf_Apple"),
		TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed")};
	for (int32 s = 0; s < 4; ++s)
	{
		ApplyOr(PergolaMesh, s, PergolaMaterials.IsValidIndex(s) ? PergolaMaterials[s] : TSoftObjectPtr<UMaterialInterface>(), PergolaFallbacks[s]);
	}
	Apply(WallsMesh, 5, CabinetSilkMaterial);
	Apply(WallsMesh, 6, StoneMaterial);
	Apply(WallsMesh, 7, OvalPlasterMaterial);
	Apply(TrimMesh, 0, TrimMaterial);
	Apply(CorniceMesh, 0, CorniceMaterial);
	Apply(CorniceMesh, 1, OvalPlasterMaterial);
	ApplyOr(VaultMesh, 0, VaultMaterial, TEXT("/Game/Museum/Materials/M_Plaster_Coffer.M_Plaster_Coffer"));
	Apply(RoofMesh, 0, RoofMaterial);
	Apply(FloorMesh, 0, FloorMaterial);
	Apply(FloorMesh, 1, ViewingStoneMaterial);
	Apply(FloorMesh, 2, BronzeMaterial);
	Apply(BenchMesh, 0, BenchMaterial);
	Apply(SteelMesh, 0, SteelMaterial);
	ApplyOr(DaylitMesh, 0, VelariumMaterial, TEXT("/Game/Museum/Materials/MI_velarium.MI_velarium"));
	Apply(DaylitMesh, 1, LaylightMaterial);

	// Brushed brass: M_Metal's parameters (Scripts/materials.py).
	if (!BrassMaterial.IsNull())
	{
		UMaterialInterface* Parent = BrassMaterial.LoadSynchronous();
		if (Parent && Parent->IsA<UMaterialInstanceConstant>())
		{
			SteleFootMesh->SetMaterial(0, Parent);   // materials.py's saved M_Brass_Brushed: the bake takes it (Lumen cards)
		}
		else if (Parent)
		{
			UMaterialInstanceDynamic* Brass = UMaterialInstanceDynamic::Create(Parent, this);
			Brass->SetVectorParameterValue(TEXT("BaseColor"), BrassColour);
			Brass->SetScalarParameterValue(TEXT("Roughness"), BrassRoughness);
			Brass->SetScalarParameterValue(TEXT("Metallic"), 1.f);
			Brass->SetScalarParameterValue(TEXT("Variation"), 0.04f);
			Brass->SetScalarParameterValue(TEXT("PatinaAmount"), 0.f);
			SteleFootMesh->SetMaterial(0, Brass);
		}
	}
}

TArray<FSalonLightLine> ASalonStructure::GetLightLines() const
{
	TArray<FSalonLightLine> Result;
	const FTransform& ToWorld = GetActorTransform();
	for (const SalonBuild::FPlanLine& L : SalonBuild::PlanLightLines())
	{
		FSalonLightLine Line;
		Line.Start = ToWorld.TransformPosition(L.A * SalonKit::MetresToCm);
		Line.End = ToWorld.TransformPosition(L.B * SalonKit::MetresToCm);
		Line.Facing = ToWorld.TransformVectorNoScale(L.Facing).GetSafeNormal();
		Line.Kind = L.Kind;
		Line.Room = L.Room;
		Line.SlotWidth = float(L.Width * SalonKit::MetresToCm);
		Result.Add(Line);
	}
	return Result;
}

TArray<FSalonLightLine> ASalonStructure::GetCoveLines() const
{
	TArray<FSalonLightLine> Result = GetLightLines();
	const FName Cove(TEXT("VaultCove"));
	Result.RemoveAll([&Cove](const FSalonLightLine& Line) { return Line.Kind != Cove; });
	return Result;
}

TArray<FBox> ASalonStructure::GetLanternOpenings() const
{
	TArray<FBox> Result;
	for (const SalonBuild::FBaySpan& Bay : SalonBuild::kBays)
	{
		const SalonBuild::FLantern L = SalonBuild::LanternOf(Bay);
		const FBox Local(FVector(L.U0, L.YN, SalonBuild::kGlassZ) * SalonKit::MetresToCm, FVector(L.U1, L.YS, SalonBuild::kRoofTop) * SalonKit::MetresToCm);
		Result.Add(Local.TransformBy(GetActorTransform()));
	}
	return Result;
}

void ASalonStructure::LanternRect(int32 Bay, double& X0, double& Y0, double& X1, double& Y1, double& GlassZ)
{
	const SalonBuild::FLantern L = SalonBuild::LanternOf(SalonBuild::kBays[FMath::Clamp(Bay, 0, 4)]);
	X0 = L.U0;
	X1 = L.U1;
	Y0 = L.YN;
	Y1 = L.YS;
	GlassZ = SalonBuild::kGlassZ;
}

double ASalonStructure::DiffuserZ(int32 Bay)
{
	return Bay == 3 ? SalonBuild::kMuslinZ - SalonBuild::kMuslinSag : SalonBuild::kGlassZ - SalonBuild::kBarDepth;
}

void ASalonStructure::VelariumRect(double& CX, double& HalfX, double& HalfY, double& Z)
{
	// The velarium sits on the cove's rim (OvalCove: 1.45 m in from the wall, at 6.35 m).
	CX = SalonBuild::kOvalX;
	HalfX = SalonBuild::kOvalA - 1.45;
	HalfY = SalonBuild::kOvalB - 1.45;
	Z = 6.35;
}

TArray<FVector> ASalonStructure::GetViewingStoneCentres() const
{
	TArray<FVector> Result;
	for (const FVector2D& St : ViewingStones)
	{
		Result.Add(GetActorTransform().TransformPosition(FVector(St.X, St.Y, 0.0) * SalonKit::MetresToCm));
	}
	return Result;
}

TArray<FString> ASalonStructure::GetReplacedImportPrims()
{
	return {
		TEXT("/Museum/Salon/Salon_floor"),
		TEXT("/Museum/Salon/Bay_1_walls"), TEXT("/Museum/Salon/Bay_2_walls"), TEXT("/Museum/Salon/Bay_3_walls"),
		TEXT("/Museum/Salon/Bay_4_walls"), TEXT("/Museum/Salon/Bay_5_walls"),
		TEXT("/Museum/Salon/Bay_1_end_wall"), TEXT("/Museum/Salon/Bay_5_end_wall"),
		TEXT("/Museum/Salon/Salon_mouldings"),
		TEXT("/Museum/Salon/Piers_and_arches"),
		TEXT("/Museum/Salon/Vault"),
		TEXT("/Museum/Salon/Lunettes"),
		TEXT("/Museum/Salon/Skylight_lanterns"),
		TEXT("/Museum/Salon/Light_grid_skylights"),
		TEXT("/Museum/Salon/Cabinet_walls"), TEXT("/Museum/Salon/Cabinet_mouldings"), TEXT("/Museum/Salon/Cabinet_floor"),
		TEXT("/Museum/Salon/Cabinet_ceiling"), TEXT("/Museum/Salon/Cabinet_laylight"),
		TEXT("/Museum/Salon/Oval_passage"), TEXT("/Museum/Salon/Oval_passage_floor"),
		TEXT("/Museum/Salon/Oval_wall"), TEXT("/Museum/Salon/Oval_mouldings"), TEXT("/Museum/Salon/Oval_floor"),
		TEXT("/Museum/Salon/Velarium"),
		TEXT("/Museum/Salon/Benches"),
		TEXT("/Museum/Salon/Stele_foot"),
	};
}
