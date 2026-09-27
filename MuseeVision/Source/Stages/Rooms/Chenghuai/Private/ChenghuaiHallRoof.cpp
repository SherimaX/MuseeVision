#include "ChenghuaiHallTrades.h"
#include "ChenghuaiParts.h"
#include "ChenghuaiPlan.h"

/**
 * A hall's roof (§3.3, §3.4; ChenghuaiHall.cpp:693-909, ChenghuaiRoof.cpp:22-292): the boarding (望板) on the rafters with
 * its riser boards at the eave's steps, the lime bed over it, the gable bands (博缝) on its ends over a hard gable; the
 * round rafters (椽) and square flying rafters (飞椽) under the boarding; the tiles in rows down the bed, each course's lower
 * end standing proud of the next; the moulded ridge with its scorpion tails, or the rolled ridge; the edge mouldings down
 * the hard gables (排山).
 *
 * The rafters, tiles, tails and gable edges are drawn as reliefs over curves of the building's section: Form draws one
 * closed solid per drawing and has no array of identical members, and its sweeps run only along paths in plan (the
 * sheet's known gaps), so a surface of rows over a curved bed, and a section swept along a curve that climbs, are carved as
 * reliefs on that curve.
 */
namespace Musee::Chenghuai::Hall
{
	namespace
	{
		/**
		 * A roof's own copy of its building and section, shared by the reliefs' height functions, which Form's making asks
		 * long after the building's description is written.
		 */
		struct FRoof
		{
			FHallSpec Spec;
			FHallSection Section;
		};
		using FRoofRef = TSharedRef<const FRoof>;

		/** The roof's outer span along u: the far ends of any mitre, at its front and back edges. */
		void OuterSpan(const FHallSpec& Spec, const FHallSection& S, double& OutA, double& OutB)
		{
			double A0 = 0.0;
			double B0 = 0.0;
			double A1 = 0.0;
			double B1 = 0.0;
			RoofSpan(Spec, S.EaveF, A0, B0);
			RoofSpan(Spec, S.BackEdge, A1, B1);
			OutA = FMath::Min(A0, A1);
			OutB = FMath::Max(B0, B1);
		}

		TArray<double> Knots(const FHallSpec& Spec, const FHallSection& S)
		{
			TArray<double> All = S.KnotV;
			All.Append(Spec.PurlinV);
			All.Add(S.EaveR);
			All.Add(S.BackR);
			All.Add(S.RidgeV);
			return All;
		}

		TArray<FVector2D> Line(TConstArrayView<double> Vs, TFunctionRef<double(double)> Z)
		{
			TArray<FVector2D> Points;
			for (const double V : Vs)
			{
				Points.Add(FVector2D(V, Z(V)));
			}
			return Points;
		}

		/** The undersides of the three zones of the boarding: over the front flying rafters, the round rafters, the back flying rafters. */
		struct FUnderside
		{
			const FHallSection& S;
			double Front(double V) const { return S.FlyUnder(V, false) + Plan::FlyingRafter; }
			double Middle(double V) const { return S.RafterTop(V); }
			double Back(double V) const { return S.FlyUnder(V, true) + Plan::FlyingRafter; }
			double MiddleEnd() const { return S.bOpenBack ? S.BackR : S.BackEdge; }
		};

		/**
		 * The boarding's section: its underside front to back, stepping at the round rafters' ends; its top back to front,
		 * a board's thickness over it, with a riser board (小连檐) standing on the round rafters' ends at each step.
		 */
		void BoardingLines(const FHallSpec& Spec, const FHallSection& S, TArray<FVector2D>& OutBottom, TArray<FVector2D>& OutTop)
		{
			const FUnderside U{S};
			const TArray<double> K = Knots(Spec, S);
			const double Board = Plan::Boarding;
			const double Step = Plan::SlabStation;
			OutBottom = Line(Stations(S.EaveF, S.EaveR, Step, K), [&U](double V) { return U.Front(V); });
			OutBottom.Append(Line(Stations(S.EaveR, U.MiddleEnd(), Step, K), [&U](double V) { return U.Middle(V); }));
			OutTop.Reset();
			if (S.bOpenBack)
			{
				OutBottom.Append(Line(Stations(S.BackR, S.BackEdge, Step, K), [&U](double V) { return U.Back(V); }));
				OutTop = Line(Stations(S.BackEdge, S.BackR - Board, Step, K), [&U, Board](double V) { return U.Back(V) + Board; });
			}
			const double MiddleFrom = S.bOpenBack ? S.BackR - Board : S.BackEdge;
			OutTop.Append(Line(Stations(MiddleFrom, S.EaveR + Board, Step, K), [&U, Board](double V) { return U.Middle(V) + Board; }));
			OutTop.Append(Line(Stations(S.EaveR + Board, S.EaveF, Step, K), [&U, Board](double V) { return U.Front(V) + Board; }));
		}

		/** The boarding and the lime bed along the roof, and a gable band on each end over a hard gable. */
		void Slab(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& S)
		{
			TArray<FVector2D> Bottom;
			TArray<FVector2D> Top;
			BoardingLines(Spec, S, Bottom, Top);
			const TArray<FVector2D> BedLine = Line(Stations(S.EaveF, S.BackEdge, Plan::SlabStation, Knots(Spec, S)), [&S](double V) { return S.Bed(V); });
			double A = 0.0;
			double B = 0.0;
			OuterSpan(Spec, S, A, B);
			const bool bBandA = HasGable(Spec, false);
			const bool bBandB = HasGable(Spec, true);
			const double SA = A + (bBandA ? Plan::GableBand : 0.0);
			const double SB = B - (bBandB ? Plan::GableBand : 0.0);
			TArray<FVector2D> Boarding = Bottom;
			Boarding.Append(Top);
			AlongU(Out, Named(Spec, TEXT("roof.boarding")), Spec.Rafters, Spec.Frame, Boarding, {}, SA, SB, EHit::None);
			TArray<FVector2D> Bed = BedLine;
			Bed.Append(Top);
			AlongU(Out, Named(Spec, TEXT("roof.bed")), TEXT("roof.bed"), Spec.Frame, Bed, {}, SA, SB, EHit::None);
			TArray<FVector2D> Whole = Bottom;
			for (int32 Index = BedLine.Num() - 1; Index >= 0; --Index)
			{
				Whole.Add(BedLine[Index]);
			}
			if (bBandA)
			{
				AlongU(Out, Named(Spec, TEXT("roof.band.1")), Spec.WallBase, Spec.Frame, Whole, {}, A, SA, EHit::None);
			}
			if (bBandB)
			{
				AlongU(Out, Named(Spec, TEXT("roof.band.2")), Spec.WallBase, Spec.Frame, Whole, {}, SB, B, EHit::None);
			}
		}

		/** Where the rafters run along u: 0.12 in from a gable's inner face, 0.08 in from an open end. */
		void RafterSpan(const FHallSpec& Spec, const FHallSection& S, double& OutA, double& OutB, int32& OutCount, double& OutPitch)
		{
			double A = 0.0;
			double B = 0.0;
			OuterSpan(Spec, S, A, B);
			OutA = HasGable(Spec, false) ? Spec.U0 + Spec.GableT + Plan::RafterInset : A + Plan::RafterInsetOpen;
			OutB = HasGable(Spec, true) ? Spec.U1 - Spec.GableT - Plan::RafterInset : B - Plan::RafterInsetOpen;
			OutCount = FMath::Max(1, FMath::FloorToInt32((OutB - OutA) / Plan::RafterPitch));
			OutPitch = (OutB - OutA) / OutCount;
		}

		/** The distance from u to the nearest rafter's axis. */
		double OffRafter(double U, double A, int32 Count, double Pitch)
		{
			const int32 K = FMath::Clamp(FMath::RoundToInt32((U - A) / Pitch), 0, Count);
			return FMath::Abs(U - (A + K * Pitch));
		}

		/** Round rafters under the boarding and square flying rafters under the flying boarding, each a relief hanging down. */
		void Rafters(TArray<FFormDrawing>& Out, const FRoofRef& Roof)
		{
			const FHallSpec& Spec = Roof->Spec;
			const FHallSection& S = Roof->Section;
			double A = 0.0;
			double B = 0.0;
			int32 Count = 0;
			double Pitch = 0.0;
			RafterSpan(Spec, S, A, B, Count, Pitch);
			const FUnderside U{S};
			const TArray<double> K = Knots(Spec, S);
			const double R = 0.5 * Plan::RafterDiameter;
			const double Fly = Plan::FlyingRafter;
			// Walked towards the eave (-v), a curve's right is down: the rafters hang from it.
			const double RoundEnd = S.bOpenBack ? S.BackR : Spec.BackWallV0;
			auto Carve = [&](const TCHAR* Part, TArray<FVector2D> Curve, double Grid, TFunction<double(double)> Profile)
			{
				TSharedRef<const FMeasuredLine> Along = MakeShared<FMeasuredLine>(Curve);
				ReliefAlongU(Out, Named(Spec, Part), Spec.Rafters, Spec.Frame, Curve, A - R, B + R, Grid, Plan::ReliefBack,
					[Roof, Along, A, Count, Pitch, Profile = MoveTemp(Profile)](double Distance, double AtU)
					{
						double MA = 0.0;
						double MB = 0.0;
						RoofSpan(Roof->Spec, Along->At(Distance).X, MA, MB);
						if (AtU < MA + Plan::RafterClearOfMitre || AtU > MB - Plan::RafterClearOfMitre)
						{
							return 0.0;
						}
						return Profile(OffRafter(AtU, A, Count, Pitch));
					});
			};
			auto Round = [R](double D) { return D < R ? R + FMath::Sqrt(R * R - D * D) : 0.0; };
			auto Square = [Fly](double D) { return D <= 0.5 * Fly ? Fly : 0.0; };
			Carve(TEXT("roof.rafters"), Line(Stations(RoundEnd, S.EaveR, Plan::SlabStation, K), [&U](double V) { return U.Middle(V); }),
				Plan::MemberGrid, Round);
			Carve(TEXT("roof.flying-rafters"), Line(Stations(S.EaveR, S.EaveF, Plan::SlabStation, K), [&U](double V) { return U.Front(V); }),
				Plan::FlyingGrid, Square);
			if (S.bOpenBack)
			{
				Carve(TEXT("roof.flying-rafters.back"), Line(Stations(S.BackEdge, S.BackR, Plan::SlabStation, K), [&U](double V) { return U.Back(V); }),
					Plan::FlyingGrid, Square);
			}
		}

		/** The tiles: whole rows between the gables' edge tiles, down both slopes from the ridge to the eave's edge. */
		void Tiles(TArray<FFormDrawing>& Out, const FRoofRef& Roof)
		{
			const FHallSpec& Spec = Roof->Spec;
			const FHallSection& S = Roof->Section;
			const Plan::FTileStyle Style = Spec.bTong ? Plan::Tong : Plan::He;
			double A = 0.0;
			double B = 0.0;
			OuterSpan(Spec, S, A, B);
			const double From = A + (HasGable(Spec, false) ? Plan::EdgeTiles : 0.0);
			const double Grid = Style.Pitch / Plan::TileStationsPerRow;
			// The band is a whole number of stations, so each row is sampled alike.
			const int32 Steps = FMath::FloorToInt32((B - (HasGable(Spec, true) ? Plan::EdgeTiles : 0.0) - From) / Grid);
			const double To = From + (Steps - UE_KINDA_SMALL_NUMBER) * Grid;
			const TArray<FVector2D> Curve = Line(Stations(S.EaveF, S.BackEdge, Plan::SlabStation, Knots(Spec, S)), [&S](double V) { return S.Bed(V); });
			TSharedRef<const FMeasuredLine> Along = MakeShared<FMeasuredLine>(Curve);
			const double RidgeClear = Spec.Ridge == ERidge::Qingshui ? Plan::RowsFromRidge : 0.0;
			const double RowsFrom = From + Style.PanHalf;
			const bool bTong = Spec.bTong;
			ReliefAlongU(Out, Named(Spec, TEXT("roof.tiles")), Style.Surface, Spec.Frame, Curve, From, To, Grid, Plan::ReliefBack,
				[Roof, Along, RidgeClear, RowsFrom, Style, bTong](double Distance, double U)
				{
					const double V = Along->At(Distance).X;
					const double RidgeV = Roof->Section.RidgeV;
					double MA = 0.0;
					double MB = 0.0;
					RoofSpan(Roof->Spec, V, MA, MB);
					if (FMath::Abs(V - RidgeV) < RidgeClear || U < MA || U > MB)
					{
						return 0.0;
					}
					return TileHeight(Style, bTong, U - RowsFrom, V < RidgeV ? Distance : Along->Length() - Distance);
				});
		}

		/** The moulded ridge (清水脊) with its scorpion tails (蝎子尾) on their plates, or the rolled ridge (过垄脊). */
		void Ridge(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& S)
		{
			if (Spec.Ridge == ERidge::None)
			{
				return;
			}
			const FBuildingFrame& F = Spec.Frame;
			double A = 0.0;
			double B = 0.0;
			RoofSpan(Spec, S.RidgeV, A, B);
			const double Z = S.Bed(S.RidgeV);
			FFormSweep Sweep;
			Sweep.ElevationMeters = Z;
			if (Spec.Ridge == ERidge::Rolled)
			{
				A += HasGable(Spec, false) ? Plan::EdgeTiles : 0.0;
				B -= HasGable(Spec, true) ? Plan::EdgeTiles : 0.0;
				Sweep.Section = Section(Plan::RolledRidge, true);
				Sweep.Path = FFormProfile::Polyline({F.Plan(A, S.RidgeV), F.Plan(B, S.RidgeV)}, false);
				Draw(Out, Named(Spec, TEXT("roof.ridge")), TEXT("roof.ridge"), MoveTemp(Sweep), EHit::None);
				return;
			}
			A += Plan::RidgeEndInset;
			B -= Plan::RidgeEndInset;
			Sweep.Section = Section(Plan::QingshuiRidge, true);
			Sweep.Path = FFormProfile::Polyline({F.Plan(A, S.RidgeV), F.Plan(B, S.RidgeV)}, false);
			Draw(Out, Named(Spec, TEXT("roof.ridge")), TEXT("roof.ridge"), MoveTemp(Sweep), EHit::None);
			for (const bool bAtB : {false, true})
			{
				const double End = bAtB ? B - Plan::TailIn : A + Plan::TailIn;
				const double Outward = bAtB ? 1.0 : -1.0;
				const FString Tail = FString::Printf(TEXT("roof.tail.%d"), bAtB ? 2 : 1);
				const double HalfL = 0.5 * Plan::TailPlateLength;
				const double HalfW = 0.5 * Plan::TailPlateWidth;
				Block(Out, Named(Spec, Tail + TEXT(".plate")), TEXT("roof.ridge"), F, End - HalfL, End + HalfL, S.RidgeV - HalfW, S.RidgeV + HalfW,
					Z + Plan::TailPlateBottom, Z + Plan::TailPlateTop, EHit::None);
				// The tail's line, walked towards +u so its section stands up on it: u = end ± (0.10 + 0.55 t), z = ridge + 0.44
				// + 0.34 t + 0.08 t², its underside 0.06 under the line.
				TArray<FVector2D> Curve;
				for (int32 K = 0; K < Plan::TailStations; ++K)
				{
					const double T = double(bAtB ? K : Plan::TailStations - 1 - K) / (Plan::TailStations - 1);
					Curve.Add(FVector2D(End + Outward * (0.10 + 0.55 * T), Z + 0.44 + 0.34 * T + 0.08 * T * T + Plan::TailUnder));
				}
				const double RidgeV = S.RidgeV;
				ReliefAlongV(Out, Named(Spec, Tail), TEXT("roof.ridge"), F, Curve, RidgeV + Plan::TailTop[0].A, RidgeV - Plan::TailTop[0].A,
					Plan::MemberGrid, Plan::ReliefBack, [RidgeV](double, double V) { return TopAt(Plan::TailTop, V - RidgeV) - Plan::TailUnder; });
			}
		}

		/** The gable edges (排山) down a hard gable over its band: the whole bed from eave to eave, 0.02 over it. */
		void GableEdges(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& S)
		{
			double A = 0.0;
			double B = 0.0;
			RoofSpan(Spec, 0.0, A, B);
			const double Past = Plan::GableEdgeOver;
			const TArray<FVector2D> Curve = Line(Stations(S.EaveF - Past, S.BackEdge + Past, Plan::SlabStation, Knots(Spec, S)),
				[&S](double V) { return S.Bed(V); });
			const double Half = Plan::GableEdgeTop[UE_ARRAY_COUNT(Plan::GableEdgeTop) - 1].A;
			for (const bool bAtB : {false, true})
			{
				if (!HasGable(Spec, bAtB))
				{
					continue;
				}
				const double Centre = bAtB ? B - 0.5 * Plan::EdgeTiles : A + 0.5 * Plan::EdgeTiles;
				ReliefAlongU(Out, Named(Spec, FString::Printf(TEXT("roof.gable-edge.%d"), bAtB ? 2 : 1)), TEXT("roof.ridge"), Spec.Frame, Curve,
					Centre - Half, Centre + Half, Plan::MemberGrid, Plan::ReliefBack,
					[Centre](double, double U) { return Plan::GableEdgeOver + TopAt(Plan::GableEdgeTop, U - Centre); });
			}
		}
	}

}

namespace Musee::Chenghuai
{
	double TileHeight(const Plan::FTileStyle& Style, bool bTong, double FromRows, double FromEave)
	{
		// Across a row: the lower pan's concave top, and the upper tile's (a round cover, or a pan laid face down).
		const double Row = FromRows / Style.Pitch;
		const double Pan = FMath::Abs(Row - FMath::RoundToDouble(Row)) * Style.Pitch / Style.PanHalf;
		const double Upper = FMath::Abs(Row - 0.5 - FMath::RoundToDouble(Row - 0.5)) * Style.Pitch / Style.TopHalf;
		// Down the slope: each course's lower end 9 mm proud, falling to nothing at its upper end under the next.
		auto Lip = [FromEave](double Course) { return Plan::TileLip * (1.0 - FMath::Frac(FromEave / Course)); };
		double Height = 0.0;
		if (Pan <= 1.0)
		{
			Height = FMath::Max(Height, Plan::PanTop + Plan::PanRise * Pan * Pan + Lip(Style.PanCourse));
		}
		if (Upper <= 1.0)
		{
			const double Top = bTong ? Plan::CoverTop + Plan::CoverRise * FMath::Sqrt(1.0 - Upper * Upper)
				: Plan::UpperPanTop + Plan::UpperPanRise * (1.0 - Upper * Upper);
			Height = FMath::Max(Height, Top + Lip(Style.TopCourse));
		}
		return Height;
	}
}

namespace Musee::Chenghuai::Hall
{
	void DrawRoof(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection&)
	{
		const TSharedRef<FRoof> Roof = MakeShared<FRoof>();
		Roof->Spec = Spec;
		Roof->Section = FHallSection::Of(Roof->Spec);
		Slab(Out, Roof->Spec, Roof->Section);
		Rafters(Out, Roof);
		Tiles(Out, Roof);
		Ridge(Out, Roof->Spec, Roof->Section);
		GableEdges(Out, Roof->Spec, Roof->Section);
	}
}
