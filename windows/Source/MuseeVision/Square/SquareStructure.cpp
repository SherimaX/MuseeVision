#include "Square/SquareStructure.h"

#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Scene.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"
#include "Geometry/MuseeBake.h"
#include "Salon/SalonKit.h"

/**
 * The Square's geometry, in metres: x east and y south from the Square's centre (the Atrium's axis),
 * z the height above the Square's floor; At() puts a point in the actor's frame (the plan's own when
 * the actor stands at the origin). A named namespace: the module builds in unity files.
 */
namespace SquareBuild
{
	namespace E = MuseePlan::Elan;
	using SalonKit::FMeshData;

	constexpr double Turn = 2.0 * UE_DOUBLE_PI;
	constexpr double Half = E::SquareHalf;              // 14: the walls' inner faces
	constexpr double Wall = E::SquareWall;              // 1.2
	constexpr double G = E::SquareGrid;                 // 4.667: the grid lines
	constexpr double Cell = 2.0 * G;                    // 9.333: from the centre to a cell's centre
	constexpr double H = E::SquareHeight;               // 7.5
	constexpr double PartHalf = E::SquarePartition / 2; // 0.3
	constexpr double ColumnHalf = E::SquareColumn / 2;  // 0.6
	constexpr double CapitalHalf = E::SquareCapital / 2;
	constexpr double GlassR = E::SquareGlassRadius;     // 4.2
	constexpr double PitDepth = E::SquarePitDepth;      // 7
	constexpr double Sink = 0.05;                       // walls, partitions and columns stand this far into the floor
	constexpr double Rise = 0.02;                       // … and rise this far into the ceiling slab
	constexpr double Into = 0.02;                       // the partitions run this far into the outer walls
	constexpr double AtriumLevel = -E::SquareFloor;     // 9: the Atrium's floor over this one
	constexpr double SlabTop = AtriumLevel - 0.05;      // the slab's top, 5 cm under the Atrium's floor
	constexpr int32 CellsPerHalf = 30;                  // the floor's and the ceiling's grid: 28/60 m cells
	constexpr int32 BlockCells = 10;                    // … ten to a grid step: the centre block (5) is ±10
	constexpr double FrameIn = 4.13;                    // the glass's bronze frame: 7 cm wide, 1.5 mm proud
	constexpr double FrameLift = 0.0015;
	constexpr double RingGlowDrop = 0.01;               // the ring of light hangs 1 cm under the ceiling
	constexpr int32 RingSegments = 256;
	constexpr double NumeralHeight = 0.62;              // the figures' height
	constexpr double NumeralLift = 0.002;               // cast in, 2 mm proud
	constexpr double FiveNorth = 3.2;                   // 5 lies north of the car, on the glass

	// Lights: the washers 3 m out from their wall, just under the ceiling, aimed below its foot.
	constexpr double WasherOff = 3.0, WasherZ = 7.4, WasherAimZ = -0.5;
	constexpr double DownZ = 7.45, ShaftZ = 8.8;
	constexpr double PitUpperZ = -1.2, PitLowerZ = -4.6;

	FVector At(double X, double Y, double Z) { return FVector(E::CentreX + X, E::CentreY + Y, E::SquareFloor + Z); }
	FVector2D PlanUV(double X, double Y) { return FVector2D(E::CentreX + X, E::CentreY + Y); }
	/** Rammed earth: U = x + y (the same on both faces at every corner), V = down from the ceiling. */
	FVector2D EarthUV(double X, double Y, double Z) { return FVector2D(E::CentreX + X + E::CentreY + Y, H - Z); }

	double GridCut(int32 K) { return Half * K / CellsPerHalf; }

	/** −Ext … Ext through every line of the grid (Ext at least Half). */
	TArray<double> Cuts(double Ext)
	{
		TArray<double> Out;
		if (Ext > Half + 1e-9) { Out.Add(-Ext); }
		for (int32 K = -CellsPerHalf; K <= CellsPerHalf; ++K) { Out.Add(GridCut(K)); }
		if (Ext > Half + 1e-9) { Out.Add(Ext); }
		return Out;
	}

	/** The centre block's edge (±G) on the grid's own points, from east (G, 0) round through south, west and north: 80. */
	TArray<FVector2D> BlockEdge()
	{
		const int32 N = BlockCells;
		const double B = GridCut(N);
		TArray<FVector2D> P;
		for (int32 K = 0; K < N; ++K) { P.Add(FVector2D(B, GridCut(K))); }      // east side, going south
		for (int32 K = N; K > -N; --K) { P.Add(FVector2D(GridCut(K), B)); }     // south side, going west
		for (int32 K = N; K > -N; --K) { P.Add(FVector2D(-B, GridCut(K))); }    // west side, going north
		for (int32 K = -N; K < N; ++K) { P.Add(FVector2D(GridCut(K), -B)); }    // north side, going east
		for (int32 K = -N; K < 0; ++K) { P.Add(FVector2D(B, GridCut(K))); }     // east side again, to y 0
		return P;
	}

	/**
	 * The angles round every circle at 5 (the glass, its frame, the pit, the ceiling's ring and hole):
	 * each ray to a point of the block's edge and one between, from 0 (east, towards south) to 2π: 161,
	 * the last the first again.
	 */
	TArray<double> CircleAngles()
	{
		const TArray<FVector2D> Edge = BlockEdge();
		auto Angle = [](const FVector2D& P)
		{
			const double A = FMath::Atan2(P.Y, P.X);
			return A < -1e-12 ? A + Turn : FMath::Max(A, 0.0);
		};
		TArray<double> A;
		for (int32 J = 0; J < Edge.Num(); ++J)
		{
			const double A0 = Angle(Edge[J]);
			const double A1 = J + 1 < Edge.Num() ? Angle(Edge[J + 1]) : Turn;
			A.Add(A0);
			A.Add((A0 + A1) / 2);
		}
		A.Add(Turn);
		return A;
	}

	/** A point of a circle at 5 by its index in CircleAngles (the last is the first). */
	FVector2D OnCircle(const TArray<double>& A, double R, int32 I)
	{
		const double T = A[I % (A.Num() - 1)];
		return FVector2D(R * FMath::Cos(T), R * FMath::Sin(T));
	}

	/**
	 * The plane at height Z, facing up or down, over [−Ext, Ext]² less a disc of radius HoleR at 5: the
	 * grid's cells, each into the mesh Pick gives for its centre (none: left out), and round the hole
	 * the centre block, fanned from its edge to the circle, into CentreMesh.
	 */
	void Region(double Z, bool bUp, double Ext, double HoleR, TFunctionRef<FMeshData*(double, double)> Pick, FMeshData& CentreMesh)
	{
		const FVector N(0, 0, bUp ? 1 : -1);
		auto V = [Z, &N](FMeshData& M, double X, double Y) { return M.Vertex(At(X, Y, Z), N, PlanUV(X, Y)); };
		const TArray<double> C = Cuts(Ext);
		for (int32 I = 0; I + 1 < C.Num(); ++I)
		{
			for (int32 J = 0; J + 1 < C.Num(); ++J)
			{
				const double X0 = C[I], X1 = C[I + 1], Y0 = C[J], Y1 = C[J + 1];
				const double Mx = (X0 + X1) / 2, My = (Y0 + Y1) / 2;
				if (FMath::Abs(Mx) < G && FMath::Abs(My) < G) { continue; }
				if (FMeshData* M = Pick(Mx, My)) { M->Quad(V(*M, X0, Y0), V(*M, X1, Y0), V(*M, X1, Y1), V(*M, X0, Y1)); }
			}
		}
		const TArray<FVector2D> Edge = BlockEdge();
		const TArray<double> A = CircleAngles();
		for (int32 J = 0; J < Edge.Num(); ++J)
		{
			const FVector2D S0 = Edge[J], S1 = Edge[(J + 1) % Edge.Num()];
			const FVector2D R0 = OnCircle(A, HoleR, 2 * J), R1 = OnCircle(A, HoleR, 2 * J + 1), R2 = OnCircle(A, HoleR, 2 * J + 2);
			const int32 IS0 = V(CentreMesh, S0.X, S0.Y), IS1 = V(CentreMesh, S1.X, S1.Y);
			const int32 IR0 = V(CentreMesh, R0.X, R0.Y), IR1 = V(CentreMesh, R1.X, R1.Y), IR2 = V(CentreMesh, R2.X, R2.Y);
			CentreMesh.Tri(IS0, IR0, IR1);
			CentreMesh.Tri(IS0, IR1, IS1);
			CentreMesh.Tri(IS1, IR1, IR2);
		}
	}

	/** An annulus at height Z round 5 on the circles' angles, facing up or down. */
	void Annulus(FMeshData& M, double Z, double RIn, double ROut, bool bUp)
	{
		const FVector N(0, 0, bUp ? 1 : -1);
		const TArray<double> A = CircleAngles();
		for (int32 I = 0; I + 1 < A.Num(); ++I)
		{
			auto V = [&](double R, int32 K) { const FVector2D P = OnCircle(A, R, K); return M.Vertex(At(P.X, P.Y, Z), N, PlanUV(P.X, P.Y)); };
			M.Quad(V(RIn, I), V(ROut, I), V(ROut, I + 1), V(RIn, I + 1));
		}
	}

	/** A disc at height Z round 5 on the circles' angles, fanned from the centre. */
	void Disc(FMeshData& M, double Z, double R, bool bUp, TFunctionRef<FVector2D(double, double)> UV)
	{
		const FVector N(0, 0, bUp ? 1 : -1);
		const TArray<double> A = CircleAngles();
		const int32 Mid = M.Vertex(At(0, 0, Z), N, UV(0, 0));
		for (int32 I = 0; I + 1 < A.Num(); ++I)
		{
			const FVector2D P0 = OnCircle(A, R, I), P1 = OnCircle(A, R, I + 1);
			M.Tri(Mid, M.Vertex(At(P0.X, P0.Y, Z), N, UV(P0.X, P0.Y)), M.Vertex(At(P1.X, P1.Y, Z), N, UV(P1.X, P1.Y)));
		}
	}

	/** A cylinder round 5 from Z0 to Z1 on the circles' angles, facing the axis (bInward) or away. U the arc from east, V −z. */
	void Cylinder(FMeshData& M, double R, double Z0, double Z1, bool bInward)
	{
		const TArray<double> A = CircleAngles();
		for (int32 I = 0; I + 1 < A.Num(); ++I)
		{
			auto V = [&](int32 K, double Z)
			{
				const FVector2D P = OnCircle(A, R, K);
				const FVector Out(FMath::Cos(A[K]), FMath::Sin(A[K]), 0);
				return M.Vertex(At(P.X, P.Y, Z), bInward ? -Out : Out, FVector2D(A[K] * R, -Z));
			};
			M.Quad(V(I, Z1), V(I + 1, Z1), V(I + 1, Z0), V(I, Z0));
		}
	}

	/**
	 * A solid made of the cells of a grid (cuts along x, y and z): every face between a filled cell and
	 * an empty one (or the outside), facing out. Closed, and free of T-junctions, whatever the cells.
	 */
	void GridSolid(FMeshData& M, const TArray<double>& Xs, const TArray<double>& Ys, const TArray<double>& Zs,
				   TFunctionRef<bool(int32, int32, int32)> Filled)
	{
		auto In = [&](int32 I, int32 J, int32 K)
		{
			return I >= 0 && J >= 0 && K >= 0 && I + 1 < Xs.Num() && J + 1 < Ys.Num() && K + 1 < Zs.Num() && Filled(I, J, K);
		};
		const TArray<double>* Axes[3] = {&Xs, &Ys, &Zs};
		for (int32 I = 0; I + 1 < Xs.Num(); ++I)
		{
			for (int32 J = 0; J + 1 < Ys.Num(); ++J)
			{
				for (int32 K = 0; K + 1 < Zs.Num(); ++K)
				{
					if (!Filled(I, J, K)) { continue; }
					const int32 Cell3[3] = {I, J, K};
					for (int32 Axis = 0; Axis < 3; ++Axis)
					{
						for (const int32 Dir : {-1, 1})
						{
							int32 Next[3] = {I, J, K};
							Next[Axis] += Dir;
							if (In(Next[0], Next[1], Next[2])) { continue; }
							// The face's plane, and the cell's extent over the other two axes.
							const int32 UAxis = (Axis + 1) % 3, VAxis = (Axis + 2) % 3;
							const double Plane = (*Axes[Axis])[Cell3[Axis] + (Dir > 0 ? 1 : 0)];
							const double U0 = (*Axes[UAxis])[Cell3[UAxis]], U1 = (*Axes[UAxis])[Cell3[UAxis] + 1];
							const double V0 = (*Axes[VAxis])[Cell3[VAxis]], V1 = (*Axes[VAxis])[Cell3[VAxis] + 1];
							FVector Nrm = FVector::ZeroVector;
							Nrm[Axis] = Dir;
							auto Corner = [&](double U, double V)
							{
								double P[3];
								P[Axis] = Plane;
								P[UAxis] = U;
								P[VAxis] = V;
								const FVector2D UV = Axis == 2 ? PlanUV(P[0], P[1]) : EarthUV(P[0], P[1], P[2]);
								return M.Vertex(At(P[0], P[1], P[2]), Nrm, UV);
							};
							M.Quad(Corner(U0, V0), Corner(U1, V0), Corner(U1, V1), Corner(U0, V1));
						}
					}
				}
			}
		}
	}

	/** An axis-aligned box as a grid solid. */
	void Box(FMeshData& M, double X0, double X1, double Y0, double Y1, double Z0, double Z1)
	{
		GridSolid(M, {X0, X1}, {Y0, Y1}, {Z0, Z1}, [](int32, int32, int32) { return true; });
	}

	/**
	 * A partition, 0.6 m thick, centred on a grid line: along y at x = Across (bAlongY) or along x at
	 * y = Across, from From to To; with a doorway (2.6 × 3.0 m) centred at Door along it if bDoor.
	 */
	struct FPartition
	{
		bool bAlongY;
		double Across, From, To;
		bool bDoor;
		double Door;
	};

	/** The eight partitions (Elan.swift W1 … W8); the doorways form a clockwise pinwheel. */
	TArray<FPartition> Partitions()
	{
		const double Out = Half + Into;
		return {
			{true, -G, -Out, -G, true, -Cell},    // W1: 6 opens east into 1
			{false, -G, -Out, -G, false, 0},      // W2
			{true, G, -Out, -G, false, 0},        // W3
			{false, -G, G, Out, true, Cell},      // W4: 8 opens south into 3
			{false, G, G, Out, false, 0},         // W5
			{true, G, G, Out, true, Cell},        // W6: 4 opens west into 9
			{true, -G, G, Out, false, 0},         // W7
			{false, G, -Out, -G, true, -Cell},    // W8: 2 opens north into 7
		};
	}

	void AddPartition(FMeshData& M, const FPartition& P)
	{
		const double DoorHalf = E::SquareDoorWidth / 2;
		TArray<double> Along = {P.From, P.To};
		TArray<double> Zs = {-Sink, H + Rise};
		if (P.bDoor)
		{
			Along = {P.From, P.Door - DoorHalf, P.Door + DoorHalf, P.To};
			Zs = {-Sink, E::SquareDoorHeight, H + Rise};
		}
		const TArray<double> Across = {P.Across - PartHalf, P.Across + PartHalf};
		const bool bDoor = P.bDoor;
		if (P.bAlongY)
		{
			GridSolid(M, Across, Along, Zs, [bDoor](int32, int32 J, int32 K) { return !(bDoor && J == 1 && K == 0); });
		}
		else
		{
			GridSolid(M, Along, Across, Zs, [bDoor](int32 I, int32, int32 K) { return !(bDoor && I == 1 && K == 0); });
		}
	}

	/** The four grid crossings, where the columns stand. */
	TArray<FVector2D> Crossings() { return {FVector2D(-G, -G), FVector2D(G, -G), FVector2D(G, G), FVector2D(-G, G)}; }

	// ---- The Lo Shu's figures: Didot-like strokes (thick verticals, hairline horizontals, ball terminals).

	constexpr double Thick = 0.15, Thin = 0.032;   // of the figure's height

	struct FStrokePoint { FVector2D P; double W; };
	struct FStroke { TArray<FStrokePoint> Points; bool bClosed = false; };
	struct FGlyph
	{
		double Width = 0.6;
		TArray<FStroke> Strokes;
		TArray<FVector> Balls;   // x, y, radius
	};

	/** Didot's stress: a stroke as thick as it is upright. */
	double StrokeWidth(const FVector2D& Tangent, double ThickW)
	{
		const double L = Tangent.Size();
		const double Up = L > 0 ? FMath::Abs(Tangent.Y) / L : 0.0;
		return Thin + (ThickW - Thin) * FMath::Pow(Up, 1.6);
	}

	struct FGlyphBuilder
	{
		FGlyph Glyph;

		/** An elliptical arc from A0 to A1 degrees (a whole turn: a closed loop). */
		void Arc(double Cx, double Cy, double Rx, double Ry, double A0, double A1, int32 N = 48, double ThickW = Thick)
		{
			FStroke S;
			S.bClosed = FMath::Abs(A1 - A0) >= 359.9;
			const int32 Count = S.bClosed ? N : N + 1;
			for (int32 I = 0; I < Count; ++I)
			{
				const double A = FMath::DegreesToRadians(A0 + (A1 - A0) * I / N);
				const FVector2D T(-Rx * FMath::Sin(A), Ry * FMath::Cos(A));
				S.Points.Add({FVector2D(Cx + Rx * FMath::Cos(A), Cy + Ry * FMath::Sin(A)), StrokeWidth(T, ThickW)});
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

		/** A cubic Bézier, its width from its slope. */
		void Curve(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, double ThickW = Thick, int32 N = 32)
		{
			FStroke S;
			for (int32 I = 0; I <= N; ++I)
			{
				const double T = double(I) / N, U = 1 - T;
				const FVector2D P = P0 * (U * U * U) + P1 * (3 * U * U * T) + P2 * (3 * U * T * T) + P3 * (T * T * T);
				const FVector2D D = (P1 - P0) * (3 * U * U) + (P2 - P1) * (6 * U * T) + (P3 - P2) * (3 * T * T);
				S.Points.Add({P, StrokeWidth(D, ThickW)});
			}
			Glyph.Strokes.Add(S);
		}

		void Ball(double X, double Y, double R) { Glyph.Balls.Add(FVector(X, Y, R)); }
	};

	/** 9 is 6 turned half round. */
	FGlyph Turned(const FGlyph& Src)
	{
		FGlyph Out = Src;
		for (FStroke& S : Out.Strokes)
		{
			for (FStrokePoint& P : S.Points) { P.P = FVector2D(Src.Width - P.P.X, 1 - P.P.Y); }
		}
		for (FVector& B : Out.Balls) { B = FVector(Src.Width - B.X, 1 - B.Y, B.Z); }
		return Out;
	}

	/** A figure 1 … 9 in a unit of its height (baseline 0, top 1; x from 0 to its width). */
	FGlyph Figure(int32 Digit)
	{
		FGlyphBuilder B;
		switch (Digit)
		{
		case 1:
			B.Glyph.Width = 0.5;
			B.Line(0.27, 0.0, 0.27, 1.0, Thick);
			B.Line(0.34, 0.995, 0.06, 0.80, Thin * 1.1, Thin * 0.9);
			B.Line(0.04, 0.016, 0.50, 0.016, Thin);
			break;
		case 2:
			B.Glyph.Width = 0.6;
			B.Arc(0.30, 0.73, 0.235, 0.25, 165, -30);
			B.Curve(FVector2D(0.30 + 0.235 * FMath::Cos(FMath::DegreesToRadians(-30.0)), 0.73 + 0.25 * FMath::Sin(FMath::DegreesToRadians(-30.0))), FVector2D(0.46, 0.44),
					FVector2D(0.12, 0.22), FVector2D(0.04, 0.02), 0.12);
			B.Line(0.04, 0.03, 0.60, 0.03, 0.06);
			B.Line(0.585, 0.0, 0.585, 0.14, Thin);
			B.Ball(0.105, 0.78, 0.072);
			break;
		case 3:
			B.Glyph.Width = 0.58;
			B.Arc(0.28, 0.76, 0.21, 0.225, 155, -90);
			B.Arc(0.30, 0.28, 0.255, 0.275, 90, -155);
			B.Line(0.20, 0.535, 0.30, 0.535, Thin);
			B.Ball(0.11, 0.84, 0.066);
			B.Ball(0.10, 0.18, 0.072);
			break;
		case 4:
			B.Glyph.Width = 0.62;
			B.Line(0.43, 1.0, 0.02, 0.30, Thin);
			B.Line(0.02, 0.29, 0.62, 0.29, Thin * 1.2);
			B.Line(0.43, 0.0, 0.43, 1.0, Thick);
			B.Line(0.24, 0.016, 0.60, 0.016, Thin);
			break;
		case 5:
			B.Glyph.Width = 0.58;
			B.Line(0.11, 0.965, 0.56, 0.965, 0.075);
			B.Line(0.11, 1.0, 0.09, 0.52, Thin * 1.2);
			B.Arc(0.30, 0.30, 0.255, 0.30, 138, -150);
			B.Ball(0.10, 0.17, 0.072);
			break;
		case 6:
		case 9:
			B.Glyph.Width = 0.6;
			B.Arc(0.31, 0.29, 0.245, 0.29, 0, 360, 64);
			B.Arc(0.34, 0.44, 0.28, 0.56, 38, 196);
			B.Ball(0.53, 0.84, 0.07);
			return Digit == 9 ? Turned(B.Glyph) : B.Glyph;
		case 7:
			B.Glyph.Width = 0.6;
			B.Line(0.03, 0.97, 0.60, 0.97, 0.07);
			B.Line(0.04, 1.0, 0.04, 0.84, Thin);
			B.Curve(FVector2D(0.60, 0.97), FVector2D(0.46, 0.66), FVector2D(0.30, 0.38), FVector2D(0.26, 0.0), 0.14);
			break;
		case 8:
		default:
			B.Glyph.Width = 0.56;
			B.Arc(0.28, 0.755, 0.20, 0.23, 0, 360, 64, 0.12);
			B.Arc(0.28, 0.275, 0.245, 0.275, 0, 360, 64);
			break;
		}
		return B.Glyph;
	}

	/** A figure cast in the floor, centred on (Px, Py), north up: its strokes as ribbons 2 mm proud, with their sides. */
	void AddFigure(FMeshData& M, const FGlyph& Glyph, double Px, double Py)
	{
		const double Top = NumeralLift, Bottom = -0.001;
		const FVector Up(0, 0, 1);
		// Glyph up is north (−y); glyph x is east.
		auto ToPlan = [&Glyph, Px, Py](const FVector2D& Q) { return FVector2D(Px + (Q.X - Glyph.Width / 2) * NumeralHeight, Py - (Q.Y - 0.5) * NumeralHeight); };
		auto V = [&M](const FVector2D& P, double Z, const FVector& N) { return M.Vertex(At(P.X, P.Y, Z), N, PlanUV(P.X, P.Y)); };
		for (const FStroke& S : Glyph.Strokes)
		{
			const int32 N = S.Points.Num();
			if (N < 2) { continue; }
			TArray<FVector2D> L, R, C;
			for (int32 I = 0; I < N; ++I)
			{
				const int32 A = S.bClosed ? (I - 1 + N) % N : FMath::Max(I - 1, 0);
				const int32 B = S.bClosed ? (I + 1) % N : FMath::Min(I + 1, N - 1);
				const FVector2D T = (S.Points[B].P - S.Points[A].P).GetSafeNormal();
				const FVector2D Side(-T.Y, T.X);
				const double W = S.Points[I].W / 2;
				C.Add(ToPlan(S.Points[I].P));
				L.Add(ToPlan(S.Points[I].P + Side * W));
				R.Add(ToPlan(S.Points[I].P - Side * W));
			}
			auto Out = [](const FVector2D& From, const FVector2D& To) { const FVector2D D = (To - From).GetSafeNormal(); return FVector(D.X, D.Y, 0); };
			const int32 Segs = S.bClosed ? N : N - 1;
			for (int32 I = 0; I < Segs; ++I)
			{
				const int32 J = (I + 1) % N;
				M.Quad(V(L[I], Top, Up), V(L[J], Top, Up), V(R[J], Top, Up), V(R[I], Top, Up));
				for (const TArray<FVector2D>* Edge : {&L, &R})
				{
					const FVector NI = Out(C[I], (*Edge)[I]), NJ = Out(C[J], (*Edge)[J]);
					M.Quad(V((*Edge)[I], Bottom, NI), V((*Edge)[J], Bottom, NJ), V((*Edge)[J], Top, NJ), V((*Edge)[I], Top, NI));
				}
			}
			if (!S.bClosed)
			{
				for (const int32 End : {0, N - 1})
				{
					const FVector N3 = End == 0 ? Out(C[1], C[0]) : Out(C[N - 2], C[N - 1]);
					M.Quad(V(L[End], Bottom, N3), V(R[End], Bottom, N3), V(R[End], Top, N3), V(L[End], Top, N3));
				}
			}
		}
		for (const FVector& Ball : Glyph.Balls)
		{
			const FVector2D BallCentre = ToPlan(FVector2D(Ball.X, Ball.Y));
			const double Radius = Ball.Z * NumeralHeight;
			constexpr int32 Segs = 24;
			const int32 Mid = V(BallCentre, Top, Up);
			for (int32 I = 0; I < Segs; ++I)
			{
				const double A0 = Turn * I / Segs, A1 = Turn * (I + 1) / Segs;
				const FVector2D P0 = BallCentre + FVector2D(FMath::Cos(A0), FMath::Sin(A0)) * Radius;
				const FVector2D P1 = BallCentre + FVector2D(FMath::Cos(A1), FMath::Sin(A1)) * Radius;
				M.Tri(Mid, V(P0, Top, Up), V(P1, Top, Up));
				const FVector N0(FMath::Cos(A0), FMath::Sin(A0), 0), N1(FMath::Cos(A1), FMath::Sin(A1), 0);
				M.Quad(V(P0, Bottom, N0), V(P1, Bottom, N1), V(P1, Top, N1), V(P0, Top, N0));
			}
		}
	}

	/** The Lo Shu (north up: 6 1 8 / 7 5 3 / 2 9 4): each figure's place on the floor, 1 … 9. */
	TArray<FVector2D> FigurePlaces()
	{
		static const int32 Square[3][3] = {{6, 1, 8}, {7, 5, 3}, {2, 9, 4}};
		TArray<FVector2D> Places;
		Places.SetNum(9);
		for (int32 Row = 0; Row < 3; ++Row)
		{
			for (int32 Col = 0; Col < 3; ++Col)
			{
				const int32 Digit = Square[Row][Col];
				Places[Digit - 1] = Digit == 5 ? FVector2D(0, -FiveNorth) : FVector2D(Cell * (Col - 1), Cell * (Row - 1));
			}
		}
		return Places;
	}

	struct FSquareMeshes
	{
		FMeshData Earth, PaleFloor, DarkFloor, Glass, Strata, Ceiling, Capitals, Bronze, Ring;
	};

	FSquareMeshes BuildMeshes()
	{
		FSquareMeshes Out;

		// ---- Rammed earth: the outer walls (a ring round the 28 m square), the partitions, the columns.
		GridSolid(Out.Earth, {-Half - Wall, -Half, Half, Half + Wall}, {-Half - Wall, -Half, Half, Half + Wall}, {-Sink, H + Rise},
				  [](int32 I, int32 J, int32) { return !(I == 1 && J == 1); });
		for (const FPartition& P : Partitions()) { AddPartition(Out.Earth, P); }
		for (const FVector2D& X : Crossings())
		{
			Box(Out.Earth, X.X - ColumnHalf, X.X + ColumnHalf, X.Y - ColumnHalf, X.Y + ColumnHalf, -Sink, H + Rise);
			Box(Out.Capitals, X.X - CapitalHalf, X.X + CapitalHalf, X.Y - CapitalHalf, X.Y + CapitalHalf, H - E::SquareCapitalDepth, H + Rise);
		}

		// ---- The floor: pale in the cross (and round the glass), dark in the corner rooms; the hole at 5.
		FMeshData* Pale = &Out.PaleFloor;
		FMeshData* Dark = &Out.DarkFloor;
		Region(0, true, Half, GlassR, [Pale, Dark](double X, double Y) { return (FMath::Abs(X) > G && FMath::Abs(Y) > G) ? Dark : Pale; }, Out.PaleFloor);

		// ---- The glass, its bronze frame, and the pit under it: the wall in the strata, the floor of bedrock.
		Disc(Out.Glass, 0, GlassR, true, [](double X, double Y) { return PlanUV(X, Y); });
		Annulus(Out.Bronze, FrameLift, FrameIn, GlassR, true);
		Cylinder(Out.Strata, GlassR, -PitDepth, 0, true);
		Disc(Out.Strata, -PitDepth, GlassR, true, [](double X, double Y)
		{
			return FVector2D(X + GlassR, PitDepth + (Y + GlassR) * PitDepth / (2 * GlassR));
		});

		// ---- The ceiling slab: its underside round the bronze ring, its top round the shaft, its edges over the walls.
		FMeshData* Slab = &Out.Ceiling;
		const double SlabHalf = Half + Wall;
		Region(H, false, SlabHalf, E::SquareRingOut, [Slab](double, double) { return Slab; }, Out.Ceiling);
		Region(SlabTop, true, SlabHalf, E::SquareRingIn, [Slab](double, double) { return Slab; }, Out.Ceiling);
		const TArray<double> C = Cuts(SlabHalf);
		for (int32 I = 0; I + 1 < C.Num(); ++I)
		{
			for (const int32 Side : {0, 1, 2, 3})
			{
				const double S = (Side % 2 == 0) ? SlabHalf : -SlabHalf;
				const bool bAlongY = Side < 2;   // faces ±x run along y
				const FVector N = bAlongY ? FVector(S > 0 ? 1 : -1, 0, 0) : FVector(0, S > 0 ? 1 : -1, 0);
				auto V = [&](double Along, double Z)
				{
					const double X = bAlongY ? S : Along, Y = bAlongY ? Along : S;
					return Out.Ceiling.Vertex(At(X, Y, Z), N, EarthUV(X, Y, Z));
				};
				Out.Ceiling.Quad(V(C[I], H), V(C[I + 1], H), V(C[I + 1], SlabTop), V(C[I], SlabTop));
			}
		}
		// The bronze: the ring round the opening, the lining up through the slab to the Atrium's floor.
		Annulus(Out.Bronze, H, E::SquareRingIn, E::SquareRingOut, false);
		Cylinder(Out.Bronze, E::SquareRingIn, H, SlabTop, true);
		Cylinder(Out.Bronze, E::SquareRingIn, SlabTop, AtriumLevel, true);

		// ---- The ring of light: a band 1 cm under the ceiling, and its two edges; in the galleries only. Over the four
		// dark corner rooms (6, 8, 4, 2) it would be a slot of daylight (600 nits) running through their ceilings, brighter
		// than their own light: there it stops, its ends inside the partitions (a segment is 34 cm, a partition 60).
		{
			const double Z0 = H - RingGlowDrop;
			const double RIn = E::SquareLightRingIn, ROut = E::SquareLightRingOut;
			FMeshData& M = Out.Ring;
			for (int32 I = 0; I < RingSegments; ++I)
			{
				const double A0 = Turn * I / RingSegments, A1 = Turn * (I + 1) / RingSegments;
				const double AM = 0.5 * (A0 + A1), RM = 0.5 * (RIn + ROut);
				if (FMath::Abs(FMath::Cos(AM) * RM) > G && FMath::Abs(FMath::Sin(AM) * RM) > G) { continue; }
				const FVector D0(FMath::Cos(A0), FMath::Sin(A0), 0), D1(FMath::Cos(A1), FMath::Sin(A1), 0);
				auto V = [&M](const FVector& Dir, double R, double Z, const FVector& N)
				{
					return M.Vertex(At(Dir.X * R, Dir.Y * R, Z), N, PlanUV(Dir.X * R, Dir.Y * R));
				};
				const FVector Down(0, 0, -1);
				M.Quad(V(D0, RIn, Z0, Down), V(D0, ROut, Z0, Down), V(D1, ROut, Z0, Down), V(D1, RIn, Z0, Down));
				M.Quad(V(D0, RIn, Z0, -D0), V(D1, RIn, Z0, -D1), V(D1, RIn, H, -D1), V(D0, RIn, H, -D0));
				M.Quad(V(D0, ROut, Z0, D0), V(D1, ROut, Z0, D1), V(D1, ROut, H, D1), V(D0, ROut, H, D0));
			}
		}

		// ---- The Lo Shu's figures, in bronze.
		const TArray<FVector2D> Places = FigurePlaces();
		for (int32 Digit = 1; Digit <= 9; ++Digit) { AddFigure(Out.Bronze, Figure(Digit), Places[Digit - 1].X, Places[Digit - 1].Y); }
		return Out;
	}

	/** A spot's placing: where (m, the Square's frame) and where it points. */
	struct FAim
	{
		FVector From;
		FVector Dir;
	};

	/**
	 * The wall-washers, six to a gallery (1 north, 3 east, 9 south, 7 west): two on the outer wall at a
	 * quarter and three quarters of it, two on the partition with the doorway (the middle of the wall on
	 * either side of it), two on the other partition; each 3 m out from its wall, just under the
	 * ceiling, aimed at the wall's line half a metre below the floor.
	 */
	TArray<FAim> WasherAims()
	{
		TArray<FAim> Out;
		const FVector2D Galleries[4] = {FVector2D(0, -1), FVector2D(1, 0), FVector2D(0, 1), FVector2D(-1, 0)};
		const double Face = G - PartHalf;                       // the partitions' faces, either side of the gallery's axis
		const double Inner = G + ColumnHalf;                    // where the partitions' faces end, at the columns
		const double Length = Half - Inner;                     // 8.73
		const double DoorHalf = E::SquareDoorWidth / 2;
		for (const FVector2D& Axis : Galleries)
		{
			const FVector2D DoorSide(Axis.Y, -Axis.X);          // the pinwheel: the doorway is on this side
			auto Add = [&Out, &Axis, &DoorSide](double S, double T, const FVector2D& Normal)
			{
				// (S along the gallery's axis, T towards the doorway's side): a point on the wall; Normal into the room.
				const FVector2D Wall2 = Axis * S + DoorSide * T;
				const FVector2D From2 = Wall2 + Normal * WasherOff;
				const FVector From(From2.X, From2.Y, WasherZ), To(Wall2.X, Wall2.Y, WasherAimZ);
				Out.Add({From, (To - From).GetSafeNormal()});
			};
			const double Across = 2 * Face;
			for (const double F : {-0.25, 0.25}) { Add(Half, F * Across, -Axis); }
			const double DoorFrom = Cell - DoorHalf, DoorTo = Cell + DoorHalf;
			for (const double S : {(Inner + DoorFrom) / 2, (DoorTo + Half) / 2}) { Add(S, Face, -DoorSide); }
			for (const double F : {0.25, 0.75}) { Add(Inner + F * Length, -Face, DoorSide); }
		}
		return Out;
	}

	/** The galleries' centres (1, 3, 9, 7) and the dark rooms' (6, 8, 4, 2), on the floor. */
	TArray<FVector2D> GalleryCentres() { return {FVector2D(0, -Cell), FVector2D(Cell, 0), FVector2D(0, Cell), FVector2D(-Cell, 0)}; }
	TArray<FVector2D> DarkRoomCentres() { return {FVector2D(-Cell, -Cell), FVector2D(Cell, -Cell), FVector2D(Cell, Cell), FVector2D(-Cell, Cell)}; }

	constexpr int32 WasherCount = 24;
}

namespace SB = SquareBuild;

ASquareStructure::ASquareStructure()
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
			// Walls, columns and floors collide as themselves (complex as simple); the floors are walkable.
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
	GlassFloor = Make(TEXT("GlassFloor"), true);
	GlassFloor->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);   // glass: the look trace goes through
	Pit = Make(TEXT("Pit"), false);
	Ceiling = Make(TEXT("Ceiling"), false);
	Bronze = Make(TEXT("Bronze"), false);
	LightRing = Make(TEXT("LightRing"), false);
	LightRing->SetCastShadow(false);
	MuseeBake::NoBake(LightRing);   // its glow is made at BeginPlay (ApplyMaterials): stays procedural

	auto MakeSpot = [this](const FString& ComponentName)
	{
		USpotLightComponent* L = CreateDefaultSubobject<USpotLightComponent>(*ComponentName);
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		return L;
	};
	ShaftLight = MakeSpot(TEXT("ShaftLight"));
	for (int32 I = 0; I < SB::WasherCount; ++I) { WallWashers.Add(MakeSpot(FString::Printf(TEXT("WallWasher%02d"), I + 1))); }
	for (int32 I = 0; I < 4; ++I) { GalleryDownlights.Add(MakeSpot(FString::Printf(TEXT("GalleryDownlight%d"), I + 1))); }
	for (int32 I = 0; I < 4; ++I) { DarkRoomLights.Add(MakeSpot(FString::Printf(TEXT("DarkRoomLight%d"), I + 1))); }
	for (int32 I = 0; I < 2; ++I)
	{
		UPointLightComponent* L = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("PitLight%d"), I + 1));
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		PitLights.Add(L);
	}

	auto Path = [](const TCHAR* Folder, const TCHAR* Name)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name)));
	};
	const TCHAR* Materials = TEXT("/Game/Museum/Materials");
	const TCHAR* Imported = TEXT("/Game/Museum/Materials/USD");
	EarthMaterial = Path(Materials, TEXT("M_RammedEarth"));
	EarthFallbackMaterial = Path(Imported, TEXT("MI_rammed_earth"));
	FloorMaterial = Path(Materials, TEXT("M_SquareFloor"));
	FloorFallbackMaterial = Path(Imported, TEXT("MI_stone_slab_F1E9DC"));
	DarkFloorMaterial = Path(Materials, TEXT("M_SquareFloorDark"));
	DarkFloorFallbackMaterial = Path(Imported, TEXT("MI_floor_dark"));
	CeilingMaterial = Path(Materials, TEXT("M_SquareCeiling"));
	CeilingFallbackMaterial = Path(Materials, TEXT("M_Plaster_Coffer"));
	StrataMaterial = Path(Materials, TEXT("M_Strata"));
	StrataFallbackMaterial = Path(Imported, TEXT("MI_strata"));
	GlassMaterial = Path(Materials, TEXT("M_Glass"));
	BronzeMaterial = Path(Imported, TEXT("MI_bronze_gold_r40"));
	BronzeFallbackMaterial = Path(Materials, TEXT("M_Gilt"));
	RingMaterial = Path(Materials, TEXT("M_Daylit"));

	AddTags();
	PlaceLights();
}

TArray<FString> ASquareStructure::GetReplacedImportPrims()
{
	return {
		TEXT("/Museum/Elan/The_Square"),
		// The Square's lamps, in Elan.swift's order: the shaft's spot, then the four downlights.
		TEXT("/Museum/Elan/SpotLight_3"), TEXT("/Museum/Elan/SpotLight_4"), TEXT("/Museum/Elan/SpotLight_5"),
		TEXT("/Museum/Elan/SpotLight_6"), TEXT("/Museum/Elan/SpotLight_7"),
	};
}

FVector ASquareStructure::GetGlassCentre() const
{
	return GetActorTransform().TransformPosition(SB::At(0, 0, 0) * MuseePlan::Cm);
}

TArray<FVector> ASquareStructure::GetNumeralPositions() const
{
	TArray<FVector> Out;
	for (const FVector2D& P : SB::FigurePlaces()) { Out.Add(GetActorTransform().TransformPosition(SB::At(P.X, P.Y, 0) * MuseePlan::Cm)); }
	return Out;
}

void ASquareStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials(false);
	PlaceLights();
	AddTags();
}

void ASquareStructure::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	PlaceLights();
	AddTags();
}

void ASquareStructure::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never
	// shows an older build; then the ring's own glow.
	Build();
	ApplyMaterials(true);
}

void ASquareStructure::AddTags()
{
	// Part of the building (hidden in the Sphere with the rest); electric light, so no musee.laylight
	// (AElanStructure would also turn off any other Élan actor's lights that carry it).
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Elan")));
}

void ASquareStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	const SB::FSquareMeshes M = SB::BuildMeshes();
	for (UProceduralMeshComponent* Component : {Structure.Get(), GlassFloor.Get(), Pit.Get(), Ceiling.Get(), Bronze.Get(), LightRing.Get()})
	{
		if (Component) { Component->ClearAllMeshSections(); }
	}
	M.Earth.Write(Structure, 0, true);
	M.PaleFloor.Write(Structure, 1, true);
	M.DarkFloor.Write(Structure, 2, true);
	M.Glass.Write(GlassFloor, 0, true);
	M.Strata.Write(Pit, 0, false);
	M.Ceiling.Write(Ceiling, 0, false);
	M.Capitals.Write(Ceiling, 1, false);
	M.Bronze.Write(Bronze, 0, false);
	M.Ring.Write(LightRing, 0, false);
}

void ASquareStructure::ApplyMaterials(bool bInstances)
{
	// Materials still being made are stood in for until they exist.
	auto Pick = [](const TSoftObjectPtr<UMaterialInterface>& Wanted, const TSoftObjectPtr<UMaterialInterface>& Fallback) -> UMaterialInterface*
	{
		if (UMaterialInterface* Loaded = Wanted.LoadSynchronous()) { return Loaded; }
		return Fallback.IsNull() ? nullptr : Fallback.LoadSynchronous();
	};
	auto Set = [](UProceduralMeshComponent* Component, int32 Section, UMaterialInterface* Material)
	{
		if (Component && Material) { Component->SetMaterial(Section, Material); }
	};
	const TSoftObjectPtr<UMaterialInterface> NoFallback;
	Set(Structure, 0, Pick(EarthMaterial, EarthFallbackMaterial));
	Set(Structure, 1, Pick(FloorMaterial, FloorFallbackMaterial));
	Set(Structure, 2, Pick(DarkFloorMaterial, DarkFloorFallbackMaterial));
	Set(GlassFloor, 0, Pick(GlassMaterial, NoFallback));
	Set(Pit, 0, Pick(StrataMaterial, StrataFallbackMaterial));
	UMaterialInterface* Plaster = Pick(CeilingMaterial, CeilingFallbackMaterial);
	Set(Ceiling, 0, Plaster);
	Set(Ceiling, 1, Plaster);
	Set(Bronze, 0, Pick(BronzeMaterial, BronzeFallbackMaterial));

	UMaterialInterface* RingBase = RingMaterial.LoadSynchronous();
	UMaterialInterface* Ring = RingBase;
	if (bInstances && RingBase)
	{
		// Play only (never saved with the map): M_Daylit held at its full glow, a white image, the
		// lamps' colour at RingNits.
		RingGlow = UMaterialInstanceDynamic::Create(RingBase, this);
		RingGlow->SetFlags(RF_Transient);
		if (UTexture* White = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture")))
		{
			RingGlow->SetTextureParameterValue(TEXT("Image"), White);
		}
		const FLinearColor Warm = FLinearColor::MakeFromColorTemperature(LightKelvin);
		const float Luma = FMath::Max(Warm.GetLuminance(), 1e-3f);
		RingGlow->SetVectorParameterValue(TEXT("Tint"), FLinearColor(Warm.R / Luma, Warm.G / Luma, Warm.B / Luma, 1.f));
		RingGlow->SetScalarParameterValue(TEXT("Luminance"), RingNits);
		RingGlow->SetScalarParameterValue(TEXT("NightFloor"), 1.f);
		Ring = RingGlow;
	}
	Set(LightRing, 0, Ring);
}

void ASquareStructure::PlaceLights()
{
	auto Place = [](USceneComponent* Light, const FVector& From, const FVector& Dir)
	{
		if (Light) { Light->SetRelativeLocationAndRotation(SB::At(From.X, From.Y, From.Z) * MuseePlan::Cm, FRotationMatrix::MakeFromX(Dir).Rotator()); }
	};
	auto Spot = [this, &Place](USpotLightComponent* L, const FVector& From, const FVector& Dir, float Candela, float Inner, float Outer,
							   float RangeCm, float SourceCm)
	{
		if (!L) { return; }
		Place(L, From, Dir);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(Candela);
		L->SetUseTemperature(true);
		L->SetTemperature(LightKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetInnerConeAngle(Inner);
		L->SetOuterConeAngle(Outer);
		L->SetAttenuationRadius(RangeCm);
		L->SetSourceRadius(SourceCm);
		L->SetSoftSourceRadius(0.f);
		L->SetCastShadows(true);
	};
	const FVector Down(0, 0, -1);

	// Down the shaft onto 5: about 400 lux there, 100 on the pit's floor.
	Spot(ShaftLight, FVector(0, 0, SB::ShaftZ), Down, ShaftCandela, 20.f, 32.f, 3000.f, 50.f);

	const TArray<SB::FAim> Aims = SB::WasherAims();
	for (int32 I = 0; I < WallWashers.Num() && I < Aims.Num(); ++I)
	{
		Spot(WallWashers[I], Aims[I].From, Aims[I].Dir, WasherCandela, 15.f, 45.f, 1800.f, 5.f);
	}
	const TArray<FVector2D> Galleries = SB::GalleryCentres();
	for (int32 I = 0; I < GalleryDownlights.Num() && I < Galleries.Num(); ++I)
	{
		Spot(GalleryDownlights[I], FVector(Galleries[I].X, Galleries[I].Y, SB::DownZ), Down, DownlightCandela, 25.f, 55.f, 1500.f, 10.f);
	}
	const TArray<FVector2D> DarkRooms = SB::DarkRoomCentres();
	for (int32 I = 0; I < DarkRoomLights.Num() && I < DarkRooms.Num(); ++I)
	{
		Spot(DarkRoomLights[I], FVector(DarkRooms[I].X, DarkRooms[I].Y, SB::DownZ), Down, DarkRoomCandela, 35.f, 80.f, 1400.f, 20.f);
	}

	const double PitZ[2] = {SB::PitUpperZ, SB::PitLowerZ};
	const float PitCandela[2] = {PitUpperCandela, PitLowerCandela};
	for (int32 I = 0; I < PitLights.Num() && I < 2; ++I)
	{
		UPointLightComponent* L = PitLights[I];
		if (!L) { continue; }
		Place(L, FVector(0, 0, PitZ[I]), Down);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(PitCandela[I]);
		L->SetUseTemperature(true);
		L->SetTemperature(LightKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetAttenuationRadius(1000.f);
		L->SetSourceRadius(20.f);
		L->SetSoftSourceRadius(0.f);
		L->SetCastShadows(true);
	}
}
