#include "ChenghuaiParts.h"
#include "ChenghuaiPlan.h"
#include "ChenghuaiPlanHalls.h"
#include "ChenghuaiTrades.h"
#include "SiteDescription.h"

/**
 * What Chenghuai declares to the site and the systems (§1, §2): its footprint with the drum's arc, its one port on the
 * Rotunda's drum, the openings of its spaces (each room's paper lattice, each court's top, the doors between its own
 * spaces, the port), the anchors its schedules name, and its fifteen enclosures with the pieces that bound them.
 */
namespace Musee::Chenghuai
{
	namespace
	{
		/** A frame at a point facing a plan direction out of an enclosure: z along it, y up. */
		FMuseeFrame FacingOut(const FVector& Origin, const FVector2D& Out)
		{
			const FVector Z(Out.X, Out.Y, 0.0);
			return FrameOfAxes(Origin, FVector::CrossProduct(FVector::UpVector, Z), FVector::UpVector);
		}

		/** A point's plan angle about a centre, degrees from +x towards +y, in [0, 360). */
		double AngleAbout(const FVector2D& Centre, const FVector2D& Point)
		{
			const double Degrees = FMath::RadiansToDegrees(FMath::Atan2(Point.Y - Centre.Y, Point.X - Centre.X));
			return Degrees < 0.0 ? Degrees + 360.0 : Degrees;
		}

		/** Adds the stations of a circle strictly between two plan angles (From < To), at a number of stations to the turn. */
		void AddStationsBetween(TArray<FVector2D>& Out, const FVector2D& Centre, double Radius, double FromDegrees, double ToDegrees, int32 PerTurn)
		{
			const double Step = 360.0 / PerTurn;
			for (int32 K = FMath::FloorToInt32(FromDegrees / Step) + 1; K * Step < ToDegrees; ++K)
			{
				const double Radians = FMath::DegreesToRadians(K * Step);
				Out.Add(Centre + Radius * FVector2D(FMath::Cos(Radians), FMath::Sin(Radians)));
			}
		}

		/**
		 * The keyhole the Classical Hall below takes out of the lower prism, from the site's south face: the gallery's strip
		 * x ±4.20 north to the tribune's circle r 8.30, round it by the west, north and east, and back; its arc at Form's
		 * chord rule, the chords standing inside the circle, 0.40 clear of the hall's shell (r 7.90).
		 */
		void AddKeyhole(TArray<FVector2D>& Out)
		{
			const FVector2D Tribune(Plan::TribuneX, Plan::TribuneY);
			const double Radius = Plan::TribuneClear;
			const double Half = Plan::GalleryClearHalf;
			const double Meets = Plan::TribuneY + FMath::Sqrt(Radius * Radius - Half * Half);
			const double From = AngleAbout(Tribune, FVector2D(-Half, Meets));
			const double To = AngleAbout(Tribune, FVector2D(Half, Meets)) + 360.0;
			const double Step = FMath::RadiansToDegrees(2.0 * FMath::Acos(1.0 - Plan::FootprintChordMeters / Radius));
			const int32 Stations = FMath::CeilToInt32((To - From) / Step);
			Out.Append({FVector2D(-Half, Plan::SiteSouth), FVector2D(-Half, Meets)});
			for (int32 K = 1; K < Stations; ++K)
			{
				const double Radians = FMath::DegreesToRadians(FMath::Lerp(From, To, double(K) / Stations));
				Out.Add(Tribune + Radius * FVector2D(FMath::Cos(Radians), FMath::Sin(Radians)));
			}
			Out.Append({FVector2D(Half, Meets), FVector2D(Half, Plan::SiteSouth)});
		}

		/**
		 * The footprint (§1): the site and the porch out to the drum's face, from the Classical Hall's clearance, -0.50, to
		 * the main hall's ridge; and under it, down to the bridge piers' footings at -0.95, the site less the keyhole over the
		 * hall below. The porch's end is the drum's arc R 11.20 about (0, 17.000) between the porch walls' outer faces, on the
		 * drum's own stations.
		 */
		void Footprint(FSiteDescription& Out)
		{
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			TArray<FVector2D>& Outline = Out.Footprint.OutlineMeters;
			const FVector2D Drum(0.0, Plan::DrumCentreY);
			const auto OnDrum = [&Drum](double X) { return FVector2D(X, Drum.Y - FMath::Sqrt(FMath::Square(Plan::DrumRadius) - X * X)); };
			Outline = {FVector2D(Plan::SiteWest, Plan::SiteSouth), FVector2D(-Plan::PorchOuter, Plan::SiteSouth), OnDrum(-Plan::PorchOuter)};
			AddStationsBetween(Outline, Drum, Plan::DrumRadius, AngleAbout(Drum, OnDrum(-Plan::PorchOuter)), AngleAbout(Drum, OnDrum(Plan::PorchOuter)),
				Plan::DrumStationsPerTurn);
			Outline.Append({OnDrum(Plan::PorchOuter), FVector2D(Plan::PorchOuter, Plan::SiteSouth), FVector2D(Plan::SiteEast, Plan::SiteSouth),
				FVector2D(Plan::SiteEast, Plan::SiteNorth), FVector2D(Plan::SiteWest, Plan::SiteNorth)});
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;

			FSitePrism& Lower = Out.Footprint.Prisms.AddDefaulted_GetRef();
			Lower.OutlineMeters = {FVector2D(Plan::SiteWest, Plan::SiteSouth)};
			AddKeyhole(Lower.OutlineMeters);
			Lower.OutlineMeters.Append({FVector2D(Plan::SiteEast, Plan::SiteSouth), FVector2D(Plan::SiteEast, Plan::SiteNorth),
				FVector2D(Plan::SiteWest, Plan::SiteNorth)});
			Lower.BottomMeters = Plan::FootprintBottom;
			Lower.TopMeters = Plan::HallClearZ;
			FSitePrism& Upper = Out.Footprint.Prisms.AddDefaulted_GetRef();
			Upper.OutlineMeters = Outline;
			Upper.BottomMeters = Plan::HallClearZ;
			Upper.TopMeters = Plan::FootprintTop;
		}

		/** The one port: the porch's end on the Rotunda's drum, facing south into the Rotunda's north door. */
		void Port(FSiteDescription& Out)
		{
			FSitePort& South = Out.Ports.AddDefaulted_GetRef();
			South.Key = Key(TEXT("south"));
			South.Kind = ESitePortKind::Doorway;
			South.Class = FMuseeKey(Plan::PortClass);
			South.Frame = FrameOfAxes(FVector(0.0, Plan::PortY, 0.0), FVector(0.0, 1.0, 0.0), FVector(-1.0, 0.0, 0.0));
			South.WallDepthMeters = 0.0;
			// The porch ends on the drum's face, wrapped round it: the drum's axis stands ahead of the port, a negative radius (FSitePort).
			South.FaceRadiusMeters = -Plan::DrumRadius;
			South.Mates = FMuseeKey(Plan::PortMates);
		}

		FFormOpening& Opening(FSiteDescription& Out, const FString& Local, EFormOpeningKind Kind, const FString& Enclosure, const FMuseeFrame& Frame,
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

		void Openings(FSiteDescription& Out)
		{
			// The port: the porch's open mouth on the drum, the class's opening.
			Opening(Out, TEXT("south"), EFormOpeningKind::Port, TEXT("porch"), FacingOut(FVector(0.0, Plan::PortY, 0.5 * Plan::PortCrown), FVector2D(0.0, 1.0)),
				FVector2D(Plan::PortClearWidth, Plan::PortCrown));
			// Each court's top, open to the sky at 3.20.
			for (const Plan::FCourtExtent& Court : Plan::Courts)
			{
				Opening(Out, FString(Court.Local) + TEXT(".top"), EFormOpeningKind::Open, Court.Local,
					FMuseeFrame(FVector(0.5 * (Court.X0 + Court.X1), 0.5 * (Court.Y0 + Court.Y1), Plan::CourtTop)),
					FVector2D(Court.X1 - Court.X0, Court.Y0 - Court.Y1));
			}
			// The doors between the stage's own spaces, each declared once from the space it faces out of, naming the one beyond.
			for (const Plan::FSharedOpening& Shared : Plan::SharedOpenings)
			{
				const FVector Centre(Shared.X, Shared.Y, Shared.Z);
				const FVector2D OutOf(Shared.OutX, Shared.OutY);
				const FVector2D Size(Shared.Width, Shared.Height);
				FFormOpening& Door = Opening(Out, FString(Shared.First) + TEXT(".") + Shared.Local, EFormOpeningKind::Open, Shared.First,
					FacingOut(Centre, OutOf), Size);
				if (Shared.Second)
				{
					Door.Beyond = Key(Shared.Second);
				}
			}
			// Each room's paper lattice and its open door pairs, each pair naming the court it opens onto.
			for (const FHallSpec& Spec : Buildings())
			{
				TArray<FFormOpening> Room;
				HallOpenings(Spec, Room);
				const Plan::FRoomCourt* Court = nullptr;
				for (const Plan::FRoomCourt& Each : Plan::RoomCourts)
				{
					Court = Spec.Room == Each.Room ? &Each : Court;
				}
				for (FFormOpening& Door : Room)
				{
					if (Court && Door.Kind == EFormOpeningKind::Open)
					{
						Door.Beyond = Key(Court->Court);
					}
				}
				Out.Openings.Append(Room);
			}
		}

		void Anchors(FSiteDescription& Out)
		{
			for (const Plan::FAnchorPlan& Anchor : Plan::Anchors)
			{
				const FVector X(Anchor.AxisX[0], Anchor.AxisX[1], Anchor.AxisX[2]);
				FVector Y(Anchor.AxisY[0], Anchor.AxisY[1], Anchor.AxisY[2]);
				if (Y.IsNearlyZero())
				{
					// A mount or a bed: z up, and y square to x in plan.
					Y = FVector::CrossProduct(FVector::UpVector, X);
				}
				const double Z = Anchor.bOnRockery ? RockeryHeight(FVector2D(Anchor.X, Anchor.Y)) : Anchor.Z;
				Out.Anchors.Add({Key(Anchor.Local), FName(Anchor.Kind), FrameOfAxes(FVector(Anchor.X, Anchor.Y, Z), X, Y)});
			}
			// The pavilion's seats, each facing its centre.
			const FVector2D Centre(Plan::PavilionX, Plan::PavilionY);
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(Plan::Seats); ++Index)
			{
				const FVector2D Seat(Plan::Seats[Index].X, Plan::Seats[Index].Y);
				const FVector2D Facing = (Centre - Seat).GetSafeNormal();
				const FVector X(Facing.X, Facing.Y, 0.0);
				Out.Anchors.Add({Key(FString::Printf(TEXT("jianshan.seat.%d"), Plan::SeatFaces[Index])), FName(TEXT("Seat")),
					FrameOfAxes(FVector(Seat.X, Seat.Y, Plan::SeatTop), X, FVector::CrossProduct(FVector::UpVector, X))});
			}
		}

		/** The pieces whose names are one of these, or lie under one of them (name.*). */
		TArray<FName> Bounding(const TArray<FFormDrawing>& Pieces, TConstArrayView<const TCHAR*> Names)
		{
			TArray<FName> Found;
			for (const FFormDrawing& Piece : Pieces)
			{
				const FString Name = Piece.Name.ToString();
				for (const TCHAR* Prefix : Names)
				{
					if (Name == Prefix || (Name.StartsWith(FString(Prefix) + TEXT(".")) && !Name.Contains(TEXT(".window."))))
					{
						Found.Add(Piece.Name);
						break;
					}
				}
			}
			return Found;
		}

		void Enclosure(FSiteDescription& Out, const TCHAR* Local, EFormEnclosureKind Kind, TConstArrayView<const TCHAR*> Names)
		{
			FFormEnclosure& Made = Out.Enclosures.AddDefaulted_GetRef();
			Made.Key = Key(Local);
			Made.Kind = Kind;
			Made.Pieces = Bounding(Out.Pieces, Names);
		}

		/** A room's shell: its floor, its gables and back wall, its roof's boarding and bed. */
		void Room(FSiteDescription& Out, const TCHAR* Local, const FString& Hall, TConstArrayView<const TCHAR*> More = {})
		{
			TArray<FString> Parts = {Hall + TEXT(".platform.floor"), Hall + TEXT(".gable.1.body"), Hall + TEXT(".gable.2.body"),
				Hall + TEXT(".back-wall.body"), Hall + TEXT(".roof.boarding"), Hall + TEXT(".roof.bed")};
			for (const TCHAR* Extra : More)
			{
				Parts.Add(Extra);
			}
			TArray<const TCHAR*> Names;
			for (const FString& Part : Parts)
			{
				Names.Add(*Part);
			}
			Enclosure(Out, Local, EFormEnclosureKind::Enclosed, Names);
		}

		void Enclosures(FSiteDescription& Out)
		{
			using E = EFormEnclosureKind;
			Enclosure(Out, TEXT("porch"), E::Outdoors, {TEXT("porch.paving"), TEXT("porch.wall.west"), TEXT("porch.wall.east")});
			Enclosure(Out, TEXT("gate-hall"), E::Enclosed, {TEXT("gate.platform.floor"), TEXT("gate.gable.1.body"), TEXT("gate.gable.2.body"),
				TEXT("gate.roof.boarding"), TEXT("gate.roof.bed")});
			Enclosure(Out, TEXT("entry-yard"), E::Court, {TEXT("entry-yard.paving"), TEXT("wall.yard.body"), TEXT("wall.court.screen.body"),
				TEXT("wall.east.south.body"), TEXT("screen-wall.body")});
			Enclosure(Out, TEXT("front-court"), E::Court, {TEXT("front-court.paving"), TEXT("court.path.1"), TEXT("court.path.2"),
				TEXT("wall.west.south.body"), TEXT("wall.court.west.body"), TEXT("wall.court.east.body"), TEXT("wall.yard.body")});
			Room(Out, TEXT("linchi"), TEXT("front-row"), {TEXT("gate.gable.1.body")});
			Enclosure(Out, TEXT("inner-court"), E::Court, {TEXT("inner-court.paving"), TEXT("court.path.3"), TEXT("court.path.4"),
				TEXT("wall.court.west.body"), TEXT("wall.court.east.body"), TEXT("wall.court.screen.body"), TEXT("wall.east.south.body"),
				TEXT("wall.cross.west.body"), TEXT("wall.cross.east.body")});
			Room(Out, TEXT("tianqing"), TEXT("tianqing-hall"));
			Room(Out, TEXT("changnan"), TEXT("changnan-hall"));
			Room(Out, TEXT("chenghuaitang"), TEXT("main-hall"));
			Room(Out, TEXT("qingbi"), TEXT("qingbi-room"), {TEXT("main-hall.gable.1.body")});
			Room(Out, TEXT("tingyun"), TEXT("tingyun-room"), {TEXT("main-hall.gable.2.body")});
			Enclosure(Out, TEXT("rear-court"), E::Court, {TEXT("rear-court.paving"), TEXT("court.path.5"), TEXT("wall.west.north.body"),
				TEXT("wall.east.north.body"), TEXT("wall.cross.west.body"), TEXT("wall.cross.east.body")});
			Room(Out, TEXT("shujuan"), TEXT("rear-row"));
			Enclosure(Out, TEXT("garden"), E::Outdoors, {TEXT("wall.garden.north.body"), TEXT("wall.garden.east.body"), TEXT("wall.garden.south.body"),
				TEXT("pond.bed"), TEXT("rockery")});
			Room(Out, TEXT("linquan"), TEXT("flower-hall"));
		}
	}

	void DescribeSite(FSiteDescription& Out)
	{
		Footprint(Out);
		Port(Out);
		Openings(Out);
		Anchors(Out);
		Enclosures(Out);
	}
}
