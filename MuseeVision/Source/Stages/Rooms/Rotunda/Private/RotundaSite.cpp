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
		void Footprint(FSiteDescription& Out)
		{
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			for (int32 Station = 0; Station < Plan::DrumStations; ++Station)
			{
				Out.Footprint.OutlineMeters.Add(Polar(Plan::DrumOuter, 360.0 * Station / Plan::DrumStations));
			}
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;
		}

		/**
		 * The ports: four doorways through the drum, their frames on the outer face at the threshold, facing out; and the
		 * stair's portal at the landing, facing north, which serves the well under the drum.
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
				Port.Mates = FMuseeKey(Door.Mates);
			}
			FSitePort& Stair = Out.Ports.AddDefaulted_GetRef();
			Stair.Key = Key(TEXT("stair"));
			Stair.Kind = ESitePortKind::Stairwell;
			Stair.Class = FMuseeKey(Plan::StairClass);
			Stair.Frame = Turned(Plan::NorthDeg, FVector(0.0, -Plan::WellRadius, Plan::LandingZ));
			Stair.WallDepthMeters = Plan::WellRadius - Plan::RevealInner;
			Stair.FaceRadiusMeters = Plan::WellRadius;
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
		 * The openings: the eye, the hall's one opening to the sky (D-004); the rough openings at the five ports; and the
		 * housings in the floor: the pocket the drum's wall slides into and the ram bores in its floor. The drum's door,
		 * which joins the hall and the stair while the sun clock is raised, is no opening to the sky, and Form has no kind
		 * for a passage between two enclosures of one stage, so it is not declared.
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
