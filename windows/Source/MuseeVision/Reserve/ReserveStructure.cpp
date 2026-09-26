#include "Reserve/ReserveStructure.h"

#include "Engine/CollisionProfile.h"
#include "Geometry/MuseeMesh.h"
#include "Materials/MaterialInterface.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"
#include "Geometry/MuseeBake.h"

/**
 * The Reserve's geometry, in metres: plan x east, plan y south, and z the height above the Reserve's floor (At()
 * lowers it by 5.8 m to the plan's height). UVs are in metres.
 *
 * The vault is a height field over the whole room: over each bay, max(A(y), B(x)) + StepDepth, where A is the
 * semi-elliptical arch across the row (its barrel runs along x) and B the one across the bay (its barrel runs along
 * y), both rising the row's rise from the springing; over the arch bands, A or B alone; over the piers, the
 * springing. Both arches of a bay are sampled at the same 40 equal angles, so the grid's diagonals are exactly the
 * groins, and each triangle takes the barrel it lies on. The arcade bands follow the aisles' (lower) arch; their
 * faces rise to the webs either side, the aisle's 6 cm up, the nave's to its own higher arch (a lunette).
 *
 * Watertight: the band steps share the bands' and webs' vertices; the walls' inner faces rise to the box's top,
 * behind the vault's edges; everything that stands on the floor starts 5 cm below it; the piers' bases and imposts
 * reach in under the brick shafts, which run 2 cm into them.
 */
namespace ReserveBuild
{
	namespace R = MuseePlan::Reserve;

	constexpr double Depth = -R::FloorZ;                // the floor is at h −5.8
	constexpr double RoomX0 = R::X0, RoomX1 = R::X1;     // the walls' inner faces
	constexpr double HalfWidth = R::HalfWidth;
	constexpr double WallThickness = R::Wall;
	constexpr double PierY = R::PierY;
	constexpr int32 PierCount = R::PierLines;
	/** The stair's bottom landing: at the floor's height, x −78.14 … −73.7, y ±1.2 (shaftEnd + 0.3). */
	constexpr double LandingEnd = MuseePlan::LongStair::LandingEnd, LandingHalf = MuseePlan::LongStair::HalfWidth;
	/** The west door: 2.4 m, springing 2.0 m (the Swift's opening), 1 cm inside the shaft's walls at ±1.2. */
	constexpr double DoorHalf = 1.19, DoorSpring = 2.0;
	constexpr int32 DoorSegments = 48;

	// The structure (look target 04).
	constexpr double PierHalf = R::PierHalf;             // 0.7 m square brick piers …
	constexpr double Chamfer = 0.04;                     // … with chamfered arrises
	constexpr double StoneHalf = 0.45;                   // bases and imposts stand 10 cm proud of the brick
	constexpr double UnderHalf = 0.30;                   // the stone reaches in under the brick
	constexpr double BaseTop = 0.54, ImpostBottom = 3.64;
	constexpr double Spring = R::Spring;                 // the top of the imposts, where every arch springs
	constexpr double NaveRise = R::NaveRise, AisleRise = R::AisleRise;
	constexpr double StepDepth = 0.06;                   // the arch bands stand this far below the webs
	constexpr double WebSpring = Spring + StepDepth;
	constexpr double SalonTop = R::SalonTopZ + Depth;    // 5.75: the box's top under the Salon
	constexpr double LawnTop = R::LawnTopZ + Depth;      // 5.47: … and under the lawn
	constexpr double SalonTopX0 = R::SalonTopX0, SalonTopX1 = R::SalonTopX1, SalonTopHalf = R::SalonTopHalf;
	constexpr double Sink = 0.05;                        // how far walls and piers go below the floor
	constexpr double OuterSink = 0.35;                   // the outer faces, past the shaft's walls (h −6.15)
	constexpr int32 Stations = 40;                       // per arch; even, so no quad straddles both groins
	constexpr int32 BandAcross = 4;
	/** Flat panels in cells of at most this: the relief fades out towards a crease over a cell's eighth (see WeldWeights). */
	constexpr double MaxCell = 0.5;

	/**
	 * The picture screens' guide slots (AReserveRacks): one in each screen's plane, where its two bronze fins run: a
	 * slot 16 mm wide and 45 mm deep lined in bronze, its 5 mm lips flush with the floor, from near the side wall to
	 * near the axis (the fins' travel, 2.2 m either side of the screen's centre, home and out).
	 */
	constexpr double SlotHalf = 0.008, SlotLip = 0.005, SlotDepth = 0.045;
	constexpr double SlotInner = 1.20, SlotOuter = 11.45;

	/**
	 * The pendants: an opal ball globe (a 17" in the nave, a 13" in the aisles) whose neck sits in a spun bronze gallery
	 * held by three thumb screws, under a socket cup; a chain (with its cable threaded through the links) in the nave, a
	 * 15 mm rod (the cable inside) in the aisles, from a bronze canopy at the vault's crown.
	 */
	struct FPendant { double X, Y, Radius, GlobeZ, CrownZ; bool bChain; };
	constexpr double NaveGlobeRadius = 0.22, NaveGlobeZ = 3.95;
	constexpr double AisleGlobeRadius = 0.17, AisleGlobeZ = 4.42;   // over the screens' tracks (3.70) by 0.55 m
	constexpr double RodRadius = 0.0075;
	constexpr double NeckRadius = 0.34;       // × R: the globe's opening (a 6" fitter on the 17")

	/** The vault uplights: on each impost's four corner ledges, aimed up the groins (58° up). */
	constexpr double UplightCorner = 0.40, UplightLift = 0.095, UplightPitch = 58.0;

	/** Plan (x, y) and height above the Reserve's floor → the actor's frame, metres. */
	FVector At(double X, double Y, double Z) { return FVector(X, Y, Z - Depth); }

	double PierX(int32 K) { return R::PierX(K); }

	struct FRect { double X0, X1, Y0, Y1; };

	/** The eight bays along the room, between the end walls and the piers' faces. */
	TArray<FVector2D> BaySpans()
	{
		TArray<FVector2D> Spans;
		double Lo = RoomX0;
		for (int32 K = 0; K < PierCount; ++K)
		{
			Spans.Add(FVector2D(Lo, PierX(K) - PierHalf));
			Lo = PierX(K) + PierHalf;
		}
		Spans.Add(FVector2D(Lo, RoomX1));
		return Spans;
	}

	/** The three rows across the room: the north aisle, the nave, the south aisle (between the walls and the piers' faces), and their rises. */
	struct FRow { double Lo, Hi, Rise; };
	TArray<FRow> Rows()
	{
		return {{-HalfWidth, -PierY - PierHalf, AisleRise}, {-PierY + PierHalf, PierY - PierHalf, NaveRise}, {PierY + PierHalf, HalfWidth, AisleRise}};
	}

	TArray<FPendant> PendantList()
	{
		TArray<FPendant> Out;
		const TArray<FVector2D> Bays = BaySpans();
		for (const FVector2D& Bay : Bays) { Out.Add({(Bay.X + Bay.Y) / 2, 0.0, NaveGlobeRadius, NaveGlobeZ, WebSpring + NaveRise, true}); }
		for (const double Y : {-R::AisleCentreY, R::AisleCentreY})
		{
			for (const FVector2D& Bay : Bays) { Out.Add({(Bay.X + Bay.Y) / 2, Y, AisleGlobeRadius, AisleGlobeZ, WebSpring + AisleRise, false}); }
		}
		return Out;
	}

	void AddTri(FMuseeMesh& Out, const FVector& A, const FVector& B, const FVector& C, const FVector& NA, const FVector& NB, const FVector& NC,
				const FVector2D& UA, const FVector2D& UB, const FVector2D& UC)
	{
		if (FVector::CrossProduct(B - A, C - A).SizeSquared() > 1e-16) { Out.Tri(A, B, C, NA, NB, NC, UA, UB, UC); }
	}

	void AddQuad(FMuseeMesh& Out, const FVector& A, const FVector& B, const FVector& C, const FVector& D,
				 const FVector& NA, const FVector& NB, const FVector& NC, const FVector& ND,
				 const FVector2D& UA, const FVector2D& UB, const FVector2D& UC, const FVector2D& UD)
	{
		AddTri(Out, A, B, C, NA, NB, NC, UA, UB, UC);
		AddTri(Out, A, C, D, NA, NC, ND, UA, UC, UD);
	}

	void AddQuad(FMuseeMesh& Out, const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& N,
				 const FVector2D& UA, const FVector2D& UB, const FVector2D& UC, const FVector2D& UD)
	{
		AddQuad(Out, A, B, C, D, N, N, N, N, UA, UB, UC, UD);
	}

	/** A flat rectangle Origin + UDir·s + VDir·t (s ≤ ULength, t ≤ VLength) in cells of at most MaxCell; UV = UVOrigin + (s, t). */
	void Panel(FMuseeMesh& Out, const FVector& Origin, const FVector& UDir, const FVector& VDir, double ULength, double VLength,
			   const FVector& Normal, const FVector2D& UVOrigin)
	{
		if (ULength <= 1e-9 || VLength <= 1e-9) { return; }
		const int32 NU = FMath::Max(1, FMath::CeilToInt32(ULength / MaxCell - 1e-9));
		const int32 NV = FMath::Max(1, FMath::CeilToInt32(VLength / MaxCell - 1e-9));
		for (int32 A = 0; A < NU; ++A)
		{
			for (int32 B = 0; B < NV; ++B)
			{
				const double S0 = ULength * A / NU, S1 = ULength * (A + 1) / NU;
				const double T0 = VLength * B / NV, T1 = VLength * (B + 1) / NV;
				AddQuad(Out, Origin + UDir * S0 + VDir * T0, Origin + UDir * S1 + VDir * T0, Origin + UDir * S1 + VDir * T1,
						Origin + UDir * S0 + VDir * T1, Normal, UVOrigin + FVector2D(S0, T0), UVOrigin + FVector2D(S1, T0),
						UVOrigin + FVector2D(S1, T1), UVOrigin + FVector2D(S0, T1));
			}
		}
	}

	/** A closed box (plan x, y, heights above the floor), UV0 in metres on each face's plane. */
	void Box(FMuseeMesh& Out, double X0, double X1, double Y0, double Y1, double Z0, double Z1)
	{
		const FVector XDir(1, 0, 0), YDir(0, 1, 0), ZDir(0, 0, 1);
		Panel(Out, At(X0, Y0, Z0), XDir, ZDir, X1 - X0, Z1 - Z0, -YDir, FVector2D(X0, Z0));
		Panel(Out, At(X0, Y1, Z0), XDir, ZDir, X1 - X0, Z1 - Z0, YDir, FVector2D(X0, Z0));
		Panel(Out, At(X0, Y0, Z0), YDir, ZDir, Y1 - Y0, Z1 - Z0, -XDir, FVector2D(Y0, Z0));
		Panel(Out, At(X1, Y0, Z0), YDir, ZDir, Y1 - Y0, Z1 - Z0, XDir, FVector2D(Y0, Z0));
		Panel(Out, At(X0, Y0, Z1), XDir, YDir, X1 - X0, Y1 - Y0, ZDir, FVector2D(X0, Y0));
		Panel(Out, At(X0, Y0, Z0), XDir, YDir, X1 - X0, Y1 - Y0, -ZDir, FVector2D(X0, Y0));
	}

	/** Length of the ellipse (across = −Half·cos t, up = Height·sin t) from t = 0 to Phi. */
	double EllipseArc(double Half, double Height, double Phi)
	{
		const int32 Steps = 96;
		double Sum = 0;
		for (int32 K = 0; K < Steps; ++K)
		{
			const double T = Phi * (K + 0.5) / Steps;
			Sum += FMath::Sqrt(FMath::Square(Half * FMath::Sin(T)) + FMath::Square(Height * FMath::Cos(T)));
		}
		return Sum * Phi / Steps;
	}

	/**
	 * A semi-elliptical arch from Lo to Hi rising Rise, sampled at Stations + 1 equal angles: the position across it,
	 * the height above its springing, its normal (across, up) facing the room, and the length along it from the Lo
	 * springing. Arches over the same span share their positions whatever their rise.
	 */
	struct FArch
	{
		double Centre = 0, Half = 1, Rise = 1;
		TArray<double> Pos, Lift, Arc;
		TArray<FVector2D> Normal;

		FArch(double Lo, double Hi, double InRise)
			: Centre((Lo + Hi) / 2), Half((Hi - Lo) / 2), Rise(InRise)
		{
			for (int32 I = 0; I <= Stations; ++I)
			{
				const double Phi = UE_DOUBLE_PI * I / Stations;
				const double CosPhi = FMath::Cos(Phi), SinPhi = FMath::Sin(Phi);
				Pos.Add(I == 0 ? Lo : (I == Stations ? Hi : Centre - Half * CosPhi));
				Lift.Add(I == 0 || I == Stations ? 0.0 : Rise * SinPhi);
				Normal.Add(FVector2D(CosPhi / Half, -SinPhi / Rise).GetSafeNormal());
				Arc.Add(EllipseArc(Half, Rise, Phi));
			}
		}

		/** −1 … 1 across the arch. */
		double Unit(int32 I) const { return (Pos[I] - Centre) / Half; }
	};

	/**
	 * One bay's groin vault: SpanX is the arch across the bay (x), SpanY the one across the row (y), with the same rise.
	 * Where |u| > |v| the barrel running along x (arch SpanY) is the higher and forms the ceiling; elsewhere the barrel
	 * running along y. Its courses run along its barrel's axis.
	 */
	void AddWeb(FMuseeMesh& Out, const FArch& SpanX, const FArch& SpanY)
	{
		auto Vertex = [&SpanX, &SpanY](int32 I, int32 J, bool bRunsAlongX, FVector& OutPos, FVector& OutNormal, FVector2D& OutUV)
		{
			const double X = SpanX.Pos[I], Y = SpanY.Pos[J];
			if (bRunsAlongX)
			{
				OutPos = At(X, Y, WebSpring + SpanY.Lift[J]);
				OutNormal = FVector(0, SpanY.Normal[J].X, SpanY.Normal[J].Y);
				OutUV = FVector2D(X, WebSpring + SpanY.Arc[J]);
			}
			else
			{
				OutPos = At(X, Y, WebSpring + SpanX.Lift[I]);
				OutNormal = FVector(SpanX.Normal[I].X, 0, SpanX.Normal[I].Y);
				OutUV = FVector2D(Y, WebSpring + SpanX.Arc[I]);
			}
		};
		// Corners of a grid quad (0 (i, j), 1 (i+1, j), 2 (i+1, j+1), 3 (i, j+1)) and its two triangles: along the
		// (0, 2) diagonal, or along (1, 3) on the anti-diagonal groin.
		static const int32 CornerI[4] = {0, 1, 1, 0};
		static const int32 CornerJ[4] = {0, 0, 1, 1};
		static const int32 Split[2][2][3] = {{{0, 1, 2}, {0, 2, 3}}, {{0, 1, 3}, {1, 2, 3}}};
		for (int32 I = 0; I < Stations; ++I)
		{
			for (int32 J = 0; J < Stations; ++J)
			{
				const int32 Diagonal = (I + J == Stations - 1) ? 1 : 0;
				for (int32 T = 0; T < 2; ++T)
				{
					double Uc = 0, Vc = 0;
					for (int32 C = 0; C < 3; ++C)
					{
						const int32 Corner = Split[Diagonal][T][C];
						Uc += SpanX.Unit(I + CornerI[Corner]) / 3;
						Vc += SpanY.Unit(J + CornerJ[Corner]) / 3;
					}
					const bool bRunsAlongX = FMath::Abs(Uc) > FMath::Abs(Vc);
					FVector P[3], N[3];
					FVector2D UV[3];
					for (int32 C = 0; C < 3; ++C)
					{
						const int32 Corner = Split[Diagonal][T][C];
						Vertex(I + CornerI[Corner], J + CornerJ[Corner], bRunsAlongX, P[C], N[C], UV[C]);
					}
					AddTri(Out, P[0], P[1], P[2], N[0], N[1], N[2], UV[0], UV[1], UV[2]);
				}
			}
		}
	}

	/** An arch band across a row (over a pier line, x XLo … XHi), and the steps up to the webs either side. */
	void AddBandAcrossRow(FMuseeMesh& Out, double XLo, double XHi, const FArch& Span)
	{
		for (int32 M = 0; M < BandAcross; ++M)
		{
			const double X0 = XLo + (XHi - XLo) * M / BandAcross, X1 = XLo + (XHi - XLo) * (M + 1) / BandAcross;
			for (int32 J = 0; J < Stations; ++J)
			{
				const FVector N0(0, Span.Normal[J].X, Span.Normal[J].Y), N1(0, Span.Normal[J + 1].X, Span.Normal[J + 1].Y);
				const double Z0 = Spring + Span.Lift[J], Z1 = Spring + Span.Lift[J + 1];
				const double V0 = Spring + Span.Arc[J], V1 = Spring + Span.Arc[J + 1];
				AddQuad(Out, At(X0, Span.Pos[J], Z0), At(X1, Span.Pos[J], Z0), At(X1, Span.Pos[J + 1], Z1), At(X0, Span.Pos[J + 1], Z1),
						N0, N0, N1, N1, FVector2D(X0, V0), FVector2D(X1, V0), FVector2D(X1, V1), FVector2D(X0, V1));
			}
		}
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double X = Side == 0 ? XLo : XHi;
			const FVector N(Side == 0 ? -1 : 1, 0, 0);
			for (int32 J = 0; J < Stations; ++J)
			{
				const double Z0 = Spring + Span.Lift[J], Z1 = Spring + Span.Lift[J + 1];
				const double U0 = Spring + Span.Arc[J], U1 = Spring + Span.Arc[J + 1];
				AddQuad(Out, At(X, Span.Pos[J], Z0), At(X, Span.Pos[J + 1], Z1), At(X, Span.Pos[J + 1], Z1 + StepDepth),
						At(X, Span.Pos[J], Z0 + StepDepth), N, FVector2D(U0, X), FVector2D(U1, X), FVector2D(U1, X + StepDepth),
						FVector2D(U0, X + StepDepth));
			}
		}
	}

	/**
	 * An arch band along an arcade (across a bay, over y YLo … YHi) on the arch Band, and its faces up to the webs
	 * either side: at YLo to the web on the arch LoWeb, at YHi to the one on HiWeb (same span; the nave's is higher,
	 * so on that side the face is a lunette). The faces are laid as wall (U along x, V up).
	 */
	void AddBandAcrossBay(FMuseeMesh& Out, double YLo, double YHi, const FArch& Band, const FArch& LoWeb, const FArch& HiWeb)
	{
		for (int32 M = 0; M < BandAcross; ++M)
		{
			const double Y0 = YLo + (YHi - YLo) * M / BandAcross, Y1 = YLo + (YHi - YLo) * (M + 1) / BandAcross;
			for (int32 I = 0; I < Stations; ++I)
			{
				const FVector N0(Band.Normal[I].X, 0, Band.Normal[I].Y), N1(Band.Normal[I + 1].X, 0, Band.Normal[I + 1].Y);
				const double Z0 = Spring + Band.Lift[I], Z1 = Spring + Band.Lift[I + 1];
				const double V0 = Spring + Band.Arc[I], V1 = Spring + Band.Arc[I + 1];
				AddQuad(Out, At(Band.Pos[I], Y0, Z0), At(Band.Pos[I + 1], Y0, Z1), At(Band.Pos[I + 1], Y1, Z1), At(Band.Pos[I], Y1, Z0),
						N0, N1, N1, N0, FVector2D(Y0, V0), FVector2D(Y0, V1), FVector2D(Y1, V1), FVector2D(Y1, V0));
			}
		}
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double Y = Side == 0 ? YLo : YHi;
			const FArch& Web = Side == 0 ? LoWeb : HiWeb;
			const FVector N(0, Side == 0 ? -1 : 1, 0);
			for (int32 I = 0; I < Stations; ++I)
			{
				const double X0 = Band.Pos[I], X1 = Band.Pos[I + 1];
				const double Z0 = Spring + Band.Lift[I], Z1 = Spring + Band.Lift[I + 1];
				const double T0 = WebSpring + Web.Lift[I], T1 = WebSpring + Web.Lift[I + 1];
				// In three rows, so a lunette's face keeps its relief between its arrises.
				constexpr int32 FaceRows = 3;
				for (int32 K = 0; K < FaceRows; ++K)
				{
					const double F0 = double(K) / FaceRows, F1 = double(K + 1) / FaceRows;
					const double A0 = FMath::Lerp(Z0, T0, F0), A1 = FMath::Lerp(Z1, T1, F0), B0 = FMath::Lerp(Z0, T0, F1), B1 = FMath::Lerp(Z1, T1, F1);
					AddQuad(Out, At(X0, Y, A0), At(X1, Y, A1), At(X1, Y, B1), At(X0, Y, B0), N,
							FVector2D(X0, A0), FVector2D(X1, A1), FVector2D(X1, B1), FVector2D(X0, B0));
				}
			}
		}
	}

	/** A square sweep about (Cx, Cy): profile (half-width, height), hard corners, normals from the profile's slope. */
	void SquareSweep(FMuseeMesh& Out, double Cx, double Cy, const TArray<FVector2D>& Profile)
	{
		static const FVector2D Faces[4] = {FVector2D(1, 0), FVector2D(0, 1), FVector2D(-1, 0), FVector2D(0, -1)};
		for (int32 F = 0; F < 4; ++F)
		{
			const FVector2D D = Faces[F], T(-D.Y, D.X);
			auto Corner = [&D, &T, Cx, Cy](const FVector2D& Q, double Along)
			{
				return At(Cx + D.X * Q.X + T.X * Along * Q.X, Cy + D.Y * Q.X + T.Y * Along * Q.X, Q.Y);
			};
			for (int32 K = 0; K + 1 < Profile.Num(); ++K)
			{
				const FVector2D P0 = Profile[K], P1 = Profile[K + 1];
				const FVector2D Seg = P1 - P0;
				const FVector2D Out2 = FVector2D(Seg.Y, -Seg.X).GetSafeNormal();   // outward in (half-width, height)
				const FVector N(D.X * Out2.X, D.Y * Out2.X, Out2.Y);
				const double U = 2.0 * F;
				AddQuad(Out, Corner(P0, -1), Corner(P0, 1), Corner(P1, 1), Corner(P1, -1), N,
						FVector2D(U - P0.X, P0.Y), FVector2D(U + P0.X, P0.Y), FVector2D(U + P1.X, P1.Y), FVector2D(U - P1.X, P1.Y));
			}
		}
	}

	/** A brick shaft about (Cx, Cy): a square of half-width Half with its arrises chamfered by C, from Z0 to Z1 (open ends: the stone covers them). */
	void ChamferedShaft(FMuseeMesh& Out, double Cx, double Cy, double Half, double C, double Z0, double Z1)
	{
		const FVector2D Ring[8] = {FVector2D(Half, -Half + C), FVector2D(Half, Half - C), FVector2D(Half - C, Half), FVector2D(-Half + C, Half),
								   FVector2D(-Half, Half - C), FVector2D(-Half, -Half + C), FVector2D(-Half + C, -Half), FVector2D(Half - C, -Half)};
		double U = 0;
		for (int32 K = 0; K < 8; ++K)
		{
			const FVector2D A = Ring[K], B = Ring[(K + 1) % 8];
			const FVector2D Edge = B - A;
			const FVector2D Out2 = FVector2D(Edge.Y, -Edge.X).GetSafeNormal();
			const FVector N(Out2.X, Out2.Y, 0);
			const double L = Edge.Size();
			// Cells no taller than MaxCell, and a wide face in columns of 0.25 m or less, so the relief has room
			// between the arrises (where it is held flat).
			const int32 Rows = FMath::Max(1, FMath::CeilToInt32((Z1 - Z0) / MaxCell));
			const int32 Cols = FMath::Max(1, FMath::CeilToInt32(L / 0.25 - 1e-9));
			for (int32 Cl = 0; Cl < Cols; ++Cl)
			{
				const FVector2D Pa = A + Edge * (double(Cl) / Cols), Pb = A + Edge * (double(Cl + 1) / Cols);
				const double Ua = U + L * Cl / Cols, Ub = U + L * (Cl + 1) / Cols;
				for (int32 Rw = 0; Rw < Rows; ++Rw)
				{
					const double Za = Z0 + (Z1 - Z0) * Rw / Rows, Zb = Z0 + (Z1 - Z0) * (Rw + 1) / Rows;
					AddQuad(Out, At(Cx + Pa.X, Cy + Pa.Y, Za), At(Cx + Pb.X, Cy + Pb.Y, Za), At(Cx + Pb.X, Cy + Pb.Y, Zb), At(Cx + Pa.X, Cy + Pa.Y, Zb), N,
							FVector2D(Ua, Za), FVector2D(Ub, Za), FVector2D(Ub, Zb), FVector2D(Ua, Zb));
				}
			}
			U += L;
		}
	}

	/** A travertine impost block on a wall, under an arch band's springing: (Cx, Cy) on the wall's face, In into the room. */
	void AddCorbel(FMuseeMesh& Out, double Cx, double Cy, const FVector2D& In)
	{
		const FVector2D T(-In.Y, In.X);
		const double Proud = StoneHalf - PierHalf;
		const FVector2D Profile[5] = {FVector2D(0, ImpostBottom), FVector2D(0.02, ImpostBottom), FVector2D(Proud, ImpostBottom + 0.12),
									  FVector2D(Proud, Spring), FVector2D(0, Spring)};
		auto Point = [&In, &T, Cx, Cy](const FVector2D& Q, double Along)
		{
			return At(Cx + In.X * Q.X + T.X * Along, Cy + In.Y * Q.X + T.Y * Along, Q.Y);
		};
		for (int32 K = 0; K < 4; ++K)
		{
			const FVector2D Seg = Profile[K + 1] - Profile[K];
			const FVector2D Out2 = FVector2D(Seg.Y, -Seg.X).GetSafeNormal();
			const FVector N(In.X * Out2.X, In.Y * Out2.X, Out2.Y);
			AddQuad(Out, Point(Profile[K], -StoneHalf), Point(Profile[K], StoneHalf), Point(Profile[K + 1], StoneHalf), Point(Profile[K + 1], -StoneHalf),
					N, FVector2D(-StoneHalf, Profile[K].Y), FVector2D(StoneHalf, Profile[K].Y), FVector2D(StoneHalf, Profile[K + 1].Y),
					FVector2D(-StoneHalf, Profile[K + 1].Y));
		}
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const double Along = Side == 0 ? -StoneHalf : StoneHalf;
			const FVector N(T.X * (Side == 0 ? -1 : 1), T.Y * (Side == 0 ? -1 : 1), 0);
			for (int32 K = 1; K + 1 < 5; ++K)
			{
				AddTri(Out, Point(Profile[0], Along), Point(Profile[K], Along), Point(Profile[K + 1], Along), N, N, N,
					   Profile[0], Profile[K], Profile[K + 1]);
			}
		}
	}

	/**
	 * A respond where a band springs from a wall: a brick pilaster the pier's width, 12 cm out from the wall's face
	 * (Cx, Cy; In into the room), on a plain travertine base, under an impost block that stands 10 cm beyond it.
	 */
	void Respond(FMuseeMesh& Brick, FMuseeMesh& Stone, double Cx, double Cy, const FVector2D& In)
	{
		constexpr double RespondProud = 0.12, BaseProud = 0.22, Back = 0.05;
		auto Rect = [Cx, Cy, &In](FMuseeMesh& Out, double Half, double Proud, double Z0, double Z1)
		{
			const FVector2D A = FVector2D(Cx, Cy) - In * Back, B = FVector2D(Cx, Cy) + In * Proud;
			const FVector2D T(FMath::Abs(In.Y), FMath::Abs(In.X));
			Box(Out, FMath::Min(A.X, B.X) - T.X * Half, FMath::Max(A.X, B.X) + T.X * Half,
				FMath::Min(A.Y, B.Y) - T.Y * Half, FMath::Max(A.Y, B.Y) + T.Y * Half, Z0, Z1);
		};
		Rect(Brick, PierHalf, RespondProud, -Sink, ImpostBottom + 0.02);
		Rect(Stone, StoneHalf, BaseProud, -Sink, 0.36);
		Rect(Stone, 0.41, BaseProud - 0.04, 0.35, BaseTop);
		AddCorbel(Stone, Cx + In.X * RespondProud, Cy + In.Y * RespondProud, In);
	}

	/** A surface of revolution about the vertical through (Cx, Cy): profile (radius, height), a normal (radial, up) at each point. */
	void Revolve(FMuseeMesh& Out, double Cx, double Cy, const TArray<FVector2D>& Profile, const TArray<FVector2D>& Normals, int32 Segments)
	{
		for (int32 K = 0; K + 1 < Profile.Num(); ++K)
		{
			for (int32 I = 0; I < Segments; ++I)
			{
				const double T0 = 2 * UE_DOUBLE_PI * I / Segments, T1 = 2 * UE_DOUBLE_PI * (I + 1) / Segments;
				auto P = [Cx, Cy](const FVector2D& Q, double T) { return At(Cx + Q.X * FMath::Cos(T), Cy + Q.X * FMath::Sin(T), Q.Y); };
				auto N = [](const FVector2D& Q, double T) { return FVector(Q.X * FMath::Cos(T), Q.X * FMath::Sin(T), Q.Y); };
				const double U0 = double(I) / Segments, U1 = double(I + 1) / Segments;
				const double V0 = double(K) / (Profile.Num() - 1), V1 = double(K + 1) / (Profile.Num() - 1);
				AddQuad(Out, P(Profile[K], T0), P(Profile[K], T1), P(Profile[K + 1], T1), P(Profile[K + 1], T0),
						N(Normals[K], T0), N(Normals[K], T1), N(Normals[K + 1], T1), N(Normals[K + 1], T0),
						FVector2D(U0, V0), FVector2D(U1, V0), FVector2D(U1, V1), FVector2D(U0, V1));
			}
		}
	}

	/** A plain cylinder's side (and, if asked, its bottom or top disc). */
	void Cylinder(FMuseeMesh& Out, double Cx, double Cy, double Radius, double Bottom, double Top, bool bBottomDisc, bool bTopDisc, int32 Segments)
	{
		Revolve(Out, Cx, Cy, {FVector2D(Radius, Bottom), FVector2D(Radius, Top)}, {FVector2D(1, 0), FVector2D(1, 0)}, Segments);
		if (bBottomDisc) { Revolve(Out, Cx, Cy, {FVector2D(0, Bottom), FVector2D(Radius, Bottom)}, {FVector2D(0, -1), FVector2D(0, -1)}, Segments); }
		if (bTopDisc) { Revolve(Out, Cx, Cy, {FVector2D(Radius, Top), FVector2D(0, Top)}, {FVector2D(0, 1), FVector2D(0, 1)}, Segments); }
	}

	/** The screens' guide slots, lips and all: one per screen, in its plane. */
	TArray<FRect> TrackRectangles()
	{
		TArray<FRect> Out;
		for (int32 K = 0; K < R::RackCount; ++K)
		{
			const R::FRackSlot Slot = R::Rack(K);
			const double Y0 = Slot.bNorth ? -SlotOuter : SlotInner, Y1 = Slot.bNorth ? -SlotInner : SlotOuter;
			Out.Add({Slot.X - SlotHalf - SlotLip, Slot.X + SlotHalf + SlotLip, Y0, Y1});
		}
		return Out;
	}

	/** The floor over Area at height 0, less the holes (the tracks, the stair's landing), in cells of at most a metre. */
	void FloorWithHoles(FMuseeMesh& Out, const FRect& Area, const TArray<FRect>& Holes)
	{
		TArray<double> Xs = {Area.X0, Area.X1};
		for (const FRect& H : Holes) { Xs.Add(H.X0); Xs.Add(H.X1); }
		for (double X = FMath::CeilToDouble(Area.X0); X < Area.X1; X += 1.0) { Xs.Add(X); }
		Xs.Sort();
		TArray<double> Cuts;
		for (const double X : Xs)
		{
			if (X < Area.X0 - 1e-9 || X > Area.X1 + 1e-9) { continue; }
			if (Cuts.Num() == 0 || X - Cuts.Last() > 1e-6) { Cuts.Add(X); }
		}
		const FVector Up(0, 0, 1);
		auto Emit = [&Out, &Up](double X0, double X1, double Y0, double Y1)
		{
			double Y = Y0;
			while (Y1 - Y > 1e-6)
			{
				const double YNext = FMath::Min(Y1, FMath::FloorToDouble(Y + 1.0 + 1e-9));
				const double YTo = YNext - Y > 1e-6 ? YNext : Y1;
				AddQuad(Out, At(X0, Y, 0), At(X1, Y, 0), At(X1, YTo, 0), At(X0, YTo, 0), Up,
						FVector2D(X0, Y), FVector2D(X1, Y), FVector2D(X1, YTo), FVector2D(X0, YTo));
				Y = YTo;
			}
		};
		for (int32 C = 0; C + 1 < Cuts.Num(); ++C)
		{
			const double X0 = Cuts[C], X1 = Cuts[C + 1], Mid = (X0 + X1) / 2;
			TArray<FVector2D> Gaps;
			for (const FRect& H : Holes)
			{
				if (H.X0 < Mid && Mid < H.X1) { Gaps.Add(FVector2D(H.Y0, H.Y1)); }
			}
			Gaps.Sort([](const FVector2D& A, const FVector2D& B) { return A.X < B.X; });
			double Y = Area.Y0;
			for (const FVector2D& G : Gaps)
			{
				if (G.X > Y) { Emit(X0, X1, Y, FMath::Min(G.X, Area.Y1)); }
				Y = FMath::Max(Y, G.Y);
			}
			if (Area.Y1 > Y) { Emit(X0, X1, Y, Area.Y1); }
		}
	}

	/** One island of plan chests, two back to back, centred on (Cx, Cy): walnut carcass and top, drawers, pulls, plinth; felt mats of Mats' sizes on top. */
	void PlanChests(FMuseeMesh& Wood, FMuseeMesh& Brass, FMuseeMesh& Dark, double Cx, double Cy, const FVector2D& Mat)
	{
		const double HX = R::ChestHalfX, HY = R::ChestDepth, Top = R::ChestTop;
		constexpr double PlinthTop = 0.09, CarcassTop = Top - 0.035, Gap = 0.012, Proud = 0.015;
		Box(Dark, Cx - HX + 0.05, Cx + HX - 0.05, Cy - HY + 0.05, Cy + HY - 0.05, -0.01, PlinthTop + 0.01);
		Box(Wood, Cx - HX, Cx + HX, Cy - HY, Cy + HY, PlinthTop, CarcassTop + 0.005);
		Box(Wood, Cx - HX - 0.02, Cx + HX + 0.02, Cy - HY - 0.02, Cy + HY + 0.02, CarcassTop, Top);
		// Ten drawers a side (two columns of five), each front 15 mm proud, with a brass pull.
		const double FaceX0 = Cx - HX + 0.03, FaceX1 = Cx + HX - 0.03, FaceZ0 = PlinthTop + 0.025, FaceZ1 = CarcassTop - 0.02;
		const double W = (FaceX1 - FaceX0 - Gap) / 2, H = (FaceZ1 - FaceZ0 - 4 * Gap) / 5;
		for (const double Side : {-1.0, 1.0})
		{
			const double Face = Cy + Side * HY;
			for (int32 Col = 0; Col < 2; ++Col)
			{
				for (int32 Row = 0; Row < 5; ++Row)
				{
					const double X0 = FaceX0 + Col * (W + Gap), Z0 = FaceZ0 + Row * (H + Gap);
					const double YIn = Face - Side * 0.01, YOut = Face + Side * Proud;
					Box(Wood, X0, X0 + W, FMath::Min(YIn, YOut), FMath::Max(YIn, YOut), Z0, Z0 + H);
					const double PX = X0 + W / 2, PZ = Z0 + H * 0.62;
					const double PIn = YOut - Side * 0.003, POut = YOut + Side * 0.022;
					Box(Brass, PX - 0.07, PX + 0.07, FMath::Min(PIn, POut), FMath::Max(PIn, POut), PZ - 0.009, PZ + 0.009);
				}
			}
		}
		if (Mat.X > 0) { Box(Dark, Cx - Mat.X / 2, Cx + Mat.X / 2, Cy - Mat.Y / 2, Cy + Mat.Y / 2, Top - 0.002, Top + 0.003); }
	}

	/**
	 * The viewing easel on the axis at the east end: an oak H-frame (two runners and a cross foot, two uprights 2.35 m
	 * high with two rails), an oak ledge on brass brackets with a lip, brass caps. A painting's back rests on the
	 * uprights' front faces (EaselFrontX), its frame on the ledge.
	 */
	void Easel(FMuseeMesh& Wood, FMuseeMesh& Brass)
	{
		const double FX = R::EaselFrontX, Up = 0.55, Ledge = R::EaselLedge, Span = R::EaselHalfSpan;
		for (const double Y : {-Up, Up})
		{
			Box(Wood, FX - 0.40, FX + 0.50, Y - 0.045, Y + 0.045, -0.01, 0.09);                      // runner
			Box(Wood, FX, FX + 0.08, Y - 0.035, Y + 0.035, 0.08, 2.35);                               // upright
			Box(Brass, FX - 0.005, FX + 0.085, Y - 0.04, Y + 0.04, 2.35, 2.38);                       // its cap
			Box(Brass, FX - 0.11, FX + 0.01, Y - 0.05, Y + 0.05, Ledge - 0.10, Ledge - 0.05);          // the ledge's bracket
		}
		Box(Wood, FX + 0.30, FX + 0.38, -Up - 0.04, Up + 0.04, -0.01, 0.09);                          // the cross foot
		Box(Wood, FX + 0.005, FX + 0.075, -Up, Up, 0.35, 0.43);                                       // the lower rail
		Box(Wood, FX + 0.005, FX + 0.075, -Up, Up, 2.18, 2.26);                                       // the upper rail
		Box(Wood, FX - 0.11, FX + 0.02, -Span, Span, Ledge - 0.05, Ledge);                            // the ledge
		Box(Wood, FX - 0.11, FX - 0.08, -Span, Span, Ledge - 0.01, Ledge + 0.035);                    // its lip
	}

	/** A surface of revolution like Revolve, with a UV0 given per profile point (the same all round). */
	void RevolveUV(FMuseeMesh& Out, double Cx, double Cy, const TArray<FVector2D>& Profile, const TArray<FVector2D>& Normals,
				   const TArray<FVector2D>& UVs, int32 Segments)
	{
		for (int32 K = 0; K + 1 < Profile.Num(); ++K)
		{
			for (int32 I = 0; I < Segments; ++I)
			{
				const double T0 = 2 * UE_DOUBLE_PI * I / Segments, T1 = 2 * UE_DOUBLE_PI * (I + 1) / Segments;
				auto P = [Cx, Cy](const FVector2D& Q, double T) { return At(Cx + Q.X * FMath::Cos(T), Cy + Q.X * FMath::Sin(T), Q.Y); };
				auto N = [](const FVector2D& Q, double T) { return FVector(Q.X * FMath::Cos(T), Q.X * FMath::Sin(T), Q.Y); };
				AddQuad(Out, P(Profile[K], T0), P(Profile[K], T1), P(Profile[K + 1], T1), P(Profile[K + 1], T0),
						N(Normals[K], T0), N(Normals[K], T1), N(Normals[K + 1], T1), N(Normals[K + 1], T0), UVs[K], UVs[K], UVs[K + 1], UVs[K + 1]);
			}
		}
	}

	/** A profile (radius, height) turned about the vertical, each segment with its own normal (turned metal: crisp edges). */
	void RevolveHard(FMuseeMesh& Out, double Cx, double Cy, const TArray<FVector2D>& Profile, bool bOutward, int32 Segments)
	{
		for (int32 K = 0; K + 1 < Profile.Num(); ++K)
		{
			const FVector2D Seg = Profile[K + 1] - Profile[K];
			FVector2D N = FVector2D(Seg.Y, -Seg.X).GetSafeNormal();
			if (!bOutward) { N = -N; }
			Revolve(Out, Cx, Cy, {Profile[K], Profile[K + 1]}, {N, N}, Segments);
		}
	}

	/** The profile moved Distance along its (outward) normals: the other face of a sheet. */
	TArray<FVector2D> OffsetProfile(const TArray<FVector2D>& Profile, double Distance)
	{
		TArray<FVector2D> Out;
		for (int32 K = 0; K < Profile.Num(); ++K)
		{
			FVector2D N(0, 0);
			for (const int32 A : {K - 1, K})
			{
				if (A < 0 || A + 1 >= Profile.Num()) { continue; }
				const FVector2D Seg = Profile[A + 1] - Profile[A];
				N += FVector2D(Seg.Y, -Seg.X).GetSafeNormal();
			}
			Out.Add(Profile[K] + N.GetSafeNormal() * Distance);
		}
		return Out;
	}

	/** A round tube of Radius along Path (plan x, y and height, metres), a ring if bClosed; frames carried along the path. */
	void Tube(FMuseeMesh& Out, const TArray<FVector>& Path, bool bClosed, double Radius, int32 Sides)
	{
		const int32 N = Path.Num();
		if (N < 2) { return; }
		TArray<FVector> T, Nm, Bn;
		for (int32 I = 0; I < N; ++I)
		{
			const FVector Prev = (I > 0) ? Path[I - 1] : (bClosed ? Path[N - 1] : Path[I]);
			const FVector Next = (I + 1 < N) ? Path[I + 1] : (bClosed ? Path[0] : Path[I]);
			T.Add((Next - Prev).GetSafeNormal());
		}
		FVector N0 = FVector::CrossProduct(T[0], FMath::Abs(T[0].Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0)).GetSafeNormal();
		for (int32 I = 0; I < N; ++I)
		{
			if (I > 0) { N0 = (N0 - T[I] * FVector::DotProduct(N0, T[I])).GetSafeNormal(); }
			Nm.Add(N0);
			Bn.Add(FVector::CrossProduct(T[I], N0));
		}
		auto Ring = [&](int32 I, int32 K, FVector& OutP, FVector& OutN)
		{
			const double A = 2 * UE_DOUBLE_PI * K / Sides;
			OutN = Nm[I] * FMath::Cos(A) + Bn[I] * FMath::Sin(A);
			const FVector Q = Path[I] + OutN * Radius;
			OutP = At(Q.X, Q.Y, Q.Z);
		};
		const int32 Spans = bClosed ? N : N - 1;
		for (int32 I = 0; I < Spans; ++I)
		{
			const int32 J = (I + 1) % N;
			for (int32 K = 0; K < Sides; ++K)
			{
				FVector P00, P01, P10, P11, N00, N01, N10, N11;
				Ring(I, K, P00, N00);
				Ring(I, K + 1, P01, N01);
				Ring(J, K, P10, N10);
				Ring(J, K + 1, P11, N11);
				const double U0 = double(K) / Sides, U1 = double(K + 1) / Sides;
				AddQuad(Out, P00, P01, P11, P10, N00, N01, N11, N10, FVector2D(U0, I), FVector2D(U1, I), FVector2D(U1, I + 1), FVector2D(U0, I + 1));
			}
		}
	}

	/** A flat disc at Centre (plan metres) facing Axis. */
	void Disc(FMuseeMesh& Out, const FVector& Centre, const FVector& Axis, double Radius, int32 Sides)
	{
		const FVector A = Axis.GetSafeNormal();
		const FVector U = FVector::CrossProduct(A, FMath::Abs(A.Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0)).GetSafeNormal();
		const FVector V = FVector::CrossProduct(A, U);
		for (int32 K = 0; K < Sides; ++K)
		{
			const double T0 = 2 * UE_DOUBLE_PI * K / Sides, T1 = 2 * UE_DOUBLE_PI * (K + 1) / Sides;
			const FVector P0 = Centre + (U * FMath::Cos(T0) + V * FMath::Sin(T0)) * Radius;
			const FVector P1 = Centre + (U * FMath::Cos(T1) + V * FMath::Sin(T1)) * Radius;
			AddTri(Out, At(Centre.X, Centre.Y, Centre.Z), At(P0.X, P0.Y, P0.Z), At(P1.X, P1.Y, P1.Z), A, A, A, FVector2D(0.5, 0.5),
				   FVector2D(0.5 + 0.5 * FMath::Cos(T0), 0.5 + 0.5 * FMath::Sin(T0)), FVector2D(0.5 + 0.5 * FMath::Cos(T1), 0.5 + 0.5 * FMath::Sin(T1)));
		}
	}

	/** A closed cylinder from A to B (plan metres). */
	void Rod(FMuseeMesh& Out, const FVector& A, const FVector& B, double Radius, int32 Sides)
	{
		Tube(Out, {A, B}, false, Radius, Sides);
		Disc(Out, A, A - B, Radius, Sides);
		Disc(Out, B, B - A, Radius, Sides);
	}

	/** A chain link's wire (round, 6.4 mm) and size: 28 mm long inside, 12 mm wide. */
	constexpr double LinkWire = 0.0032, LinkBend = 0.0092, LinkStraight = 0.016;

	/** One chain link (a stadium of round wire) centred at C, its long axis vertical, in the plane of Across. */
	void ChainLink(FMuseeMesh& Out, const FVector& C, const FVector& Across)
	{
		TArray<FVector> Path;
		constexpr int32 Arc = 10;
		for (int32 End = 0; End < 2; ++End)
		{
			const double Sign = End == 0 ? 1.0 : -1.0;
			for (int32 K = 0; K <= Arc; ++K)
			{
				const double A = UE_DOUBLE_PI * K / Arc;
				Path.Add(C + Across * (Sign * LinkBend * FMath::Cos(A)) + FVector(0, 0, Sign * (LinkStraight / 2 + LinkBend * FMath::Sin(A))));
			}
		}
		Tube(Out, Path, true, LinkWire, 8);
	}

	/**
	 * Weights for the brick's relief, as vertex colour R: 0 where the surface isn't continuous (vertices at the same
	 * place with different normals or UVs: a groin, an arris, a band's edge; and, with bOpenEdges, the mesh's open
	 * edges), 1 elsewhere. The material holds the displacement to 0 there, so Nanite's tessellation can't part a crease.
	 */
	TArray<FColor> WeldWeights(const FMuseeMesh& M, bool bOpenEdges)
	{
		const int32 Num = M.Vertices.Num();
		TMap<FIntVector, int32> Ids;
		TArray<int32> Pid;
		Pid.SetNumUninitialized(Num);
		TArray<int32> First;
		TArray<bool> Crease;
		for (int32 I = 0; I < Num; ++I)
		{
			const FVector& V = M.Vertices[I];   // cm
			const FIntVector Key(FMath::RoundToInt32(V.X * 50.0), FMath::RoundToInt32(V.Y * 50.0), FMath::RoundToInt32(V.Z * 50.0));   // 0.2 mm
			if (const int32* Found = Ids.Find(Key))
			{
				const int32 G = *Found;
				Pid[I] = G;
				const int32 F = First[G];
				if (FVector::DotProduct(M.Normals[I], M.Normals[F]) < 0.9995 || !(M.UVs[I] - M.UVs[F]).IsNearlyZero(2e-4)) { Crease[G] = true; }
			}
			else
			{
				Pid[I] = First.Num();
				Ids.Add(Key, First.Num());
				First.Add(I);
				Crease.Add(false);
			}
		}
		if (bOpenEdges)
		{
			TMap<uint64, int32> Edges;
			for (int32 T = 0; T + 2 < M.Triangles.Num(); T += 3)
			{
				for (int32 E = 0; E < 3; ++E)
				{
					const int32 A = Pid[M.Triangles[T + E]], B = Pid[M.Triangles[T + (E + 1) % 3]];
					if (A == B) { continue; }
					Edges.FindOrAdd((uint64(FMath::Min(A, B)) << 32) | uint64(FMath::Max(A, B)))++;
				}
			}
			for (const TPair<uint64, int32>& E : Edges)
			{
				if (E.Value == 1)
				{
					Crease[int32(E.Key >> 32)] = true;
					Crease[int32(E.Key & 0xffffffffull)] = true;
				}
			}
		}
		TArray<FColor> Colours;
		Colours.SetNumUninitialized(Num);
		for (int32 I = 0; I < Num; ++I) { Colours[I] = Crease[Pid[I]] ? FColor(0, 255, 255, 255) : FColor(255, 255, 255, 255); }
		return Colours;
	}

	/** The vault uplights: the lamp (plan metres and height) and its beam, four per pier. */
	struct FUplight { FVector At; FVector Dir; double Yaw; };
	TArray<FUplight> Uplights()
	{
		TArray<FUplight> Out;
		const double Pitch = FMath::DegreesToRadians(UplightPitch);
		for (int32 K = 0; K < PierCount; ++K)
		{
			for (const double Y : {-PierY, PierY})
			{
				for (const double Sx : {-1.0, 1.0})
				{
					for (const double Sy : {-1.0, 1.0})
					{
						const double Yaw = FMath::Atan2(Sy, Sx);
						const FVector Dir(FMath::Cos(Pitch) * FMath::Cos(Yaw), FMath::Cos(Pitch) * FMath::Sin(Yaw), FMath::Sin(Pitch));
						Out.Add({FVector(PierX(K) + Sx * UplightCorner, Y + Sy * UplightCorner, Spring + UplightLift), Dir, FMath::RadiansToDegrees(Yaw)});
					}
				}
			}
		}
		return Out;
	}

	/** An uplight's fitting: a small bronze can behind the lamp (a bezel round a recessed lens) on a knuckle on the ledge. */
	void UplightFitting(FMuseeMesh& Out, const FUplight& L)
	{
		constexpr double Can = 0.022, Length = 0.075, Gap = 0.008, Bezel = 0.004;
		const FVector Front = L.At - L.Dir * Gap, Back = Front - L.Dir * Length;
		Tube(Out, {Back, Front}, false, Can, 20);
		Disc(Out, Back, -L.Dir, Can, 20);
		const FVector U = FVector::CrossProduct(L.Dir, FVector(0, 0, 1)).GetSafeNormal(), V = FVector::CrossProduct(L.Dir, U);
		for (int32 K = 0; K < 20; ++K)
		{
			const double T0 = 2 * UE_DOUBLE_PI * K / 20, T1 = 2 * UE_DOUBLE_PI * (K + 1) / 20;
			const FVector D0 = U * FMath::Cos(T0) + V * FMath::Sin(T0), D1 = U * FMath::Cos(T1) + V * FMath::Sin(T1);
			const FVector A0 = Front + D0 * Can, A1 = Front + D1 * Can, B0 = Front + D0 * (Can - Bezel), B1 = Front + D1 * (Can - Bezel);
			AddQuad(Out, At(A0.X, A0.Y, A0.Z), At(A1.X, A1.Y, A1.Z), At(B1.X, B1.Y, B1.Z), At(B0.X, B0.Y, B0.Z), L.Dir,
					FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1));
		}
		TArray<FVector> Inside = {Front - L.Dir * 0.006, Front};
		Tube(Out, Inside, false, Can - Bezel, 20);
		Disc(Out, Front - L.Dir * 0.006, L.Dir, Can - Bezel, 20);
		const FVector Mid = (Front + Back) * 0.5;
		Rod(Out, FVector(Mid.X, Mid.Y, Spring - 0.005), Mid, 0.007, 10);
	}

	struct FReserveMeshes
	{
		FMuseeMesh Walls, PierBrick, Stone, Floor, Tracks, Threshold, Vault, Bronze, Cable, Glass, Wood, Brass, Dark;
	};

	/**
	 * One pendant (plan metres; heights above the Reserve's floor). The globe: an opal ball with a neck and a rolled lip
	 * (UV0 (R, 0) on the ball, (R, 1) on the neck). Its neck sits in a spun bronze gallery (a beaded skirt over the ball's shoulder, a band, a shoulder up to
	 * the socket cup) held by three thumb screws; a chain (with the cable) or a rod, up to a canopy at the crown.
	 */
	void Pendant(FReserveMeshes& M, const FPendant& P)
	{
		const double R = P.Radius, Zc = P.GlobeZ, Rn = NeckRadius * R;
		const double ZCut = Zc + FMath::Sqrt(R * R - Rn * Rn);
		const double NeckTop = ZCut + 0.12 * R;
		constexpr int32 Seg = 64;
		// The glass: one shell (its material is the lit opal under a glossy cased skin, see M_GlobeLamp).
		const double ThetaNeck = FMath::Asin(Rn / R);
		TArray<FVector2D> Prof, Nrm, UV;
		constexpr int32 Rings = 40;
		for (int32 J = 0; J <= Rings; ++J)
		{
			const double Th = UE_DOUBLE_PI - (UE_DOUBLE_PI - ThetaNeck) * J / Rings;
			const FVector2D D(FMath::Sin(Th), FMath::Cos(Th));
			Prof.Add(FVector2D(R * D.X, Zc + R * D.Y));
			Nrm.Add(D);
			UV.Add(FVector2D(R, 0));
		}
		// The neck, up to the rolled lip the thumb screws bear under.
		const TArray<FVector2D> Neck = {FVector2D(Rn, ZCut), FVector2D(Rn, NeckTop - 0.006), FVector2D(Rn + 0.004, NeckTop - 0.004),
										FVector2D(Rn + 0.005, NeckTop - 0.002), FVector2D(Rn - 0.004, NeckTop)};
		const TArray<FVector2D> NeckN = {FVector2D(1, 0), FVector2D(1, 0), FVector2D(0.7, -0.7), FVector2D(1, 0), FVector2D(0, 1)};
		for (int32 K = 0; K < Neck.Num(); ++K)
		{
			Prof.Add(Neck[K]);
			Nrm.Add(NeckN[K]);
			UV.Add(FVector2D(R, 1));
		}
		RevolveUV(M.Glass, P.X, P.Y, Prof, Nrm, UV, Seg);

		// The gallery: spun bronze sheet, both faces, closed at the skirt's bead.
		const TArray<FVector2D> Gallery = {
			FVector2D(0.52 * R, ZCut - 0.013), FVector2D(0.535 * R, ZCut - 0.009), FVector2D(0.52 * R, ZCut - 0.005), FVector2D(0.44 * R, ZCut + 0.004),
			FVector2D(0.41 * R, ZCut + 0.008), FVector2D(0.41 * R, ZCut + 0.030), FVector2D(0.38 * R, ZCut + 0.035), FVector2D(0.20 * R, ZCut + 0.041),
			FVector2D(0.13 * R, ZCut + 0.043)};
		const TArray<FVector2D> Inside = OffsetProfile(Gallery, -0.0015);
		RevolveHard(M.Bronze, P.X, P.Y, Gallery, true, Seg);
		RevolveHard(M.Bronze, P.X, P.Y, Inside, false, Seg);
		RevolveHard(M.Bronze, P.X, P.Y, {Inside[0], Gallery[0]}, true, Seg);
		// The socket cup over it, beaded at its top.
		const double CupTop = ZCut + 0.10;
		RevolveHard(M.Bronze, P.X, P.Y, {FVector2D(0.13 * R, ZCut + 0.042), FVector2D(0.13 * R, CupTop - 0.009), FVector2D(0.15 * R, CupTop - 0.007),
										 FVector2D(0.15 * R, CupTop - 0.003), FVector2D(0.12 * R, CupTop), FVector2D(0.04 * R, CupTop + 0.004),
										 FVector2D(0, CupTop + 0.004)}, true, Seg);
		// Three thumb screws through the band, bearing on the neck under its lip: shank, knurled head, a smaller boss.
		const double ScrewZ = ZCut + 0.012, HeadR = R > 0.2 ? 0.0065 : 0.0055;
		for (int32 K = 0; K < 3; ++K)
		{
			const double A = FMath::DegreesToRadians(90.0 + 120.0 * K);
			const FVector D(FMath::Cos(A), FMath::Sin(A), 0), C(P.X, P.Y, ScrewZ);
			Rod(M.Bronze, C + D * (Rn - 0.0005), C + D * (0.41 * R + 0.005), 0.0022, 10);
			Rod(M.Bronze, C + D * (0.41 * R + 0.004), C + D * (0.41 * R + 0.0085), HeadR, 18);
			Rod(M.Bronze, C + D * (0.41 * R + 0.0085), C + D * (0.41 * R + 0.010), HeadR * 0.8, 18);
		}

		// The canopy at the crown (its top inside the vault).
		const double Cz = P.CrownZ, Ck = P.bChain ? 1.0 : 0.85;
		RevolveHard(M.Bronze, P.X, P.Y, {FVector2D(0.070 * Ck, Cz + 0.02), FVector2D(0.070 * Ck, Cz - 0.006), FVector2D(0.064 * Ck, Cz - 0.020),
										 FVector2D(0.045 * Ck, Cz - 0.034), FVector2D(0.020 * Ck, Cz - 0.041), FVector2D(0.012, Cz - 0.043),
										 FVector2D(0.012, Cz - 0.050), FVector2D(0, Cz - 0.050)}, true, Seg);
		if (P.bChain)
		{
			// A loop under the canopy and one on the cup, the chain between (each link turned 90° to the last), and
			// the cable threaded through it: through each link's opening, round each joint on alternate diagonals.
			constexpr double Loop = 0.0095, LoopWire = 0.003;
			const FVector Top(P.X, P.Y, Cz - 0.050 - Loop), Bottom(P.X, P.Y, CupTop + 0.004 + Loop);
			TArray<FVector> Loop1, Loop2;
			for (int32 K = 0; K < 16; ++K)
			{
				const double A = 2 * UE_DOUBLE_PI * K / 16;
				Loop1.Add(Top + FVector(Loop * FMath::Cos(A), 0, Loop * FMath::Sin(A)));
				Loop2.Add(Bottom + FVector(0, Loop * FMath::Cos(A), Loop * FMath::Sin(A)));
			}
			Tube(M.Bronze, Loop1, true, LoopWire, 8);
			Tube(M.Bronze, Loop2, true, LoopWire, 8);
			const double Half = LinkStraight / 2 + LinkBend;   // a link's centre to its wire's end
			const double ChainTop = Top.Z - Loop + Half, ChainBottom = Bottom.Z + Loop - Half;
			const int32 Links = FMath::Max(2, FMath::RoundToInt32((ChainTop - ChainBottom) / 0.028) + 1);
			const double Step = (ChainTop - ChainBottom) / (Links - 1);
			TArray<FVector> Cable = {FVector(P.X, P.Y, Cz - 0.045), FVector(P.X + 0.0065, P.Y + 0.0065, Top.Z)};
			for (int32 K = 0; K < Links; ++K)
			{
				const double Z = ChainTop - Step * K;
				ChainLink(M.Bronze, FVector(P.X, P.Y, Z), K % 2 == 0 ? FVector(0, 1, 0) : FVector(1, 0, 0));
				Cable.Add(FVector(P.X, P.Y, Z));
				if (K + 1 < Links)
				{
					const double S = (K % 2 == 0) ? 1.0 : -1.0;
					Cable.Add(FVector(P.X + S * 0.0068, P.Y + 0.0068, Z - Step / 2));
				}
			}
			Cable.Add(FVector(P.X - 0.0065, P.Y - 0.0065, Bottom.Z));
			Cable.Add(FVector(P.X, P.Y, CupTop + 0.003));
			Tube(M.Cable, Cable, false, 0.0026, 8);
		}
		else
		{
			// A swivel ball under the canopy and a rod (the cable inside it) down to a locknut on the cup.
			TArray<FVector2D> Ball, BallN;
			for (int32 J = 0; J <= 12; ++J)
			{
				const double Th = UE_DOUBLE_PI - UE_DOUBLE_PI * J / 12;
				const FVector2D D(FMath::Sin(Th), FMath::Cos(Th));
				Ball.Add(FVector2D(0.013 * D.X, Cz - 0.062 + 0.013 * D.Y));
				BallN.Add(D);
			}
			Revolve(M.Bronze, P.X, P.Y, Ball, BallN, 24);
			Rod(M.Bronze, FVector(P.X, P.Y, CupTop + 0.002), FVector(P.X, P.Y, Cz - 0.066), RodRadius, 16);
			Rod(M.Bronze, FVector(P.X, P.Y, CupTop + 0.002), FVector(P.X, P.Y, CupTop + 0.014), 0.011, 6);
		}
	}

	FReserveMeshes BuildMeshes()
	{
		FReserveMeshes M;
		const FVector XDir(1, 0, 0), YDir(0, 1, 0), ZDir(0, 0, 1);
		const double OuterX0 = RoomX0 - WallThickness, OuterX1 = RoomX1 + WallThickness, OuterHalf = HalfWidth + WallThickness;

		// ---- Walls: inner faces from under the floor to the box's top (the vault meets them), outer faces, the top.
		Panel(M.Walls, At(RoomX0, -HalfWidth, -Sink), XDir, ZDir, RoomX1 - RoomX0, LawnTop + Sink, YDir, FVector2D(RoomX0, -Sink));
		Panel(M.Walls, At(RoomX0, HalfWidth, -Sink), XDir, ZDir, RoomX1 - RoomX0, LawnTop + Sink, -YDir, FVector2D(RoomX0, -Sink));
		// The east wall: up to the Salon's top between ±SalonTopHalf, the lawn's beyond.
		Panel(M.Walls, At(RoomX1, -HalfWidth, -Sink), YDir, ZDir, HalfWidth - SalonTopHalf, LawnTop + Sink, -XDir, FVector2D(-HalfWidth, -Sink));
		Panel(M.Walls, At(RoomX1, -SalonTopHalf, -Sink), YDir, ZDir, 2 * SalonTopHalf, SalonTop + Sink, -XDir, FVector2D(-SalonTopHalf, -Sink));
		Panel(M.Walls, At(RoomX1, SalonTopHalf, -Sink), YDir, ZDir, HalfWidth - SalonTopHalf, LawnTop + Sink, -XDir, FVector2D(SalonTopHalf, -Sink));
		Panel(M.Walls, At(OuterX0, -OuterHalf, -OuterSink), XDir, ZDir, OuterX1 - OuterX0, LawnTop + OuterSink, -YDir, FVector2D(OuterX0, -OuterSink));
		Panel(M.Walls, At(OuterX0, OuterHalf, -OuterSink), XDir, ZDir, OuterX1 - OuterX0, LawnTop + OuterSink, YDir, FVector2D(OuterX0, -OuterSink));
		Panel(M.Walls, At(OuterX1, -OuterHalf, -OuterSink), YDir, ZDir, 2 * OuterHalf, LawnTop + OuterSink, XDir, FVector2D(-OuterHalf, -OuterSink));
		// The top: the lawn's level round a raised block under the Salon, its risers facing out.
		Panel(M.Walls, At(OuterX0, -OuterHalf, LawnTop), XDir, YDir, OuterX1 - OuterX0, OuterHalf - SalonTopHalf, ZDir, FVector2D(OuterX0, -OuterHalf));
		Panel(M.Walls, At(OuterX0, SalonTopHalf, LawnTop), XDir, YDir, OuterX1 - OuterX0, OuterHalf - SalonTopHalf, ZDir, FVector2D(OuterX0, SalonTopHalf));
		Panel(M.Walls, At(OuterX0, -SalonTopHalf, LawnTop), XDir, YDir, SalonTopX0 - OuterX0, 2 * SalonTopHalf, ZDir, FVector2D(OuterX0, -SalonTopHalf));
		Panel(M.Walls, At(SalonTopX1, -SalonTopHalf, LawnTop), XDir, YDir, OuterX1 - SalonTopX1, 2 * SalonTopHalf, ZDir, FVector2D(SalonTopX1, -SalonTopHalf));
		Panel(M.Walls, At(SalonTopX0, -SalonTopHalf, LawnTop), XDir, ZDir, SalonTopX1 - SalonTopX0, SalonTop - LawnTop, -YDir, FVector2D(SalonTopX0, LawnTop));
		Panel(M.Walls, At(SalonTopX0, SalonTopHalf, LawnTop), XDir, ZDir, SalonTopX1 - SalonTopX0, SalonTop - LawnTop, YDir, FVector2D(SalonTopX0, LawnTop));
		Panel(M.Walls, At(SalonTopX0, -SalonTopHalf, LawnTop), YDir, ZDir, 2 * SalonTopHalf, SalonTop - LawnTop, -XDir, FVector2D(-SalonTopHalf, LawnTop));
		Panel(M.Walls, At(SalonTopX1, -SalonTopHalf, LawnTop), YDir, ZDir, 2 * SalonTopHalf, SalonTop - LawnTop, XDir, FVector2D(-SalonTopHalf, LawnTop));
		Panel(M.Walls, At(SalonTopX0, -SalonTopHalf, SalonTop), XDir, YDir, SalonTopX1 - SalonTopX0, 2 * SalonTopHalf, ZDir, FVector2D(SalonTopX0, -SalonTopHalf));

		// The west wall, both faces, with the door where the long stair arrives; its jambs and arched soffit. The inner
		// face rises to the Salon's top between ±SalonTopHalf (the nave's vault meets it there), the outer to the lawn's.
		for (int32 Face = 0; Face < 2; ++Face)
		{
			const bool bInner = Face == 0;
			const double X = bInner ? RoomX0 : OuterX0;
			const double Half = bInner ? HalfWidth : OuterHalf;
			const double Bottom = bInner ? -Sink : -OuterSink;
			const double MidTop = bInner ? SalonTop : LawnTop;
			const FVector N = bInner ? XDir : -XDir;
			for (const double Sign : {-1.0, 1.0})
			{
				// From the door's jamb out to ±SalonTopHalf at MidTop, then on to the corner at LawnTop.
				const double A = Sign * DoorHalf, B = Sign * SalonTopHalf, C = Sign * Half;
				Panel(M.Walls, At(X, FMath::Min(A, B), Bottom), YDir, ZDir, FMath::Abs(B - A), MidTop - Bottom, N, FVector2D(FMath::Min(A, B), Bottom));
				Panel(M.Walls, At(X, FMath::Min(B, C), Bottom), YDir, ZDir, FMath::Abs(C - B), LawnTop - Bottom, N, FVector2D(FMath::Min(B, C), Bottom));
			}
			for (int32 K = 0; K < DoorSegments; ++K)
			{
				const double A0 = UE_DOUBLE_PI * K / DoorSegments, A1 = UE_DOUBLE_PI * (K + 1) / DoorSegments;
				const double Y0 = -DoorHalf * FMath::Cos(A0), Y1 = -DoorHalf * FMath::Cos(A1);
				const double Z0 = DoorSpring + DoorHalf * FMath::Sin(A0), Z1 = DoorSpring + DoorHalf * FMath::Sin(A1);
				AddQuad(M.Walls, At(X, Y0, Z0), At(X, Y1, Z1), At(X, Y1, MidTop), At(X, Y0, MidTop), N,
						FVector2D(Y0, Z0), FVector2D(Y1, Z1), FVector2D(Y1, MidTop), FVector2D(Y0, MidTop));
			}
		}
		Panel(M.Walls, At(OuterX0, -DoorHalf, -Sink), XDir, ZDir, WallThickness, DoorSpring + Sink, YDir, FVector2D(OuterX0, -Sink));
		Panel(M.Walls, At(OuterX0, DoorHalf, -Sink), XDir, ZDir, WallThickness, DoorSpring + Sink, -YDir, FVector2D(OuterX0, -Sink));
		for (int32 K = 0; K < DoorSegments; ++K)
		{
			const double A0 = UE_DOUBLE_PI * K / DoorSegments, A1 = UE_DOUBLE_PI * (K + 1) / DoorSegments;
			const double Y0 = -DoorHalf * FMath::Cos(A0), Y1 = -DoorHalf * FMath::Cos(A1);
			const double Z0 = DoorSpring + DoorHalf * FMath::Sin(A0), Z1 = DoorSpring + DoorHalf * FMath::Sin(A1);
			const FVector N0(0, FMath::Cos(A0), -FMath::Sin(A0)), N1(0, FMath::Cos(A1), -FMath::Sin(A1));
			const double V0 = DoorSpring + DoorHalf * A0, V1 = DoorSpring + DoorHalf * A1;
			AddQuad(M.Walls, At(OuterX0, Y0, Z0), At(RoomX0, Y0, Z0), At(RoomX0, Y1, Z1), At(OuterX0, Y1, Z1), N0, N0, N1, N1,
					FVector2D(OuterX0, V0), FVector2D(RoomX0, V0), FVector2D(RoomX0, V1), FVector2D(OuterX0, V1));
		}

		// ---- Piers: moulded travertine base, chamfered brick shaft, travertine impost (a cavetto under a plain block).
		const TArray<FVector2D> BaseProfile = {FVector2D(StoneHalf, -Sink), FVector2D(StoneHalf, 0.36), FVector2D(0.41, 0.44), FVector2D(0.41, 0.50),
											   FVector2D(0.37, BaseTop), FVector2D(UnderHalf, BaseTop)};
		const TArray<FVector2D> ImpostProfile = {FVector2D(UnderHalf, ImpostBottom), FVector2D(0.37, ImpostBottom), FVector2D(0.41, ImpostBottom + 0.05),
												 FVector2D(StoneHalf, ImpostBottom + 0.14), FVector2D(StoneHalf, Spring), FVector2D(UnderHalf, Spring)};
		for (int32 K = 0; K < PierCount; ++K)
		{
			for (const double Y : {-PierY, PierY})
			{
				SquareSweep(M.Stone, PierX(K), Y, BaseProfile);
				ChamferedShaft(M.PierBrick, PierX(K), Y, PierHalf, Chamfer, BaseTop - 0.02, ImpostBottom + 0.02);
				SquareSweep(M.Stone, PierX(K), Y, ImpostProfile);
			}
			// Responds on the side walls, under the bands that cross the aisles.
			Respond(M.PierBrick, M.Stone, PierX(K), -HalfWidth, FVector2D(0, 1));
			Respond(M.PierBrick, M.Stone, PierX(K), HalfWidth, FVector2D(0, -1));
		}
		// … and on the end walls, under the arcades' first and last bands.
		for (const double Y : {-PierY, PierY})
		{
			Respond(M.PierBrick, M.Stone, RoomX0, Y, FVector2D(1, 0));
			Respond(M.PierBrick, M.Stone, RoomX1, Y, FVector2D(-1, 0));
		}

		// ---- The floor, the tracks flush in it, and the notch for the stair's landing (which is at the same height).
		const TArray<FRect> TrackRects = TrackRectangles();
		TArray<FRect> Holes = TrackRects;
		Holes.Add({RoomX0, LandingEnd, -LandingHalf, LandingHalf});
		FloorWithHoles(M.Floor, {RoomX0, RoomX1, -HalfWidth, HalfWidth}, Holes);
		for (const FRect& S : TrackRects)
		{
			// The slot's bronze lining: its lips flush with the floor, its sides, ends and bottom (closed: nothing under the floor shows).
			const double SX0 = S.X0 + SlotLip, SX1 = S.X1 - SlotLip, D = -SlotDepth;
			for (const FVector2D Lip : {FVector2D(S.X0, SX0), FVector2D(SX1, S.X1)})
			{
				AddQuad(M.Tracks, At(Lip.X, S.Y0, 0), At(Lip.Y, S.Y0, 0), At(Lip.Y, S.Y1, 0), At(Lip.X, S.Y1, 0), ZDir,
						FVector2D(S.Y0, Lip.X), FVector2D(S.Y0, Lip.Y), FVector2D(S.Y1, Lip.Y), FVector2D(S.Y1, Lip.X));
			}
			AddQuad(M.Tracks, At(SX0, S.Y0, D), At(SX0, S.Y1, D), At(SX0, S.Y1, 0), At(SX0, S.Y0, 0), XDir,
					FVector2D(S.Y0, D), FVector2D(S.Y1, D), FVector2D(S.Y1, 0), FVector2D(S.Y0, 0));
			AddQuad(M.Tracks, At(SX1, S.Y0, D), At(SX1, S.Y1, D), At(SX1, S.Y1, 0), At(SX1, S.Y0, 0), -XDir,
					FVector2D(S.Y0, D), FVector2D(S.Y1, D), FVector2D(S.Y1, 0), FVector2D(S.Y0, 0));
			AddQuad(M.Tracks, At(SX0, S.Y0, D), At(SX1, S.Y0, D), At(SX1, S.Y1, D), At(SX0, S.Y1, D), ZDir,
					FVector2D(S.Y0, SX0), FVector2D(S.Y0, SX1), FVector2D(S.Y1, SX1), FVector2D(S.Y1, SX0));
			for (const double Y : {S.Y0, S.Y1})
			{
				const FVector In = Y == S.Y0 ? YDir : -YDir;
				AddQuad(M.Tracks, At(SX0, Y, D), At(SX1, Y, D), At(SX1, Y, 0), At(SX0, Y, 0), In,
						FVector2D(SX0, D), FVector2D(SX1, D), FVector2D(SX1, 0), FVector2D(SX0, 0));
			}
		}
		// Hidden, collision only: under the landing and the door, in case the stair's own collision is off.
		Panel(M.Threshold, At(OuterX0, -LandingHalf, 0), XDir, YDir, LandingEnd - OuterX0, 2 * LandingHalf, ZDir, FVector2D(OuterX0, -LandingHalf));

		// ---- The vault.
		const TArray<FVector2D> Spans = BaySpans();
		const TArray<FRow> RowList = Rows();
		TArray<FArch> NaveBays, AisleBays;
		for (const FVector2D& S : Spans) { NaveBays.Emplace(S.X, S.Y, NaveRise); AisleBays.Emplace(S.X, S.Y, AisleRise); }
		TArray<FArch> RowArches;
		for (const FRow& Row : RowList) { RowArches.Emplace(Row.Lo, Row.Hi, Row.Rise); }
		for (int32 B = 0; B < Spans.Num(); ++B)
		{
			for (int32 Rw = 0; Rw < RowList.Num(); ++Rw)
			{
				AddWeb(M.Vault, Rw == 1 ? NaveBays[B] : AisleBays[B], RowArches[Rw]);
			}
		}
		for (int32 K = 0; K < PierCount; ++K)
		{
			for (const FArch& Row : RowArches) { AddBandAcrossRow(M.Vault, PierX(K) - PierHalf, PierX(K) + PierHalf, Row); }
		}
		for (int32 B = 0; B < Spans.Num(); ++B)
		{
			// North arcade: the aisle on its y−, the nave on its y+; south arcade the other way round.
			AddBandAcrossBay(M.Vault, -PierY - PierHalf, -PierY + PierHalf, AisleBays[B], AisleBays[B], NaveBays[B]);
			AddBandAcrossBay(M.Vault, PierY - PierHalf, PierY + PierHalf, AisleBays[B], NaveBays[B], AisleBays[B]);
		}
		// Over the piers, the springing (inside the brick: it closes the vault's surface).
		for (int32 K = 0; K < PierCount; ++K)
		{
			for (const double Y : {-PierY, PierY})
			{
				// Split as the bands are across, so every edge is shared vertex for vertex.
				for (int32 A = 0; A < BandAcross; ++A)
				{
					for (int32 B = 0; B < BandAcross; ++B)
					{
						const double X0 = PierX(K) - PierHalf + 2 * PierHalf * A / BandAcross, X1 = PierX(K) - PierHalf + 2 * PierHalf * (A + 1) / BandAcross;
						const double Y0 = Y - PierHalf + 2 * PierHalf * B / BandAcross, Y1 = Y - PierHalf + 2 * PierHalf * (B + 1) / BandAcross;
						AddQuad(M.Vault, At(X0, Y0, Spring), At(X1, Y0, Spring), At(X1, Y1, Spring), At(X0, Y1, Spring), -ZDir,
								FVector2D(X0, Y0), FVector2D(X1, Y0), FVector2D(X1, Y1), FVector2D(X0, Y1));
					}
				}
			}
		}

		// ---- The pendants, at the crown of each bay, and the uplights' fittings on the imposts.
		for (const FPendant& P : PendantList()) { Pendant(M, P); }
		for (const FUplight& L : Uplights()) { UplightFitting(M.Bronze, L); }

		// ---- The void over the vault, closed: a ceiling just under the box's top (over the nave, under the Salon's
		// floor; over the aisles, under the lawn) and a riser between, so nothing behind the vault is lit or seen.
		const double CapHigh = SalonTop - 0.03, CapLow = LawnTop - 0.03;
		Panel(M.Walls, At(RoomX0, -SalonTopHalf, CapHigh), XDir, YDir, RoomX1 - RoomX0, 2 * SalonTopHalf, -ZDir, FVector2D(RoomX0, -SalonTopHalf));
		for (const double Sign : {-1.0, 1.0})
		{
			const double Y0 = Sign < 0 ? -HalfWidth : SalonTopHalf;
			Panel(M.Walls, At(RoomX0, Y0, CapLow), XDir, YDir, RoomX1 - RoomX0, HalfWidth - SalonTopHalf, -ZDir, FVector2D(RoomX0, Y0));
			Panel(M.Walls, At(RoomX0, Sign * SalonTopHalf, CapLow), XDir, ZDir, RoomX1 - RoomX0, CapHigh - CapLow, YDir * Sign, FVector2D(RoomX0, CapLow));
		}

		// ---- The plan chests: an island in each aisle's last two bays; on the south ones a felt mat for each pastel.
		const FVector2D Mats[2] = {FVector2D(0.83 + 0.16, 0.60 + 0.16), FVector2D(0.42 + 0.16, 0.58 + 0.16)};   // The Tub, The Star
		for (int32 I = 0; I < 2; ++I)
		{
			PlanChests(M.Wood, M.Brass, M.Dark, R::BayCentreX(R::ChestBays[I]), -R::AisleCentreY, FVector2D::ZeroVector);
			PlanChests(M.Wood, M.Brass, M.Dark, R::BayCentreX(R::ChestBays[I]), R::AisleCentreY, Mats[I]);
		}
		Easel(M.Wood, M.Brass);
		return M;
	}
}

AReserveStructure::AReserveStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* ComponentName, bool bCollision)
	{
		UProceduralMeshComponent* Component = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		Component->SetupAttachment(RootComponent);
		Component->bUseAsyncCooking = true;
		if (bCollision)
		{
			// Walls, piers, floor and chests collide as themselves (complex as simple); the floor is walkable.
			Component->bUseComplexAsSimpleCollision = true;
			Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Component;
	};
	Structure = Make(TEXT("Structure"), true);
	Vault = Make(TEXT("Vault"), false);
	Pendants = Make(TEXT("Pendants"), false);
	Globes = Make(TEXT("Globes"), false);
	Globes->SetCastShadow(false);   // the lamp sits inside (the gallery, in Pendants, does cast)
	Furniture = Make(TEXT("Furniture"), true);

	auto Path = [](const TCHAR* Folder, const TCHAR* Name)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name)));
	};
	const TCHAR* Materials = TEXT("/Game/Museum/Materials");
	const TCHAR* Imported = TEXT("/Game/Museum/Materials/USD");
	BrickMaterial = Path(Materials, TEXT("MI_Brick_Vault"));        // Scripts/reserve_materials.py
	BrickFallbackMaterial = Path(Materials, TEXT("MI_Brick_Coursed"));
	StoneMaterial = Path(Materials, TEXT("M_Travertine_Honed"));
	FloorMaterial = Path(Imported, TEXT("MI_stone_slab_EFE6DA"));   // the Reserve's dark polished concrete (materials.py)
	TrackMaterial = Path(Imported, TEXT("MI_bronze_warm"));
	BronzeMaterial = Path(Imported, TEXT("MI_bronze"));
	GlobeMaterial = Path(Materials, TEXT("M_GlobeLamp"));           // Scripts/reserve_materials.py
	GlobeFallbackMaterial = Path(Materials, TEXT("M_OpalGlobe"));
	CableMaterial = Path(Materials, TEXT("M_Cable"));
	TimberMaterial = Path(Imported, TEXT("MI_plan_chest"));
	BrassMaterial = Path(Imported, TEXT("MI_gilt_r35"));
	FeltMaterial = Path(Imported, TEXT("MI_bronze_dark"));
}

void AReserveStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

TArray<FString> AReserveStructure::GetReplacedImportPrims()
{
	return {
		TEXT("/Museum/Reserve/The_Reserve/Reserve_walls"), TEXT("/Museum/Reserve/The_Reserve/Reserve_floor"),
		TEXT("/Museum/Reserve/The_Reserve/Viewing_aisle"), TEXT("/Museum/Reserve/The_Reserve/Reserve_vault"),
		TEXT("/Museum/Reserve/The_Reserve/Reserve_columns"),
		// The export's plan chests (plain boxes) and their glass: the native islands replace them; the pastels stay.
		TEXT("/Museum/Reserve/The_Reserve/Plan_chest"), TEXT("/Museum/Reserve/The_Reserve/Plan_chest_2"),
		TEXT("/Museum/Reserve/The_Reserve/Chest_glass"), TEXT("/Museum/Reserve/The_Reserve/Chest_glass_2"),
		// The export's easel (three thin sticks): the native oak easel replaces it.
		TEXT("/Museum/Reserve/The_Reserve/Viewing_easel"),
	};
}

TArray<FVector> AReserveStructure::PendantLocalPositions()
{
	TArray<FVector> Positions;
	for (const ReserveBuild::FPendant& P : ReserveBuild::PendantList())
	{
		Positions.Add(MuseePlan::At(P.X, P.Y, P.GlobeZ - ReserveBuild::Depth));
	}
	return Positions;
}

TArray<FVector> AReserveStructure::GetPendantPositions() const
{
	TArray<FVector> Positions = PendantLocalPositions();
	const FTransform& ActorXf = GetActorTransform();
	for (FVector& P : Positions) { P = ActorXf.TransformPosition(P); }
	return Positions;
}

TArray<FTransform> AReserveStructure::GetUplightTransforms()
{
	TArray<FTransform> Out;
	for (const ReserveBuild::FUplight& L : ReserveBuild::Uplights())
	{
		const FRotator Rotation(ReserveBuild::UplightPitch, L.Yaw, 0.0);
		Out.Add(FTransform(Rotation, MuseePlan::At(L.At.X, L.At.Y, L.At.Z - ReserveBuild::Depth)));
	}
	return Out;
}

TArray<FVector> AReserveStructure::GetPierImposts()
{
	TArray<FVector> Out;
	for (int32 K = 0; K < ReserveBuild::PierCount; ++K)
	{
		for (const double Y : {-ReserveBuild::PierY, ReserveBuild::PierY})
		{
			Out.Add(MuseePlan::At(ReserveBuild::PierX(K), Y, ReserveBuild::Spring - ReserveBuild::Depth));
		}
	}
	return Out;
}

void AReserveStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	const ReserveBuild::FReserveMeshes Meshes = ReserveBuild::BuildMeshes();

	// Materials still being made are skipped (or stood in for) until they exist.
	auto Pick = [](const TSoftObjectPtr<UMaterialInterface>& Wanted, const TSoftObjectPtr<UMaterialInterface>& Fallback) -> UMaterialInterface*
	{
		if (UMaterialInterface* Loaded = Wanted.LoadSynchronous()) { return Loaded; }
		return Fallback.IsNull() ? nullptr : Fallback.LoadSynchronous();
	};
	auto Write = [](UProceduralMeshComponent* Component, int32 Section, const FMuseeMesh& Data, bool bCollision, UMaterialInterface* Material,
					const TArray<FColor>& Colours = TArray<FColor>())
	{
		Component->CreateMeshSection(Section, Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, Colours, Data.Tangents, bCollision);
		if (Material) { Component->SetMaterial(Section, Material); }
	};
	const TSoftObjectPtr<UMaterialInterface> NoFallback;
	UMaterialInterface* Brick = Pick(BrickMaterial, BrickFallbackMaterial);

	for (UProceduralMeshComponent* Component : {Structure.Get(), Vault.Get(), Pendants.Get(), Globes.Get(), Furniture.Get()}) { Component->ClearAllMeshSections(); }
	// The brick's relief is held flat on its creases (vertex colour R, ReserveBuild::WeldWeights); the vault's open
	// edges (against the walls) too.
	Write(Structure, 0, Meshes.Walls, true, Brick, ReserveBuild::WeldWeights(Meshes.Walls, false));
	Write(Structure, 1, Meshes.PierBrick, true, Brick, ReserveBuild::WeldWeights(Meshes.PierBrick, false));
	Write(Structure, 2, Meshes.Stone, true, Pick(StoneMaterial, NoFallback));
	Write(Structure, 3, Meshes.Floor, true, Pick(FloorMaterial, NoFallback));
	Write(Structure, 4, Meshes.Tracks, true, Pick(TrackMaterial, BronzeMaterial));
	Write(Structure, 5, Meshes.Threshold, true, nullptr);
	Structure->SetMeshSectionVisible(5, false);
	Write(Vault, 0, Meshes.Vault, false, Brick, ReserveBuild::WeldWeights(Meshes.Vault, true));
	Write(Pendants, 0, Meshes.Bronze, false, Pick(BronzeMaterial, NoFallback));
	Write(Pendants, 1, Meshes.Cable, false, Pick(CableMaterial, FeltMaterial));
	Write(Globes, 0, Meshes.Glass, false, Pick(GlobeMaterial, GlobeFallbackMaterial));
	Write(Furniture, 0, Meshes.Wood, true, Pick(TimberMaterial, NoFallback));
	Write(Furniture, 1, Meshes.Brass, true, Pick(BrassMaterial, NoFallback));
	Write(Furniture, 2, Meshes.Dark, true, Pick(FeltMaterial, NoFallback));
}
