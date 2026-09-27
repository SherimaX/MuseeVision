#include "AlbionCoursing.h"
#include "AlbionParts.h"

/**
 * The masonry inside (§3.1): the court's leaves of the side and end walls, banded and pierced by the lancets and the
 * court door; the pilasters and their stepped corbels, the corner corbels and the aisle cornices; the slate skirting
 * round every face; the nave's cills; the two tablets; the hood mould over the court door.
 */
namespace Musee::Albion
{
	namespace
	{
		/** The side walls' court leaves: from the floor to the springing, between the end walls' glass lines. */
		void SideLeaves(TArray<FFormDrawing>& Out)
		{
			for (const bool bWest : {true, false})
			{
				FLeaf Leaf;
				Leaf.Name = bWest ? TEXT("wall.west.inner") : TEXT("wall.east.inner");
				Leaf.Frame = AcrossX(bWest ? -Plan::SideGlassX : Plan::SideInnerX);
				Leaf.DepthMeters = Plan::SideGlassX - Plan::SideInnerX;
				Leaf.Blocks.Add({-Plan::EndGlassY, Plan::EndGlassY, 0.0, Plan::Springing, true});
				for (int32 Bay = 1; Bay <= Plan::Lines - 1; ++Bay)
				{
					Leaf.Openings.Add(Lancet(Bay));
				}
				LayLeaf(Leaf, Out);
			}
		}

		/**
		 * The end walls' court leaves: the nave's stone to its cill's bed, the aisles' ends to the springing, meeting on
		 * the arcade lines behind the half-columns; the north one pierced by the court door.
		 */
		void EndLeaves(TArray<FFormDrawing>& Out)
		{
			for (const bool bNorth : {true, false})
			{
				FLeaf Leaf;
				Leaf.Name = bNorth ? TEXT("wall.north.inner") : TEXT("wall.south.inner");
				Leaf.Frame = AcrossY(bNorth ? -Plan::EndInnerY : Plan::EndGlassY);
				Leaf.DepthMeters = Plan::EndGlassY - Plan::EndInnerY;
				Leaf.Blocks.Add({-Plan::ArcadeX, Plan::ArcadeX, 0.0, Plan::CillBed, true});
				Leaf.Blocks.Add({-Plan::SideInnerX, -Plan::ArcadeX, 0.0, Plan::Springing, true});
				Leaf.Blocks.Add({Plan::ArcadeX, Plan::SideInnerX, 0.0, Plan::Springing, true});
				if (bNorth)
				{
					Leaf.Openings.Add(CourtDoor());
				}
				LayLeaf(Leaf, Out);
			}
		}

		/** The nave's cills on the end walls' stone: 0.02 proud inside, weathered out to 0.05 beyond the outer face. */
		void Cills(TArray<FFormDrawing>& Out)
		{
			for (const bool bNorth : {true, false})
			{
				const double S = bNorth ? -1.0 : 1.0;
				const double In = S * (Plan::EndInnerY - Plan::CillProud);
				const double Beyond = S * (Plan::EndOuterY + Plan::CillOuterProud);
				FFormExtrusion Shape;
				Shape.Outline = Section({FVector2D(In, Plan::CillBed), FVector2D(Beyond, Plan::CillBed), FVector2D(Beyond, Plan::CillOuterTop),
					FVector2D(In, Plan::CillFrontTop)});
				Shape.DepthMeters = 2.0 * Plan::ArcadeX;
				Out.Add(Drawn(bNorth ? TEXT("wall.north.cill") : TEXT("wall.south.cill"), TEXT("dressing"), MoveTemp(Shape), Solid,
					AcrossX(-Plan::ArcadeX)));
			}
		}

		/** The pilasters on lines 2 to 6 of the side walls, each with its corbel of three steps: a main rib's shoe. */
		void Pilasters(TArray<FFormDrawing>& Out)
		{
			for (const bool bWest : {true, false})
			{
				const double S = bWest ? -1.0 : 1.0;
				const double Face = S * Plan::SideInnerX;
				for (int32 Line = 2; Line <= Plan::Lines - 1; ++Line)
				{
					const double Y = Plan::LineY(Line);
					const FString Name = FString::Printf(TEXT("pilaster.%s.%d"), bWest ? TEXT("west") : TEXT("east"), Line);
					auto Dress = [&](const FString& Piece, double Half, double Proud, double Foot, double Top)
					{
						const double Tip = Face - S * Proud;
						Out.Add(Box(Piece, TEXT("dressing"), FVector(FMath::Min(Face, Tip), Y - Half, Foot), FVector(FMath::Max(Face, Tip), Y + Half, Top),
							Solid));
					};
					Dress(Name, Plan::PilasterHalf, Plan::PilasterProud, Plan::PilasterFoot, Plan::PilasterTop);
					double Foot = Plan::PilasterTop;
					for (int32 Step = 0; Step < int32(UE_ARRAY_COUNT(Plan::CorbelSteps)); ++Step)
					{
						const Plan::FCorbelStep& C = Plan::CorbelSteps[Step];
						Dress(FString::Printf(TEXT("%s.corbel.%d"), *Name, Step + 1), C.Half, C.Proud, Foot, C.Top);
						Foot = C.Top;
					}
				}
			}
			// The corner corbels in each aisle's corners.
			for (const double SX : {-1.0, 1.0})
			{
				for (const double SY : {-1.0, 1.0})
				{
					const FVector2D Corner(SX * Plan::SideInnerX, SY * Plan::EndInnerY);
					const FVector2D Inner = Corner - FVector2D(SX, SY) * Plan::CornerCorbel;
					const FString Name = FString::Printf(TEXT("corner-corbel.%s%s"), SY < 0.0 ? TEXT("n") : TEXT("s"), SX < 0.0 ? TEXT("w") : TEXT("e"));
					Out.Add(Box(Name, TEXT("dressing"), FVector(FMath::Min(Corner.X, Inner.X), FMath::Min(Corner.Y, Inner.Y), Plan::CornerCorbelFoot),
						FVector(FMath::Max(Corner.X, Inner.X), FMath::Max(Corner.Y, Inner.Y), Plan::CornerCorbelTop), Solid));
				}
			}
		}

		/**
		 * The aisle cornices, swept round each aisle on its three walls, from the south wall to the north, their section
		 * out into the court. They stop against the girders' flanges on the arcade lines.
		 */
		void Cornices(TArray<FFormDrawing>& Out)
		{
			const double End = Plan::ArcadeX + Plan::TopFlangeHalf;
			const double X = Plan::SideInnerX;
			const double Y = Plan::EndInnerY;
			const TArray<FVector2D> West = {FVector2D(-End, Y), FVector2D(-X, Y), FVector2D(-X, -Y), FVector2D(-End, -Y)};
			const TArray<FVector2D> East = {FVector2D(End, -Y), FVector2D(X, -Y), FVector2D(X, Y), FVector2D(End, Y)};
			for (const bool bWest : {true, false})
			{
				FFormSweep Shape;
				Shape.Section = SectionOf(Plan::CornicePoints);
				Shape.Path = FFormProfile::Polyline(bWest ? West : East, false);
				Out.Add(Drawn(bWest ? TEXT("cornice.west") : TEXT("cornice.east"), TEXT("dressing"), MoveTemp(Shape), Solid));
			}
		}

		/** The skirting's section: 0.20 high, 0.015 proud of the face, its top arris bevelled 4 mm. */
		FFormProfile SkirtingSection()
		{
			const double P = Plan::SkirtingProud;
			const double H = Plan::SkirtingTop;
			const double B = Plan::SkirtingBevel;
			return Section({FVector2D(0.0, 0.0), FVector2D(P, 0.0), FVector2D(P, H - B), FVector2D(P - B, H), FVector2D(0.0, H)});
		}

		/** A side wall's face run from one end to the other, jogging round the pilasters on lines 2 to 6. */
		TArray<FVector2D> AisleRun(bool bWest)
		{
			const double S = bWest ? -1.0 : 1.0;
			const double Face = S * Plan::SideInnerX;
			const double Pilaster = Face - S * Plan::PilasterProud;
			const double Stop = Plan::ArcadeX + Plan::PlinthHalf;
			// The west runs north from the south wall, the east south from the north wall, so the court lies to the right.
			const double From = bWest ? Plan::EndInnerY : -Plan::EndInnerY;
			TArray<FVector2D> Run = {FVector2D(S * Stop, From), FVector2D(Face, From)};
			for (int32 Step = 2; Step <= Plan::Lines - 1; ++Step)
			{
				const int32 Line = bWest ? Plan::Lines + 1 - Step : Step;
				const double Y = Plan::LineY(Line);
				// The pilaster's side met first, then its far side.
				const double Near = Y - S * Plan::PilasterHalf;
				const double Far = Y + S * Plan::PilasterHalf;
				Run.Append({FVector2D(Face, Near), FVector2D(Pilaster, Near), FVector2D(Pilaster, Far), FVector2D(Face, Far)});
			}
			Run.Append({FVector2D(Face, -From), FVector2D(S * Stop, -From)});
			return Run;
		}

		/**
		 * The slate skirting: round each aisle from half-column to half-column, jogging round the pilasters; along the
		 * nave's ends between the half-columns, stopped at the court door; and round the porch from the court door's
		 * porch face to the surround's clearance on the lining.
		 */
		void Skirting(TArray<FFormDrawing>& Out)
		{
			const double Stop = Plan::ArcadeX - Plan::PlinthHalf;
			const double Y = Plan::EndInnerY;
			const double PY = -Plan::EndOuterY;
			const double LY = Plan::LiningFaceY;
			const double PX = Plan::PorchInnerX;
			const TPair<const TCHAR*, TArray<FVector2D>> Runs[] = {
				{TEXT("skirting.west"), AisleRun(true)},
				{TEXT("skirting.east"), AisleRun(false)},
				{TEXT("skirting.south"), TArray<FVector2D>({FVector2D(Stop, Y), FVector2D(-Stop, Y)})},
				{TEXT("skirting.north.west"), TArray<FVector2D>({FVector2D(-Stop, -Y), FVector2D(-Plan::CourtSkirtingStopX, -Y)})},
				{TEXT("skirting.north.east"), TArray<FVector2D>({FVector2D(Plan::CourtSkirtingStopX, -Y), FVector2D(Stop, -Y)})},
				{TEXT("skirting.porch.west"), TArray<FVector2D>({FVector2D(-Plan::PorchSkirtingStopX, PY), FVector2D(-PX, PY), FVector2D(-PX, LY),
					FVector2D(-Plan::SurroundClearHalf, LY)})},
				{TEXT("skirting.porch.east"), TArray<FVector2D>({FVector2D(Plan::SurroundClearHalf, LY), FVector2D(PX, LY), FVector2D(PX, PY),
					FVector2D(Plan::PorchSkirtingStopX, PY)})}};
			for (const TPair<const TCHAR*, TArray<FVector2D>>& Run : Runs)
			{
				FFormSweep Shape;
				Shape.Section = SkirtingSection();
				Shape.Path = FFormProfile::Polyline(Run.Value, false);
				Out.Add(Drawn(Run.Key, TEXT("skirting"), MoveTemp(Shape), Solid));
			}
		}

		/**
		 * A tablet on the north wall's court face: its face 0.03 proud, its edge chamfered 0.02 in the dressing, the pair
		 * meeting face to face. Bounds are x from, x to, z from, z to.
		 */
		void Tablet(TArray<FFormDrawing>& Out, const TCHAR* Name, const double (&Bounds)[4])
		{
			// On the face, u runs west (u = -x) and v up.
			const FMuseeFrame Face = OnFace(FVector(0.0, -Plan::EndInnerY, 0.0), FVector(0.0, 1.0, 0.0));
			const double U0 = -Bounds[1];
			const double U1 = -Bounds[0];
			const double V0 = Bounds[2];
			const double V1 = Bounds[3];
			const double C = Plan::TabletChamfer;
			FFormExtrusion Core;
			Core.Outline = FFormProfile::Rectangle(FVector2D(U0 + C, V0 + C), FVector2D(U1 - C, V1 - C));
			Core.DepthMeters = Plan::TabletProud;
			Out.Add(Drawn(FString::Printf(TEXT("tablet.%s"), Name), TEXT("tablet"), MoveTemp(Core), Solid, Face));
			// The edge: run clockwise on the face so the chamfer's section reaches in to the core.
			FFormSweep Edge;
			Edge.Section = Section({FVector2D(0.0, 0.0), FVector2D(C, 0.0), FVector2D(C, Plan::TabletProud), FVector2D(0.0, Plan::TabletProud - C)});
			Edge.Path = FFormProfile::Polyline({FVector2D(U0, V1), FVector2D(U1, V1), FVector2D(U1, V0), FVector2D(U0, V0)}, true);
			Out.Add(Drawn(FString::Printf(TEXT("tablet.%s.edge"), Name), TEXT("dressing"), MoveTemp(Edge), Solid, Face));
		}

		/** The hood mould over the court door: the door's outline grown 0.16, with level returns at the springing. */
		void HoodMould(TArray<FFormDrawing>& Out)
		{
			const FArch Hood = CourtDoor().Grown(Plan::HoodGrow);
			const double S = Hood.Springing;
			FFormProfile Path = FFormProfile::From(FVector2D(Hood.Centre - Hood.Half - Plan::HoodReturn, S));
			Path.LineTo(FVector2D(Hood.Centre - Hood.Half, S), EFormJoint::Crisp);
			Hood.OverHead(Path, S, S);
			Path.LineTo(FVector2D(Hood.Centre + Hood.Half + Plan::HoodReturn, S), EFormJoint::Crisp);
			FFormSweep Shape;
			Shape.Section = SectionOf(Plan::HoodPoints);
			Shape.Path = MoveTemp(Path);
			Out.Add(Drawn(TEXT("hood.court-door"), TEXT("dressing"), MoveTemp(Shape), Solid,
				OnFace(FVector(0.0, -Plan::EndInnerY, 0.0), FVector(0.0, 1.0, 0.0))));
		}
	}

	void DrawMasonry(TArray<FFormDrawing>& Out)
	{
		SideLeaves(Out);
		EndLeaves(Out);
		Cills(Out);
		Pilasters(Out);
		Cornices(Out);
		Skirting(Out);
		Tablet(Out, TEXT("ruskin"), Plan::RuskinTablet);
		Tablet(Out, TEXT("albion"), Plan::AlbionTablet);
		HoodMould(Out);
	}
}
