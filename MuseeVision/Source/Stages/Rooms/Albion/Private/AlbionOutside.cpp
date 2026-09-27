#include "AlbionCoursing.h"
#include "AlbionParts.h"

/**
 * The masonry outside (§3.2): the walls' weathered leaves, banded from the footings to the parapets and pierced by the
 * lancets and the court door; the plinth course round court and porch from the port plane; the eighteen buttresses on
 * their base courses; the copings over the parapets; the side walls' lead gutters behind them.
 */
namespace Musee::Albion
{
	namespace
	{
		/** The side walls' weathered leaves, from the footings to the copings' bed, between the end walls' outer faces. */
		void OuterSideLeaves(TArray<FFormDrawing>& Out)
		{
			for (const bool bWest : {true, false})
			{
				FLeaf Leaf;
				Leaf.Name = bWest ? TEXT("wall.west.outer") : TEXT("wall.east.outer");
				Leaf.Frame = AcrossX(bWest ? -Plan::SideOuterX : Plan::SideGlassX);
				Leaf.DepthMeters = Plan::SideOuterX - Plan::SideGlassX;
				Leaf.Blocks.Add({-Plan::EndOuterY, Plan::EndOuterY, Plan::FootingZ, Plan::CopingBed, false});
				for (int32 Bay = 1; Bay <= Plan::Lines - 1; ++Bay)
				{
					Leaf.Openings.Add(Lancet(Bay));
				}
				LayLeaf(Leaf, Out);
			}
		}

		/**
		 * The end walls' weathered leaves: the nave's to its cill, the aisles' ends rising as the end parapets to the
		 * copings. On the north the porch stands against the nave's leaf: inside it, to its ceiling, the leaf is the porch's
		 * banded face, pierced by the court door.
		 */
		void OuterEndLeaves(TArray<FFormDrawing>& Out)
		{
			const double Nave = Plan::ArcadeX;
			const double Side = Plan::SideGlassX;
			const double Porch = Plan::PorchInnerX;
			for (const bool bNorth : {true, false})
			{
				FLeaf Leaf;
				Leaf.Name = bNorth ? TEXT("wall.north.outer") : TEXT("wall.south.outer");
				Leaf.Frame = AcrossY(bNorth ? -Plan::EndGlassY : Plan::EndOuterY);
				Leaf.DepthMeters = Plan::EndOuterY - Plan::EndGlassY;
				Leaf.Blocks.Add({-Side, -Nave, Plan::FootingZ, Plan::CopingBed, false});
				Leaf.Blocks.Add({Nave, Side, Plan::FootingZ, Plan::CopingBed, false});
				if (!bNorth)
				{
					Leaf.Blocks.Add({-Nave, Nave, Plan::FootingZ, Plan::CillBed, false});
					LayLeaf(Leaf, Out);
					continue;
				}
				Leaf.Blocks.Add({-Nave, -Porch, Plan::FootingZ, Plan::CillBed, false});
				Leaf.Blocks.Add({Porch, Nave, Plan::FootingZ, Plan::CillBed, false});
				Leaf.Blocks.Add({-Porch, Porch, Plan::FootingZ, 0.0, false});
				Leaf.Blocks.Add({-Porch, Porch, 0.0, Plan::PorchCeiling, true});
				Leaf.Blocks.Add({-Porch, Porch, Plan::PorchCeiling, Plan::CillBed, false});
				Leaf.Openings.Add(CourtDoor());
				LayLeaf(Leaf, Out);
			}
		}

		/** The buttresses: on the side walls at lines 1 to 7, on the end walls on the arcade lines. Face point, outward normal, top. */
		struct FButtress
		{
			FString Name;
			FVector2D Face;
			FVector2D Out;
			double Top;
		};

		TArray<FButtress> Buttresses()
		{
			TArray<FButtress> All;
			for (int32 Line = 1; Line <= Plan::Lines; ++Line)
			{
				const double Y = Plan::LineY(Line);
				All.Add({FString::Printf(TEXT("buttress.west.%d"), Line), FVector2D(-Plan::SideOuterX, Y), FVector2D(-1.0, 0.0), Plan::SideButtressTop});
				All.Add({FString::Printf(TEXT("buttress.east.%d"), Line), FVector2D(Plan::SideOuterX, Y), FVector2D(1.0, 0.0), Plan::SideButtressTop});
			}
			for (const double SX : {-1.0, 1.0})
			{
				const TCHAR* Side = SX < 0.0 ? TEXT("w") : TEXT("e");
				All.Add({FString::Printf(TEXT("buttress.n%s"), Side), FVector2D(SX * Plan::ArcadeX, -Plan::EndOuterY), FVector2D(0.0, -1.0), Plan::EndButtressTop});
				All.Add({FString::Printf(TEXT("buttress.s%s"), Side), FVector2D(SX * Plan::ArcadeX, Plan::EndOuterY), FVector2D(0.0, 1.0), Plan::EndButtressTop});
			}
			return All;
		}

		/** Along the wall from a buttress's face point, so a leg out along the normal has the buttress's side to its right. */
		FVector2D AlongWall(const FVector2D& Normal) { return FVector2D(Normal.Y, -Normal.X); }

		/**
		 * One buttress: its body drawn in section (out from the wall, up) and pushed across its width, full depth to the
		 * set-off, weathered back to the upper stage, then steeply into the wall at its top; and its base course, 0.06 proud
		 * round its three faces, weathered from 0.52 to 0.60.
		 */
		void Buttress(const FButtress& B, TArray<FFormDrawing>& Out)
		{
			const FVector2D A = AlongWall(B.Out);
			const double H = Plan::ButtressHalf;
			// The section's plane (out from the wall, up) through the buttress's side at -A; its z, out × up, is +A.
			const FMuseeFrame Plane = FrameOfAxes(FVector(B.Face.X - A.X * H, B.Face.Y - A.Y * H, 0.0), FVector(B.Out.X, B.Out.Y, 0.0), FVector::UpVector);
			FFormExtrusion Body;
			Body.Outline = Section({FVector2D(0.0, Plan::FootingZ), FVector2D(Plan::ButtressDepth, Plan::FootingZ), FVector2D(Plan::ButtressDepth, Plan::ButtressSetOff),
				FVector2D(Plan::ButtressUpperDepth, Plan::ButtressSetOffTop), FVector2D(Plan::ButtressUpperDepth, B.Top - Plan::ButtressTopWeathering),
				FVector2D(0.0, B.Top)});
			Body.DepthMeters = 2.0 * H;
			Out.Add(Drawn(B.Name, TEXT("ext.buff"), MoveTemp(Body), Solid, Plane));

			const double D = Plan::ButtressDepth;
			FFormSweep Base;
			Base.Section = Section({FVector2D(0.0, Plan::FootingZ), FVector2D(Plan::BaseCourseProud, Plan::FootingZ),
				FVector2D(Plan::BaseCourseProud, Plan::BaseCourseTop), FVector2D(0.0, Plan::BaseCourseWeathered)});
			Base.Path = FFormProfile::Polyline({B.Face - A * H, B.Face - A * H + B.Out * D, B.Face + A * H + B.Out * D, B.Face + A * H}, false);
			Out.Add(Drawn(B.Name + TEXT(".base"), TEXT("ext.dressing"), MoveTemp(Base), Solid));
		}

		/**
		 * The plinth course round the court and the porch, from the port plane round to the port plane, stopped at each
		 * buttress's base course: the outer faces as a path run so the outside lies to its right, cut into its runs.
		 */
		void Plinth(TArray<FFormDrawing>& Out)
		{
			const double PX = Plan::PorchOuterX;
			const double SX = Plan::SideOuterX;
			const double EY = Plan::EndOuterY;
			const TArray<FVector2D> Faces = {FVector2D(-PX, Plan::PortPlaneY), FVector2D(-PX, -EY), FVector2D(-SX, -EY), FVector2D(-SX, EY),
				FVector2D(SX, EY), FVector2D(SX, -EY), FVector2D(PX, -EY), FVector2D(PX, Plan::PortPlaneY)};
			const double Stop = Plan::ButtressHalf + Plan::BaseCourseProud;
			const TArray<FButtress> All = Buttresses();
			TArray<TArray<FVector2D>> Runs;
			Runs.Add({Faces[0]});
			for (int32 Edge = 0; Edge + 1 < Faces.Num(); ++Edge)
			{
				const FVector2D From = Faces[Edge];
				const FVector2D Direction = (Faces[Edge + 1] - From).GetSafeNormal();
				const double Length = FVector2D::Distance(From, Faces[Edge + 1]);
				TArray<double> Stations;
				for (const FButtress& B : All)
				{
					const FVector2D Offset = B.Face - From;
					const double T = FVector2D::DotProduct(Direction, Offset);
					if (FMath::Abs(FVector2D::CrossProduct(Direction, Offset)) < 1e-6 && T >= 0.0 && T <= Length)
					{
						Stations.Add(T);
					}
				}
				Stations.Sort();
				for (const double T : Stations)
				{
					Runs.Last().Add(From + Direction * FMath::Max(0.0, T - Stop));
					Runs.Add({From + Direction * FMath::Min(Length, T + Stop)});
				}
				Runs.Last().Add(Faces[Edge + 1]);
			}
			const FFormProfile Course = SectionOf(Plan::PlinthPoints);
			int32 Index = 0;
			for (const TArray<FVector2D>& Run : Runs)
			{
				FFormSweep Shape;
				Shape.Section = Course;
				Shape.Path = FFormProfile::Polyline(Run, false);
				Out.Add(Drawn(FString::Printf(TEXT("plinth.%02d"), ++Index), TEXT("ext.dressing"), MoveTemp(Shape), Solid));
			}
		}

		/**
		 * The copings: two U runs, from the arcade line on the north round each side to the arcade line on the south,
		 * over the side and the end parapets, their section out over the outer face.
		 */
		void ParapetCopings(TArray<FFormDrawing>& Out)
		{
			const double X = Plan::SideOuterX;
			const double Y = Plan::EndOuterY;
			const double N = Plan::ArcadeX;
			const TArray<FVector2D> West = {FVector2D(-N, -Y), FVector2D(-X, -Y), FVector2D(-X, Y), FVector2D(-N, Y)};
			const TArray<FVector2D> East = {FVector2D(N, Y), FVector2D(X, Y), FVector2D(X, -Y), FVector2D(N, -Y)};
			for (const bool bWest : {true, false})
			{
				FFormSweep Shape;
				Shape.Section = SectionOf(Plan::CopingPoints);
				Shape.Path = FFormProfile::Polyline(bWest ? West : East, false);
				Out.Add(Drawn(bWest ? TEXT("coping.west") : TEXT("coping.east"), TEXT("ext.dressing"), MoveTemp(Shape), Overhead));
			}
		}

		/** The side gutters: lead on the court leaf's top from its face to the parapet, turned up the parapet 0.15. */
		void Gutters(TArray<FFormDrawing>& Out)
		{
			for (const bool bWest : {true, false})
			{
				const double S = bWest ? -1.0 : 1.0;
				const double Face = S * Plan::SideInnerX;
				const double Parapet = S * Plan::SideGlassX;
				const double Turn = Parapet - S * Plan::LeadMeters;
				FFormExtrusion Shape;
				Shape.Outline = Section({FVector2D(Face, Plan::Springing), FVector2D(Parapet, Plan::Springing),
					FVector2D(Parapet, Plan::Springing + Plan::GutterTurnUp), FVector2D(Turn, Plan::Springing + Plan::GutterTurnUp),
					FVector2D(Turn, Plan::SideGutterTop), FVector2D(Face, Plan::SideGutterTop)});
				Shape.DepthMeters = 2.0 * Plan::EndGlassY;
				Out.Add(Drawn(bWest ? TEXT("gutter.west") : TEXT("gutter.east"), TEXT("lead"), MoveTemp(Shape), Overhead, AcrossY(Plan::EndGlassY)));
			}
		}
	}

	void DrawOutside(TArray<FFormDrawing>& Out)
	{
		OuterSideLeaves(Out);
		OuterEndLeaves(Out);
		for (const FButtress& B : Buttresses())
		{
			Buttress(B, Out);
		}
		Plinth(Out);
		ParapetCopings(Out);
		Gutters(Out);
	}
}
