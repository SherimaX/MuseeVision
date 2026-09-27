#include "GroundsParts.h"
#include "SiteDescription.h"

/**
 * What the Grounds declares to the site and the systems (§1, §2): its Ground footprint, no ports (the site plan places it
 * at the anchor's datum), the one Outdoors enclosure under the museum's sky, and the anchors the systems furnish: the two
 * basins' water (Nature), the lamps' globes (Light), the lawn, hedges and trees (Nature) and the copings' seats (Visitor).
 */
namespace Musee::Grounds
{
	namespace
	{
		const FName Water(TEXT("Water"));
		const FName Mount(TEXT("Mount"));
		const FName Bed(TEXT("Bed"));
		const FName Seat(TEXT("Seat"));

		void Anchor(FSiteDescription& Out, const FString& Local, FName Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({Key(Local), Kind, Frame});
		}

		/** A frame on the plan at a point, its x turned to face a plan direction, z up. */
		FMuseeFrame FacingInPlan(const FVector& Origin, const FVector2D& Direction)
		{
			return FrameOfAxes(Origin, FVector(Direction.X, Direction.Y, 0.0), FVector(-Direction.Y, Direction.X, 0.0));
		}

		/** The 800 m square, from the soil's bottom to the tallest birch's sway; the one Ground stage. */
		void Footprint(FSiteDescription& Out)
		{
			const double Half = Plan::FootprintHalf;
			Out.Footprint.Kind = ESiteFootprintKind::Ground;
			Out.Footprint.OutlineMeters = {FVector2D(-Half, -Half), FVector2D(Half, -Half), FVector2D(Half, Half), FVector2D(-Half, Half)};
			Out.Footprint.BottomMeters = Plan::SoilBottom;
			Out.Footprint.TopMeters = Plan::StageTop;
		}

		/** Whether a seat on the canal's coping stands at least 0.60 clear of every lamp standard's base. */
		bool ClearOfLamps(const FVector2D& Point)
		{
			for (const Plan::FPlanPoint& Lamp : Plan::Lamps)
			{
				if (FVector2D::Distance(Point, At(Lamp)) < Plan::PostBaseRadius + Plan::SeatClear)
				{
					return false;
				}
			}
			return true;
		}

		/**
		 * The canal coping's seats: on its seat line, 0.25 in from the outer face at the coping's top, a frame every 0.80 set
		 * out from each side's middle, none within 0.60 of a corner or of a lamp's base; each faces out, away from the water. Each
		 * seat is a place of its own, canal.coping.seat.<n>, numbered round the canal from its north-west corner.
		 */
		void CanalSeats(FSiteDescription& Out)
		{
			const double In = Plan::SeatLineIn;
			const FVector2D Min(Plan::CanalWestX + In, Plan::CanalNorthY + In);
			const FVector2D Max(Plan::CanalEastX - In, Plan::CanalSouthY - In);
			const FVector2D Corners[] = {Min, FVector2D(Min.X, Max.Y), Max, FVector2D(Max.X, Min.Y)};
			int32 Seats = 0;
			for (int32 Side = 0; Side < 4; ++Side)
			{
				const FVector2D A = Corners[Side];
				const FVector2D B = Corners[(Side + 1) % 4];
				const double Half = 0.5 * FVector2D::Distance(A, B);
				const FVector2D Along = (B - A) / (2.0 * Half);
				// The side runs anticlockwise on the plan, so out of the canal is to its right, (-y, x) of its direction.
				const FVector2D Outward(-Along.Y, Along.X);
				const int32 Reach = FMath::FloorToInt32((Half - Plan::SeatClear) / Plan::SeatPitch + UE_KINDA_SMALL_NUMBER);
				for (int32 Step = -Reach; Step <= Reach; ++Step)
				{
					const FVector2D Point = A + Along * (Half + Plan::SeatPitch * Step);
					if (ClearOfLamps(Point))
					{
						Anchor(Out, FString::Printf(TEXT("canal.coping.seat.%d"), ++Seats), Seat, FacingInPlan(FVector(Point, Plan::CopingTop), Outward));
					}
				}
			}
		}

		/** The round basin's seats: evenly round the r 2.75 seat line, as many as fit at 0.80 or more apart, each facing out. */
		void BasinSeats(FSiteDescription& Out)
		{
			const FVector2D Centre = At(Plan::RondPoint);
			const int32 Count = FMath::FloorToInt32(2.0 * UE_DOUBLE_PI * Plan::BasinSeatRadius / Plan::SeatPitch);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const double Degrees = 360.0 * Index / Count;
				const FVector2D Point = OnCircle(Centre, Plan::BasinSeatRadius, Degrees);
				Anchor(Out, FString::Printf(TEXT("rond-point.coping.seat.%d"), Index + 1), Seat, FacingInPlan(FVector(Point, Plan::CopingTop),
					(Point - Centre).GetSafeNormal()));
			}
		}

		/** The plants' beds (§3.8, §7): the lawn on the terrain's top, the box loops, balls and yew exedra, and the trees. */
		void Beds(FSiteDescription& Out)
		{
			Anchor(Out, TEXT("lawn"), Bed, FMuseeFrame(FVector(0.0, 0.0, Plan::LawnTop)));
			// Each loop's anchor stands at its centre, halfway between opposite corners; Planting.json gives its line.
			Anchor(Out, TEXT("hedge.panel.west"), Bed, FMuseeFrame(FVector(0.5 * (At(Plan::LoopWest[0]) + At(Plan::LoopWest[2])), Plan::LawnTop)));
			Anchor(Out, TEXT("hedge.panel.east"), Bed, FMuseeFrame(FVector(0.5 * (At(Plan::LoopEast[0]) + At(Plan::LoopEast[2])), Plan::LawnTop)));
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::Balls)); ++Index)
			{
				Anchor(Out, FString::Printf(TEXT("hedge.ball.%02d"), Index + 1), Bed, FMuseeFrame(FVector(At(Plan::Balls[Index]), Plan::LawnTop)));
			}
			const FMuseeFrame Exedra(FVector(At(Plan::ExedraCentre), Plan::LawnTop));
			Anchor(Out, TEXT("exedra.east"), Bed, Exedra);
			Anchor(Out, TEXT("exedra.west"), Bed, Exedra);
			for (const Plan::FTree& Tree : Plan::Trees)
			{
				Anchor(Out, Tree.Local, Bed, FMuseeFrame(FVector(Tree.X, Tree.Y, Tree.bInGravel ? Plan::GravelTop : Plan::LawnTop)));
			}
		}

		/**
		 * The anchors (§1): the basins' water at their levels, the lamps' globes (their frames facing down, x to the foot),
		 * the beds and the seats. No Body, Use, Wall or Glazing: nothing moves, no works hang, no glass is stained.
		 */
		void Anchors(FSiteDescription& Out)
		{
			const double CanalMiddle = 0.5 * (Plan::CanalNorthY + Plan::CanalSouthY);
			Anchor(Out, TEXT("canal.water"), Water, FMuseeFrame(FVector(Plan::AxisX, CanalMiddle, Plan::WaterLevel)));
			Anchor(Out, TEXT("rond-point.water"), Water, FMuseeFrame(FVector(At(Plan::RondPoint), Plan::WaterLevel)));
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::Lamps)); ++Index)
			{
				const FVector Globe(At(Plan::Lamps[Index]), Plan::GlobeCentreZ);
				Anchor(Out, FString::Printf(TEXT("lamp.%02d"), Index + 1), Mount, FrameOfAxes(Globe, -FVector::UpVector, FVector(0.0, 1.0, 0.0)));
			}
			Beds(Out);
			CanalSeats(Out);
			BasinSeats(Out);
		}

		/** The one enclosure: the open ground under the museum's sky, bounded by nothing but the terrain it stands on. */
		void Enclosures(FSiteDescription& Out)
		{
			FFormEnclosure& Open = Out.Enclosures.AddDefaulted_GetRef();
			Open.Key = Key(TEXT("open"));
			Open.Kind = EFormEnclosureKind::Outdoors;
			Open.Pieces = {FName(Pieces::Terrain)};
		}
	}

	void DescribeSite(FSiteDescription& Out)
	{
		Footprint(Out);
		Anchors(Out);
		Enclosures(Out);
	}
}
