#include "SalonParts.h"
#include "SalonPlan.h"
#include "SiteDescription.h"

/**
 * What the Salon declares to the site and the systems (§1, §2): its footprint, its two ports, the openings in its
 * shells (the five lanterns' weather glazing, the ports' rough openings, the cabinet's door), the anchors the systems
 * furnish (the hanging walls and the stele, the case and the dancer's plinth, the benches' seats, the pergola's
 * planters), and its three enclosures with the pieces that bound them.
 */
namespace Musee::Salon
{
	namespace
	{
		void Footprint(FSiteDescription& Out)
		{
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			for (const Plan::FPoint& Point : Plan::Footprint)
			{
				Out.Footprint.OutlineMeters.Add(FVector2D(Point.X, Point.Y));
			}
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;
		}

		/** A frame on the floor plan at a point, its x turned to face a direction, z up. */
		FMuseeFrame FacingInPlan(const FVector& Origin, const FVector& Direction)
		{
			return FMuseeFrame(Origin, FRotationMatrix::MakeFromX(Direction).ToQuat());
		}

		/**
		 * The ports: the east door on bay 1's end wall, the Rotunda's arch (its class), which steps down inside the wall to
		 * the Salon's own door; and the west door through bay 5's far wall into the oval's passage. The Salon draws the P5
		 * architrave on the bay face round both (§2, §3.3), and finishes the east door itself behind the step, the inner
		 * 0.300 of the wall, where the joint's lining stops against the step (§2, the step down).
		 */
		void Ports(FSiteDescription& Out)
		{
			FSitePort& East = Out.Ports.AddDefaulted_GetRef();
			East.Key = Key(TEXT("east"));
			East.Kind = ESitePortKind::Doorway;
			East.Class = FMuseeKey(Plan::EastClass);
			East.Frame = FacingInPlan(FVector(Plan::EastPortX, 0.0, 0.0), FMuseeFrame::East());
			East.WallDepthMeters = Plan::PortWallDepth;
			East.bOwnSurround = true;
			East.OwnLiningMeters = Plan::EastStepX - Plan::EndSilkX;
			East.Mates = FMuseeKey(Plan::EastMates);

			FSitePort& West = Out.Ports.AddDefaulted_GetRef();
			West.Key = Key(TEXT("west"));
			West.Kind = ESitePortKind::Doorway;
			West.Class = FMuseeKey(Plan::WestClass);
			West.Frame = FacingInPlan(FVector(Plan::WestPortX, 0.0, 0.0), FMuseeFrame::West());
			West.WallDepthMeters = Plan::PortWallDepth;
			West.bOwnSurround = true;
			West.Mates = FMuseeKey(Plan::WestMates);
		}

		FFormOpening& Opening(FSiteDescription& Out, const FString& Local, EFormOpeningKind Kind, const TCHAR* Enclosure, const FMuseeFrame& Frame,
			const FVector2D& Size)
		{
			FFormOpening& Made = Out.Openings.AddDefaulted_GetRef();
			Made.Key = Key(*Local);
			Made.Kind = Kind;
			Made.Enclosure = Key(Enclosure);
			Made.Frame = Frame;
			Made.SizeMeters = Size;
			return Made;
		}

		/**
		 * The openings: each lantern's weather glazing on its kerb, looking up; the ports' openings, the classes' clear
		 * openings on the port faces, looking out; and the cabinet's door, its one opening, looking out into bay 1.
		 */
		void Openings(FSiteDescription& Out)
		{
			const FVector Up = FMuseeFrame::Up();
			for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
			{
				const FBox2D L = Lantern(Bay);
				const FVector2D Centre = L.GetCenter();
				Opening(Out, FString::Printf(TEXT("bay%d.lantern"), Bay + 1), EFormOpeningKind::Glazed, TEXT("bays"),
					FMuseeFrame(FVector(Centre.X, Centre.Y, Plan::KerbTop)), L.GetSize()).Glazing = Surface(TEXT("lantern.outer.glass"));
			}
			const double Width = Plan::DoorWidth;
			const double EastCrown = Plan::EastClassSpringing + 0.5 * Width;
			const double Crown = Plan::DoorSpringing + 0.5 * Width;
			Opening(Out, TEXT("east"), EFormOpeningKind::Port, TEXT("bays"), FrameOfAxes(FVector(Plan::EastPortX, 0.0, 0.5 * EastCrown), FMuseeFrame::South(), Up),
				FVector2D(Width, EastCrown));
			Opening(Out, TEXT("west"), EFormOpeningKind::Port, TEXT("bays"), FrameOfAxes(FVector(Plan::WestPortX, 0.0, 0.5 * Crown), FMuseeFrame::North(), Up),
				FVector2D(Width, Crown));
			Opening(Out, TEXT("cabinet.door"), EFormOpeningKind::Open, TEXT("cabinet"),
				FrameOfAxes(FVector(Plan::CabinetDoorX, Plan::CabinetSouth, 0.5 * Crown), FMuseeFrame::West(), Up), FVector2D(Width, Crown));
		}

		void Anchor(FSiteDescription& Out, const FString& Local, const TCHAR* Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({Key(*Local), FName(Kind), Frame});
		}

		/** A hanging wall: origin on its face at its left end as the room sees it, x along it, y out of it into the room. */
		void WallAnchor(FSiteDescription& Out, const FString& Local, const FVector2D& Origin, const FVector& X, const FVector& Y)
		{
			Anchor(Out, Local, TEXT("Wall"), FrameOfAxes(FVector(Origin.X, Origin.Y, 0.0), X, Y));
		}

		void WallAnchors(FSiteDescription& Out)
		{
			const FVector N = FMuseeFrame::North();
			const FVector S = FMuseeFrame::South();
			const FVector E = FMuseeFrame::East();
			const FVector W = FMuseeFrame::West();
			const double Silk = Plan::SilkHalf;
			for (int32 Bay = 0; Bay < Plan::BayCount; ++Bay)
			{
				WallAnchor(Out, FString::Printf(TEXT("bay%d.wall.north"), Bay + 1), {Plan::Bays[Bay].West, -Silk}, E, S);
				WallAnchor(Out, FString::Printf(TEXT("bay%d.wall.south"), Bay + 1), {Plan::Bays[Bay].East, Silk}, W, N);
			}
			WallAnchor(Out, TEXT("bay1.wall.east"), {Plan::EndSilkX, -Silk}, S, W);
			WallAnchor(Out, TEXT("bay1.stele.east"), {Plan::SteleX + 0.5 * Plan::SteleThickness, Plan::SteleSouth}, N, E);
			WallAnchor(Out, TEXT("cabinet.wall.north"), {Plan::CabinetWest, Plan::CabinetNorth}, E, S);
			WallAnchor(Out, TEXT("cabinet.wall.east"), {Plan::CabinetEast, Plan::CabinetNorth}, S, W);
			WallAnchor(Out, TEXT("cabinet.wall.west"), {Plan::CabinetWest, Plan::CabinetSouth}, N, E);
		}

		/**
		 * The mounts (the case's felt, the dancer's plinth top), the benches' seats (two frames on each face at ±0.60 along,
		 * facing either wall) and the planters' soil, where the vines grow.
		 */
		void OtherAnchors(FSiteDescription& Out)
		{
			Anchor(Out, TEXT("cabinet.case"), TEXT("Mount"), FMuseeFrame(FVector(Plan::CaseX, Plan::CaseY, Plan::FeltTop)));
			Anchor(Out, TEXT("bay2.dancer"), TEXT("Mount"), FMuseeFrame(FVector(Plan::DancerX, Plan::DancerY, Plan::DancerPlinthTop)));
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::BenchX)); ++Index)
			{
				for (const double Side : {-1.0, 1.0})
				{
					const FString Bench = FString::Printf(TEXT("bench.bay%d.%s"), Index + 2, Side < 0.0 ? TEXT("n") : TEXT("s"));
					for (const double Along : {-Plan::SeatAlong, Plan::SeatAlong})
					{
						const FVector Seat(Plan::BenchX[Index] + Along, Side * Plan::BenchY, Plan::SeatTop);
						Anchor(Out, Bench, TEXT("Seat"), FacingInPlan(Seat, FMuseeFrame::South()));
						Anchor(Out, Bench, TEXT("Seat"), FacingInPlan(Seat, FMuseeFrame::North()));
					}
				}
			}
			const double Soil = Plan::RoofTop + Plan::PlanterHeight - Plan::SoilBelowRim;
			for (const double X : Plan::PlanterX)
			{
				for (const double Side : {-1.0, 1.0})
				{
					Anchor(Out, FString::Printf(TEXT("pergola.planter.%s%s"), Side < 0.0 ? TEXT("n") : TEXT("s"), X < Plan::PlanterX[1] ? TEXT("w") : TEXT("e")),
						TEXT("Bed"), FMuseeFrame(FVector(X, Side * Plan::PlanterY, Soil)));
				}
			}
		}

		/** The bays: the masonry, lunettes and floors, each bay's vault shell and bands, the spandrels, the lanterns to their glazing, the roof. */
		TArray<FName> BaysPieces()
		{
			TArray<FName> Names;
			for (const TCHAR* Piece : {TEXT("wall.north.east"), Pieces::NorthWallCabinet, TEXT("wall.north.west"), TEXT("wall.south"), TEXT("wall.east.inner"),
					 TEXT("wall.east.outer"), TEXT("wall.west"), TEXT("lunette.east"), TEXT("lunette.west"), Pieces::Roof})
			{
				Names.Add(FName(Piece));
			}
			for (int32 Bay = 1; Bay <= Plan::BayCount; ++Bay)
			{
				Names.Add(FName(*FString::Printf(TEXT("floor.bay%d"), Bay)));
				for (const TCHAR* Part : {TEXT("shell.north"), TEXT("shell.south"), TEXT("shell.crown.west"), TEXT("shell.crown.east"), TEXT("band.west"),
						 TEXT("band.east")})
				{
					Names.Add(FName(*FString::Printf(TEXT("vault.bay%d.%s"), Bay, Part)));
				}
				for (const TCHAR* Part : {TEXT("shaft.north"), TEXT("shaft.south"), TEXT("shaft.west"), TEXT("shaft.east"), TEXT("outer.kerb"),
						 TEXT("outer.glass.north"), TEXT("outer.glass.south"), TEXT("outer.gable.west"), TEXT("outer.gable.east")})
				{
					Names.Add(FName(*FString::Printf(TEXT("lantern.bay%d.%s"), Bay, Part)));
				}
			}
			for (int32 Pier = 1; Pier <= Plan::PierCount; ++Pier)
			{
				Names.Add(FName(*FString::Printf(TEXT("floor.arch.%d"), Pier)));
				Names.Add(FName(*FString::Printf(TEXT("arch.%d.spandrel"), Pier)));
			}
			return Names;
		}

		/** The roof outdoors: the slab, the outer lanterns' kerbs, the pergola's posts and beams, the planters. */
		TArray<FName> RoofPieces()
		{
			TArray<FName> Names = {FName(Pieces::Roof), FName(Pieces::CabinetRoof)};
			for (int32 Bay = 1; Bay <= Plan::BayCount; ++Bay)
			{
				Names.Add(FName(*FString::Printf(TEXT("lantern.bay%d.outer.kerb"), Bay)));
			}
			for (const TCHAR* Corner : {TEXT("nw"), TEXT("ne"), TEXT("sw"), TEXT("se")})
			{
				Names.Add(FName(*FString::Printf(TEXT("pergola.post.%s"), Corner)));
				Names.Add(FName(*FString::Printf(TEXT("pergola.planter.%s"), Corner)));
			}
			Names.Add(FName(TEXT("pergola.beam.north")));
			Names.Add(FName(TEXT("pergola.beam.south")));
			return Names;
		}

		/**
		 * The enclosures: the five bays under their one vault, closed to the sky by the lanterns' glazing; the cabinet, lit
		 * only through its laylight from the void over it and open only through its door; and the roof, outdoors.
		 */
		void Enclosures(FSiteDescription& Out)
		{
			FFormEnclosure& Bays = Out.Enclosures.AddDefaulted_GetRef();
			Bays.Key = Key(TEXT("bays"));
			Bays.Kind = EFormEnclosureKind::Enclosed;
			Bays.Pieces = BaysPieces();

			FFormEnclosure& Cabinet = Out.Enclosures.AddDefaulted_GetRef();
			Cabinet.Key = Key(TEXT("cabinet"));
			Cabinet.Kind = EFormEnclosureKind::Enclosed;
			Cabinet.Pieces = {Pieces::CabinetWall, Pieces::NorthWallCabinet, Pieces::CabinetFloor, Pieces::CabinetCeiling, Pieces::CabinetRoof};

			FFormEnclosure& Roof = Out.Enclosures.AddDefaulted_GetRef();
			Roof.Key = Key(TEXT("outdoors.roof"));
			Roof.Kind = EFormEnclosureKind::Outdoors;
			Roof.Pieces = RoofPieces();
		}
	}

	void DescribeSite(FSiteDescription& Out)
	{
		Footprint(Out);
		Ports(Out);
		Openings(Out);
		WallAnchors(Out);
		OtherAnchors(Out);
		Enclosures(Out);
	}
}
