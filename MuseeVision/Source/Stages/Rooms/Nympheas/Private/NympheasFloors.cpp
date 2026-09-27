#include "NympheasParts.h"
#include "NympheasPlan.h"
#include "SitePortClass.h"

/**
 * The floors (§3.4) and the well's head (§3.7): the honed travertine over the whole oval and the passage, with the two
 * viewing stones in their gilt rings; the curb round the pond's opening, carrying the guard rails' chases and the posts'
 * sleeves; the slab over the shaft east of the opening, whose soffit is the well's ceiling; the east rail's housing.
 */
namespace Musee::Nympheas
{
	namespace
	{
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** The rails' plan lines along the opening, stopped either side of the posts (§3.6), west to east. */
		TArray<TPair<double, double>> RailStretches()
		{
			const double Stop = Plan::RailStopAtPost;
			return {{Plan::RailWestX, -Plan::PostX - Stop}, {-Plan::PostX + Stop, Plan::PostX - Stop}, {Plan::PostX + Stop, Plan::RailEastX}};
		}

		/** The chases along the north side (y negative): the stretches inside the curb, as holes; the last runs out at its east end. */
		TArray<FFormProfile> NorthChases()
		{
			const double Inner = -Plan::RailY + 0.5 * Plan::ChaseWidth;
			const double Outer = -Plan::RailY - 0.5 * Plan::ChaseWidth;
			TArray<FFormProfile> Holes;
			const TArray<TPair<double, double>> Stretches = RailStretches();
			for (int32 Index = 0; Index + 1 < Stretches.Num(); ++Index)
			{
				Holes.Add(FFormProfile::Rectangle(FVector2D(Stretches[Index].Key, Outer), FVector2D(Stretches[Index].Value, Inner)));
			}
			return Holes;
		}

		/** The north side's outline over the shaft wall, notched where the last chase runs out at the slab's edge. */
		TArray<FVector2D> NorthBand()
		{
			const double Inner = -Plan::RailY + 0.5 * Plan::ChaseWidth;
			const double Outer = -Plan::RailY - 0.5 * Plan::ChaseWidth;
			const double Notch = RailStretches().Last().Key;
			const double East = Plan::OpeningEastX;
			return {FVector2D(Plan::OpeningWestX, -Plan::ShaftOuterHalf), FVector2D(East, -Plan::ShaftOuterHalf), FVector2D(East, Outer),
				FVector2D(Notch, Outer), FVector2D(Notch, Inner), FVector2D(East, Inner), FVector2D(East, -Plan::ShaftHalf),
				FVector2D(Plan::OpeningWestX, -Plan::ShaftHalf)};
		}

		/** The posts' sleeves on the north side. */
		TArray<FFormProfile> NorthSleeves()
		{
			return {Circle(FVector2D(-Plan::PostX, -Plan::PostY), 0.5 * Plan::SleeveDiameter),
				Circle(FVector2D(Plan::PostX, -Plan::PostY), 0.5 * Plan::SleeveDiameter)};
		}

		TArray<FFormProfile> Mirrored(const TArray<FFormProfile>& Holes)
		{
			TArray<FFormProfile> Out;
			for (const FFormProfile& Hole : Holes)
			{
				FFormProfile Flipped = Hole;
				Flipped.Start[1] = -Flipped.Start[1];
				for (FFormProfileStep& Step : Flipped.Steps)
				{
					if (Step.IsArc())
					{
						Step.Arc.CenterMeters[1] = -Step.Arc.CenterMeters[1];
						Step.Arc.FromDegrees = -Step.Arc.FromDegrees;
						Step.Arc.ToDegrees = -Step.Arc.ToDegrees;
					}
					else
					{
						Step.Line[1] = -Step.Line[1];
					}
				}
				Out.Add(MoveTemp(Flipped));
			}
			return Out;
		}

		FFormProfile EastChase()
		{
			const double Half = 0.5 * Plan::ChaseWidth;
			return FFormProfile::Rectangle(FVector2D(Plan::EastRailX - Half, -Plan::EastRailHalf), FVector2D(Plan::EastRailX + Half, Plan::EastRailHalf));
		}

		/**
		 * The floor's outline: the footprint, stopped at the port face across the passage half the class's strip short, so the
		 * joint's bronze strip lies on the mating plane between the two stages' floors.
		 */
		TArray<FVector2D> FloorOutline()
		{
			const TValueOrError<FSitePortClass, FString> Door = Musee::Site::FindPortClass(FMuseeKey(Plan::EastClass));
			TArray<FVector2D> Outline = FootprintOutline();
			if (Door.HasValue() && Door.GetValue().StripMeters > 0.0)
			{
				const double Half = 0.5 * Door.GetValue().ClearWidthMeters + Door.GetValue().AllowanceMeters;
				const double Short = Plan::PortFaceX - 0.5 * Door.GetValue().StripMeters;
				Outline.Append({FVector2D(Plan::PortFaceX, -Half), FVector2D(Short, -Half), FVector2D(Short, Half), FVector2D(Plan::PortFaceX, Half)});
			}
			return Outline;
		}

		/** The finished floor: the travertine over the footprint, round the opening, the viewing stones and the east chase. */
		void Floor(TArray<FFormDrawing>& Out)
		{
			TArray<FFormProfile> Holes = {FFormProfile::Rectangle(FVector2D(Plan::OpeningWestX, -Plan::ShaftOuterHalf),
				FVector2D(Plan::OpeningEastX, Plan::ShaftOuterHalf)), EastChase()};
			for (const double X : {-Plan::ViewingStoneX, Plan::ViewingStoneX})
			{
				Holes.Add(Circle(FVector2D(X, 0.0), Plan::ViewingRingRadius));
			}
			const FMuseeFrame Bed(FVector(0.0, 0.0, -Plan::FloorStone));
			Out.Add(Drawn(Pieces::Floor, TEXT("floor"), Slab(Polygon(FloorOutline()), MoveTemp(Holes), -Plan::FloorStone, 0.0), Walkable, Bed));
			for (const double X : {-Plan::ViewingStoneX, Plan::ViewingStoneX})
			{
				const FVector2D Centre(X, 0.0);
				const TCHAR* Side = X < 0.0 ? TEXT("west") : TEXT("east");
				Out.Add(Drawn(*FString::Printf(TEXT("floor.viewing-stone.%s"), Side), TEXT("floor.viewing-stone"),
					Slab(Circle(Centre, Plan::ViewingStoneRadius), {}, -Plan::FloorStone, 0.0), Walkable, Bed));
				Out.Add(Drawn(*FString::Printf(TEXT("floor.viewing-stone.%s.ring"), Side), TEXT("floor.viewing-stone.ring"),
					Slab(Circle(Centre, Plan::ViewingRingRadius), {Circle(Centre, Plan::ViewingStoneRadius)}, -Plan::FloorStone, 0.0), Walkable, Bed));
			}
		}

		/** The curb on the shaft's side walls along the opening, pierced by the chases and the sleeves. */
		void Curbs(TArray<FFormDrawing>& Out)
		{
			TArray<FFormProfile> Holes = NorthChases();
			Holes.Append(NorthSleeves());
			const FMuseeFrame Bed(FVector(0.0, 0.0, -Plan::CurbDepth));
			Out.Add(Drawn(Pieces::CurbNorth, TEXT("well.curb"), Slab(Polygon(NorthBand()), Holes, -Plan::CurbDepth, 0.0), Walkable, Bed));
			Out.Add(Drawn(Pieces::CurbSouth, TEXT("well.curb"), Slab(Polygon(MirroredInY(NorthBand())), Mirrored(Holes), -Plan::CurbDepth, 0.0),
				Walkable, Bed));
		}

		/**
		 * The shaft's side walls under the curb: the bands the chases and sleeves pass through (the sleeves end 1.00 down,
		 * the chases 1.05), then the brick below them, and past the opening's ends the wall up to the floor or the slab.
		 */
		void ShaftWalls(TArray<FFormDrawing>& Out)
		{
			const double SleeveFoot = -Plan::SleeveDepth;
			const double ChaseFoot = -Plan::ChaseDepth;
			for (const bool bNorth : {true, false})
			{
				const TCHAR* Side = bNorth ? TEXT("north") : TEXT("south");
				const double Sign = bNorth ? -1.0 : 1.0;
				TArray<FFormProfile> Chases = bNorth ? NorthChases() : Mirrored(NorthChases());
				TArray<FFormProfile> Both = Chases;
				Both.Append(bNorth ? NorthSleeves() : Mirrored(NorthSleeves()));
				const FFormProfile Band = Polygon(bNorth ? NorthBand() : MirroredInY(NorthBand()));
				auto Name = [Side](const TCHAR* Part) { return FString::Printf(TEXT("well.wall.%s.%s"), Side, Part); };
				Out.Add(Drawn(*Name(TEXT("sleeves")), TEXT("well.wall"), Slab(Band, Both, SleeveFoot, -Plan::CurbDepth), Solid,
					FMuseeFrame(FVector(0.0, 0.0, SleeveFoot))));
				Out.Add(Drawn(*Name(TEXT("chases")), TEXT("well.wall"), Slab(Band, Chases, ChaseFoot, SleeveFoot), Solid,
					FMuseeFrame(FVector(0.0, 0.0, ChaseFoot))));
				const FVector2D Wall(Sign * Plan::ShaftHalf, Sign * Plan::ShaftOuterHalf);
				const double Low = FMath::Min(Wall.X, Wall.Y);
				const double High = FMath::Max(Wall.X, Wall.Y);
				Out.Add(Drawn(*Name(TEXT("lower")), TEXT("well.wall"), Block(FVector(Plan::EndWallWestX, Low, Plan::WallBed),
					FVector(Plan::PortFaceX, High, ChaseFoot)), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::WallBed))));
				Out.Add(Drawn(*Name(TEXT("west")), TEXT("well.wall"), Block(FVector(Plan::EndWallWestX, Low, ChaseFoot),
					FVector(Plan::OpeningWestX, High, -Plan::FloorStone)), Solid, FMuseeFrame(FVector(0.0, 0.0, ChaseFoot))));
				Out.Add(Drawn(*Name(TEXT("east")), TEXT("well.wall"), Block(FVector(Plan::OpeningEastX, Low, ChaseFoot),
					FVector(Plan::PortFaceX, High, Plan::SlabSoffit)), Solid, FMuseeFrame(FVector(0.0, 0.0, ChaseFoot))));
			}
		}

		/**
		 * The slab over the shaft east of the opening, under the floor's stone, pierced by the east chase; and the east
		 * rail's closed bronze housing below its soffit, its sides the chase's walls and its bottom closing it.
		 */
		void SlabAndHousing(TArray<FFormDrawing>& Out)
		{
			const FFormProfile Over = FFormProfile::Rectangle(FVector2D(Plan::OpeningEastX, -Plan::ShaftOuterHalf),
				FVector2D(Plan::PortFaceX, Plan::ShaftOuterHalf));
			Out.Add(Drawn(Pieces::WellSlab, TEXT("well.soffit"), Slab(Over, {EastChase()}, Plan::SlabSoffit, -Plan::FloorStone), Solid,
				FMuseeFrame(FVector(0.0, 0.0, Plan::SlabSoffit))));

			// The housing's sides are as thick as the space either side of the chase within its width; its bottom as thick again.
			const double Side = 0.5 * (Plan::HousingWidth - Plan::ChaseWidth);
			const double Foot = Plan::SlabSoffit - Plan::HousingDepth;
			const FVector2D Min(Plan::EastRailX - 0.5 * Plan::HousingWidth, -Plan::EastRailHalf - Side);
			const FVector2D Max(Plan::EastRailX + 0.5 * Plan::HousingWidth, Plan::EastRailHalf + Side);
			Out.Add(Drawn(TEXT("guard.housing.east"), TEXT("guard.housing"), Slab(FFormProfile::Rectangle(Min, Max), {EastChase()}, Foot + Side,
				Plan::SlabSoffit), Solid, FMuseeFrame(FVector(0.0, 0.0, Foot + Side))));
			Out.Add(Drawn(TEXT("guard.housing.east.bottom"), TEXT("guard.housing"), Block(FVector(Min.X, Min.Y, Foot), FVector(Max.X, Max.Y, Foot + Side)),
				Solid, FMuseeFrame(FVector(0.0, 0.0, Foot))));
		}
	}

	namespace Pieces
	{
		TArray<FName> ShaftWalls()
		{
			TArray<FName> Names;
			for (const TCHAR* Side : {TEXT("north"), TEXT("south")})
			{
				for (const TCHAR* Part : {TEXT("sleeves"), TEXT("chases"), TEXT("lower"), TEXT("west"), TEXT("east")})
				{
					Names.Add(FName(FString::Printf(TEXT("well.wall.%s.%s"), Side, Part)));
				}
			}
			return Names;
		}
	}

	void DrawFloors(TArray<FFormDrawing>& Out)
	{
		Floor(Out);
		Curbs(Out);
		ShaftWalls(Out);
		SlabAndHousing(Out);
	}
}
