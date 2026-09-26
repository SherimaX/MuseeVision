#include "Atrium/AtriumBaseStructure.h"

#include "MuseeVision.h"
#include "Components/BoxComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Texture.h"
#include "Geometry/MuseeBake.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"
#include "Salon/SalonKit.h"

/**
 * The Atrium's level 0, in plan metres (x east, y south, z up; the actor at the world origin). Plan
 * angles run from east towards south about the Atrium's centre (54, 0): the west door is at 180°, the
 * pilasters at 15° + 30°k (the bay boundaries, bearings 105° + 30°k), the bay letters at 210° + 30°i.
 * A named namespace (the module builds in unity files).
 */
namespace AtriumBaseKit
{
	namespace E = MuseePlan::Elan;
	namespace A = MuseePlan::Atrium;
	using SalonKit::FMeshData;
	using SalonKit::FProfile;
	using SalonKit::FFrame;

	constexpr double Pi = UE_DOUBLE_PI;
	constexpr double Turn = 2.0 * UE_DOUBLE_PI;
	constexpr double Deg = UE_DOUBLE_PI / 180.0;
	constexpr double R = E::Radius;                    // 14: the drum's inner face (and its glass)
	constexpr double ROut = E::Radius + E::Wall;       // 14.8: the stone base's outer face
	constexpr double Top = E::BaseHeight;              // 6: the coping's top, where the glass stands
	constexpr double Sink = 0.002;                     // how far a face that stops runs on into the solid it meets
	constexpr int32 RoundSegments = 384;               // round a full circle (0.94°: 0.5 mm off the true circle at r 14.8)
	constexpr double RoundStep = Turn / RoundSegments;

	// The floor: rings of radial slabs; the joints a shade darker.
	constexpr double JointWidth = 0.006;
	constexpr double RingJoints[6] = {4.583, 6.107, 7.630, 9.153, 10.677, 12.2};

	// The LED cove at the wall's foot.
	constexpr double CoveBack = R + 0.09;
	constexpr double CoveTop = 0.14;
	constexpr double LedBottom = 0.03, LedTop = 0.055;
	constexpr double CoveStop = 0.25;                  // the cove stops this far short of the door's jambs
	constexpr int32 LightsPerBay = 3;
	constexpr double LightInset = 0.02;                // the lights stand this far in front of the cove's back
	constexpr double LightTiltDeg = 20.0;              // … facing out of the cove, a little down

	// The coping band (5.72 → 6 m), 10 cm proud of the wall, round the pilasters' capitals.
	constexpr double BandBottom = 5.72;
	constexpr double BandFace = R - 0.10;
	constexpr double BandEase = 0.03;                  // the chamfer on its top edge

	// The pilasters: shafts 1.6 × 0.64 m, capitals and plinths 3–4 cm proud of them.
	constexpr int32 Pilasters = 12;
	constexpr double FirstPilasterDeg = 15.0;
	constexpr double ShaftFront = 13.36, ShaftHalf = 0.80;
	constexpr double CapFront = 13.32, CapHalf = 0.83;
	constexpr double PlinthFront = 13.32, PlinthHalf = 0.83, PlinthTop = 0.30, PlinthEase = 0.01;

	// The west door to the Hall of Light: 4 m, square-headed at 4.5 m.
	constexpr double DoorHalf = E::DoorWidth / 2, DoorHead = E::DoorHeight;
	constexpr int32 DoorSteps = 16;
	constexpr double OuterSink = 0.30;                 // the outer face goes this far below the ground

	// Bronze inlays, cast 2 mm proud.
	constexpr double LetterHeight = 0.30, InlayTop = 0.002, InlayBottom = -0.001;
	constexpr double LetterThick = 0.14, LetterThin = 0.034;   // of the letter's height

	// The stele: low-iron glass set into a brass foot.
	constexpr double GlassThick = 0.06, GlassBottom = 0.06;
	constexpr double FootHalfX = 0.2, FootHalfY = 0.85, FootTop = 0.08, FootEase = 0.006;

	const FVector2D Hub(E::CentreX, E::CentreY);
	const FVector Up(0, 0, 1);

	FVector2D Dir(double T) { return FVector2D(FMath::Cos(T), FMath::Sin(T)); }
	FVector2D OnCircle(double Rad, double T) { return Hub + Dir(T) * Rad; }
	FVector At(const FVector2D& P, double Z) { return FVector(P.X, P.Y, Z); }
	FVector Ring3(double Rad, double T, double Z) { return At(OnCircle(Rad, T), Z); }
	FVector Radial(double T) { return FVector(FMath::Cos(T), FMath::Sin(T), 0); }
	FVector Along(double T) { return FVector(-FMath::Sin(T), FMath::Cos(T), 0); }
	double Wrap(double T) { T = FMath::Fmod(T, Turn); return T < 0 ? T + Turn : T; }
	double AngleOf(const FVector2D& P) { return Wrap(FMath::Atan2(P.Y - Hub.Y, P.X - Hub.X)); }
	FVector2D WallUV(double Rad, double T, double Z) { return FVector2D(Rad * T, -Z); }

	/** The door's jambs (y ±2 about the centre) on a circle: y = +2 is the south jamb (the smaller angle). */
	double JambSouth(double Rad) { return Pi - FMath::Asin(DoorHalf / Rad); }
	double JambNorth(double Rad) { return Pi + FMath::Asin(DoorHalf / Rad); }
	/** The point on the circle Rad at plan y (about the centre), on the west side. */
	FVector2D WestAt(double Rad, double Y) { return FVector2D(Hub.X - FMath::Sqrt(Rad * Rad - Y * Y), Hub.Y + Y); }

	double PilasterAngle(int32 K) { return (FirstPilasterDeg + 30.0 * K) * Deg; }

	TArray<double> SortedUnique(TArray<double> Values, double Tolerance = 1e-9)
	{
		Values.Sort();
		TArray<double> Out;
		for (const double V : Values)
		{
			if (Out.Num() == 0 || V - Out.Last() > Tolerance) { Out.Add(V); }
		}
		return Out;
	}

	/**
	 * Triangles between two chains that run the same way round (ascending angle), from a shared first
	 * rung to a shared last one: each step advances the chain whose next point comes first.
	 */
	void Zipper(FMeshData& M, const TArray<FVector>& In, const TArray<double>& InA, const TArray<FVector>& Out, const TArray<double>& OutA,
				const FVector& N, TFunctionRef<FVector2D(const FVector&)> UV)
	{
		if (In.Num() < 1 || Out.Num() < 1) { return; }
		const int32 BI = M.Positions.Num();
		for (const FVector& P : In) { M.Vertex(P, N, UV(P)); }
		const int32 BO = M.Positions.Num();
		for (const FVector& P : Out) { M.Vertex(P, N, UV(P)); }
		int32 I = 0, J = 0;
		while (I + 1 < In.Num() || J + 1 < Out.Num())
		{
			const bool bIn = J + 1 >= Out.Num() || (I + 1 < In.Num() && InA[I + 1] <= OutA[J + 1]);
			if (bIn) { M.Tri(BI + I, BI + I + 1, BO + J); ++I; }
			else { M.Tri(BI + I, BO + J + 1, BO + J); ++J; }
		}
	}

	/** A quad of a curved face: corners at angles T0 and T1, heights Z0 and Z1 on the circle Rad, facing out (Sign +1) or in. */
	void CurvedQuad(FMeshData& M, double Rad, double T0, double T1, double Z0, double Z1, double Sign)
	{
		const FVector N0 = Radial(T0) * Sign, N1 = Radial(T1) * Sign;
		const int32 A0 = M.Vertex(Ring3(Rad, T0, Z0), N0, WallUV(Rad, T0, Z0));
		const int32 A1 = M.Vertex(Ring3(Rad, T1, Z0), N1, WallUV(Rad, T1, Z0));
		const int32 B1 = M.Vertex(Ring3(Rad, T1, Z1), N1, WallUV(Rad, T1, Z1));
		const int32 B0 = M.Vertex(Ring3(Rad, T0, Z1), N0, WallUV(Rad, T0, Z1));
		M.Quad(A0, A1, B1, B0);
	}

	/** A column of a curved face between T0 and T1, split at every height in Zs (ascending). */
	void CurvedColumn(FMeshData& M, double Rad, double T0, double T1, const TArray<double>& Zs, double Sign)
	{
		for (int32 K = 0; K + 1 < Zs.Num(); ++K) { CurvedQuad(M, Rad, T0, T1, Zs[K], Zs[K + 1], Sign); }
	}

	// ============================================================================ The floor

	struct FFloorBand { double R0, R1; int32 Joints; };

	TArray<FFloorBand> FloorBands()
	{
		const double* J = RingJoints;
		return {{A::FloorInnerRadius, A::WaitRingIn, 12}, {A::WaitRingOut, J[0], 12}, {J[0], J[1], 24}, {J[1], J[2], 24},
				{J[2], J[3], 36}, {J[3], J[4], 36}, {J[4], J[5], 48}, {J[5], CoveBack, 36}};
	}

	/** A band's radial joints: one on every pilaster's line (15° + 30°k), and evenly between. */
	TArray<double> JointAngles(int32 Count)
	{
		TArray<double> Out;
		for (int32 K = 0; K < Count; ++K) { Out.Add(Wrap(FirstPilasterDeg * Deg + Turn * K / Count)); }
		return Out;
	}

	double JointHalfAngle(const FFloorBand& Band) { return 0.5 * JointWidth / (0.5 * (Band.R0 + Band.R1)); }

	/** The angular distance between two angles. */
	double AngleGap(double X, double Y) { const double D = Wrap(X - Y); return FMath::Min(D, Turn - D); }

	struct FFloorMeshes { FMeshData Slab, Joint, Ring; };

	void BuildFloor(FFloorMeshes& Out)
	{
		const TArray<FFloorBand> Bands = FloorBands();
		const double DoorS = JambSouth(R), DoorN = JambNorth(R);

		TArray<double> Ts;
		for (int32 I = 0; I < RoundSegments; ++I) { Ts.Add(RoundStep * I); }
		for (const FFloorBand& Band : Bands)
		{
			const double H = JointHalfAngle(Band);
			for (const double T : JointAngles(Band.Joints)) { Ts.Add(Wrap(T - H)); Ts.Add(Wrap(T + H)); }
		}
		Ts.Add(DoorS);
		Ts.Add(DoorN);
		Ts = SortedUnique(Ts);

		TArray<double> Rs = {A::FloorInnerRadius, A::WaitRingIn, A::WaitRingOut, R, CoveBack};
		for (const double J : RingJoints) { Rs.Add(J - JointWidth / 2); Rs.Add(J + JointWidth / 2); }
		Rs = SortedUnique(Rs);

		auto InDoor = [DoorS, DoorN](double T) { return T > DoorS && T < DoorN; };
		for (int32 I = 0; I < Ts.Num(); ++I)
		{
			const double T0 = Ts[I], T1 = I + 1 < Ts.Num() ? Ts[I + 1] : Ts[0] + Turn;
			const double TM = Wrap(0.5 * (T0 + T1));
			for (int32 K = 0; K + 1 < Rs.Num(); ++K)
			{
				const double R0 = Rs[K], R1 = Rs[K + 1], RM = 0.5 * (R0 + R1);
				if (R0 >= R - 1e-9 && InDoor(TM)) { continue; }   // the doorway: see below
				FMeshData* Target = &Out.Slab;
				if (RM > A::WaitRingIn && RM < A::WaitRingOut) { Target = &Out.Ring; }
				else
				{
					for (const double J : RingJoints)
					{
						if (FMath::Abs(RM - J) < JointWidth / 2) { Target = &Out.Joint; }
					}
					for (const FFloorBand& Band : Bands)
					{
						if (RM <= Band.R0 || RM >= Band.R1) { continue; }
						const double H = JointHalfAngle(Band);
						for (const double T : JointAngles(Band.Joints))
						{
							if (AngleGap(TM, T) < H) { Target = &Out.Joint; }
						}
					}
				}
				Target->Rect(Ring3(R0, T0, 0), Ring3(R1, T0, 0), Ring3(R1, T1, 0), Ring3(R0, T1, 0), Up);
			}
		}

		// The floor's edge round the car's shaft: a 3 cm lip, 1 cm inside the Square's bronze lining (r 2.3).
		{
			constexpr double Lip = 0.03;
			for (int32 I = 0; I < Ts.Num(); ++I)
			{
				const double T0 = Ts[I], T1 = I + 1 < Ts.Num() ? Ts[I + 1] : Ts[0] + Turn;
				CurvedQuad(Out.Slab, A::FloorInnerRadius, T0, T1, -Lip, 0.0, -1.0);
				// Its underside, on to 2 mm behind the lining, so the slit between them is closed.
				Out.Slab.Rect(Ring3(A::FloorInnerRadius, T0, -Lip), Ring3(A::FloorInnerRadius + 0.012, T0, -Lip),
							  Ring3(A::FloorInnerRadius + 0.012, T1, -Lip), Ring3(A::FloorInnerRadius, T1, -Lip), -Up);
			}
		}

		// The doorway, from the wall's inner face out to the export's 96-gon (r 14.3), which the Hall of
		// Light's floor meets vertex for vertex; its sides on the jambs' lines (y ±2).
		TArray<FVector> In, Outer;
		TArray<double> InA, OutA;
		for (const double T : Ts)
		{
			if (T >= DoorS - 1e-12 && T <= DoorN + 1e-12) { In.Add(Ring3(R, T, 0)); InA.Add(T); }
		}
		const double PolyStep = Turn / A::FloorSegments;
		auto Poly = [PolyStep](int32 I) { return Hub + Dir(I * PolyStep) * A::FloorRadius; };
		auto Crossing = [&Poly](double Y) -> FVector2D
		{
			for (int32 I = A::FloorSegments / 2 - 8; I < A::FloorSegments / 2 + 8; ++I)
			{
				const FVector2D P = Poly(I), Q = Poly(I + 1);
				const double DP = P.Y - Hub.Y - Y, DQ = Q.Y - Hub.Y - Y;
				if (DP * DQ <= 0 && DP != DQ) { return P + (Q - P) * (DP / (DP - DQ)); }
			}
			return WestAt(A::FloorRadius, Y);
		};
		const FVector2D South = Crossing(DoorHalf), North = Crossing(-DoorHalf);
		Outer.Add(At(South, 0));
		OutA.Add(AngleOf(South));
		for (int32 I = 0; I < A::FloorSegments; ++I)
		{
			const double T = I * PolyStep;
			if (T > AngleOf(South) && T < AngleOf(North)) { Outer.Add(At(Poly(I), 0)); OutA.Add(T); }
		}
		Outer.Add(At(North, 0));
		OutA.Add(AngleOf(North));
		Zipper(Out.Slab, In, InA, Outer, OutA, Up, [](const FVector& P) { return FVector2D(P.X, P.Y); });
	}

	// ============================================================================ The stone base

	struct FWallMeshes { FMeshData Stone, Led; };

	/** Whether the twelve pilasters stand (AAtriumBaseStructure::bPilasters, set before a build). */
	bool GPilasters = false;

	/** The coping band's path (its face in plan): arcs 10 cm in front of the wall, round each capital. */
	TArray<FVector2D> BandPath()
	{
		TArray<FVector2D> Path;
		if (!GPilasters)
		{
			// No pilasters (the Élan's bone ribs stand in their place): one round band.
			for (int32 I = 0; I < RoundSegments; ++I) { Path.Add(OnCircle(BandFace, FirstPilasterDeg * Deg + RoundStep * I)); }
			return Path;
		}
		const double RhoCorner = FMath::Sqrt(BandFace * BandFace - CapHalf * CapHalf);
		const double CornerAngle = FMath::Atan2(CapHalf, RhoCorner);
		for (int32 K = 0; K < Pilasters; ++K)
		{
			const double Phi = PilasterAngle(K);
			const FVector2D Rho = Dir(Phi), Tau(-FMath::Sin(Phi), FMath::Cos(Phi));
			Path.Add(Hub + Rho * RhoCorner - Tau * CapHalf);
			Path.Add(Hub + Rho * CapFront - Tau * CapHalf);
			Path.Add(Hub + Rho * CapFront + Tau * CapHalf);
			Path.Add(Hub + Rho * RhoCorner + Tau * CapHalf);
			const double A0 = Phi + CornerAngle, A1 = PilasterAngle(K + 1) - CornerAngle;
			const int32 N = FMath::Max(2, FMath::CeilToInt32((A1 - A0) / RoundStep - 1e-9));
			for (int32 I = 1; I < N; ++I) { Path.Add(OnCircle(BandFace, A0 + (A1 - A0) * I / N)); }
		}
		return Path;
	}

	/**
	 * The coping band and its top. Returns the angles of the top's outer edge (the wall's outer face
	 * shares them at 6 m): the chamfer's inner line round the circle, and the door's.
	 */
	void BuildCoping(FWallMeshes& Out, TArray<double>& OuterAngles)
	{
		const TArray<FVector2D> Path = BandPath();
		FProfile Band;
		Band.Add(-0.10 - Sink, BandBottom).Add(0.0, BandBottom).Add(0.0, Top - BandEase).Add(-BandEase, Top);
		SalonKit::SweepPlan(Out.Stone, Path, true, Band);

		// The chamfer's inner line at 6 m, per path vertex (the sweep's own frames, so the same points).
		TArray<FVector> Line;
		for (const TArray<FFrame>& Run : SalonKit::PlanRuns(Path, true))
		{
			for (const FFrame& F : Run)
			{
				const FVector P = F.At(FVector2D(-BandEase, Top));
				if (Line.Num() == 0 || !P.Equals(Line.Last(), 1e-9)) { Line.Add(P); }
			}
		}
		if (Line.Num() > 1 && Line[0].Equals(Line.Last(), 1e-9)) { Line.Pop(); }
		// PlanRuns starts at the first hard corner (the first pilaster's back corner): four points to a
		// capital (back, front, front, back), then its bay's arc.
		TArray<FVector> Inner;
		TArray<double> InnerA;
		const int32 Per = Line.Num() / Pilasters;
		for (int32 K = 0; K < (GPilasters ? Pilasters : 0); ++K)
		{
			const int32 B = K * Per;
			// The capital's top between its back corners (the ring below runs straight across behind it).
			Out.Stone.Poly({Line[B], Line[B + 1], Line[B + 2], Line[B + 3]}, Up);
			for (int32 I = B; I < B + Per; ++I)
			{
				if (I == B + 1 || I == B + 2) { continue; }
				Inner.Add(Line[I]);
				InnerA.Add(AngleOf(FVector2D(Line[I].X, Line[I].Y)));
			}
		}
		if (!GPilasters)
		{
			for (const FVector& P : Line)
			{
				Inner.Add(P);
				InnerA.Add(AngleOf(FVector2D(P.X, P.Y)));
			}
		}
		// Ascending from the first point, once round.
		for (int32 I = 1; I < InnerA.Num(); ++I)
		{
			while (InnerA[I] < InnerA[I - 1]) { InnerA[I] += Turn; }
		}
		OuterAngles = InnerA;
		for (double& T : OuterAngles) { T = Wrap(T); }
		const double OS = JambSouth(ROut), ON = JambNorth(ROut);
		OuterAngles.Add(OS);
		OuterAngles.Add(ON);
		for (int32 J = 1; J < DoorSteps; ++J)
		{
			const double Y = DoorHalf - 2.0 * DoorHalf * J / DoorSteps;
			OuterAngles.Add(AngleOf(WestAt(ROut, Y)));
		}
		OuterAngles = SortedUnique(OuterAngles, 1e-7);

		// The top: from the chamfer to the outer face, once round (closed), at exactly 6 m.
		TArray<FVector> OuterPts;
		TArray<double> OuterA;
		const double Start = InnerA[0];
		for (const double T : OuterAngles)
		{
			double U = T;
			while (U < Start) { U += Turn; }
			OuterA.Add(U);
		}
		OuterA.Sort();
		for (const double T : OuterA) { OuterPts.Add(Ring3(ROut, T, Top)); }
		const FVector First = Inner[0];
		Inner.Add(First);
		InnerA.Add(InnerA[0] + Turn);
		OuterPts.Insert(Ring3(ROut, Start, Top), 0);
		OuterA.Insert(Start, 0);
		OuterPts.Add(Ring3(ROut, Start + Turn, Top));
		OuterA.Add(Start + Turn);
		Zipper(Out.Stone, Inner, InnerA, OuterPts, OuterA, Up, [](const FVector& P) { return FVector2D(P.X, P.Y); });
	}

	void BuildWall(FWallMeshes& Out, const TArray<double>& OuterAngles)
	{
		const double DoorS = JambSouth(R), DoorN = JambNorth(R);
		const double CoveN = DoorN + CoveStop / R, CoveS = DoorS + Turn - CoveStop / R;

		// ---- The inner face, from the north jamb round to the south jamb; the cove at its foot.
		TArray<double> Ts = {DoorN, CoveN, CoveS, DoorS + Turn};
		{
			const int32 N = FMath::CeilToInt32((DoorS + Turn - DoorN) / RoundStep);
			for (int32 I = 1; I < N; ++I) { Ts.Add(DoorN + (DoorS + Turn - DoorN) * I / N); }
		}
		Ts = SortedUnique(Ts, 1e-7);
		for (int32 I = 0; I + 1 < Ts.Num(); ++I)
		{
			const double T0 = Ts[I], T1 = Ts[I + 1], TM = 0.5 * (T0 + T1);
			const bool bCove = TM > CoveN && TM < CoveS;
			TArray<double> Zs = {CoveTop, DoorHead, BandBottom + Sink};
			if (!bCove) { Zs.Insert(-Sink, 0); }
			CurvedColumn(Out.Stone, R, T0, T1, Zs, -1.0);
			if (bCove)
			{
				// The cove: its soffit, and its back with the diffuser low on it.
				Out.Stone.Rect(Ring3(R, T0, CoveTop), Ring3(CoveBack, T0, CoveTop), Ring3(CoveBack, T1, CoveTop), Ring3(R, T1, CoveTop), -Up);
				CurvedQuad(Out.Stone, CoveBack, T0, T1, -Sink, LedBottom, -1.0);
				CurvedQuad(Out.Led, CoveBack, T0, T1, LedBottom, LedTop, -1.0);
				CurvedQuad(Out.Stone, CoveBack, T0, T1, LedTop, CoveTop, -1.0);
			}
		}
		// The cove's ends, short of the jambs.
		for (const double T : {CoveN, CoveS})
		{
			const FVector N = Along(T) * (T == CoveN ? 1.0 : -1.0);
			Out.Stone.Rect(Ring3(R, T, -Sink), Ring3(CoveBack, T, -Sink), Ring3(CoveBack, T, CoveTop), Ring3(R, T, CoveTop), N);
		}

		// ---- The door: over it the inner face, the soffit and the outer face; its jambs (y ±2).
		TArray<double> InDoor;
		for (int32 J = 0; J <= DoorSteps; ++J)
		{
			const double Y = DoorHalf - 2.0 * DoorHalf * J / DoorSteps;
			InDoor.Add(J == 0 ? DoorS : (J == DoorSteps ? DoorN : AngleOf(WestAt(R, Y))));
		}
		for (int32 J = 0; J + 1 < InDoor.Num(); ++J) { CurvedQuad(Out.Stone, R, InDoor[J], InDoor[J + 1], DoorHead, BandBottom + Sink, -1.0); }
		const double OS = JambSouth(ROut), ON = JambNorth(ROut);
		TArray<double> OutDoor;
		for (const double T : OuterAngles)
		{
			if (T >= OS - 1e-9 && T <= ON + 1e-9) { OutDoor.Add(T); }
		}
		{
			TArray<FVector> In, Outer;
			for (const double T : InDoor) { In.Add(Ring3(R, T, DoorHead)); }
			for (const double T : OutDoor) { Outer.Add(Ring3(ROut, T, DoorHead)); }
			// Zip by y (the jambs are parallel, not radial): rung by rung, as the plan's y falls.
			TArray<double> InY, OutY;
			for (const FVector& P : In) { InY.Add(-(P.Y - Hub.Y)); }
			for (const FVector& P : Outer) { OutY.Add(-(P.Y - Hub.Y)); }
			Zipper(Out.Stone, In, InY, Outer, OutY, -Up, [](const FVector& P) { return FVector2D(P.X, P.Y); });
		}
		for (const int32 Side : {-1, 1})
		{
			// Side +1: the south jamb (y +2), facing north into the opening.
			const double Y = Side * DoorHalf;
			const FVector2D PIn = WestAt(R, Y), POut = WestAt(ROut, Y);
			const FVector N(0, -Side, 0);
			for (const FVector2D& Z : {FVector2D(-Sink, CoveTop), FVector2D(CoveTop, DoorHead)})
			{
				Out.Stone.Rect(At(PIn, Z.X), At(POut, Z.X), At(POut, Z.Y), At(PIn, Z.Y), N);
			}
		}

		// ---- The outer face, to the coping's top (its vertices), and over the door from its head.
		for (int32 I = 0; I < OuterAngles.Num(); ++I)
		{
			const double T0 = OuterAngles[I], T1 = I + 1 < OuterAngles.Num() ? OuterAngles[I + 1] : OuterAngles[0] + Turn;
			const double TM = Wrap(0.5 * (T0 + T1));
			const bool bDoor = TM > OS && TM < ON;
			const TArray<double> Zs = bDoor ? TArray<double>({DoorHead, Top}) : TArray<double>({-OuterSink, -Sink, CoveTop, DoorHead, Top});
			CurvedColumn(Out.Stone, ROut, T0, T1, Zs, 1.0);
		}
	}

	void BuildPilasters(FWallMeshes& Out)
	{
		FProfile Plinth;
		Plinth.Add(0.0, -Sink).Add(0.0, PlinthTop - PlinthEase).Add(-PlinthEase, PlinthTop);
		const double RhoPlinthBack = FMath::Sqrt((R + Sink) * (R + Sink) - PlinthHalf * PlinthHalf);
		const double RhoShaftBack = FMath::Sqrt(R * R - ShaftHalf * ShaftHalf) + 0.004;
		for (int32 K = 0; K < Pilasters; ++K)
		{
			const double Phi = PilasterAngle(K);
			const FVector2D Rho = Dir(Phi), Tau(-FMath::Sin(Phi), FMath::Cos(Phi));
			auto P = [&Rho, &Tau](double Rh, double Ta) { return Hub + Rho * Rh + Tau * Ta; };
			const FVector RhoN(Rho.X, Rho.Y, 0), TauN(Tau.X, Tau.Y, 0);

			// The shaft: its front and sides, from inside the plinth to inside the band, into the wall.
			const double Z0 = PlinthTop - Sink, Z1 = BandBottom + Sink;
			Out.Stone.Rect(At(P(ShaftFront, -ShaftHalf), Z0), At(P(ShaftFront, ShaftHalf), Z0), At(P(ShaftFront, ShaftHalf), Z1),
						   At(P(ShaftFront, -ShaftHalf), Z1), -RhoN);
			for (const double S : {-1.0, 1.0})
			{
				Out.Stone.Rect(At(P(ShaftFront, S * ShaftHalf), Z0), At(P(RhoShaftBack, S * ShaftHalf), Z0), At(P(RhoShaftBack, S * ShaftHalf), Z1),
							   At(P(ShaftFront, S * ShaftHalf), Z1), TauN * S);
			}

			// The plinth: its eased faces from the wall round the front, its top, and its back in the cove.
			const TArray<FVector2D> Path = {P(RhoPlinthBack, -PlinthHalf), P(PlinthFront, -PlinthHalf), P(PlinthFront, PlinthHalf), P(RhoPlinthBack, PlinthHalf)};
			SalonKit::SweepPlan(Out.Stone, Path, false, Plinth);
			TArray<FVector> Corners;
			for (const TArray<FFrame>& Run : SalonKit::PlanRuns(Path, false))
			{
				for (const FFrame& F : Run)
				{
					const FVector Q = F.At(FVector2D(-PlinthEase, PlinthTop));
					if (Corners.Num() == 0 || !Q.Equals(Corners.Last(), 1e-9)) { Corners.Add(Q); }
				}
			}
			if (Corners.Num() == 4)
			{
				Out.Stone.Poly({Corners[0], Corners[1], Corners[2], Corners[3]}, Up);
				// Behind the chord of its back corners, out to the wall's circle (2 mm into it).
				const double TA = AngleOf(FVector2D(Corners[3].X, Corners[3].Y)), TB = AngleOf(FVector2D(Corners[0].X, Corners[0].Y));
				double Span = TA - TB;
				if (Span < 0) { Span += Turn; }
				TArray<FVector> Lens = {Corners[3]};
				constexpr int32 LensSteps = 8;
				for (int32 I = 0; I <= LensSteps; ++I) { Lens.Add(Ring3(R + Sink, TA - Span * I / LensSteps, PlinthTop)); }
				Lens.Add(Corners[0]);
				Out.Stone.Poly(Lens, Up);
			}
			const double BackHalf = FMath::Asin(PlinthHalf / R);
			constexpr int32 BackSteps = 8;
			for (int32 I = 0; I < BackSteps; ++I)
			{
				const double T0 = Phi - BackHalf + 2 * BackHalf * I / BackSteps, T1 = Phi - BackHalf + 2 * BackHalf * (I + 1) / BackSteps;
				CurvedQuad(Out.Stone, R, T0, T1, -Sink, CoveTop + Sink, 1.0);
			}
		}
	}

	// ============================================================================ Bronze inlays

	struct FStrokePoint { FVector2D P; double W; };
	struct FStroke { TArray<FStrokePoint> Points; bool bClosed = false; };
	struct FGlyph { TArray<FStroke> Strokes; TArray<FVector> Balls; };

	/** Didot's stress: a stroke as thick as it is upright. */
	double StressWidth(const FVector2D& Tangent, double ThickW)
	{
		const double L = Tangent.Size();
		const double Upright = L > 0 ? FMath::Abs(Tangent.Y) / L : 0.0;
		return LetterThin + (ThickW - LetterThin) * FMath::Pow(Upright, 1.6);
	}

	struct FGlyphPen
	{
		FGlyph Glyph;

		/** An elliptical arc from A0 to A1 degrees (either way round). */
		void Arc(double Cx, double Cy, double Rx, double Ry, double A0, double A1, int32 N = 40)
		{
			FStroke S;
			for (int32 I = 0; I <= N; ++I)
			{
				const double Ang = (A0 + (A1 - A0) * I / N) * Deg;
				const FVector2D T(-Rx * FMath::Sin(Ang), Ry * FMath::Cos(Ang));
				S.Points.Add({FVector2D(Cx + Rx * FMath::Cos(Ang), Cy + Ry * FMath::Sin(Ang)), StressWidth(T, LetterThick)});
			}
			Glyph.Strokes.Add(S);
		}

		void Line(double X0, double Y0, double X1, double Y1, double W0, double W1 = -1)
		{
			FStroke S;
			S.Points.Add({FVector2D(X0, Y0), W0});
			S.Points.Add({FVector2D(X1, Y1), W1 < 0 ? W0 : W1});
			Glyph.Strokes.Add(S);
		}

		/** Serifs: a hairline across the stem's foot or head. */
		void Serif(double X, double Y, double Half) { Line(X - Half, Y, X + Half, Y, LetterThin); }
		void Ball(double X, double Y, double Rad) { Glyph.Balls.Add(FVector(X, Y, Rad)); }
	};

	/** A capital A … K in a unit of its height (baseline 0, cap height 1). */
	FGlyph Letter(TCHAR C)
	{
		constexpr double K = LetterThick, H = LetterThin;
		constexpr double Base = H / 2, Cap = 1.0 - H / 2;   // the hairlines' centres
		FGlyphPen G;
		switch (C)
		{
		case 'A':
			G.Line(0.06, Base, 0.37, 1.0, H);
			G.Line(0.37, 1.0, 0.68, Base, K);
			G.Line(0.16, 0.32, 0.57, 0.32, H);
			G.Serif(0.06, Base, 0.12);
			G.Serif(0.68, Base, 0.14);
			break;
		case 'B':
			G.Line(0.12, 0.0, 0.12, 1.0, K);
			G.Serif(0.10, Base, 0.10);
			G.Serif(0.10, Cap, 0.10);
			G.Line(0.12, Cap, 0.36, Cap, H);
			G.Arc(0.36, 0.762, 0.21, 0.221, 90, -90);
			G.Line(0.12, 0.541, 0.38, 0.541, H);
			G.Arc(0.38, 0.279, 0.25, 0.262, 90, -90);
			G.Line(0.12, Base, 0.38, Base, H);
			break;
		case 'C':
			G.Arc(0.42, 0.5, 0.36, 0.483, 42, 318);
			G.Ball(0.69, 0.79, 0.065);
			break;
		case 'D':
			G.Line(0.12, 0.0, 0.12, 1.0, K);
			G.Serif(0.10, Base, 0.10);
			G.Serif(0.10, Cap, 0.10);
			G.Line(0.12, Cap, 0.34, Cap, H);
			G.Line(0.12, Base, 0.34, Base, H);
			G.Arc(0.34, 0.5, 0.38, 0.483, 90, -90, 48);
			break;
		case 'E':
			G.Line(0.12, 0.0, 0.12, 1.0, K);
			G.Line(0.02, Cap, 0.62, Cap, H * 1.3);
			G.Line(0.12, 0.52, 0.50, 0.52, H);
			G.Line(0.02, 0.025, 0.66, 0.025, 0.05);
			G.Line(0.61, 1.0, 0.61, 0.80, H);
			G.Line(0.49, 0.61, 0.49, 0.43, H);
			G.Line(0.65, 0.0, 0.65, 0.22, H);
			break;
		case 'F':
			G.Line(0.12, 0.0, 0.12, 1.0, K);
			G.Line(0.02, Cap, 0.62, Cap, H * 1.3);
			G.Line(0.12, 0.50, 0.48, 0.50, H);
			G.Line(0.61, 1.0, 0.61, 0.80, H);
			G.Line(0.47, 0.59, 0.47, 0.41, H);
			G.Serif(0.14, Base, 0.14);
			break;
		case 'G':
			G.Arc(0.42, 0.5, 0.36, 0.483, 42, 322);
			G.Ball(0.69, 0.79, 0.065);
			G.Line(0.72, 0.43, 0.72, 0.10, K * 0.85);
			G.Line(0.54, 0.43, 0.84, 0.43, H);
			break;
		case 'H':
			G.Line(0.12, 0.0, 0.12, 1.0, K);
			G.Line(0.66, 0.0, 0.66, 1.0, K);
			G.Line(0.12, 0.52, 0.66, 0.52, H);
			for (const double X : {0.12, 0.66}) { G.Serif(X, Base, 0.12); G.Serif(X, Cap, 0.12); }
			break;
		case 'I':
			G.Line(0.14, 0.0, 0.14, 1.0, K);
			G.Serif(0.14, Base, 0.13);
			G.Serif(0.14, Cap, 0.13);
			break;
		case 'J':
			G.Line(0.36, 1.0, 0.36, 0.24, K);
			G.Serif(0.36, Cap, 0.13);
			G.Arc(0.20, 0.24, 0.16, 0.22, 0, -165, 32);
			G.Ball(0.05, 0.19, 0.06);
			break;
		case 'K':
		default:
			G.Line(0.12, 0.0, 0.12, 1.0, K);
			G.Serif(0.12, Base, 0.12);
			G.Serif(0.12, Cap, 0.12);
			G.Line(0.14, 0.40, 0.62, Cap, H);
			G.Serif(0.62, Cap, 0.12);
			G.Line(0.32, 0.62, 0.67, 0.0, K);
			G.Serif(0.68, Base, 0.13);
			break;
		}
		return G.Glyph;
	}

	/** A glyph cast in the floor, centred on Place, its top towards UpDir: strokes as ribbons 2 mm proud, with their sides. */
	void AddGlyph(FMeshData& M, const FGlyph& Glyph, const FVector2D& Place, const FVector2D& UpDir, double Height)
	{
		double MinX = 1e9, MaxX = -1e9;
		for (const FStroke& S : Glyph.Strokes)
		{
			for (const FStrokePoint& P : S.Points) { MinX = FMath::Min(MinX, P.P.X - P.W / 2); MaxX = FMath::Max(MaxX, P.P.X + P.W / 2); }
		}
		const double Cx = 0.5 * (MinX + MaxX);
		// Facing UpDir (as the visitor walking towards the bay does), the letter's right is (−up.y, up.x).
		const FVector2D RightDir(-UpDir.Y, UpDir.X);
		auto ToPlan = [&](const FVector2D& Q) { return Place + RightDir * ((Q.X - Cx) * Height) + UpDir * ((Q.Y - 0.5) * Height); };
		auto V = [&M](const FVector2D& P, double Z, const FVector& N) { return M.Vertex(At(P, Z), N, P); };
		auto Outward = [](const FVector2D& From, const FVector2D& To) { const FVector2D D = (To - From).GetSafeNormal(); return FVector(D.X, D.Y, 0); };
		for (const FStroke& S : Glyph.Strokes)
		{
			const int32 N = S.Points.Num();
			if (N < 2) { continue; }
			TArray<FVector2D> L, Rt, C;
			for (int32 I = 0; I < N; ++I)
			{
				const int32 IA = FMath::Max(I - 1, 0), IB = FMath::Min(I + 1, N - 1);
				const FVector2D T = (S.Points[IB].P - S.Points[IA].P).GetSafeNormal();
				const FVector2D Side(-T.Y, T.X);
				const double W = S.Points[I].W / 2;
				C.Add(ToPlan(S.Points[I].P));
				L.Add(ToPlan(S.Points[I].P + Side * W));
				Rt.Add(ToPlan(S.Points[I].P - Side * W));
			}
			for (int32 I = 0; I + 1 < N; ++I)
			{
				M.Quad(V(L[I], InlayTop, Up), V(L[I + 1], InlayTop, Up), V(Rt[I + 1], InlayTop, Up), V(Rt[I], InlayTop, Up));
				for (const TArray<FVector2D>* Edge : {&L, &Rt})
				{
					const FVector NI = Outward(C[I], (*Edge)[I]), NJ = Outward(C[I + 1], (*Edge)[I + 1]);
					M.Quad(V((*Edge)[I], InlayBottom, NI), V((*Edge)[I + 1], InlayBottom, NJ), V((*Edge)[I + 1], InlayTop, NJ), V((*Edge)[I], InlayTop, NI));
				}
			}
			for (const int32 End : {0, N - 1})
			{
				const FVector N3 = End == 0 ? Outward(C[1], C[0]) : Outward(C[N - 2], C[N - 1]);
				M.Quad(V(L[End], InlayBottom, N3), V(Rt[End], InlayBottom, N3), V(Rt[End], InlayTop, N3), V(L[End], InlayTop, N3));
			}
		}
		for (const FVector& Ball : Glyph.Balls)
		{
			const FVector2D BC = ToPlan(FVector2D(Ball.X, Ball.Y));
			const double Rad = Ball.Z * Height;
			constexpr int32 Segs = 24;
			const int32 Mid = V(BC, InlayTop, Up);
			for (int32 I = 0; I < Segs; ++I)
			{
				const double A0 = Turn * I / Segs, A1 = Turn * (I + 1) / Segs;
				const FVector2D P0 = BC + Dir(A0) * Rad, P1 = BC + Dir(A1) * Rad;
				M.Tri(Mid, V(P0, InlayTop, Up), V(P1, InlayTop, Up));
				M.Quad(V(P0, InlayBottom, Radial(A0)), V(P1, InlayBottom, Radial(A1)), V(P1, InlayTop, Radial(A1)), V(P0, InlayTop, Radial(A0)));
			}
		}
	}

	/** The bay letters' places (plan) and the direction their tops face (out, towards the bay). */
	void BayLetterPlaces(TArray<FVector2D>& Places, TArray<FVector2D>& UpDirs)
	{
		for (int32 I = 0; I < A::BayLetters; ++I)
		{
			const double T = (A::FirstBayBearing + A::BayStepDegrees * I - 90.0) * Deg;
			Places.Add(OnCircle(A::BayLetterRadius, T));
			UpDirs.Add(Dir(T));
		}
	}

	/** The waiting mark west of the ring: a half ring (open to the car) and a dot, 2 mm proud. */
	void BuildWaitMark(FMeshData& M)
	{
		const FVector2D MarkC(A::WaitMarkX, Hub.Y), DotC(A::WaitDotX, Hub.Y);
		constexpr int32 Segs = 36;
		auto V = [&M](const FVector2D& P, double Z, const FVector& N) { return M.Vertex(At(P, Z), N, P); };
		for (int32 I = 0; I < Segs; ++I)
		{
			const double A0 = Pi / 2 + Pi * I / Segs, A1 = Pi / 2 + Pi * (I + 1) / Segs;
			const FVector2D I0 = MarkC + Dir(A0) * A::WaitMarkIn, I1 = MarkC + Dir(A1) * A::WaitMarkIn;
			const FVector2D O0 = MarkC + Dir(A0) * A::WaitMarkOut, O1 = MarkC + Dir(A1) * A::WaitMarkOut;
			M.Quad(V(I0, InlayTop, Up), V(I1, InlayTop, Up), V(O1, InlayTop, Up), V(O0, InlayTop, Up));
			M.Quad(V(O0, InlayBottom, Radial(A0)), V(O1, InlayBottom, Radial(A1)), V(O1, InlayTop, Radial(A1)), V(O0, InlayTop, Radial(A0)));
			M.Quad(V(I0, InlayBottom, -Radial(A0)), V(I1, InlayBottom, -Radial(A1)), V(I1, InlayTop, -Radial(A1)), V(I0, InlayTop, -Radial(A0)));
		}
		for (const double A0 : {Pi / 2, 3 * Pi / 2})
		{
			const FVector N = Along(A0) * (A0 < Pi ? -1.0 : 1.0);
			const FVector2D P0 = MarkC + Dir(A0) * A::WaitMarkIn, P1 = MarkC + Dir(A0) * A::WaitMarkOut;
			M.Quad(V(P0, InlayBottom, N), V(P1, InlayBottom, N), V(P1, InlayTop, N), V(P0, InlayTop, N));
		}
		const int32 Mid = V(DotC, InlayTop, Up);
		for (int32 I = 0; I < Segs; ++I)
		{
			const double A0 = Turn * I / Segs, A1 = Turn * (I + 1) / Segs;
			const FVector2D P0 = DotC + Dir(A0) * A::WaitDotRadius, P1 = DotC + Dir(A1) * A::WaitDotRadius;
			M.Tri(Mid, V(P0, InlayTop, Up), V(P1, InlayTop, Up));
			M.Quad(V(P0, InlayBottom, Radial(A0)), V(P1, InlayBottom, Radial(A1)), V(P1, InlayTop, Radial(A1)), V(P0, InlayTop, Radial(A0)));
		}
	}

	// ============================================================================ The stele

	FBox SteleGlass()
	{
		return FBox(FVector(A::SteleWestX, A::SteleY - A::SteleHalfWidth, GlassBottom),
					FVector(A::SteleWestX + GlassThick, A::SteleY + A::SteleHalfWidth, A::SteleTop));
	}

	FBox SteleFoot()
	{
		const double Cx = A::SteleWestX + GlassThick / 2;
		return FBox(FVector(Cx - FootHalfX, A::SteleY - FootHalfY, -Sink), FVector(Cx + FootHalfX, A::SteleY + FootHalfY, FootTop));
	}

	void BuildStele(FMeshData& Glass, FMeshData& Brass)
	{
		const FBox G = SteleGlass();
		Glass.Box(G.Min, G.Max, FMeshData::AllFaces);
		// The foot, as the Salon's: its top edges eased.
		const FBox F = SteleFoot();
		FProfile Edge;
		Edge.Add(FootEase, FootTop).Add(0.0, FootTop - FootEase).Add(0.0, -Sink);
		const TArray<TArray<FFrame>> Runs = SalonKit::PlanRuns({FVector2D(F.Min.X, F.Min.Y), FVector2D(F.Max.X, F.Min.Y), FVector2D(F.Max.X, F.Max.Y),
																  FVector2D(F.Min.X, F.Max.Y)}, true);
		TArray<FVector> TopFace;
		for (const TArray<FFrame>& Run : Runs)
		{
			SalonKit::Sweep(Brass, Run, Edge);
			TopFace.Add(Run[0].At(FVector2D(FootEase, FootTop)));
		}
		Brass.Poly(TopFace, Up);
	}

	// ============================================================================ The cove's lights

	struct FCoveLight { FVector At; FVector Facing; FVector Across; double Width; double Length; };

	TArray<FCoveLight> CoveLightPlaces()
	{
		TArray<FCoveLight> Out;
		const double PlinthAngle = FMath::Asin(PlinthHalf / R);
		const double RL = CoveBack - LightInset;
		const double Tilt = LightTiltDeg * Deg;
		for (int32 K = 0; K < Pilasters; ++K)
		{
			const double Mid = PilasterAngle(K) + 15.0 * Deg;
			if (AngleGap(Mid, Pi) < 1.0 * Deg) { continue; }   // the door's bay
			const double A0 = PilasterAngle(K) + PlinthAngle, A1 = PilasterAngle(K + 1) - PlinthAngle;
			for (int32 J = 0; J < LightsPerBay; ++J)
			{
				const double T0 = A0 + (A1 - A0) * J / LightsPerBay, T1 = A0 + (A1 - A0) * (J + 1) / LightsPerBay, TM = 0.5 * (T0 + T1);
				FCoveLight L;
				L.At = Ring3(RL, TM, 0.5 * (LedBottom + LedTop));
				L.Facing = (-Radial(TM) * FMath::Cos(Tilt) - Up * FMath::Sin(Tilt)).GetSafeNormal();
				L.Across = Along(TM);
				L.Width = 2.0 * RL * FMath::Sin(0.5 * (T1 - T0)) * 0.97;
				L.Length = R * (T1 - T0);
				Out.Add(L);
			}
		}
		return Out;
	}

	constexpr int32 CoveLightCount = (Pilasters - 1) * LightsPerBay;
}

namespace ABK = AtriumBaseKit;

AAtriumBaseStructure::AAtriumBaseStructure()
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
			// The floor and the walls collide as themselves (complex as simple); the floor is walkable.
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
	Floor = Make(TEXT("Floor"), true);
	Walls = Make(TEXT("Walls"), true);
	Inlays = Make(TEXT("Inlays"), false);
	CoveLed = Make(TEXT("CoveLed"), false);
	CoveLed->SetCastShadow(false);
	Stele = Make(TEXT("Stele"), false);
	SteleFoot = Make(TEXT("SteleFoot"), false);
	// The diffuser's material is made at play (ApplyMaterials): the bake leaves it procedural. The floor and the stele's
	// foot take saved instances (materials.py: M_Elan_Mirror, M_Elan_Mirror_Joint, M_Brass_Brushed), so the bake makes
	// them Nanite meshes with Lumen cards (procedural, the floor had no surface cache: no bounce from it).
	MuseeBake::NoBake(CoveLed);

	SteleBlock = CreateDefaultSubobject<UBoxComponent>(TEXT("SteleBlock"));
	SteleBlock->SetupAttachment(RootComponent);
	SteleBlock->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SteleBlock->SetCollisionObjectType(ECC_WorldStatic);
	SteleBlock->SetCollisionResponseToAllChannels(ECR_Ignore);
	SteleBlock->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	SteleBlock->SetGenerateOverlapEvents(false);
	SteleBlock->SetHiddenInGame(true);
	SteleBlock->CanCharacterStepUpOn = ECB_No;

	for (int32 I = 0; I < ABK::CoveLightCount; ++I)
	{
		URectLightComponent* L = CreateDefaultSubobject<URectLightComponent>(*FString::Printf(TEXT("CoveLight%02d"), I + 1));
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		CoveLights.Add(L);
	}

	auto Path = [](const TCHAR* Folder, const TCHAR* Name)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name)));
	};
	const TCHAR* Materials = TEXT("/Game/Museum/Materials");
	const TCHAR* Imported = TEXT("/Game/Museum/Materials/USD");
	FloorMaterial = Path(Materials, TEXT("M_Elan_Mirror"));   // the Rotunda's cream marble, glass-smooth (era III)
	StoneMaterial = Path(Imported, TEXT("MI_travertine_honed_F7EFE2"));
	BronzeMaterial = Path(Materials, TEXT("M_Gilt"));
	GlassMaterial = Path(Materials, TEXT("M_Glass"));
	BrassMaterial = Path(Materials, TEXT("M_Brass_Brushed"));
	JointMaterial = Path(Materials, TEXT("M_Elan_Mirror_Joint"));
	GlowMaterial = Path(Materials, TEXT("M_Daylit"));

	AddTags();
	PlaceLights();
}

TArray<FString> AAtriumBaseStructure::GetReplacedImportPrims()
{
	TArray<FString> Prims = {
		TEXT("/Museum/Elan/Atrium_floor"),
		TEXT("/Museum/Elan/Bronze_ring"),
		TEXT("/Museum/Elan/Atrium_stone_base"),
		TEXT("/Museum/Elan/Atrium_coping"),
		TEXT("/Museum/Elan/Atrium_pilasters"),
		TEXT("/Museum/Elan/Starry_Night_stele"),
	};
	for (TCHAR C = 'A'; C <= 'K'; ++C) { Prims.Add(FString::Printf(TEXT("/Museum/Elan/Bay_letter_%c"), C)); }
	return Prims;
}

TArray<FVector> AAtriumBaseStructure::GetBayLetterPositions() const
{
	TArray<FVector2D> Places, UpDirs;
	ABK::BayLetterPlaces(Places, UpDirs);
	TArray<FVector> Out;
	for (const FVector2D& P : Places) { Out.Add(GetActorTransform().TransformPosition(ABK::At(P, 0) * MuseePlan::Cm)); }
	return Out;
}

FBox AAtriumBaseStructure::GetSteleGlassBounds() const
{
	const FBox G = ABK::SteleGlass();
	return FBox(G.Min * MuseePlan::Cm, G.Max * MuseePlan::Cm).TransformBy(GetActorTransform());
}

TArray<FVector> AAtriumBaseStructure::GetCoveLightPositions() const
{
	TArray<FVector> Out;
	for (const URectLightComponent* L : CoveLights)
	{
		if (L) { Out.Add(L->GetComponentLocation()); }
	}
	return Out;
}

void AAtriumBaseStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials(false);
	PlaceLights();
	AddTags();
}

void AAtriumBaseStructure::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	PlaceLights();
	AddTags();
}

void AAtriumBaseStructure::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never
	// shows an older build; then the play-time materials; then retire what it replaces.
	Build();
	ApplyMaterials(true);
	RetireImported();
}

void AAtriumBaseStructure::AddTags()
{
	// Part of the building (hidden in the Sphere with the rest); electric light, so no musee.laylight.
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Elan")));
}

void AAtriumBaseStructure::RetireImported()
{
	// As Scripts/relight.py's retire(): hidden, no collision, off the building list (so leaving the
	// Sphere, which shows the building again, leaves them out).
	const TArray<FString> Prims = GetReplacedImportPrims();
	int32 Count = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == this) { continue; }
		bool bMatch = false;
		for (const FName& Tag : Actor->Tags)
		{
			FString S = Tag.ToString();
			if (!S.RemoveFromStart(TEXT("prim:"))) { continue; }
			for (const FString& P : Prims) { bMatch |= S == P || S.StartsWith(P + TEXT("/")); }
		}
		if (!bMatch) { continue; }
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);
		Actor->Tags.Remove(FName(TEXT("musee.building")));
		Actor->Tags.AddUnique(FName(TEXT("musee.retired")));
		++Count;
	}
	if (Count > 0) { UE_LOG(LogMusee, Log, TEXT("Atrium: %d imported pieces retired (the native floor, base, letters and stele replace them)."), Count); }
}

void AAtriumBaseStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	ABK::FFloorMeshes F;
	ABK::BuildFloor(F);
	ABK::GPilasters = bPilasters;
	ABK::FWallMeshes W;
	TArray<double> OuterAngles;
	ABK::BuildCoping(W, OuterAngles);
	ABK::BuildWall(W, OuterAngles);
	if (bPilasters) { ABK::BuildPilasters(W); }
	SalonKit::FMeshData Bronze, Glass, Brass;
	TArray<FVector2D> Places, UpDirs;
	ABK::BayLetterPlaces(Places, UpDirs);
	for (int32 I = 0; I < Places.Num(); ++I) { ABK::AddGlyph(Bronze, ABK::Letter(TCHAR('A' + I)), Places[I], UpDirs[I], ABK::LetterHeight); }
	ABK::BuildWaitMark(Bronze);
	ABK::BuildStele(Glass, Brass);

	for (UProceduralMeshComponent* Component : {Floor.Get(), Walls.Get(), Inlays.Get(), CoveLed.Get(), Stele.Get(), SteleFoot.Get()})
	{
		if (Component) { Component->ClearAllMeshSections(); }
	}
	F.Slab.Write(Floor, 0, true);
	F.Joint.Write(Floor, 1, true);
	F.Ring.Write(Floor, 2, true);
	W.Stone.Write(Walls, 0, true);
	W.Led.Write(CoveLed, 0, false);
	Bronze.Write(Inlays, 0, false);
	Glass.Write(Stele, 0, false);
	Brass.Write(SteleFoot, 0, false);
}

void AAtriumBaseStructure::ApplyMaterials(bool bInstances)
{
	auto Set = [](UProceduralMeshComponent* Component, int32 Section, UMaterialInterface* Material)
	{
		if (Component && Material) { Component->SetMaterial(Section, Material); }
	};
	UMaterialInterface* FloorBase = FloorMaterial.LoadSynchronous();
	UMaterialInterface* Slab = FloorBase;
	UMaterialInterface* Joint = FloorBase;
	UMaterialInterface* BrassBase = BrassMaterial.LoadSynchronous();
	UMaterialInterface* Brass = BrassBase;
	UMaterialInterface* GlowBase = GlowMaterial.LoadSynchronous();
	UMaterialInterface* Glow = GlowBase;
	// Saved instances (materials.py) serve in the editor, at play and in the bake alike.
	UMaterialInterface* JointSaved = JointMaterial.IsNull() ? nullptr : JointMaterial.LoadSynchronous();
	const bool bSavedJoint = JointSaved && JointSaved->IsA<UMaterialInstanceConstant>();
	const bool bSavedBrass = BrassBase && BrassBase->IsA<UMaterialInstanceConstant>();
	if (bSavedJoint) { Joint = JointSaved; }
	if (bInstances)
	{
		// Play only (never saved with the map). The marble's world grid off: its rings and radial slabs are
		// the joints; the joints the same marble a shade darker.
		if (FloorBase && !bSavedJoint)
		{
			UMaterialInstanceDynamic* SlabMID = UMaterialInstanceDynamic::Create(FloorBase, this);
			SlabMID->SetFlags(RF_Transient);
			SlabMID->SetScalarParameterValue(TEXT("FloorJoints"), 0.f);
			Slab = SlabMID;
			UMaterialInstanceDynamic* JointMID = UMaterialInstanceDynamic::Create(FloorBase, this);
			JointMID->SetFlags(RF_Transient);
			JointMID->SetScalarParameterValue(TEXT("FloorJoints"), 0.f);
			FLinearColor Base;
			if (FloorBase->GetVectorParameterValue(FName(TEXT("BaseColor")), Base))
			{
				JointMID->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(Base.R * JointShade, Base.G * JointShade, Base.B * JointShade, 1.f));
			}
			Joint = JointMID;
		}
		// Brushed brass: M_Metal's parameters (Scripts/materials.py), as the Salon's stele foot.
		if (BrassBase && !bSavedBrass)
		{
			UMaterialInstanceDynamic* BrassMID = UMaterialInstanceDynamic::Create(BrassBase, this);
			BrassMID->SetFlags(RF_Transient);
			BrassMID->SetVectorParameterValue(TEXT("BaseColor"), BrassColour);
			BrassMID->SetScalarParameterValue(TEXT("Roughness"), BrassRoughness);
			BrassMID->SetScalarParameterValue(TEXT("Metallic"), 1.f);
			BrassMID->SetScalarParameterValue(TEXT("Variation"), 0.04f);
			BrassMID->SetScalarParameterValue(TEXT("PatinaAmount"), 0.f);
			Brass = BrassMID;
		}
		// The diffuser: M_Daylit held at its full glow (night floor 1), a white image, the lights' colour.
		if (GlowBase)
		{
			UMaterialInstanceDynamic* GlowMID = UMaterialInstanceDynamic::Create(GlowBase, this);
			GlowMID->SetFlags(RF_Transient);
			if (UTexture* White = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture")))
			{
				GlowMID->SetTextureParameterValue(TEXT("Image"), White);
			}
			const FLinearColor Warm = FLinearColor::MakeFromColorTemperature(CoveKelvin);
			const float Luma = FMath::Max(Warm.GetLuminance(), 1e-3f);
			GlowMID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(Warm.R / Luma, Warm.G / Luma, Warm.B / Luma, 1.f));
			GlowMID->SetScalarParameterValue(TEXT("Luminance"), CoveNits);
			GlowMID->SetScalarParameterValue(TEXT("NightFloor"), 1.f);
			Glow = GlowMID;
		}
	}
	UMaterialInterface* Stone = StoneMaterial.LoadSynchronous();
	UMaterialInterface* Bronze = BronzeMaterial.LoadSynchronous();
	Set(Floor, 0, Slab);
	Set(Floor, 1, Joint);
	Set(Floor, 2, Bronze);
	Set(Walls, 0, Stone);
	Set(CoveLed, 0, Glow);
	Set(Inlays, 0, Bronze);
	Set(Stele, 0, GlassMaterial.LoadSynchronous());
	Set(SteleFoot, 0, Brass);
}

void AAtriumBaseStructure::PlaceLights()
{
	const FBox Foot = ABK::SteleFoot();
	const FBox Block(FVector(Foot.Min.X, Foot.Min.Y, 0.0), FVector(Foot.Max.X, Foot.Max.Y, MuseePlan::Atrium::SteleTop));
	if (SteleBlock)
	{
		SteleBlock->SetRelativeLocation(Block.GetCenter() * MuseePlan::Cm);
		SteleBlock->SetBoxExtent(Block.GetExtent() * MuseePlan::Cm);
	}
	const TArray<ABK::FCoveLight> Places = ABK::CoveLightPlaces();
	for (int32 I = 0; I < CoveLights.Num(); ++I)
	{
		URectLightComponent* L = CoveLights[I];
		if (!L) { continue; }
		if (!Places.IsValidIndex(I))
		{
			L->SetVisibility(false);
			continue;
		}
		const ABK::FCoveLight& P = Places[I];
		L->SetRelativeLocationAndRotation(P.At * MuseePlan::Cm, FRotationMatrix::MakeFromXY(P.Facing, P.Across).Rotator());
		L->SetSourceWidth(static_cast<float>(P.Width * MuseePlan::Cm));
		L->SetSourceHeight(2.f);
		L->SetBarnDoorAngle(88.f);
		L->SetBarnDoorLength(0.f);
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(static_cast<float>(CoveLumensPerMetre * P.Length));
		L->SetUseTemperature(true);
		L->SetTemperature(CoveKelvin);
		L->SetLightColor(FLinearColor::White);
		// Unshadowed, so short: under 1.54 m it never reaches through the 1.45 m slab to the Square's ceiling (−1.5),
		// whose dark rooms lie under more than half the cove's circle; it still lights the floor 1.4 m out.
		L->SetAttenuationRadius(140.f);
		L->SetCastShadows(false);
		L->SetVisibility(true);
	}
}
