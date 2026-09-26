#include "Albion/AlbionBuild.h"

/**
 * Albion's furniture (the Details board): two oak benches in the nave, off the axis; four oak table cases for William
 * De Morgan's lustreware in the aisles on lines 3 and 5, low enough to see the paintings over. In the museum's oak
 * (MI_Albion_Oak, the museum's oak in a Victorian finish: FurnitureKit's members carry their grain along U, each its own board), with a felt bed in the cases.
 */
namespace AlbionFurnitureImpl
{
	namespace AP = AlbionPlan;
	using namespace AlbionKit;
	using namespace AlbionBuild;
	using FurnitureKit::FPath;
	using FurnitureKit::RectSection;

	const FVector kUp(0, 0, 1);

	/** A member from A to B, its section HA × HB (half sizes) with eased edges; UpHint orients B. */
	void Member(FMeshData& M, const FVector& A, const FVector& B, double HA, double HB, const FVector& UpHint = FVector(0, 0, 1), double Ease = 0.004)
	{
		FurnitureKit::FBarEnds Ends;
		Ends.E0 = Ends.E1 = FMath::Min(Ease, FMath::Min(HA, HB) * 0.5);
		FurnitureKit::Bar(M, FPath::Line(A, B, UpHint), RectSection(0.0, 0.0, HA, HB, Ease), Ends);
	}

	/** A gallery bench: a thick oak top on two shaped ends, a stretcher tenoned through them, long along y. */
	void Bench(FParts& P, double X, double Y)
	{
		FMeshData& O = P[SlotOak];
		const double L = AP::BenchLength, D = AP::BenchDepth, H = AP::BenchHeight;
		const double T = 0.065;
		// The top: two boards glued along the middle, a rounded arris.
		for (int32 s = -1; s <= 1; s += 2)
		{
			const double XC = X + s * D * 0.25;
			Member(O, FVector(XC, Y - L * 0.5, H - T * 0.5), FVector(XC, Y + L * 0.5, H - T * 0.5), D * 0.25, T * 0.5, kUp, 0.012);
		}
		// The ends: a board on edge 0.30 m in from each end, a bridge cut under it, a foot each side.
		for (int32 e = -1; e <= 1; e += 2)
		{
			const double YE = Y + e * (L * 0.5 - 0.30);
			Member(O, FVector(X, YE, 0.10), FVector(X, YE, H - T), 0.19, 0.035, FVector(0, 1, 0), 0.006);
			for (int32 s = -1; s <= 1; s += 2)
			{
				Member(O, FVector(X + s * 0.15, YE, 0.0), FVector(X + s * 0.15, YE, 0.10), 0.07, 0.04, FVector(0, 1, 0), 0.006);
			}
			// A cleat under the top along the end.
			Member(O, FVector(X - 0.21, YE, H - T - 0.03), FVector(X + 0.21, YE, H - T - 0.03), 0.03, 0.05, kUp, 0.004);
		}
		// The stretcher, its tenons wedged through the ends.
		Member(O, FVector(X, Y - L * 0.5 + 0.22, 0.20), FVector(X, Y + L * 0.5 - 0.22, 0.20), 0.03, 0.055, kUp, 0.005);
	}

	/**
	 * A table case: four legs with a stretcher frame, an apron carrying the case's deck at 0.66 m, a glazed box to 0.95 m
	 * with oak corner posts and top rails; a felt bed inside. Long along y.
	 */
	void Case(FParts& P, double X, double Y)
	{
		FMeshData& O = P[SlotOak];
		FMeshData& G = P[SlotCaseGlass];
		const double HX = AP::CaseDepth * 0.5, HY = AP::CaseLength * 0.5, Top = AP::CaseTop;
		const double Deck = 0.52, Apron = 0.14, Leg = 0.035;   // (the deck low: 0.4 m under the glass for the De Morgan jars)
		for (const FVector2D& C : {FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1)})
		{
			const FVector2D At(X + C.X * (HX - 0.05), Y + C.Y * (HY - 0.05));
			// Turned legs: a square block under the apron, a taper to a small foot.
			Member(O, FVector(At.X, At.Y, Deck - Apron - 0.02), FVector(At.X, At.Y, Deck + 0.02), Leg, Leg, FVector(1, 0, 0), 0.003);
			TArray<FLathePoint> Turn = {{0.0, 0.0, false}, {0.03, 0.0, false}, {0.032, 0.03, true}, {0.024, 0.06, true}, {0.022, 0.30, true},
										{0.03, 0.40, true}, {0.028, 0.44, true}, {0.033, Deck - Apron - 0.02, false}, {0.0, Deck - Apron - 0.02, false}};
			Lathe(O, FVector(At.X, At.Y, 0.0), Turn, 20);
		}
		// Stretchers low down.
		Member(O, FVector(X - HX + 0.05, Y - HY + 0.05, 0.12), FVector(X - HX + 0.05, Y + HY - 0.05, 0.12), 0.018, 0.025, kUp);
		Member(O, FVector(X + HX - 0.05, Y - HY + 0.05, 0.12), FVector(X + HX - 0.05, Y + HY - 0.05, 0.12), 0.018, 0.025, kUp);
		Member(O, FVector(X - HX + 0.05, Y, 0.12), FVector(X + HX - 0.05, Y, 0.12), 0.018, 0.025, kUp);
		// The apron and the deck's moulded edge.
		Member(O, FVector(X - HX, Y - HY + 0.02, Deck - Apron * 0.5), FVector(X - HX, Y + HY - 0.02, Deck - Apron * 0.5), 0.012, Apron * 0.5, kUp);
		Member(O, FVector(X + HX, Y - HY + 0.02, Deck - Apron * 0.5), FVector(X + HX, Y + HY - 0.02, Deck - Apron * 0.5), 0.012, Apron * 0.5, kUp);
		Member(O, FVector(X - HX + 0.02, Y - HY, Deck - Apron * 0.5), FVector(X + HX - 0.02, Y - HY, Deck - Apron * 0.5), 0.012, Apron * 0.5, kUp);
		Member(O, FVector(X - HX + 0.02, Y + HY, Deck - Apron * 0.5), FVector(X + HX - 0.02, Y + HY, Deck - Apron * 0.5), 0.012, Apron * 0.5, kUp);
		O.Box(FVector(X - HX - 0.015, Y - HY - 0.015, Deck - 0.02), FVector(X + HX + 0.015, Y + HY + 0.015, Deck + 0.015), FMeshData::AllFaces);
		// The felt bed.
		P[SlotFelt].Box(FVector(X - HX + 0.03, Y - HY + 0.03, Deck + 0.015), FVector(X + HX - 0.03, Y + HY - 0.03, Deck + 0.03), FMeshData::PosZ);
		// Corner posts and top rails of the glazed box.
		const double GZ0 = Deck + 0.015, GZ1 = Top;
		for (const FVector2D& C : {FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1)})
		{
			Member(O, FVector(X + C.X * (HX - 0.012), Y + C.Y * (HY - 0.012), GZ0), FVector(X + C.X * (HX - 0.012), Y + C.Y * (HY - 0.012), GZ1), 0.014, 0.014, FVector(1, 0, 0), 0.002);
		}
		for (int32 s = -1; s <= 1; s += 2)
		{
			Member(O, FVector(X + s * (HX - 0.012), Y - HY + 0.02, GZ1 - 0.012), FVector(X + s * (HX - 0.012), Y + HY - 0.02, GZ1 - 0.012), 0.014, 0.012, kUp, 0.002);
			Member(O, FVector(X - HX + 0.02, Y + s * (HY - 0.012), GZ1 - 0.012), FVector(X + HX - 0.02, Y + s * (HY - 0.012), GZ1 - 0.012), 0.014, 0.012, kUp, 0.002);
		}
		// The glass: four sides and the top.
		const double GX = HX - 0.012, GY = HY - 0.012;
		G.Rect(FVector(X - GX, Y - GY, GZ0), FVector(X + GX, Y - GY, GZ0), FVector(X + GX, Y - GY, GZ1), FVector(X - GX, Y - GY, GZ1), FVector(0, -1, 0));
		G.Rect(FVector(X - GX, Y + GY, GZ0), FVector(X + GX, Y + GY, GZ0), FVector(X + GX, Y + GY, GZ1), FVector(X - GX, Y + GY, GZ1), FVector(0, 1, 0));
		G.Rect(FVector(X - GX, Y - GY, GZ0), FVector(X - GX, Y + GY, GZ0), FVector(X - GX, Y + GY, GZ1), FVector(X - GX, Y - GY, GZ1), FVector(-1, 0, 0));
		G.Rect(FVector(X + GX, Y - GY, GZ0), FVector(X + GX, Y + GY, GZ0), FVector(X + GX, Y + GY, GZ1), FVector(X + GX, Y - GY, GZ1), FVector(1, 0, 0));
		G.Rect(FVector(X - GX, Y - GY, GZ1), FVector(X + GX, Y - GY, GZ1), FVector(X + GX, Y + GY, GZ1), FVector(X - GX, Y + GY, GZ1), kUp);
		// A plain collision box round the glass (the visitor can't reach in): the glass itself has none.
		O.Box(FVector(X - GX + 0.002, Y - GY + 0.002, GZ1 - 0.004), FVector(X + GX - 0.002, Y + GY - 0.002, GZ1 - 0.002), FMeshData::NegZ);
		// The guard (never drawn): a sheer box round the whole case to 1.5 m, so the deck's moulded edge is no ledge to perch
		// on and no jump lands on the glass (a jump from the edge once carried the visitor over the top).
		P[SlotGuard].Box(FVector(X - HX - 0.03, Y - HY - 0.03, 0.0), FVector(X + HX + 0.03, Y + HY + 0.03, 1.5), FMeshData::AllFaces);
	}
}

namespace AlbionBuild
{
	using namespace AlbionFurnitureImpl;

	void BuildFurniture(FParts& P)
	{
		for (int32 s = -1; s <= 1; s += 2)
		{
			Bench(P, s * AlbionPlan::BenchX, AlbionPlan::BenchY);
			for (const double Y : AlbionPlan::CaseY) { Case(P, s * AlbionPlan::CaseX, Y); }
		}
	}
}
