#include "ChenghuaiHallTrades.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlan.h"

/**
 * The platform (台基) and the timber frame (大木) of a hall (§3.2, §3.3; ChenghuaiHall.cpp:424-453, 485-690): the brick body
 * on its footing course, the stone kerb and corner stones, the floor inside the kerb; the purlins on their seats, the eave
 * and facade lintels with their boards, and at each free column a frame of beams stepping up on struts to the ridge.
 */
namespace Musee::Chenghuai::Hall
{
	void DrawPlatform(TArray<FFormDrawing>& Out, const FHallSpec& Spec)
	{
		const FBuildingFrame& F = Spec.Frame;
		const double U0 = Spec.PlatformU0;
		const double U1 = Spec.PlatformU1;
		const double V0 = Spec.PlatformFront;
		const double V1 = Spec.PlatformBack;
		const double Out5 = Plan::FootingProud;
		const double KerbFoot = Spec.Floor - Plan::KerbDepth;
		const bool bLow = Spec.Floor <= Plan::LowPlatform;
		Block(Out, Named(Spec, TEXT("platform.footing")), TEXT("stone.kerb"), F, U0 - Out5, U1 + Out5, V0 - Out5, V1 + Out5, Plan::FootingBottom,
			Plan::FootingTop, EHit::Walkable);
		const double W = Plan::KerbWidth;
		const TArray<FVector2D> Inside = Rectangle(U0 + W, V0 + W, U1 - W, V1 - W);
		// A low platform's kerbs are solid from the footing to the floor; a high one's stand on its brick body.
		Raised(Out, Named(Spec, TEXT("platform.kerb")), TEXT("stone.kerb"), F, Rectangle(U0, V0, U1, V1), {Inside},
			bLow ? Plan::FootingTop : KerbFoot, Spec.Floor, EHit::Walkable);
		Raised(Out, Named(Spec, TEXT("platform.floor")), Spec.FloorSurface, F, Inside, {}, KerbFoot, Spec.Floor, EHit::Walkable);
		if (bLow)
		{
			return;
		}
		Block(Out, Named(Spec, TEXT("platform.body")), Spec.PlatformBody, F, U0, U1, V0, V1, Plan::FootingBottom, KerbFoot, EHit::Solid);
		// The corner stones (埋头) at the front corners, standing a hair proud of the body's faces.
		const double Proud = Plan::CornerStoneProud;
		const double C = Plan::CornerStone;
		Block(Out, Named(Spec, TEXT("platform.corner.1")), TEXT("stone.kerb"), F, U0 - Proud, U0 + C, V0 - Proud, V0 + C, Plan::FootingTop,
			KerbFoot, EHit::Solid);
		Block(Out, Named(Spec, TEXT("platform.corner.2")), TEXT("stone.kerb"), F, U1 - C, U1 + Proud, V0 - Proud, V0 + C, Plan::FootingTop,
			KerbFoot, EHit::Solid);
	}

	namespace
	{
		/** The purlins (檩): round, radius Pr, their tops on the seats, 0.12 into the gables, else to the roof's own ends. */
		void Purlins(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section)
		{
			const FBuildingFrame& F = Spec.Frame;
			const double Pr = Section.PurlinR;
			const TCHAR* Surface = PaintedOr(Spec, Spec.Rafters);
			for (int32 I = 0; I < Spec.PurlinV.Num(); ++I)
			{
				if (Spec.SkipPurlins.Contains(I))
				{
					continue;
				}
				const double V = Spec.PurlinV[I];
				double A = 0.0;
				double B = 0.0;
				RoofSpan(Spec, V, A, B);
				A = HasGable(Spec, false) ? Spec.U0 + Spec.GableT - Plan::PurlinIntoGable : A + Plan::RidgeEndInset;
				B = HasGable(Spec, true) ? Spec.U1 - Spec.GableT + Plan::PurlinIntoGable : B - Plan::RidgeEndInset;
				const double Z = Section.Seat[I] - Pr;
				Rod(Out, Named(Spec, FString::Printf(TEXT("purlin.%d"), I + 1)), Surface, F.At(A, V, Z), F.At(B, V, Z), Pr, EHit::None);
			}
		}

		/** A lintel and its board along a line of columns at v, the lintel's top at Top, the board up to its purlin. */
		void LintelLine(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FString& Part, double V, double Top, double BoardTop)
		{
			if (Spec.ColU.Num() < 2)
			{
				return;
			}
			const FBuildingFrame& F = Spec.Frame;
			const double D = Spec.ColD;
			const double Height = Plan::LintelPerDiameter * D + Plan::LintelOver;
			const double Half = 0.5 * Plan::LintelWidthPerDiameter * D;
			const double A = Spec.ColU[0];
			const double B = Spec.ColU.Last();
			const TCHAR* Surface = PaintedOr(Spec, Spec.Timber);
			Block(Out, Named(Spec, Part + TEXT(".lintel")), Surface, F, A, B, V - Half, V + Half, Top - Height, Top, EHit::None);
			if (BoardTop > Top)
			{
				const double Board = 0.5 * Plan::LintelBoard;
				Block(Out, Named(Spec, Part + TEXT(".board")), Surface, F, A, B, V - Board, V + Board, Top, BoardTop, EHit::None);
			}
		}

		void Lintels(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section)
		{
			const double Pr = Section.PurlinR;
			LintelLine(Out, Spec, TEXT("eave"), 0.0, Section.Zc, Section.Seat[0] - 2.0 * Pr + Plan::LintelBoardUnderPurlin);
			const int32 Facade = Spec.FacadeV > 0.0 ? Section.PurlinAt(Spec.FacadeV) : INDEX_NONE;
			if (Facade != INDEX_NONE)
			{
				LintelLine(Out, Spec, TEXT("facade"), Spec.FacadeV, Section.InnerColumnTop(Facade),
					Section.Seat[Facade] - 2.0 * Pr + Plan::LintelBoardUnderPurlin);
			}
			if (Spec.bBackColumns)
			{
				const int32 Back = Spec.PurlinV.Num() - 1;
				const double Top = Section.Seat[Back] - Plan::SeatOver - 2.0 * Pr;
				LintelLine(Out, Spec, TEXT("back"), Spec.PurlinV[Back], Top, Section.Seat[Back] - 2.0 * Pr + Plan::LintelBoardUnderPurlin);
			}
		}

		/** One frame across (梁架) at u: the veranda's beam and tie, then beams stepping up on struts to the ridge's strut. */
		void FrameAcross(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section, double U, int32 Index)
		{
			const FBuildingFrame& F = Spec.Frame;
			const TArray<double>& P = Spec.PurlinV;
			const int32 N = P.Num();
			const double D = Spec.ColD;
			const double Pr = Section.PurlinR;
			const FString Frame = FString::Printf(TEXT("frame.%d"), Index);
			const TCHAR* Beams = PaintedOr(Spec, Spec.Timber);
			int32 Count = 0;
			auto Beam = [&](double Va, double Vb, double Top, double W, double H)
			{
				Block(Out, Named(Spec, FString::Printf(TEXT("%s.beam.%d"), *Frame, ++Count)), Beams, F, U - 0.5 * W, U + 0.5 * W, Va, Vb, Top - H,
					Top, EHit::None);
			};
			auto Strut = [&](double V, double Z0, double Z1, double W)
			{
				if (Z1 - Z0 > Plan::SeatStep)
				{
					Block(Out, Named(Spec, FString::Printf(TEXT("%s.strut.%d"), *Frame, ++Count)), Spec.Timber, F, U - 0.5 * W, U + 0.5 * W, V - 0.5 * W,
						V + 0.5 * W, Z0, Z1, EHit::None);
				}
			};
			const double W0 = Plan::BeamWidthPerDiameter * D + Plan::BeamWidthOver;
			const double H0 = Plan::BeamHeightPerDiameter * D + Plan::BeamHeightOver;
			const int32 Front = Spec.FacadeV > 0.0 ? FMath::Max(0, Section.PurlinAt(Spec.FacadeV)) : 0;
			const int32 Back = Spec.InnerColV.IsEmpty() ? N - 1 : FMath::Max(Front + 1, Section.PurlinAt(Spec.InnerColV.Last()));
			if (Front > 0)
			{
				Beam(-Plan::BeamPastFront, P[Front] + Plan::VerandaBeamPast, Section.Seat[0] - Plan::VerandaBeamUnderSeat * Pr, W0, H0);
				const double TieHalf = 0.5 * Plan::TieWidthPerDiameter * D;
				const double TieZ = Section.Zc - Plan::TieUnderTop;
				const double TieH = 0.5 * Plan::TieHeightPerDiameter * D;
				Block(Out, Named(Spec, FString::Printf(TEXT("%s.tie"), *Frame)), Beams, F, U - TieHalf, U + TieHalf, Plan::TieFrom, P[Front], TieZ - TieH,
					TieZ + TieH, EHit::None);
			}
			if (Back < N - 1)
			{
				Beam(P[Back] - Plan::VerandaBeamPast, P[N - 1] + Plan::BeamPastBack, Section.Seat[N - 1] - Plan::VerandaBeamUnderSeat * Pr, W0, H0);
			}
			double LastTop = -1.0;
			double W = W0;
			double H = H0;
			int32 A = Front;
			int32 B = Back;
			while (B - A >= 1)
			{
				const double Top = FMath::Min(Section.Seat[A], Section.Seat[B]) - Plan::BeamUnderSeat * Pr;
				const double Va = P[A] - (A == Front ? Plan::BeamPastFront : Plan::BeamPastInner);
				const double Vb = P[B] + (B == N - 1 ? Plan::BeamPastBack : Plan::BeamPastInner);
				if (LastTop > 0.0)
				{
					Strut(P[A], LastTop, Top - H, Plan::StrutPerBeam * W);
					Strut(P[B], LastTop, Top - H, Plan::StrutPerBeam * W);
				}
				Beam(Va, Vb, Top, W, H);
				if (FMath::Abs(Section.Seat[A] - Section.Seat[B]) > Plan::SeatStep)
				{
					const int32 High = Section.Seat[A] > Section.Seat[B] ? A : B;
					Strut(P[High], Top, Section.Seat[High] - Plan::BeamUnderSeat * Pr, Plan::BlockPerBeam * W);
				}
				LastTop = Top;
				W *= Plan::BeamTierWidth;
				H *= Plan::BeamTierHeight;
				++A;
				--B;
			}
			if (A == B && LastTop > 0.0)
			{
				Strut(P[A], LastTop, Section.Seat[A] - Plan::RidgeStrutUnderSeat * Pr, Plan::BlockPerBeam * W);
			}
		}
	}

	void DrawTimber(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section)
	{
		Purlins(Out, Spec, Section);
		Lintels(Out, Spec, Section);
		if (!Spec.bFrames)
		{
			return;
		}
		const TArray<double> At = Spec.FrameU.IsEmpty() ? FreeColumns(Spec) : Spec.FrameU;
		for (int32 Index = 0; Index < At.Num(); ++Index)
		{
			FrameAcross(Out, Spec, Section, At[Index], Index + 1);
		}
	}
}
