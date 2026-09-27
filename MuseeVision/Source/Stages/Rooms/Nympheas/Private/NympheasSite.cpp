#include "NympheasParts.h"
#include "NympheasPlan.h"
#include "SiteDescription.h"

/**
 * What the oval declares to the site and the systems (§1, §2): its footprint and the well it serves, its two ports, the
 * openings in its shells, the anchors the systems furnish, and its two enclosures with the pieces that bound them.
 */
namespace Musee::Nympheas
{
	namespace
	{
		/** The stage's frame turned so x, y and z are the given axes: for a face that looks along its z. */
		FMuseeFrame Facing(const FVector& Origin, const FVector& X, const FVector& Y) { return FrameOfAxes(Origin, X, Y); }

		/** A frame on the floor plan at a point, its x turned to face a plan direction, z up. */
		FMuseeFrame FacingInPlan(const FVector& Origin, const FVector2D& Direction)
		{
			return FMuseeFrame(Origin, FRotationMatrix::MakeFromX(FVector(Direction.X, Direction.Y, 0.0)).ToQuat());
		}

		void Footprint(FSiteDescription& Out)
		{
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			Out.Footprint.OutlineMeters = FootprintOutline();
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;
		}

		/**
		 * The ports: the east doorway on the passage's port face, which meets the Salon's west door (the oval draws its own
		 * architrave round it, P6); and the stair's foot at the Reserve's west door, whose well (the stair's shaft under the
		 * oval) the oval serves. The Reserve's door is the shaft's whole width, so no wall stands beside it on the well's
		 * side to carry a surround (§3.7: the head wall is plain brick): the oval owns that face bare, and the Reserve draws
		 * its own on the hall face.
		 */
		void Ports(FSiteDescription& Out)
		{
			FSitePort& East = Out.Ports.AddDefaulted_GetRef();
			East.Key = Key(TEXT("east"));
			East.Kind = ESitePortKind::Doorway;
			East.Class = FMuseeKey(Plan::EastClass);
			East.Frame = FMuseeFrame(FVector(Plan::PortFaceX, 0.0, 0.0));
			East.WallDepthMeters = Plan::PortFaceX - Plan::OvalA;
			East.Mates = FMuseeKey(TEXT("salon.west"));
			East.bOwnSurround = true;

			FSitePort& Stair = Out.Ports.AddDefaulted_GetRef();
			Stair.Key = Key(TEXT("stair"));
			Stair.Kind = ESitePortKind::Stairwell;
			Stair.Class = FMuseeKey(Plan::StairClass);
			Stair.Frame = FMuseeFrame(FVector(Plan::PortFaceX, 0.0, Plan::WellFloor));
			Stair.WallDepthMeters = Plan::PortFaceX - Plan::HeadWallX;
			Stair.Mates = FMuseeKey(TEXT("reserve.stair"));
			Stair.bOwnSurround = true;
			Stair.bHasWell = true;
			Stair.Well.bServing = true;
			Stair.Well.MinMeters = FVector(Plan::EndWallWestX, -Plan::ShaftOuterHalf, Plan::FoundationUnderside);
			Stair.Well.MaxMeters = FVector(Plan::PortFaceX, Plan::ShaftOuterHalf, 0.0);
		}

		FFormOpening& Opening(FSiteDescription& Out, const TCHAR* Local, EFormOpeningKind Kind, const TCHAR* Enclosure, const FMuseeFrame& Frame,
			const FVector2D& Size)
		{
			FFormOpening& Made = Out.Openings.AddDefaulted_GetRef();
			Made.Key = Key(Local);
			Made.Kind = Kind;
			Made.Enclosure = Key(Enclosure);
			Made.Frame = Frame;
			Made.SizeMeters = Size;
			return Made;
		}

		/**
		 * The openings: the glass dome, the oval's one opening to the sky; the rough openings at the two ports; and the
		 * housings in the oval's floor, the posts' sleeves and the guard rails' chases, each looking down into its pocket.
		 */
		void Openings(FSiteDescription& Out)
		{
			const FVector East(1.0, 0.0, 0.0);
			const FVector South(0.0, 1.0, 0.0);
			const FVector Up(0.0, 0.0, 1.0);
			Opening(Out, TEXT("roof"), EFormOpeningKind::Glazed, TEXT("oval"), FMuseeFrame(FVector(0.0, 0.0, Plan::RingTop)),
				FVector2D(2.0 * Plan::RimA, 2.0 * Plan::RimB)).Glazing = Surface(TEXT("roof.glass"));
			const double Crown = Plan::DoorSpringing + 0.5 * Plan::DoorWidth;
			Opening(Out, TEXT("east"), EFormOpeningKind::Port, TEXT("oval"), Facing(FVector(Plan::PortFaceX, 0.0, 0.5 * Crown), South, Up),
				FVector2D(Plan::DoorWidth, Crown));
			const double StairCrown = Plan::ReserveDoorSpringing + Plan::ShaftHalf;
			Opening(Out, TEXT("stair"), EFormOpeningKind::Port, TEXT("well"), Facing(FVector(Plan::PortFaceX, 0.0, Plan::WellFloor + 0.5 * StairCrown),
				South, Up), FVector2D(2.0 * Plan::ShaftHalf, StairCrown));

			for (const double X : {-Plan::PostX, Plan::PostX})
			{
				for (const double Y : {-Plan::PostY, Plan::PostY})
				{
					const FString Name = FString::Printf(TEXT("pond.post.%s%s.sleeve"), Y < 0.0 ? TEXT("north") : TEXT("south"),
						X < 0.0 ? TEXT("west") : TEXT("east"));
					Opening(Out, *Name, EFormOpeningKind::Housing, TEXT("oval"), Facing(FVector(X, Y, 0.0), East, -South),
						FVector2D(Plan::SleeveDiameter, Plan::SleeveDiameter));
				}
			}
			const double Middle = 0.5 * (Plan::RailWestX + Plan::RailEastX);
			const FVector2D Along(Plan::RailEastX - Plan::RailWestX, Plan::ChaseWidth);
			Opening(Out, TEXT("guard.chase.north"), EFormOpeningKind::Housing, TEXT("oval"), Facing(FVector(Middle, -Plan::RailY, 0.0), East, -South), Along);
			Opening(Out, TEXT("guard.chase.south"), EFormOpeningKind::Housing, TEXT("oval"), Facing(FVector(Middle, Plan::RailY, 0.0), East, -South), Along);
			Opening(Out, TEXT("guard.chase.east"), EFormOpeningKind::Housing, TEXT("oval"), Facing(FVector(Plan::EastRailX, 0.0, 0.0), East, -South),
				FVector2D(Plan::ChaseWidth, 2.0 * Plan::EastRailHalf));
		}

		void Anchor(FSiteDescription& Out, const TCHAR* Local, const TCHAR* Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({Key(Local), FName(Kind), Frame});
		}

		/**
		 * The anchors (§2), with the pond Down. The golden iris is both the lift's control (Use, read by Motion) and the
		 * bed its cast specimen grows in (Bed, read by Nature), as the schedules name it.
		 */
		void Anchors(FSiteDescription& Out)
		{
			Anchor(Out, TEXT("basin"), TEXT("Body"), FMuseeFrame());
			Anchor(Out, TEXT("guard-rails"), TEXT("Body"), FMuseeFrame());
			for (int32 Stage = 0; Stage < int32(UE_ARRAY_COUNT(Plan::StageDiameters)); ++Stage)
			{
				Out.Anchors.Add({PostStageBody(Stage), TEXT("Body"), FMuseeFrame()});
			}
			const FMuseeFrame Iris(FVector(Plan::IrisX, 0.0, Plan::SeatZ));
			Anchor(Out, TEXT("golden-iris"), TEXT("Use"), Iris);
			Anchor(Out, TEXT("golden-iris"), TEXT("Bed"), Iris);
			Anchor(Out, TEXT("pond.water"), TEXT("Water"), FMuseeFrame(FVector(0.0, 0.0, Plan::WaterLevel)));
			Anchor(Out, TEXT("pond.lilies"), TEXT("Bed"), FMuseeFrame(FVector(0.0, 0.0, Plan::WaterLevel)));
			Anchor(Out, TEXT("pond.shelf.east"), TEXT("Bed"), FMuseeFrame(FVector(Plan::ShelfBedX, 0.0, Plan::ShelfTop)));
			Anchor(Out, TEXT("pond.shelf.west"), TEXT("Bed"), FMuseeFrame(FVector(-Plan::ShelfBedX, 0.0, Plan::ShelfTop)));

			// The willows' wall starts on the inner face at the door's north jamb: y into the room, z up, x along it.
			const double From = FMath::DegreesToRadians(Plan::WillowsFromDeg);
			const FVector2D Start = Oval().Point(From);
			const FVector2D In = -Oval().Outward(From);
			const FVector Y(In.X, In.Y, 0.0);
			Anchor(Out, TEXT("wall.willows"), TEXT("Wall"), FrameOfAxes(FVector(Start.X, Start.Y, 0.0), FVector::CrossProduct(Y, FVector::UpVector), Y));

			// The benches' seats at their centrelines' middles, facing out to the willows.
			const TCHAR* Benches[] = {TEXT("bench.b1"), TEXT("bench.b2"), TEXT("bench.b3"), TEXT("bench.b4")};
			const FVector2D Faces[] = {{-Plan::BenchFacing[0], Plan::BenchFacing[1]}, {Plan::BenchFacing[0], Plan::BenchFacing[1]},
				{Plan::BenchFacing[0], -Plan::BenchFacing[1]}, {-Plan::BenchFacing[0], -Plan::BenchFacing[1]}};
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const TArray<FVector2D> Line = BenchLine(Index);
				FVector2D Middle;
				FVector2D Direction;
				PolylineAt(Line, 0.5 * PolylineLength(Line), Middle, Direction);
				Anchor(Out, Benches[Index], TEXT("Seat"), FacingInPlan(FVector(Middle.X, Middle.Y, Plan::SeatTop), Faces[Index]));
			}
			// The coping's seat ring, riding on the pond: its frame at the east apex, facing out; the ring is its parallel.
			const FVector2D Seat = Pond().Parallel(0.0, Plan::SeatD);
			Anchor(Out, TEXT("coping.seat"), TEXT("Seat"), FacingInPlan(FVector(Seat.X, Seat.Y, Plan::SeatZ), FVector2D(1.0, 0.0)));
		}

		/**
		 * The enclosures. The oval is bounded by its wall, the passage block, its floor and the curb round the opening,
		 * the attic's upstand, the roof ring and the glass dome. The well is bounded by the shaft's walls, its foundation,
		 * the slab over it, the stair and its landings.
		 */
		void Enclosures(FSiteDescription& Out)
		{
			FFormEnclosure& Oval = Out.Enclosures.AddDefaulted_GetRef();
			Oval.Key = Key(TEXT("oval"));
			Oval.Kind = EFormEnclosureKind::Enclosed;
			Oval.Pieces = {Pieces::OvalWall, Pieces::PassageBlock, Pieces::Floor, Pieces::CurbNorth,
				Pieces::CurbSouth, Pieces::Upstand, Pieces::RoofRing, Pieces::RoofGlass};

			FFormEnclosure& Well = Out.Enclosures.AddDefaulted_GetRef();
			Well.Key = Key(TEXT("well"));
			Well.Kind = EFormEnclosureKind::Enclosed;
			Well.Pieces = Pieces::ShaftWalls();
			Well.Pieces.Append({Pieces::WellBase, Pieces::EndWall, Pieces::HeadWall, Pieces::WellSlab, Pieces::Stair, Pieces::TopLanding,
				Pieces::BottomLanding});
		}
	}

	void DescribeSite(FSiteDescription& Out)
	{
		Footprint(Out);
		Ports(Out);
		Openings(Out);
		Anchors(Out);
		Enclosures(Out);
	}
}
