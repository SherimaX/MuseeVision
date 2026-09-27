#include "ReserveParts.h"
#include "ReservePlan.h"
#include "SiteDescription.h"

/**
 * What the undercroft declares to the site and the systems (§1, §2): its footprint, the one port at the west door with
 * the oval's well it receives, the port's rough opening, the anchors the systems furnish, and the one enclosure with the
 * pieces that bound it.
 */
namespace Musee::Reserve
{
	namespace
	{
		/**
		 * The footprint (§1): the box between the walls' outer faces, from the footings to the top under the lawn, and the
		 * top's rise under the Salon's floor over x ±30.550, |y| ≤ 7.550.
		 */
		void Footprint(FSiteDescription& Out)
		{
			const TArray<FVector2D> Box = {FVector2D(-Plan::OuterX, -Plan::OuterY), FVector2D(Plan::OuterX, -Plan::OuterY), FVector2D(Plan::OuterX, Plan::OuterY),
				FVector2D(-Plan::OuterX, Plan::OuterY)};
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			Out.Footprint.OutlineMeters = Box;
			Out.Footprint.BottomMeters = Plan::Footing;
			Out.Footprint.TopMeters = Plan::TopHigh;
			FSitePrism& Lower = Out.Footprint.Prisms.AddDefaulted_GetRef();
			Lower.OutlineMeters = Box;
			Lower.BottomMeters = Plan::Footing;
			Lower.TopMeters = Plan::TopLow;
			FSitePrism& Upper = Out.Footprint.Prisms.AddDefaulted_GetRef();
			Upper.OutlineMeters = {FVector2D(-Plan::SalonHalfX, -Plan::SalonHalfY), FVector2D(Plan::SalonHalfX, -Plan::SalonHalfY),
				FVector2D(Plan::SalonHalfX, Plan::SalonHalfY), FVector2D(-Plan::SalonHalfX, Plan::SalonHalfY)};
			Upper.BottomMeters = Plan::TopLow;
			Upper.TopMeters = Plan::TopHigh;
		}

		/**
		 * The port: the west door on the west wall's outer face, facing west, where the oval's stair lands. The oval serves
		 * the well its stair runs down (museum x -90.80…-74.60, y ±1.60, z -6.15…0.00), which the Reserve receives and
		 * builds nothing in; the Reserve draws its own architrave on the hall face, finishes its door at the class opening
		 * through the wall, and lays its own threshold stone from the strip to the flags (§2).
		 */
		void Port(FSiteDescription& Out)
		{
			FSitePort& Stair = Out.Ports.AddDefaulted_GetRef();
			Stair.Key = Key(TEXT("stair"));
			Stair.Kind = ESitePortKind::Stairwell;
			Stair.Class = FMuseeKey(Plan::PortClass);
			Stair.Frame = FMuseeFrame(FVector(-Plan::OuterX, 0.0, 0.0), FQuat(FVector::UpVector, UE_DOUBLE_PI));
			Stair.WallDepthMeters = Plan::WallMeters;
			Stair.bOwnSurround = true;
			Stair.OwnLiningMeters = Stair.WallDepthMeters;
			Stair.bOwnThreshold = true;
			Stair.Mates = FMuseeKey(Plan::PortMates);
			Stair.bHasWell = true;
			Stair.Well.bServing = false;
			Stair.Well.MinMeters = FVector(Plan::WellWestX, -Plan::WellHalfY, Plan::Footing);
			Stair.Well.MaxMeters = FVector(-Plan::OuterX, Plan::WellHalfY, Plan::WellTop);
		}

		/** The port's rough opening in the west wall, looking west out of the undercroft. */
		void Openings(FSiteDescription& Out)
		{
			const double Crown = Plan::DoorSpringing + 0.5 * Plan::DoorWidth;
			FFormOpening& Door = Out.Openings.AddDefaulted_GetRef();
			Door.Key = Key(TEXT("stair"));
			Door.Kind = EFormOpeningKind::Port;
			Door.Enclosure = Key(TEXT("undercroft"));
			Door.Frame = FrameOfAxes(FVector(-Plan::OuterX, 0.0, 0.5 * Crown), FMuseeFrame::North(), FMuseeFrame::Up());
			Door.SizeMeters = FVector2D(Plan::DoorWidth, Crown);
		}

		void Anchor(FSiteDescription& Out, const FMuseeKey& Key, const TCHAR* Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({Key, FName(Kind), Frame});
		}

		/**
		 * A rack's hung face (§2): origin at the face's left end seen from the room, on the works' back plane 0.012 off the
		 * rack's centre plane, x along the face, y out of it towards the viewer, z up; at home.
		 */
		FMuseeFrame HungFace(const FRack& Rack, int32 Face)
		{
			const double F = Face;
			const double Reach = Plan::PostY + 0.5 * Plan::PostAlong;
			const FVector Origin(Rack.X + F * Plan::WorksBack, Rack.HomeY() + F * Reach, 0.0);
			return FrameOfAxes(Origin, FVector(0.0, -F, 0.0), FVector(F, 0.0, 0.0));
		}

		/**
		 * The anchors (§2), every rack at home: each rack's body and the pull that works it, facing the nave; the hung faces
		 * of the lettered racks and the easel (Wall); the felts on the two south chests (Mount), the works' top edges south.
		 */
		void Anchors(FSiteDescription& Out)
		{
			for (const FRack& Rack : Racks())
			{
				Anchor(Out, Rack.Body(), TEXT("Body"), FMuseeFrame(FVector(Rack.X, Rack.HomeY(), 0.0)));
				const double N = Rack.TowardsNave();
				const double Pull = Rack.HomeY() + N * (Plan::PostY + 0.5 * Plan::PostAlong + Plan::PullOff);
				Anchor(Out, FMuseeKey(Rack.Body().ToString() + TEXT(".pull")), TEXT("Use"), FrameOfAxes(FVector(Rack.X, Pull, Plan::PullMiddle),
					FVector(0.0, N, 0.0), FMuseeFrame::Up()));
			}
			const TArray<FRack> All = Racks();
			for (const Plan::FLitFace& Hung : Plan::LitFaces)
			{
				const FRack* Rack = All.FindByPredicate([&Hung](const FRack& R) { return R.Name == Hung.Rack; });
				check(Rack);
				Anchor(Out, FMuseeKey(FString::Printf(TEXT("%s.%s"), *Rack->Body().ToString(), Hung.Face < 0 ? TEXT("west") : TEXT("east"))), TEXT("Wall"),
					HungFace(*Rack, Hung.Face));
			}
			Anchor(Out, Key(TEXT("easel")), TEXT("Wall"), FrameOfAxes(FVector(Plan::EaselAnchorX, Plan::EaselAnchorY, 0.0), FMuseeFrame::South(),
				FMuseeFrame::West()));
			for (int32 Bay = 0; Bay < int32(UE_ARRAY_COUNT(Plan::ChestXs)); ++Bay)
			{
				Anchor(Out, Key(*FString::Printf(TEXT("chest.bay%d-south"), Bay + 7)), TEXT("Mount"), FMuseeFrame(FVector(Plan::ChestXs[Bay], Plan::ChestY,
					Plan::ChestTop + Plan::FeltThickness)));
			}
		}

		/** The undercroft: bounded by its walls, the floor's bed, the vaults' masonry and the top. */
		void Enclosure(FSiteDescription& Out)
		{
			FFormEnclosure& Undercroft = Out.Enclosures.AddDefaulted_GetRef();
			Undercroft.Key = Key(TEXT("undercroft"));
			Undercroft.Kind = EFormEnclosureKind::Enclosed;
			Undercroft.Pieces = {Pieces::Walls, Pieces::Bed, Pieces::Top};
			Undercroft.Pieces.Append(Pieces::Vaults());
		}
	}

	void DescribeSite(FSiteDescription& Out)
	{
		Footprint(Out);
		Port(Out);
		Openings(Out);
		Anchors(Out);
		Enclosure(Out);
	}
}
