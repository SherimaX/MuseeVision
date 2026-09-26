#include "Albion/AlbionBuild.h"

/**
 * Albion's glass (AlbionBuild.h): the three vaults in flat panes on their arcs (0.6 m round the arc, each pane its own
 * plane, so the reflections break at every lap as real glazing's do); the end screens and the aisles' gables; the
 * south screen's twelve Tristram and Isolde panels (UVs into a 6 × 2 atlas, top row first, read from inside); the
 * lancets (a 6 × 1 atlas each side, read from inside, north to south on the east wall).
 */
namespace AlbionGlassImpl
{
	namespace AP = AlbionPlan;
	using namespace AlbionKit;
	using namespace AlbionBuild;

	const FVector kUp(0, 0, 1);
	constexpr double kGlassY0 = AP::Y0 - AP::Wall * 0.5, kGlassY1 = AP::Y1 + AP::Wall * 0.5;

	/** A vault half's panes, from just above the valley to the crown. */
	void VaultPanes(FMeshData& M, const FArcHalf& H)
	{
		const double T0 = H.AngleAtHeight(AP::Spring + 0.10), T1 = H.CrownAngle();
		const int32 N = FMath::Max(8, FMath::CeilToInt32(H.Length() / AP::PanePitch));
		for (int32 i = 0; i < N; ++i)
		{
			const double Ta = FMath::Lerp(T0, T1, double(i) / N), Tb = FMath::Lerp(T0, T1, double(i + 1) / N);
			const FVector2D A = H.At(Ta), B = H.At(Tb);
			const FVector2D Mid = H.Normal(0.5 * (Ta + Tb));
			const FVector N3(Mid.X, 0.0, Mid.Y);
			// Each pane laps 1 cm over the one below it.
			const FVector2D Dn = (A - B).GetSafeNormal() * 0.01;
			const FVector2D A2 = A + (i > 0 ? Dn : FVector2D::ZeroVector);
			M.Rect(FVector(A2.X, kGlassY0, A2.Y), FVector(A2.X, kGlassY1, A2.Y), FVector(B.X, kGlassY1, B.Y), FVector(B.X, kGlassY0, B.Y), N3);
		}
	}

	/** The outline (x, z) of a screen's opening under a vault from Z0 up, x from XL to XR under the arcs. */
	TArray<FVector2D> UnderArc(double XL, double XR, double Z0, bool bNave, int32 Steps)
	{
		TArray<FVector2D> Out;
		Out.Add(FVector2D(XL, Z0));
		Out.Add(FVector2D(XR, Z0));
		auto Top = [&](double X)
		{
			if (bNave)
			{
				const double Dx = FMath::Abs(X) + AP::NaveCentreOffset;
				return AP::Spring + FMath::Sqrt(FMath::Max(0.0, AP::NaveRadius * AP::NaveRadius - Dx * Dx));
			}
			const double Dx = FMath::Abs(FMath::Abs(X) - AP::AisleCentreX) + AP::AisleCentreOffset;
			return AP::Spring + FMath::Sqrt(FMath::Max(0.0, AP::AisleRadius * AP::AisleRadius - Dx * Dx));
		};
		for (int32 i = 0; i <= Steps; ++i)
		{
			const double X = FMath::Lerp(XR, XL, double(i) / Steps);
			Out.Add(FVector2D(X, FMath::Max(Z0, Top(X) - 0.01)));
		}
		// Drop the duplicate corners.
		TArray<FVector2D> Clean;
		for (const FVector2D& P : Out) { if (Clean.Num() == 0 || !P.Equals(Clean.Last(), 1e-6)) { Clean.Add(P); } }
		if (Clean.Num() > 1 && Clean[0].Equals(Clean.Last(), 1e-6)) { Clean.Pop(); }
		return Clean;
	}

	/** A flat outline in the screen plane y = Y (x across, z up), facing into the court (and, the glass being two-sided, out). */
	void ScreenPane(FMeshData& M, const TArray<FVector2D>& Outline, double Y, double NY)
	{
		Planar(M, Outline, FVector(0, Y, 0), FVector(1, 0, 0), kUp, FVector(0, NY, 0));
	}

	void Screens(FParts& P)
	{
		FMeshData& G = P[SlotGlass];
		for (const bool bNorth : {true, false})
		{
			const double Y = bNorth ? kGlassY0 : kGlassY1;
			const double NY = bNorth ? 1.0 : -1.0;
			// The aisles' gables.
			ScreenPane(G, UnderArc(-AP::X1, -AP::NaveHalf, AP::Spring, false, 48), Y, NY);
			ScreenPane(G, UnderArc(AP::NaveHalf, AP::X1, AP::Spring, false, 48), Y, NY);
			if (bNorth)
			{
				ScreenPane(G, UnderArc(-AP::NaveHalf, AP::NaveHalf, AP::NaveStone + 0.02, true, 96), Y, NY);
				continue;
			}
			// The south: clear glass round the band.
			const double B0 = AP::LowerRow0, B1 = AP::UpperRow1, BX = AP::BandX1;
			ScreenPane(G, {FVector2D(-AP::NaveHalf, AP::NaveStone + 0.02), FVector2D(AP::NaveHalf, AP::NaveStone + 0.02), FVector2D(AP::NaveHalf, B0), FVector2D(-AP::NaveHalf, B0)}, Y, NY);
			ScreenPane(G, {FVector2D(-AP::NaveHalf, B0), FVector2D(-BX, B0), FVector2D(-BX, B1), FVector2D(-AP::NaveHalf, B1)}, Y, NY);
			ScreenPane(G, {FVector2D(BX, B0), FVector2D(AP::NaveHalf, B0), FVector2D(AP::NaveHalf, B1), FVector2D(BX, B1)}, Y, NY);
			ScreenPane(G, {FVector2D(-BX, AP::LowerRow1), FVector2D(BX, AP::LowerRow1), FVector2D(BX, AP::UpperRow0), FVector2D(-BX, AP::UpperRow0)}, Y, NY);
			ScreenPane(G, UnderArc(-AP::NaveHalf, AP::NaveHalf, B1, true, 96), Y, NY);
			// The twelve panels: T1 … T6 in the upper row, T7 … T12 in the lower, each read left to right from inside
			// (looking south, the left is east).
			FMeshData& T = P[SlotTristram];
			for (int32 Row = 0; Row < 2; ++Row)
			{
				const double Z0 = Row == 0 ? AP::UpperRow0 : AP::LowerRow0, Z1 = Row == 0 ? AP::UpperRow1 : AP::LowerRow1;
				for (int32 C = 0; C < 6; ++C)
				{
					const double XL = AP::BandX1 - AP::BandPanelW * C, XR = XL - AP::BandPanelW;   // left (east) and right edges
					const double U0 = C / 6.0, U1 = (C + 1) / 6.0, V0 = Row * 0.5, V1 = V0 + 0.5;
					const FVector N(0, -1, 0);   // facing the court
					const int32 Base = T.Positions.Num();
					T.Vertex(FVector(XL, Y, Z1), N, FVector2D(U0, V0));
					T.Vertex(FVector(XR, Y, Z1), N, FVector2D(U1, V0));
					T.Vertex(FVector(XR, Y, Z0), N, FVector2D(U1, V1));
					T.Vertex(FVector(XL, Y, Z0), N, FVector2D(U0, V1));
					T.Quad(Base, Base + 1, Base + 2, Base + 3);
				}
			}
		}
	}

	/** The lancets' glass at the glass line (0.30 m into the wall), in a 6 × 1 atlas read from inside. */
	void Lancets(FParts& P)
	{
		for (const bool bWest : {true, false})
		{
			FMeshData& M = P[bWest ? SlotLancetWest : SlotLancetEast];
			const double X = bWest ? AP::X0 - 0.30 : AP::X1 + 0.30;
			const FVector N(bWest ? 1 : -1, 0, 0);
			for (int32 b = 0; b < 6; ++b)
			{
				const double YC = AP::BayCentreY(b);
				const FOpening O = LancetOpening(YC, 0.01);
				// The outline, from the sill round the head.
				TArray<FVector2D> Outline;   // (y, z)
				Outline.Add(FVector2D(YC - O.HalfAt(O.Sill + 1e-6), O.Sill));
				Outline.Add(FVector2D(YC + O.HalfAt(O.Sill + 1e-6), O.Sill));
				for (int32 i = 1; i < O.Stations.Num(); ++i)
				{
					const double Z = O.Stations[i];
					Outline.Add(FVector2D(YC + (i == O.Stations.Num() - 1 ? 0.0 : O.HalfAt(Z)), Z));
				}
				for (int32 i = O.Stations.Num() - 2; i >= 1; --i)
				{
					const double Z = O.Stations[i];
					Outline.Add(FVector2D(YC - O.HalfAt(Z), Z));
				}
				// Read from inside: on the west wall (facing west) the left is south; on the east, north. Bays run north to
				// south: the east's atlas cell is its bay; the west's too (birds and flowers, each lancet its own).
				const double Half = AP::LancetHalf + 0.01;
				const double Z0 = AP::LancetSill, Z1 = AP::LancetHead + 0.01;
				Planar(M, Outline, FVector(X, 0, 0), FVector(0, 1, 0), kUp, N, [=](const FVector& Pt)
				{
					double U = (Pt.Y - (YC - Half)) / (2.0 * Half);
					if (bWest) { U = 1.0 - U; }
					return FVector2D((b + FMath::Clamp(U, 0.0, 1.0)) / 6.0, (Z1 - Pt.Z) / (Z1 - Z0));
				});
			}
		}
	}
}

namespace AlbionBuild
{
	using namespace AlbionGlassImpl;

	void BuildGlass(FParts& P)
	{
		for (int32 h = 0; h < 6; ++h) { VaultPanes(P[SlotGlass], VaultHalf(h)); }
		Screens(P);
		Lancets(P);
	}
}
