#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The floors (§3.6), all finished at z 0 on the 50 mm build-up over the Reserve: oak in each bay's field and in the
 * cabinet, honed travertine under every arch and in the cabinet door's reveal, the bronze strips where oak meets stone
 * and at the doors, and the sixteen viewing stones flush in the oak in their gilt rings.
 */
namespace Musee::Salon
{
	namespace
	{
		const double FloorBottom = -Plan::FloorBuildUp;

		/** The rings of the viewing stones that fall inside a rectangle of the plan: the holes they leave in its floor. */
		TArray<FFormProfile> StonesWithin(const FBox2D& Area)
		{
			TArray<FFormProfile> Holes;
			for (const Plan::FPoint& Stone : Plan::ViewingStones)
			{
				if (Area.IsInside(FVector2D(Stone.X, Stone.Y)))
				{
					Holes.Add(Circle(FVector2D(Stone.X, Stone.Y), Plan::ViewingRingRadius));
				}
			}
			return Holes;
		}

		/** The oak of each bay from pier face to pier face and wall to wall, and of the cabinet. */
		void Oak(TArray<FFormDrawing>& Out)
		{
			for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
			{
				const FBox2D Area(FVector2D(Plan::Bays[Bay].West, -Plan::SilkHalf), FVector2D(Plan::Bays[Bay].East, Plan::SilkHalf));
				Out.Add(PlanSlab(FString::Printf(TEXT("floor.bay%d"), Bay + 1), TEXT("floor.parquet"), FFormProfile::Rectangle(Area.Min, Area.Max),
					StonesWithin(Area), FloorBottom, 0.0, Walkable));
			}
			const FBox2D Cabinet(FVector2D(Plan::CabinetWest, Plan::CabinetNorth), FVector2D(Plan::CabinetEast, Plan::CabinetSouth));
			Out.Add(PlanSlab(Pieces::CabinetFloor, TEXT("floor.parquet"), FFormProfile::Rectangle(Cabinet.Min, Cabinet.Max), StonesWithin(Cabinet),
				FloorBottom, 0.0, Walkable));
		}

		/** The travertine under the arches, between the piers' jambs, and in the cabinet door's reveal. */
		void ArchStones(TArray<FFormDrawing>& Out)
		{
			for (int32 Index = 0; Index < Plan::PierCount; ++Index)
			{
				const double X = Plan::PierX[Index];
				Out.Add(Box(FString::Printf(TEXT("floor.arch.%d"), Index + 1), TEXT("floor.stone"), FVector(X - Plan::PierHalf, -Plan::JambHalf, FloorBottom),
					FVector(X + Plan::PierHalf, Plan::JambHalf, 0.0), Walkable));
			}
			const double Half = 0.5 * Plan::DoorWidth;
			Out.Add(Box(TEXT("floor.threshold.cabinet"), TEXT("floor.stone"), FVector(Plan::CabinetDoorX - Half, Plan::CabinetSouth, FloorBottom),
				FVector(Plan::CabinetDoorX + Half, -Plan::SilkHalf, 0.0), Walkable));
		}

		/** The viewing stones in their rings. */
		void ViewingStones(TArray<FFormDrawing>& Out)
		{
			int32 Index = 0;
			for (const Plan::FPoint& Stone : Plan::ViewingStones)
			{
				const FVector2D Centre(Stone.X, Stone.Y);
				const FString Name = FString::Printf(TEXT("floor.viewing-stone.%02d"), ++Index);
				Out.Add(PlanSlab(Name, TEXT("floor.viewing-stone"), Circle(Centre, Plan::ViewingStoneRadius), {}, FloorBottom, 0.0, Walkable));
				Out.Add(PlanSlab(Name + TEXT(".ring"), TEXT("floor.viewing-stone.ring"), Circle(Centre, Plan::ViewingRingRadius),
					{Circle(Centre, Plan::ViewingStoneRadius)}, FloorBottom, 0.0, Walkable));
			}
		}

		/**
		 * A strip 12 wide on a line of the plan, from 12 mm down to half a millimetre proud: centred on it, or, at a port,
		 * wholly on the room's side (Toward, -1 or +1 across the line), for the joint's threshold runs through the wall to it.
		 */
		void Strip(TArray<FFormDrawing>& Out, const FString& Name, const FVector2D& From, const FVector2D& To, double Toward = 0.0)
		{
			const double H = 0.5 * Plan::StripWidth;
			const FVector2D Shift = Toward * H * (From.X == To.X ? FVector2D(1.0, 0.0) : FVector2D(0.0, 1.0));
			const FVector2D Min = Shift + FVector2D(FMath::Min(From.X, To.X) - (From.X == To.X ? H : 0.0), FMath::Min(From.Y, To.Y) - (From.Y == To.Y ? H : 0.0));
			const FVector2D Max = Shift + FVector2D(FMath::Max(From.X, To.X) + (From.X == To.X ? H : 0.0), FMath::Max(From.Y, To.Y) + (From.Y == To.Y ? H : 0.0));
			Out.Add(Box(Name, TEXT("floor.strip"), FVector(Min.X, Min.Y, Plan::StripProud - Plan::StripDepth), FVector(Max.X, Max.Y, Plan::StripProud),
				Walkable));
		}

		/** The strips: along both faces of every arch's stone, at both port doors' thresholds, and either side of the cabinet door. */
		void Strips(TArray<FFormDrawing>& Out)
		{
			const double J = Plan::JambHalf;
			for (int32 Index = 0; Index < Plan::PierCount; ++Index)
			{
				for (const double Side : {-1.0, 1.0})
				{
					const double X = Plan::PierX[Index] + Side * Plan::PierHalf;
					Strip(Out, FString::Printf(TEXT("floor.strip.arch.%d.%s"), Index + 1, Side < 0.0 ? TEXT("west") : TEXT("east")), {X, -J}, {X, J});
				}
			}
			const double Half = 0.5 * Plan::DoorWidth;
			// The port doors' strips lie in the bays, oak to the joint's threshold at the wall's inner face (§2).
			Strip(Out, TEXT("floor.strip.east"), {Plan::EndSilkX, -Half}, {Plan::EndSilkX, Half}, -1.0);
			Strip(Out, TEXT("floor.strip.west"), {Plan::FarSilkX, -Half}, {Plan::FarSilkX, Half}, 1.0);
			const double X0 = Plan::CabinetDoorX - Half;
			const double X1 = Plan::CabinetDoorX + Half;
			Strip(Out, TEXT("floor.strip.cabinet.bay"), {X0, -Plan::SilkHalf}, {X1, -Plan::SilkHalf});
			Strip(Out, TEXT("floor.strip.cabinet.cabinet"), {X0, Plan::CabinetSouth}, {X1, Plan::CabinetSouth});
		}
	}

	void DrawFloors(TArray<FFormDrawing>& Out)
	{
		Oak(Out);
		ArchStones(Out);
		ViewingStones(Out);
		Strips(Out);
	}
}
