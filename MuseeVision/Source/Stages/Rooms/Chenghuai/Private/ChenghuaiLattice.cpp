#include "ChenghuaiLattice.h"

#include "ChenghuaiParts.h"
#include "Math/RandomStream.h"

namespace Musee::Chenghuai
{
	namespace
	{
		/** Two coordinates nearer than a micrometre are one. */
		constexpr double Same = 1.0e-6;

		/** A bar of an axis-square pattern: a rectangle in the panel's (a, b). */
		struct FBar
		{
			double A0, A1, B0, B1;
		};

		/** Drops the points that lie on a straight line between their neighbours. */
		TArray<FVector2D> Straightened(const TArray<FVector2D>& Loop)
		{
			TArray<FVector2D> Out;
			const int32 N = Loop.Num();
			for (int32 Index = 0; Index < N; ++Index)
			{
				const FVector2D& Prev = Loop[(Index + N - 1) % N];
				const FVector2D& Here = Loop[Index];
				const FVector2D& Next = Loop[(Index + 1) % N];
				if (FMath::Abs(FVector2D::CrossProduct(Here - Prev, Next - Here)) > Same * Same)
				{
					Out.Add(Here);
				}
			}
			return Out;
		}

		/** The bars of the grid, lantern (灯笼框) and step-brocade (步步锦) patterns inside a window, half-width W. */
		TArray<FBar> SquareBars(ELattice Pattern, double A0, double A1, double B0, double B1)
		{
			const double W = 0.5 * Plan::LatticeBar;
			TArray<FBar> Bars;
			auto Upright = [&](double A, double From, double To) { Bars.Add({A - W, A + W, From, To}); };
			auto Level = [&](double B, double From, double To) { Bars.Add({From, To, B - W, B + W}); };
			if (Pattern == ELattice::Grid)
			{
				const int32 NU = FMath::Max(1, FMath::RoundToInt32((A1 - A0) / Plan::GridCellWide));
				const int32 NZ = FMath::Max(1, FMath::RoundToInt32((B1 - B0) / Plan::GridCellHigh));
				for (int32 I = 1; I < NU; ++I) { Upright(FMath::Lerp(A0, A1, double(I) / NU), B0, B1); }
				for (int32 J = 1; J < NZ; ++J) { Level(FMath::Lerp(B0, B1, double(J) / NZ), A0, A1); }
				return Bars;
			}
			// One ring of cells round an open centre (the lantern), or two with their ties alternating (the step brocade).
			const int32 Rings = Pattern == ELattice::Lantern ? 1 : 2;
			const int32 Stride = Rings > 1 ? 2 : 1;
			for (int32 Ring = 0; Ring < Rings; ++Ring)
			{
				const double M = FMath::Min(Plan::LanternMargin, Plan::LanternMarginShare * FMath::Min(A1 - A0, B1 - B0));
				const double N0 = A0 + M;
				const double N1 = A1 - M;
				const double O0 = B0 + M;
				const double O1 = B1 - M;
				if (N1 - N0 < 2.0 * Plan::LatticeRim || O1 - O0 < 2.0 * Plan::LatticeRim)
				{
					break;
				}
				Level(O0, N0 - W, N1 + W);
				Level(O1, N0 - W, N1 + W);
				Upright(N0, O0, O1);
				Upright(N1, O0, O1);
				const int32 KU = FMath::Max(2, FMath::RoundToInt32((N1 - N0) / Plan::LanternTie));
				for (int32 K = Ring % 2; K <= KU; K += Stride)
				{
					const double A = FMath::Lerp(N0, N1, double(K) / KU);
					Upright(A, B0, O0);
					Upright(A, O1, B1);
				}
				const int32 KZ = FMath::Max(2, FMath::RoundToInt32((O1 - O0) / Plan::LanternTie));
				for (int32 K = Ring % 2; K <= KZ; K += Stride)
				{
					const double B = FMath::Lerp(O0, O1, double(K) / KZ);
					Level(B, A0, N0);
					Level(B, N1, A1);
				}
				A0 = N0 + W;
				A1 = N1 - W;
				B0 = O0 + W;
				B1 = O1 - W;
			}
			if (Pattern == ELattice::Lantern)
			{
				if (A1 - A0 > Plan::LanternCross) { Upright(0.5 * (A0 + A1), B0, B1); }
				if (B1 - B0 > Plan::LanternCross) { Level(0.5 * (B0 + B1), A0, A1); }
			}
			return Bars;
		}

		/** A convex cell of an ice-crack lattice: its corners, and whether each edge (corner i to i + 1) is a bar. */
		struct FCell
		{
			TArray<FVector2D> Corners;
			TArray<bool> bBar;
		};

		/** Splits a convex cell along the line through Point in Direction; false when the line misses it. */
		bool Split(const FCell& Cell, const FVector2D& Point, const FVector2D& Direction, FCell& OutLeft, FCell& OutRight, FVector2D& OutA,
			FVector2D& OutB)
		{
			const int32 N = Cell.Corners.Num();
			TArray<FVector2D> Cut;
			OutLeft = FCell();
			OutRight = FCell();
			for (int32 Index = 0; Index < N; ++Index)
			{
				const FVector2D& P = Cell.Corners[Index];
				const FVector2D& Q = Cell.Corners[(Index + 1) % N];
				const double SP = FVector2D::CrossProduct(Direction, P - Point);
				const double SQ = FVector2D::CrossProduct(Direction, Q - Point);
				FCell& Side = SP >= 0.0 ? OutLeft : OutRight;
				Side.Corners.Add(P);
				Side.bBar.Add(Cell.bBar[Index]);
				if ((SP >= 0.0) != (SQ >= 0.0))
				{
					const FVector2D Cross = P + (Q - P) * (SP / (SP - SQ));
					Cut.Add(Cross);
					// The edge is split at the crossing: this side ends there along the line; the other starts there.
					Side.Corners.Add(Cross);
					Side.bBar.Add(true);
					FCell& Other = SP >= 0.0 ? OutRight : OutLeft;
					Other.Corners.Add(Cross);
					Other.bBar.Add(Cell.bBar[Index]);
				}
			}
			if (Cut.Num() != 2 || OutLeft.Corners.Num() < 3 || OutRight.Corners.Num() < 3)
			{
				return false;
			}
			OutA = Cut[0];
			OutB = Cut[1];
			return true;
		}

		/** Keeps the part of a convex loop on the left of the line through A along D (for a loop of the given sense). */
		TArray<FVector2D> KeepLeft(const TArray<FVector2D>& Loop, const FVector2D& A, const FVector2D& D, double Sense)
		{
			TArray<FVector2D> Out;
			for (int32 Index = 0; Index < Loop.Num(); ++Index)
			{
				const FVector2D& P = Loop[Index];
				const FVector2D& Q = Loop[(Index + 1) % Loop.Num()];
				const double SP = Sense * FVector2D::CrossProduct(D, P - A);
				const double SQ = Sense * FVector2D::CrossProduct(D, Q - A);
				if (SP >= 0.0)
				{
					Out.Add(P);
				}
				if ((SP >= 0.0) != (SQ >= 0.0))
				{
					Out.Add(P + (Q - P) * (SP / (SP - SQ)));
				}
			}
			return Out;
		}

		/**
		 * A convex cell with each bar edge moved in by half a bar: the opening between the bars, as the cell cut by each moved
		 * edge's half-plane, so a short edge that the others overrun simply vanishes.
		 */
		TArray<FVector2D> Inset(const FCell& Cell, double HalfBar)
		{
			const int32 N = Cell.Corners.Num();
			const double Sense = SignedArea(Cell.Corners) >= 0.0 ? 1.0 : -1.0;
			TArray<FVector2D> Out = Cell.Corners;
			for (int32 Edge = 0; Edge < N && Out.Num() >= 3; ++Edge)
			{
				if (!Cell.bBar[Edge])
				{
					continue;
				}
				const FVector2D P = Cell.Corners[Edge];
				const FVector2D D = (Cell.Corners[(Edge + 1) % N] - P).GetSafeNormal();
				Out = KeepLeft(Out, P + Sense * FVector2D(-D.Y, D.X) * HalfBar, D, Sense);
			}
			// A cell too small for its bars is all bar.
			if (Out.Num() < 3 || SignedArea(Out) * Sense < Plan::IceCellLeast * 0.1)
			{
				return {};
			}
			return Straightened(Out);
		}

		/** Keeps the part of a convex loop inside a convex window (Sutherland-Hodgman). */
		TArray<FVector2D> ClipConvex(const TArray<FVector2D>& Subject, TConstArrayView<FVector2D> Window)
		{
			TArray<FVector2D> Out = Subject;
			const double Sense = SignedArea(Window) >= 0.0 ? 1.0 : -1.0;
			for (int32 Edge = 0; Edge < Window.Num() && Out.Num() >= 3; ++Edge)
			{
				const FVector2D A = Window[Edge];
				const FVector2D B = Window[(Edge + 1) % Window.Num()];
				auto Inside = [&](const FVector2D& P) { return Sense * FVector2D::CrossProduct(B - A, P - A) >= 0.0; };
				TArray<FVector2D> In = MoveTemp(Out);
				Out.Reset();
				for (int32 Index = 0; Index < In.Num(); ++Index)
				{
					const FVector2D& P = In[Index];
					const FVector2D& Q = In[(Index + 1) % In.Num()];
					if (Inside(P)) { Out.Add(P); }
					if (Inside(P) != Inside(Q))
					{
						const double SP = FVector2D::CrossProduct(B - A, P - A);
						const double SQ = FVector2D::CrossProduct(B - A, Q - A);
						Out.Add(P + (Q - P) * (SP / (SP - SQ)));
					}
				}
			}
			return Out.Num() >= 3 && FMath::Abs(SignedArea(Out)) > Plan::IceCellLeast * 0.1 ? Out : TArray<FVector2D>();
		}

		/** The window moved in by a distance (a convex loop, each edge moved along its inward normal). */
		TArray<FVector2D> Shrunk(TConstArrayView<FVector2D> Window, double By)
		{
			FCell Cell;
			Cell.Corners = TArray<FVector2D>(Window.GetData(), Window.Num());
			Cell.bBar.Init(true, Window.Num());
			return Inset(Cell, By);
		}

		/** 冰裂纹 on a front's panel: the largest cell split again and again by a straight cut through a point near its middle. */
		FLoops IceCrack(double A0, double A1, double B0, double B1, int32 Seed)
		{
			FRandomStream Random(Seed);
			TArray<FCell> Cells;
			Cells.Add({Rectangle(A0, B0, A1, B1), {false, false, false, false}});
			const int32 Splits = FMath::Clamp(FMath::RoundToInt32((A1 - A0) * (B1 - B0) / Plan::IceCellArea), 1, 160);
			for (int32 Made = 0, Tries = 0; Made < Splits && Tries < 4 * Splits; ++Tries)
			{
				int32 Largest = 0;
				for (int32 Index = 1; Index < Cells.Num(); ++Index)
				{
					Largest = FMath::Abs(SignedArea(Cells[Index].Corners)) > FMath::Abs(SignedArea(Cells[Largest].Corners)) ? Index : Largest;
				}
				const FCell Cell = Cells[Largest];
				FVector2D Middle = FVector2D::ZeroVector;
				for (const FVector2D& Corner : Cell.Corners)
				{
					Middle += Corner / Cell.Corners.Num();
				}
				const double Size = FMath::Sqrt(FMath::Abs(SignedArea(Cell.Corners)));
				const FVector2D Point = Middle + 0.35 * Size * FVector2D(Random.FRand() - 0.5, Random.FRand() - 0.5);
				const double Angle = Random.FRand() * UE_DOUBLE_PI;
				FCell Left;
				FCell Right;
				FVector2D CutA;
				FVector2D CutB;
				if (!Split(Cell, Point, FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)), Left, Right, CutA, CutB)
					|| FMath::Abs(SignedArea(Left.Corners)) < Plan::IceCellLeast || FMath::Abs(SignedArea(Right.Corners)) < Plan::IceCellLeast
					|| FVector2D::Distance(CutA, CutB) < Plan::IceCutLeast)
				{
					continue;
				}
				Cells[Largest] = MoveTemp(Left);
				Cells.Add(MoveTemp(Right));
				++Made;
			}
			FLoops Loops;
			for (const FCell& Cell : Cells)
			{
				TArray<FVector2D> Opening = Inset(Cell, 0.5 * Plan::LatticeBar);
				if (!Opening.IsEmpty())
				{
					Loops.Add(MoveTemp(Opening));
				}
			}
			return Loops;
		}

		/** The signed distance from a point to a convex loop's boundary: positive inside. */
		double InsideBy(TConstArrayView<FVector2D> Loop, const FVector2D& P)
		{
			const double Sense = SignedArea(Loop) >= 0.0 ? 1.0 : -1.0;
			double Least = TNumericLimits<double>::Max();
			bool bInside = true;
			for (int32 Index = 0; Index < Loop.Num(); ++Index)
			{
				const FVector2D A = Loop[Index];
				const FVector2D B = Loop[(Index + 1) % Loop.Num()];
				bInside &= Sense * FVector2D::CrossProduct(B - A, P - A) >= 0.0;
				const FVector2D AB = B - A;
				const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / AB.SizeSquared(), 0.0, 1.0);
				Least = FMath::Min(Least, FVector2D::Distance(A + AB * T, P));
			}
			return bInside ? Least : -Least;
		}

		/**
		 * The loops round the region where a field is positive, traced on a square grid over a box by marching squares (each
		 * crossing placed along its cell's edge by linear interpolation, a saddle decided by its cell's middle), then
		 * thinned to within a tenth of the grid.
		 */
		FLoops Traced(TFunctionRef<double(const FVector2D&)> Field, const FVector2D& Min, const FVector2D& Max, double Grid)
		{
			const int32 NX = FMath::CeilToInt32((Max.X - Min.X) / Grid);
			const int32 NY = FMath::CeilToInt32((Max.Y - Min.Y) / Grid);
			auto At = [&](int32 I, int32 J) { return Min + FVector2D(I, J) * Grid; };
			TArray<double> Value;
			Value.SetNumUninitialized((NX + 1) * (NY + 1));
			for (int32 J = 0; J <= NY; ++J)
			{
				for (int32 I = 0; I <= NX; ++I)
				{
					Value[J * (NX + 1) + I] = Field(At(I, J));
				}
			}
			auto V = [&](int32 I, int32 J) { return Value[J * (NX + 1) + I]; };
			// An edge's crossing, keyed by the edge: level edges even, upright ones odd.
			TMap<int32, FVector2D> Crossings;
			auto Crossing = [&](int32 I, int32 J, bool bUpright)
			{
				const int32 Key = 2 * (J * (NX + 1) + I) + (bUpright ? 1 : 0);
				if (!Crossings.Contains(Key))
				{
					const double A = V(I, J);
					const double B = bUpright ? V(I, J + 1) : V(I + 1, J);
					const double T = A / (A - B);
					Crossings.Add(Key, At(I, J) + (bUpright ? FVector2D(0.0, T) : FVector2D(T, 0.0)) * Grid);
				}
				return Key;
			};
			TMultiMap<int32, int32> Links;
			for (int32 J = 0; J < NY; ++J)
			{
				for (int32 I = 0; I < NX; ++I)
				{
					const bool In[4] = {V(I, J) > 0.0, V(I + 1, J) > 0.0, V(I + 1, J + 1) > 0.0, V(I, J + 1) > 0.0};
					// The cell's four edges: bottom, right, top, left; a crossing on each whose corners differ.
					TArray<int32> Keys;
					if (In[0] != In[1]) { Keys.Add(Crossing(I, J, false)); }
					if (In[1] != In[2]) { Keys.Add(Crossing(I + 1, J, true)); }
					if (In[2] != In[3]) { Keys.Add(Crossing(I, J + 1, false)); }
					if (In[3] != In[0]) { Keys.Add(Crossing(I, J, true)); }
					if (Keys.Num() == 2)
					{
						Links.Add(Keys[0], Keys[1]);
						Links.Add(Keys[1], Keys[0]);
					}
					else if (Keys.Num() == 4)
					{
						// A saddle: join the crossings round the corners the middle agrees with.
						const bool bMiddle = 0.25 * (V(I, J) + V(I + 1, J) + V(I + 1, J + 1) + V(I, J + 1)) > 0.0;
						const bool bJoinFirst = bMiddle == In[0];
						const int32 Pairs[2][4] = {{0, 3, 1, 2}, {0, 1, 2, 3}};
						const int32* P = Pairs[bJoinFirst ? 1 : 0];
						Links.Add(Keys[P[0]], Keys[P[1]]);
						Links.Add(Keys[P[1]], Keys[P[0]]);
						Links.Add(Keys[P[2]], Keys[P[3]]);
						Links.Add(Keys[P[3]], Keys[P[2]]);
					}
				}
			}
			TSet<int32> Seen;
			FLoops Loops;
			for (const TPair<int32, FVector2D>& Start : Crossings)
			{
				if (Seen.Contains(Start.Key))
				{
					continue;
				}
				TArray<FVector2D> Loop;
				int32 Here = Start.Key;
				int32 Prev = INDEX_NONE;
				while (Here != INDEX_NONE && !Seen.Contains(Here))
				{
					Seen.Add(Here);
					Loop.Add(Crossings[Here]);
					TArray<int32> Next;
					Links.MultiFind(Here, Next);
					const int32 Came = Prev;
					Prev = Here;
					Here = INDEX_NONE;
					for (const int32 Candidate : Next)
					{
						Here = Candidate != Came && !Seen.Contains(Candidate) ? Candidate : Here;
					}
				}
				// Thin the loop: keep a point only where the line through its neighbours strays more than a tenth of the grid.
				TArray<FVector2D> Thin;
				for (int32 Index = 0; Index < Loop.Num(); ++Index)
				{
					const FVector2D& A = Thin.IsEmpty() ? Loop.Last() : Thin.Last();
					const FVector2D& B = Loop[(Index + 1) % Loop.Num()];
					const FVector2D AB = (B - A).GetSafeNormal();
					if (FMath::Abs(FVector2D::CrossProduct(AB, Loop[Index] - A)) > 0.1 * Grid)
					{
						Thin.Add(Loop[Index]);
					}
				}
				if (Thin.Num() >= 3 && FMath::Abs(SignedArea(Thin)) > Grid * Grid)
				{
					Loops.Add(MoveTemp(Thin));
				}
			}
			return Loops;
		}

		/** The distance from a point to an octagon's boundary (a coin's ring of bars, turned 22.5°). */
		double OffRing(const FVector2D& P, const FVector2D& Centre, double Radius)
		{
			const TArray<FVector2D> Ring = RegularPolygon(Centre, Radius, 8, 22.5);
			return FMath::Abs(InsideBy(Ring, P));
		}
	}

	FLoops PanelOpenings(ELattice Pattern, double Width, double Height, int32 Seed)
	{
		const double Rim = Plan::LatticeRim;
		if (Width <= 2.0 * Rim || Height <= 2.0 * Rim)
		{
			return {};
		}
		if (Pattern == ELattice::IceCrack)
		{
			return IceCrack(Rim, Width - Rim, Rim, Height - Rim, Seed);
		}
		TArray<FBox2D> Bars;
		for (const FBar& Bar : SquareBars(Pattern, Rim, Width - Rim, Rim, Height - Rim))
		{
			Bars.Add(FBox2D(FVector2D(Bar.A0, Bar.B0), FVector2D(Bar.A1, Bar.B1)));
		}
		return OpenRegions(Rim, Width - Rim, Rim, Height - Rim, Bars);
	}

	TArray<FVector2D> LeakShape(Plan::ELeak Leak, const FVector2D& Centre, double Radius)
	{
		switch (Leak)
		{
		case Plan::ELeak::Hexagons: return RegularPolygon(Centre, Radius, 6, 0.0);
		case Plan::ELeak::Octagon: return RegularPolygon(Centre, Radius, 8, 22.5);
		default: return RegularPolygon(Centre, Radius, Plan::LeakCircleSides, 0.0);
		}
	}

	FLoops LeakOpenings(Plan::ELeak Leak, TConstArrayView<FVector2D> Shape, const FVector2D& Centre, double Radius, int32 Seed)
	{
		const TArray<FVector2D> Inner = Shrunk(Shape, Plan::LeakRim);
		const double HalfBar = 0.5 * Plan::LeakBar;
		FLoops Loops;
		if (Inner.IsEmpty())
		{
			return Loops;
		}
		if (Leak == Plan::ELeak::Hexagons)
		{
			// A honeycomb of hexagons of side 0.11 (pointed up), each opening its hexagon less half a bar, clipped to the rim.
			const double Side = Plan::HexagonSide;
			const double Across = Side * FMath::Sqrt(3.0);
			const int32 Rows = FMath::CeilToInt32(Radius / (1.5 * Side)) + 1;
			const int32 Columns = FMath::CeilToInt32(Radius / Across) + 1;
			for (int32 Row = -Rows; Row <= Rows; ++Row)
			{
				for (int32 Column = -Columns; Column <= Columns; ++Column)
				{
					const FVector2D Cell = Centre + FVector2D((Column + 0.5 * (Row & 1)) * Across, 1.5 * Side * Row);
					const double Apothem = 0.5 * Across - HalfBar;
					TArray<FVector2D> Opening = ClipConvex(RegularPolygon(Cell, Apothem / FMath::Cos(UE_DOUBLE_PI / 6.0), 6, 30.0), Inner);
					if (!Opening.IsEmpty())
					{
						Loops.Add(MoveTemp(Opening));
					}
				}
			}
			return Loops;
		}
		if (Leak == Plan::ELeak::Coins)
		{
			// Interlocking rings (钱纹) on a grid: open wherever no ring's bar is, inside the rim.
			const double Pitch = Plan::CoinPitch;
			const int32 Reach = FMath::CeilToInt32(Radius / Pitch) + 1;
			TArray<FVector2D> Rings;
			for (int32 J = -Reach; J <= Reach; ++J)
			{
				for (int32 I = -Reach; I <= Reach; ++I)
				{
					Rings.Add(Centre + FVector2D(I, J) * Pitch);
				}
			}
			auto Field = [&](const FVector2D& P)
			{
				double Open = InsideBy(Inner, P);
				for (const FVector2D& Ring : Rings)
				{
					if (FVector2D::DistSquared(P, Ring) < FMath::Square(Plan::CoinRadius + Pitch))
					{
						Open = FMath::Min(Open, OffRing(P, Ring, Plan::CoinRadius) - HalfBar);
					}
				}
				return Open;
			};
			const FVector2D Pad(2.0 * Plan::LeakTraceGrid, 2.0 * Plan::LeakTraceGrid);
			return Traced(Field, Centre - FVector2D(Radius, Radius) - Pad, Centre + FVector2D(Radius, Radius) + Pad, Plan::LeakTraceGrid);
		}
		// Ice-crack (冰裂) and the plain octagon: straight cracks right across, each splitting every cell it crosses.
		FRandomStream Random(Seed);
		TArray<FCell> Cells;
		FCell Whole;
		Whole.Corners = Inner;
		Whole.bBar.Init(false, Inner.Num());
		Cells.Add(MoveTemp(Whole));
		for (int32 Crack = 0; Crack < Plan::LeakCracks; ++Crack)
		{
			const double Angle = Random.FRand() * UE_DOUBLE_PI;
			const FVector2D Point = Centre + Radius * FVector2D(Random.FRand() - 0.5, Random.FRand() - 0.5);
			const FVector2D Direction(FMath::Cos(Angle), FMath::Sin(Angle));
			TArray<FCell> Next;
			for (const FCell& Cell : Cells)
			{
				FCell Left;
				FCell Right;
				FVector2D A;
				FVector2D B;
				if (Split(Cell, Point, Direction, Left, Right, A, B))
				{
					Next.Add(MoveTemp(Left));
					Next.Add(MoveTemp(Right));
				}
				else
				{
					Next.Add(Cell);
				}
			}
			Cells = MoveTemp(Next);
		}
		for (const FCell& Cell : Cells)
		{
			TArray<FVector2D> Opening = Inset(Cell, HalfBar);
			if (!Opening.IsEmpty())
			{
				Loops.Add(MoveTemp(Opening));
			}
		}
		return Loops;
	}

	void DrawPanel(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* Surface, const FBuildingFrame& F, double U, double V, double Z,
		double Width, double Height, ELattice Pattern, bool bPaper)
	{
		FLoops Holes = PanelOpenings(Pattern, Width, Height, Musee::Seed(Key(Name)));
		for (TArray<FVector2D>& Hole : Holes)
		{
			for (FVector2D& Point : Hole)
			{
				Point += FVector2D(U, Z);
			}
		}
		AlongV(Out, Name, Surface, F, Rectangle(U, Z, U + Width, Z + Height), Holes, V, V + Plan::LatticeDepth, EHit::Solid);
		if (bPaper)
		{
			const double Margin = 0.5 * Plan::LatticeRim;
			FFormDrawing& Paper = AlongV(Out, Name + TEXT(".paper"), TEXT("lattice.paper"), F,
				Rectangle(U + Margin, Z + Margin, U + Width - Margin, Z + Height - Margin), {}, V + Plan::LatticeDepth,
				V + Plan::LatticeDepth + Plan::Paper, EHit::Solid);
			Paper.bNanite = false;
		}
	}

	void DrawLeaf(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* Surface, const FBuildingFrame& F, double U, double V, double Z,
		double Width, double Height, ELattice Pattern)
	{
		const double Stile = Plan::LeafStile;
		const double Rail = Plan::LeafRail;
		const double Apron = Z + Plan::LeafApron * Height;
		const double Waist = Z + Plan::LeafWaist * Height;
		const double TopRail = Z + Plan::LeafTopRail * Height;
		const double Head = Z + Height;
		const double A0 = U + Stile;
		const double A1 = U + Width - Stile;
		// The openings the frame is made round, bottom to top: the apron, the waist, the lattice head, the top panel.
		const TArray<FVector2D> Openings[4] = {Rectangle(A0, Z + Rail, A1, Apron), Rectangle(A0, Apron + Rail, A1, Waist),
			Rectangle(A0, Waist + Rail, A1, TopRail), Rectangle(A0, TopRail + Rail, A1, Head - Rail)};
		AlongV(Out, Name + TEXT(".frame"), Surface, F, Rectangle(U, Z, U + Width, Head), Openings, V, V + Plan::LeafThick, EHit::Solid);
		const double Back = V + 0.5 * (Plan::LeafThick - Plan::LeafPanel);
		const double Groove = 0.5 * Plan::LeafPanelBack;
		const TCHAR* Panels[3] = {TEXT(".apron"), TEXT(".waist"), TEXT(".top")};
		const int32 PanelOf[3] = {0, 1, 3};
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const TArray<FVector2D>& O = Openings[PanelOf[Index]];
			AlongV(Out, Name + Panels[Index], Surface, F, Rectangle(O[0].X - Groove, O[0].Y - Groove, O[2].X + Groove, O[2].Y + Groove), {}, Back,
				Back + Plan::LeafPanel, EHit::Solid);
		}
		// The apron's raised field, 0.07 in from its frame, standing proud of the panel's face.
		const double Field = Plan::LeafField;
		AlongV(Out, Name + TEXT(".field"), Surface, F, Rectangle(A0 + Field, Z + Rail + Field, A1 - Field, Apron - Field), {},
			Back - Plan::LeafFieldProud, Back, EHit::Solid);
		DrawPanel(Out, Name + TEXT(".lattice"), Surface, F, A0, Back - 0.5 * Plan::LatticeDepth + 0.5 * Plan::LeafPanel, Waist + Rail, A1 - A0,
			TopRail - Waist - Rail, Pattern, true);
	}
}
