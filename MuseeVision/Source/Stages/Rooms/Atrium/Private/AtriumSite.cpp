#include "AtriumParts.h"
#include "SiteDescription.h"

/**
 * What the Atrium declares to the site and the systems (§1, §2): its footprint, its three ports (the west door from the
 * Hall of Light, and the two vertical ports whose wells it serves), the openings in its shells, the anchors the systems
 * furnish, and the hall with the pieces that bound it.
 *
 * The drum's glazing and the cove are bands of the base's inner face, bent round the axis: each is declared on that face
 * from the door's centre line, its developed width all round. The wells are Site's boxes about the axis (the class's
 * circle r 2.300 with its rail notches is Site's to make, KIT-2).
 */
namespace Musee::Atrium
{
	namespace
	{
		void Footprint(FSiteDescription& Out)
		{
			const double Half = Plan::PortalHalfWidth;
			const double R = Plan::FootprintRadius;
			// G1, G2, the arc clockwise on the plan (increasing plan angle) from G2 through north, east and south to G3, G4.
			const double Open = FMath::Atan2(Half, -FMath::Sqrt(R * R - Half * Half));
			const int32 Stations = FMath::CeilToInt32(Plan::FootprintStations * Open / UE_DOUBLE_PI);
			TArray<FVector2D>& Outline = Out.Footprint.OutlineMeters;
			Outline.Add(FVector2D(Plan::PortalFrontX, -Half));
			for (int32 Index = 0; Index <= Stations; ++Index)
			{
				const double Angle = -Open + 2.0 * Open * Index / Stations;
				Outline.Add(R * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
			}
			Outline.Add(FVector2D(Plan::PortalFrontX, Half));
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;
		}

		/** A vertical port's frame: x out of the stage along ±z. */
		FMuseeFrame Vertical(double Height, double Facing)
		{
			return FrameOfAxes(FVector(0.0, 0.0, Height), FVector(0.0, 0.0, Facing), FVector(1.0, 0.0, 0.0));
		}

		/**
		 * The ports: the west doorway on the base's convex face (R 14.800), which meets the Hall of Light's east port; the
		 * down port on the slab's soffit, meeting the Cube's up port; the up port on the collar's top, meeting the Sphere's
		 * down port. The Atrium serves both wells: the car and all that moves in them are its own.
		 */
		void Ports(FSiteDescription& Out)
		{
			FSitePort& West = Out.Ports.AddDefaulted_GetRef();
			West.Key = Key(TEXT("west"));
			West.Kind = ESitePortKind::Doorway;
			West.Class = FMuseeKey(TEXT("doorway.4000x4500.square"));
			West.Frame = FMuseeFrame(FVector(-Plan::BaseOuter, 0.0, 0.0), FQuat(FVector::UpVector, UE_DOUBLE_PI));
			West.WallDepthMeters = Plan::BaseOuter - Plan::BaseInner;
			West.FaceRadiusMeters = Plan::BaseOuter;
			West.Mates = FMuseeKey(TEXT("hall-of-light.east"));

			FSitePort& Down = Out.Ports.AddDefaulted_GetRef();
			Down.Key = Key(TEXT("down"));
			Down.Kind = ESitePortKind::Shaft;
			Down.Class = FMuseeKey(TEXT("iris.4600"));
			Down.Frame = Vertical(Plan::DownPlane, -1.0);
			Down.WallDepthMeters = Plan::DownDepth;
			Down.Mates = FMuseeKey(TEXT("cube.up"));
			Down.bHasWell = true;
			Down.Well.bServing = true;
			Down.Well.MinMeters = FVector(-Plan::WellRadius, -Plan::WellRadius, Plan::WellBottom);
			Down.Well.MaxMeters = FVector(Plan::WellRadius, Plan::WellRadius, Plan::DownPlane);

			FSitePort& Up = Out.Ports.AddDefaulted_GetRef();
			Up.Key = Key(TEXT("up"));
			Up.Kind = ESitePortKind::Shaft;
			Up.Class = FMuseeKey(TEXT("iris.4600"));
			Up.Frame = Vertical(Plan::UpPlane, 1.0);
			Up.WallDepthMeters = Plan::UpDepth;
			Up.Mates = FMuseeKey(TEXT("sphere.down"));
			Up.bHasWell = true;
			Up.Well.bServing = true;
			Up.Well.MinMeters = FVector(-Plan::WellRadius, -Plan::WellRadius, Plan::UpPlane);
			Up.Well.MaxMeters = FVector(Plan::WellRadius, Plan::WellRadius, Plan::WellTop);
		}

		FFormOpening& Opening(FSiteDescription& Out, const TCHAR* Local, EFormOpeningKind Kind, const FMuseeFrame& Frame, const FVector2D& Size)
		{
			FFormOpening& Made = Out.Openings.AddDefaulted_GetRef();
			Made.Key = Key(Local);
			Made.Kind = Kind;
			Made.Enclosure = Key(TEXT("hall"));
			Made.Frame = Frame;
			Made.SizeMeters = Size;
			return Made;
		}

		/**
		 * The openings (Form's one list): the three ports' rough openings; the drum's glazing, the hall's one daylight
		 * opening; and the housings, the cove the luminaires stand in and the collar's pocket the iris stows in.
		 */
		void Openings(FSiteDescription& Out)
		{
			const FVector East(1.0, 0.0, 0.0);
			const FVector South(0.0, 1.0, 0.0);
			const FVector Up(0.0, 0.0, 1.0);
			Opening(Out, TEXT("west"), EFormOpeningKind::Port, FrameOfAxes(FVector(-Plan::BaseOuter, 0.0, 0.5 * Plan::DoorHead), -South, Up),
				FVector2D(Plan::DoorClear, Plan::DoorHead));
			const FVector2D Well(2.0 * Plan::WellRadius, 2.0 * Plan::WellRadius);
			Opening(Out, TEXT("down"), EFormOpeningKind::Port, FrameOfAxes(FVector(0.0, 0.0, Plan::DownPlane), East, -South), Well);
			Opening(Out, TEXT("up"), EFormOpeningKind::Port, FrameOfAxes(FVector(0.0, 0.0, Plan::UpPlane), East, South), Well);

			// On the inner face at the door's centre line: x along the face the way bearings rise, y up, z out of the hall.
			const double Round = 2.0 * UE_DOUBLE_PI * Plan::BaseInner;
			const FVector2D Face = Polar(Plan::BaseInner, Plan::DoorBearing);
			const FVector2D Round2 = Along(Plan::DoorBearing);
			auto OnFace = [&Face, &Round2, &Up](double Z) { return FrameOfAxes(FVector(Face.X, Face.Y, Z), FVector(Round2.X, Round2.Y, 0.0), Up); };
			const double DrumHeight = Plan::CanopyMeetsDrum - Plan::WallTop;
			FFormOpening& Drum = Opening(Out, TEXT("drum"), EFormOpeningKind::Glazed, OnFace(Plan::WallTop + 0.5 * DrumHeight), FVector2D(Round, DrumHeight));
			Drum.Glazing = Surface(TEXT("drum.inner"));
			Drum.RadiusMeters = Plan::BaseInner;
			Opening(Out, TEXT("cove"), EFormOpeningKind::Housing, OnFace(0.5 * Plan::CoveTop), FVector2D(Round, Plan::CoveTop)).RadiusMeters = Plan::BaseInner;
			Opening(Out, TEXT("collar.pocket"), EFormOpeningKind::Housing, FMuseeFrame(FVector(0.0, 0.0, 0.5 * (Plan::PocketBottom + Plan::PocketTop))),
				FVector2D(2.0 * Plan::PocketOut, 2.0 * Plan::PocketOut));
		}

		void Anchor(FSiteDescription& Out, const FMuseeKey& Key, const TCHAR* Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({Key, FName(Kind), Frame});
		}

		/** A Wall anchor on the base's inner face: origin at a bearing on it, x along it towards increasing bearing, y towards the axis. */
		FMuseeFrame OnInnerFace(double Bearing)
		{
			const FVector2D Point = Polar(Plan::BaseInner, Bearing);
			const FVector2D X = Along(Bearing);
			const FVector2D Y = -Outward(Bearing);
			return FrameOfAxes(FVector(Point.X, Point.Y, 0.0), FVector(X.X, X.Y, 0.0), FVector(Y.X, Y.Y, 0.0));
		}

		/**
		 * The anchors (§2): the moving bodies, drawn closed (the car at the Atrium stop, its leaves and the landing's
		 * shut, the iris's blades shut); the two controls; the stele's wall and the eleven bays' walls. Anchors on the car
		 * are declared where they stand with the car at the Atrium stop, whose frame is the stage's.
		 */
		void Anchors(FSiteDescription& Out)
		{
			Anchor(Out, CarBody(), TEXT("Body"), FMuseeFrame());
			Anchor(Out, MastHeadBody(), TEXT("Body"), MastHeadFrame());
			for (const TCHAR* Leaf : {TEXT("car.leaf.north"), TEXT("car.leaf.south"), TEXT("landing.leaf.north"), TEXT("landing.leaf.south")})
			{
				Anchor(Out, Key(Leaf), TEXT("Body"), FMuseeFrame());
			}
			for (int32 Blade = 0; Blade < Plan::IrisBlades; ++Blade)
			{
				Anchor(Out, IrisBladeBody(Blade), TEXT("Body"), IrisPinFrame(Blade));
			}
			Anchor(Out, Key(TEXT("call")), TEXT("Use"), FMuseeFrame(FVector(Plan::CallX, 0.0, 0.0)));
			Anchor(Out, Key(TEXT("car.panel")), TEXT("Use"), FMuseeFrame());
			Anchor(Out, Key(TEXT("stele")), TEXT("Wall"), FrameOfAxes(FVector(Plan::SteleWestX, Plan::SteleNorthY, 0.0), FVector(0.0, 1.0, 0.0),
				FVector(-1.0, 0.0, 0.0)));

			// The bays A-K, each starting at its left rib's side face on the base's inner face (the sheet's arc of 7.030 between them).
			const double SideFace = HalfAngle(Plan::RibHalf, Plan::BaseInner);
			for (int32 Bay = 0; Bay < Plan::Bays; ++Bay)
			{
				const double Centre = FMath::Fmod(Plan::FirstBayBearing + Plan::BayStep * Bay, 360.0);
				const FString Local = FString::Printf(TEXT("bay.%c"), TCHAR('a' + Bay));
				Anchor(Out, Key(*Local), TEXT("Wall"), OnInnerFace(Centre - 0.5 * Plan::BayStep + SideFace));
			}
		}

		/** The names of the drawn pieces that bound the hall. */
		TArray<FName> HallPieces(const FSiteDescription& Out)
		{
			const TCHAR* const Prefixes[] = {Pieces::Floor, Pieces::Base, Pieces::InnerLeaf, Pieces::Closer, Pieces::Canopy, Pieces::Ring, Pieces::Collar,
				Pieces::LandingGlass, Pieces::TubeGlass, Pieces::LandingLeaf, Pieces::Sill};
			TArray<FName> Names;
			for (const FFormDrawing& Piece : Out.Pieces)
			{
				const FString Name = Piece.Name.ToString();
				for (const TCHAR* Prefix : Prefixes)
				{
					if (Name.StartsWith(Prefix, ESearchCase::CaseSensitive) && !Name.StartsWith(TEXT("collar.iris"), ESearchCase::CaseSensitive))
					{
						Names.Add(Piece.Name);
						break;
					}
				}
			}
			return Names;
		}

		/**
		 * The hall: its floor and the base, the drum's inner leaf and its head closer, the canopy's glass, the ring and the
		 * collar, and the landing's and tube's glass, leaves and sill. The car stands inside it.
		 */
		void Enclosures(FSiteDescription& Out)
		{
			FFormEnclosure& Hall = Out.Enclosures.AddDefaulted_GetRef();
			Hall.Key = Key(TEXT("hall"));
			Hall.Kind = EFormEnclosureKind::Enclosed;
			Hall.Pieces = HallPieces(Out);
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
