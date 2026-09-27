#include "ReserveParts.h"
#include "ReservePlan.h"

/**
 * The piers and responds (§3.3): fourteen piers on the arcade lines, each a square-splayed travertine base, a chamfered
 * brick shaft and a square-splayed travertine impost whose top is every arch's springing; and eighteen responds where a
 * band springs from a wall, a brick pilaster on a two-stage base under a solid impost block. The bases stand on the
 * flags' bed; the flags are cut round them.
 */
namespace Musee::Reserve
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** A square-splayed section (half-width, z) turned round a square, closed to the axis at its ends. */
		FFormLathe SquareSplayed(TConstArrayView<FVector2D> Edge)
		{
			TArray<FVector2D> Points = {FVector2D(0.0, Edge[0].Y)};
			Points.Append(Edge.GetData(), Edge.Num());
			Points.Add(FVector2D(0.0, Edge.Last().Y));
			return Turned(Points, 4);
		}

		void Pier(TArray<FFormDrawing>& Out, int32 Line, double Side)
		{
			const FVector At(PierX(Line), Side * Plan::ArcadeY, 0.0);
			const FString Name = FString::Printf(TEXT("pier.%d%s"), Line + 1, Side < 0.0 ? TEXT("n") : TEXT("s"));
			// The base's plumb foot runs on down through the flags to their bed.
			TArray<FVector2D> Base = {FVector2D(Plan::BaseSection[0][0], -Plan::FlagThickness)};
			for (int32 Index = 1; Index < int32(UE_ARRAY_COUNT(Plan::BaseSection)); ++Index)
			{
				Base.Add(FVector2D(Plan::BaseSection[Index][0], Plan::BaseSection[Index][1]));
			}
			Out.Add(Drawn(Name + TEXT(".base"), TEXT("stone.base"), SquareSplayed(Base), Solid, FMuseeFrame(At)));

			const double H = Plan::ShaftHalf;
			const double C = H - Plan::ShaftChamfer;
			FFormExtrusion Shaft;
			Shaft.Outline = FFormProfile::Polygon({FVector2D(-C, -H), FVector2D(C, -H), FVector2D(H, -C), FVector2D(H, C), FVector2D(C, H), FVector2D(-C, H),
				FVector2D(-H, C), FVector2D(-H, -C)});
			Shaft.DepthMeters = Plan::ImpostBottom - Plan::BaseTop;
			Out.Add(Drawn(Name + TEXT(".shaft"), TEXT("pier.brick"), MoveTemp(Shaft), Solid, FMuseeFrame(At + FVector(0.0, 0.0, Plan::BaseTop))));

			TArray<FVector2D> Impost;
			for (const auto& Point : Plan::ImpostSection)
			{
				Impost.Add(FVector2D(Point[0], Point[1]));
			}
			Out.Add(Drawn(Name + TEXT(".impost"), TEXT("stone.impost"), SquareSplayed(Impost), Solid, FMuseeFrame(At)));
		}

		/**
		 * A block of a respond, drawn in its wall's frame: a section in (d out of the wall, z) pushed along the wall, centred
		 * on the respond's line, Half either side.
		 */
		FFormDrawing RespondBlock(const FString& Name, const TCHAR* LocalSurface, const FVector& Face, const FVector& Out, double Half,
			TArray<FVector2D> Section)
		{
			const FVector Along = FVector::CrossProduct(Out, FVector::UpVector);
			FFormExtrusion Shape;
			Shape.Outline = FFormProfile::Polygon(Section);
			Shape.DepthMeters = 2.0 * Half;
			return Drawn(Name, LocalSurface, MoveTemp(Shape), Solid, FrameOfAxes(Face - Along * Half, Out, FVector::UpVector));
		}

		/** A respond on a wall's face at a point, standing out along Out: its base in two stages, pilaster and impost. */
		void Respond(TArray<FFormDrawing>& Out, const FString& Name, const FVector& Face, const FVector& Outward)
		{
			const auto Box = [](double D, double Z0, double Z1) { return TArray<FVector2D>{{0.0, Z0}, {D, Z0}, {D, Z1}, {0.0, Z1}}; };
			Out.Add(RespondBlock(Name + TEXT(".base"), TEXT("stone.base"), Face, Outward, Plan::RespondBaseHalf,
				Box(Plan::RespondBaseD, -Plan::FlagThickness, Plan::RespondBaseTop)));
			Out.Add(RespondBlock(Name + TEXT(".plinth"), TEXT("stone.base"), Face, Outward, Plan::RespondPlinthHalf,
				Box(Plan::RespondPlinthD, Plan::RespondBaseTop, Plan::BaseTop)));
			Out.Add(RespondBlock(Name + TEXT(".pilaster"), TEXT("wall.brick"), Face, Outward, Plan::PilasterHalf,
				Box(Plan::PilasterD, Plan::BaseTop, Plan::ImpostBottom)));
			TArray<FVector2D> Impost;
			for (const auto& Point : Plan::RespondImpostSection)
			{
				Impost.Add(FVector2D(Point[0], Point[1]));
			}
			Out.Add(RespondBlock(Name + TEXT(".impost"), TEXT("stone.impost"), Face, Outward, Plan::RespondBaseHalf, Impost));
		}
	}

	void DrawPiers(TArray<FFormDrawing>& Out)
	{
		for (int32 Line = 0; Line < Plan::PierLines; ++Line)
		{
			for (const double Side : {-1.0, 1.0})
			{
				Pier(Out, Line, Side);
				Respond(Out, FString::Printf(TEXT("respond.%d%s"), Line + 1, Side < 0.0 ? TEXT("n") : TEXT("s")),
					FVector(PierX(Line), Side * Plan::InnerY, 0.0), FVector(0.0, -Side, 0.0));
			}
		}
		for (const double End : {-1.0, 1.0})
		{
			for (const double Side : {-1.0, 1.0})
			{
				Respond(Out, FString::Printf(TEXT("respond.%s%s"), End < 0.0 ? TEXT("w") : TEXT("e"), Side < 0.0 ? TEXT("n") : TEXT("s")),
					FVector(End * Plan::InnerX, Side * Plan::ArcadeY, 0.0), FVector(-End, 0.0, 0.0));
			}
		}
	}
}
