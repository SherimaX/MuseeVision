#include "HallOfLightEnds.h"
#include "HallOfLightParts.h"
#include "HallOfLightPlan.h"

/**
 * The glass walls (sec. 3.2, 3.3), north and south, on the plinths: ten pearl-steel posts and an end post on each
 * lining, the glazing shoe on the plinth top and the transom inside the glass, both broken at the posts, the box eaves
 * over it all, and eleven panes of laminate, one per post bay, standing in the shoe and bearing on the transom.
 */
namespace Musee::HallOfLight
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** The number of bays along a wall between its end posts: one more than the posts. */
		constexpr int32 WallBays = Plan::Posts + 1;

		FRunLimit Lining(EHallEnd End, double Shift = 0.0)
		{
			return FRunLimit::OnFace(InnerFace(End, Shift));
		}

		/**
		 * The limits of one bay along a wall (0 from the west end post to post 0, ... 10 from post 9 to the east end post),
		 * for a member that stops Clear short of a post's centre and Gap short of an end post's face.
		 */
		TPair<FRunLimit, FRunLimit> BayLimits(int32 Bay, double Clear, double Gap)
		{
			const double AtEndPost = Plan::EndPostAlong + Gap;
			const FRunLimit West = Bay == 0 ? Lining(EHallEnd::West, AtEndPost) : FRunLimit::OnPlane(PostX(Bay - 1) + Clear);
			const FRunLimit East = Bay == WallBays - 1 ? Lining(EHallEnd::East, AtEndPost) : FRunLimit::OnPlane(PostX(Bay) - Clear);
			return {West, East};
		}

		/** The posts: an I standing on the plinth to the eaves' underside, its web through the glass line. */
		void Posts(TArray<FFormDrawing>& Out, int32 Side)
		{
			for (int32 Post = 0; Post < Plan::Posts; ++Post)
			{
				TArray<FVector2D> Outline;
				for (const Plan::FSectionPoint& Point : Plan::PostSection)
				{
					Outline.Add(FVector2D(PostX(Post) + Point.A, WallY(Side, Point.B)));
				}
				FFormExtrusion Shape;
				Shape.Outline = FFormProfile::Polygon(Outline);
				Shape.DepthMeters = Plan::PostHead - Plan::PostFoot;
				Out.Add(Drawn(FString::Printf(TEXT("post.%s.%d"), SideName(Side), Post), Surface(TEXT("wall.steel")), MoveTemp(Shape), Solid,
					FMuseeFrame(FVector(0.0, 0.0, Plan::PostFoot))));
			}
		}

		/** The end posts, fixed to the linings: their backs on the lining's face, their fronts 0.070 in from it. */
		void EndPosts(TArray<FFormDrawing>& Out, int32 Side)
		{
			const double Y0 = WallY(Side, Plan::EndPostFromD);
			const double Y1 = WallY(Side, Plan::EndPostToD);
			const FName Steel = Surface(TEXT("wall.steel"));
			const FString Name = FString::Printf(TEXT("end-post.%s"), SideName(Side));
			Out.Add(PlanRun(Name + TEXT(".west"), Steel, Lining(EHallEnd::West), Lining(EHallEnd::West, Plan::EndPostAlong), Y0, Y1, Plan::PostFoot,
				Plan::PostHead, Solid));
			Out.Add(PlanRun(Name + TEXT(".east"), Steel, Lining(EHallEnd::East, Plan::EndPostAlong), Lining(EHallEnd::East), Y0, Y1, Plan::PostFoot,
				Plan::PostHead, Solid));
		}

		/**
		 * The glazing shoes: a U of three plates on the plinth top, the bed under the glass and a cheek either side of it,
		 * in lengths between the posts (which stand on the plinth through it) and the end posts.
		 */
		void Shoes(TArray<FFormDrawing>& Out, int32 Side)
		{
			const double Half = Plan::ShoeHalf;
			const double Cheek = Plan::ShoeCheekInner;
			const struct
			{
				const TCHAR* Name;
				double FromD;
				double ToD;
				double Bottom;
				double Top;
			} Plates[] = {{TEXT("bed"), -Half, Half, Plan::ShoeBottom, Plan::ShoeBed}, {TEXT("outer"), -Half, -Cheek, Plan::ShoeBed, Plan::ShoeTop},
				{TEXT("inner"), Cheek, Half, Plan::ShoeBed, Plan::ShoeTop}};
			for (int32 Bay = 0; Bay < WallBays; ++Bay)
			{
				const TPair<FRunLimit, FRunLimit> Limits = BayLimits(Bay, Plan::PostHalf, 0.0);
				for (const auto& Plate : Plates)
				{
					const FString Name = FString::Printf(TEXT("shoe.%s.%d.%s"), SideName(Side), Bay, Plate.Name);
					Out.Add(PlanRun(Name, Surface(TEXT("wall.steel")), Limits.Key, Limits.Value, WallY(Side, Plate.FromD), WallY(Side, Plate.ToD),
						Plate.Bottom, Plate.Top, Solid));
				}
			}
		}

		/** The transoms inside the glass, the panes bearing on them through a gasket, in lengths between the post webs. */
		void Transoms(TArray<FFormDrawing>& Out, int32 Side)
		{
			for (int32 Bay = 0; Bay < WallBays; ++Bay)
			{
				const TPair<FRunLimit, FRunLimit> Limits = BayLimits(Bay, Plan::PostWebHalf, 0.0);
				Out.Add(PlanRun(FString::Printf(TEXT("transom.%s.%d"), SideName(Side), Bay), Surface(TEXT("wall.steel")), Limits.Key, Limits.Value,
					WallY(Side, Plan::TransomFromD), WallY(Side, Plan::TransomToD), Plan::TransomBottom, Plan::TransomTop, nullptr));
			}
		}

		/** The panes, one per bay, centred on the glass line, gasketed against the post webs and the end posts. */
		void Panes(TArray<FFormDrawing>& Out, int32 Side)
		{
			const double Half = 0.5 * Plan::GlassMeters;
			for (int32 Bay = 0; Bay < WallBays; ++Bay)
			{
				const TPair<FRunLimit, FRunLimit> Limits = BayLimits(Bay, Plan::PostWebHalf + Plan::GasketMeters, Plan::GasketMeters);
				FFormDrawing Pane = PlanRun(FString::Printf(TEXT("wall.glass.%s.%d"), SideName(Side), Bay), Surface(TEXT("wall.glass")), Limits.Key,
					Limits.Value, WallY(Side, -Half), WallY(Side, Half), Plan::WallGlassFoot, Plan::WallGlassHead, Solid);
				Pane.bNanite = false;
				Out.Add(MoveTemp(Pane));
			}
		}

		/** The eaves box, lining to lining; the arches and the vault's panes bear on its top. */
		void Eaves(TArray<FFormDrawing>& Out, int32 Side)
		{
			TArray<FVector2D> Section;
			for (const Plan::FSectionPoint& Point : Plan::EavesSection)
			{
				Section.Add(FVector2D(WallY(Side, Point.A), Point.B));
			}
			SectionRun(Out, FString::Printf(TEXT("eaves.%s"), SideName(Side)), Surface(TEXT("wall.steel")), Section, Lining(EHallEnd::West),
				Lining(EHallEnd::East), nullptr);
		}
	}

	void DrawWalls(TArray<FFormDrawing>& Out)
	{
		for (const int32 Side : Sides)
		{
			Posts(Out, Side);
			EndPosts(Out, Side);
			Shoes(Out, Side);
			Transoms(Out, Side);
			Panes(Out, Side);
			Eaves(Out, Side);
		}
	}
}
