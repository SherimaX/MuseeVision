#include "RotundaParts.h"
#include "RotundaPlan.h"
#include "SiteDescription.h"

/**
 * What the Rotunda declares to the site and the systems (§1, §2): its footprint, the circle of the drum's outer face,
 * and the well it serves; its five ports; the openings in its shells; the anchors the systems furnish; and its two
 * enclosures with the pieces that bound them.
 */
namespace Musee::Rotunda
{
	namespace
	{
		/** A circle about the axis at a number of stations per turn, the first on +x. */
		TArray<FVector2D> Circle(double Radius, int32 Stations)
		{
			TArray<FVector2D> Points;
			for (int32 Station = 0; Station < Stations; ++Station)
			{
				Points.Add(Polar(Radius, 360.0 * Station / Stations));
			}
			return Points;
		}

		/**
		 * The footprint (§1): the drum's circle from the floor structure's underside to the top, and under it the well the
		 * stair serves, the cylinder r 7.60 down to the landing's foundation; the Classical Hall reserves the well's prism
		 * and builds round it, under the room.
		 */
		void Footprint(FSiteDescription& Out)
		{
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			Out.Footprint.OutlineMeters = Circle(Plan::DrumOuter, Plan::DrumStations);
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;
			FSitePrism& Well = Out.Footprint.Prisms.AddDefaulted_GetRef();
			Well.OutlineMeters = Circle(Plan::WellRadius, Plan::WellStations);
			Well.BottomMeters = Plan::WellBottom;
			Well.TopMeters = Plan::WellTop;
			FSitePrism& Room = Out.Footprint.Prisms.AddDefaulted_GetRef();
			Room.OutlineMeters = Out.Footprint.OutlineMeters;
			Room.BottomMeters = Plan::FloorBed;
			Room.TopMeters = Plan::FootprintTop;
		}

		/**
		 * The ports: four doorways through the drum, their frames on the outer face at the threshold, facing out; and the
		 * stair's portal at the landing, facing north, which serves the well under the drum. The Rotunda draws each door's
		 * archivolt on the drum's inner face (§3.3) and the stair port's dressings on its stair side (§3.7), so the joints
		 * draw no surround there; and the landing runs through the portal to the port face as the stair port's threshold
		 * (§3.7), stopped half the class's strip short.
		 */
		void Ports(FSiteDescription& Out)
		{
			for (const Plan::FDoor& Door : Plan::Doors)
			{
				FSitePort& Port = Out.Ports.AddDefaulted_GetRef();
				Port.Key = Key(Door.Name);
				Port.Kind = ESitePortKind::Doorway;
				Port.Class = FMuseeKey(Door.Class);
				const FVector2D Face = Polar(Plan::DrumOuter, Door.Deg);
				Port.Frame = Turned(Door.Deg, FVector(Face.X, Face.Y, 0.0));
				Port.WallDepthMeters = Plan::DrumOuter - Plan::DrumInner;
				Port.FaceRadiusMeters = Plan::DrumOuter;
				Port.bOwnSurround = true;
				Port.Mates = FMuseeKey(Door.Mates);
			}
			FSitePort& Stair = Out.Ports.AddDefaulted_GetRef();
			Stair.Key = Key(TEXT("stair"));
			Stair.Kind = ESitePortKind::Stairwell;
			Stair.Class = FMuseeKey(Plan::StairClass);
			Stair.Frame = Turned(Plan::NorthDeg, FVector(0.0, -Plan::WellRadius, Plan::LandingZ));
			Stair.WallDepthMeters = Plan::WellRadius - Plan::RevealInner;
			Stair.FaceRadiusMeters = Plan::WellRadius;
			Stair.bOwnSurround = true;
			Stair.bOwnThreshold = true;
			Stair.Mates = FMuseeKey(TEXT("classical-hall.stair"));
			Stair.bHasWell = true;
			Stair.Well.bServing = true;
			Stair.Well.MinMeters = FVector(-Plan::WellRadius, -Plan::WellRadius, Plan::WellBottom);
			Stair.Well.MaxMeters = FVector(Plan::WellRadius, Plan::WellRadius, Plan::WellTop);
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

		/** A housing's opening, lying in the floor at a height and looking down into its pocket. */
		FMuseeFrame LookingDown(double Z)
		{
			return FrameOfAxes(FVector(0.0, 0.0, Z), FMuseeFrame::East(), FMuseeFrame::North());
		}

		/**
		 * The openings: the eye, the hall's one opening to the sky (D-004); the rough openings at the five ports; the
		 * housings in the floor: the pocket the drum's wall slides into and the ram bores in its floor; and the sun clock's
		 * door, a passage from the stair within the lifting drum to the hall, which rides the drum and opens only raised.
		 */
		void Openings(FSiteDescription& Out)
		{
			const double EyeTop = Plan::GlassBottom + Plan::GlassMeters;
			Opening(Out, TEXT("eye"), EFormOpeningKind::Glazed, TEXT("hall"), Raised(EyeTop),
				FVector2D(2.0 * Plan::GlassRadius, 2.0 * Plan::GlassRadius)).Glazing = Surface(TEXT("eye.glass"));

			for (const Plan::FDoor& Door : Plan::Doors)
			{
				const double Half = 0.5 * Door.Clear + Plan::LiningAllowance;
				const double Crown = Plan::DoorSpringing + Half;
				const FVector2D Face = Polar(Plan::DrumOuter, Door.Deg);
				const FVector Centre(Face.X, Face.Y, 0.5 * (Plan::RoughSill + Crown));
				Opening(Out, Door.Name, EFormOpeningKind::Port, TEXT("hall"), FrameOfAxes(Centre, Tangent(Door.Deg), FVector::UpVector),
					FVector2D(2.0 * Half, Crown - Plan::RoughSill));
			}
			const double StairHalf = 0.5 * Plan::StairPortClear + Plan::LiningAllowance;
			const double StairCrown = Plan::StairPortSpringing + StairHalf;
			const FVector StairCentre(0.0, -Plan::WellRadius, Plan::LandingZ + 0.5 * StairCrown);
			Opening(Out, TEXT("stair"), EFormOpeningKind::Port, TEXT("stair"), FrameOfAxes(StairCentre, Tangent(Plan::NorthDeg), FVector::UpVector),
				FVector2D(2.0 * StairHalf, StairCrown));

			Opening(Out, TEXT("sun-clock.pocket"), EFormOpeningKind::Housing, TEXT("hall"), LookingDown(0.0),
				FVector2D(2.0 * Plan::PocketOuter, 2.0 * Plan::PocketOuter));
			const double Bores = 2.0 * (Plan::BoreRadius + 0.5 * Plan::BoreDiameter);
			Opening(Out, TEXT("sun-clock.rams"), EFormOpeningKind::Housing, TEXT("hall"), LookingDown(Plan::PocketFloor), FVector2D(Bores, Bores));

			// The door on the drum's wall face, in the drum's frame: its radial jambs 2.60 apart there, from the sill to the crown.
			const double DoorHalf = Plan::SunWallFace * FMath::DegreesToRadians(Plan::SunDoorHalfDeg);
			const double DoorCrown = Plan::SunDoorSpringing + DoorHalf;
			const FVector2D DoorFace = Polar(Plan::SunWallFace, Plan::SunDoorDeg);
			const FVector DoorCentre(DoorFace.X, DoorFace.Y, 0.5 * (Plan::SunSill + DoorCrown));
			FFormOpening& Door = Opening(Out, TEXT("sun-clock.door"), EFormOpeningKind::Passage, TEXT("stair"),
				FrameOfAxes(DoorCentre, Tangent(Plan::SunDoorDeg), FVector::UpVector), FVector2D(2.0 * DoorHalf, DoorCrown - Plan::SunSill));
			Door.Beyond = Key(TEXT("hall"));
			Door.Body = DrumBody();
		}

		void Anchor(FSiteDescription& Out, const TCHAR* Local, const TCHAR* Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({Key(Local), FName(Kind), Frame});
		}

		/**
		 * The anchors (§2), with the drum lowered: the two moving bodies, the sun clock's control on the gilt sun's top
		 * (its usable volume is Motion's, from the ring), and the four casts' mounts on the niche sills, x towards the
		 * centre and z up.
		 */
		void Anchors(FSiteDescription& Out)
		{
			Anchor(Out, TEXT("sun-clock.drum"), TEXT("Body"), FMuseeFrame());
			Anchor(Out, TEXT("stair.balustrade.panels"), TEXT("Body"), FMuseeFrame());
			Anchor(Out, TEXT("sun-clock.control"), TEXT("Use"), FMuseeFrame(FVector(0.0, 0.0, Plan::BossTop)));
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::NicheDegrees)); ++Index)
			{
				const double Deg = Plan::NicheDegrees[Index];
				const FVector2D At = Polar(Plan::CastRadius, Deg);
				const FString Local = FString::Printf(TEXT("niche.%s.cast"), Plan::NicheNames[Index]);
				Anchor(Out, *Local, TEXT("Mount"), FrameOfAxes(FVector(At.X, At.Y, Plan::NicheSill), -Radial(Deg), -Tangent(Deg)));
			}
		}

		/**
		 * The enclosures. The hall is bounded by the drum's wall, footing and top, the dome's bands, shell and curb, the
		 * eye's glass, the ring's core and the drum's deck under the dial. The stair is bounded by its wall, the shaft's
		 * masonry, the foundation under the landing, and the lifting drum's wall and deck.
		 */
		void Enclosures(FSiteDescription& Out)
		{
			FFormEnclosure& Hall = Out.Enclosures.AddDefaulted_GetRef();
			Hall.Key = Key(TEXT("hall"));
			Hall.Kind = EFormEnclosureKind::Enclosed;
			Hall.Pieces = {Pieces::DrumWall, Pieces::DrumFooting, Pieces::DrumTop, Pieces::DomeBandLower, Pieces::DomeShell, Pieces::DomeBandUpper,
				Pieces::EyeCurb, Pieces::EyeGlass, Pieces::FloorCore, Pieces::SunDeck};

			FFormEnclosure& Stair = Out.Enclosures.AddDefaulted_GetRef();
			Stair.Key = Key(TEXT("stair"));
			Stair.Kind = EFormEnclosureKind::Enclosed;
			Stair.Pieces = {Pieces::Sleeve, Pieces::MasonryInner, Pieces::MasonryTympanum, Pieces::MasonryOuter, Pieces::Foundation, Pieces::SunWall,
				Pieces::SunDeck};
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
