#include "AtriumParts.h"
#include "SiteDescription.h"

/**
 * What the Atrium declares to the site and the systems (§1, §2): its footprint, its three ports (the west door from the
 * Hall of Light, and the two vertical ports whose wells it serves), the openings in its shells, the anchors the systems
 * furnish, and the hall with the pieces that bound it.
 *
 * The drum's glazing and the cove are bands of the base's inner face, bent round the axis: each is declared on that face
 * from the door's centre line, its developed width all round. The wells are boxes about the axis, which Site shapes as
 * the class's circle r 2.300 less its rail notches.
 */
namespace Musee::Atrium
{
	namespace
	{
		/**
		 * The Atrium turned about its axis, for the check of the stages stacked on it (§1, the seams; the Sphere's sheet
		 * draws the other side): out to the portal's corners, the farthest it reaches; up to the corona's top and in along
		 * the wash to the drum's head, r 14.080; down to the equator; in to the seam sphere R 14.000 about C and down it,
		 * outside it, to the neck column; and down the column to the collar's top, the Sphere's port plane.
		 */
		void TurnedVolume(FSiteDescription& Out)
		{
			const Plan::FStep& WashOuter = Plan::EntablatureUpper[UE_ARRAY_COUNT(Plan::EntablatureUpper) - 2];
			const Plan::FStep& WashInner = Plan::EntablatureUpper[UE_ARRAY_COUNT(Plan::EntablatureUpper) - 1];
			const double Reach = FVector2D(Plan::PortalFrontX, Plan::PortalHalfWidth).Size();
			TArray<FVector2D>& Profile = Out.Footprint.TurnedProfileMeters;
			Profile = {{0.0, Plan::FootprintBottom}, {Reach, Plan::FootprintBottom}, {Reach, WashOuter.B}, {WashOuter.A, WashOuter.B},
				{WashInner.A, WashInner.B}, {WashInner.A, Plan::SphereCentreZ}};
			// The seam, from its equator down to the neck column: angles from the sphere's lowest point, its sides tangent to it.
			const double R = Plan::SeamRadius;
			const double From = 0.5 * UE_DOUBLE_PI;
			const double To = FMath::Asin(Plan::NeckColumn / R);
			const double Widest = 2.0 * FMath::Acos(R / (R + Plan::SeamToleranceMeters));
			const int32 Sides = FMath::CeilToInt32((From - To) / Widest);
			const double Step = (From - To) / Sides;
			auto On = [R](double Angle, double Radius) { return FVector2D(Radius * FMath::Sin(Angle), Plan::SphereCentreZ - Radius * FMath::Cos(Angle)); };
			Profile.Add(On(From, R));
			for (int32 Side = 0; Side < Sides; ++Side)
			{
				Profile.Add(On(From - (Side + 0.5) * Step, R / FMath::Cos(0.5 * Step)));
			}
			Profile.Append({On(To, R), {Plan::NeckColumn, Plan::UpPlane}, {0.0, Plan::UpPlane}});
		}

		/** The point of a circle about the axis at a plan angle, degrees. */
		FVector2D OnCircle(double Radius, double Degrees)
		{
			const double Radians = FMath::DegreesToRadians(Degrees);
			return Radius * FVector2D(FMath::Cos(Radians), FMath::Sin(Radians));
		}

		/**
		 * A circle about the axis from one plan angle to a larger one: the footprint's stations strictly between (every
		 * half degree of the full turn, which the Hall of Light's arcs share, so the two meet vertex for vertex), then the end.
		 */
		void AddArc(TArray<FVector2D>& Out, double Radius, double FromDegrees, double ToDegrees)
		{
			const double Step = 360.0 / Plan::FootprintStations;
			for (int32 K = FMath::FloorToInt32(FromDegrees / Step + UE_DOUBLE_KINDA_SMALL_NUMBER) + 1; K * Step < ToDegrees - UE_DOUBLE_KINDA_SMALL_NUMBER; ++K)
			{
				Out.Add(OnCircle(Radius, K * Step));
			}
			Out.Add(OnCircle(Radius, ToDegrees));
		}

		/** One band of the footprint: its plan, as a disc outside a sector either side of west. */
		struct FBandPlan
		{
			double Radius = 0.0;
			double SectorDeg = 0.0;
			bool bPortal = false;
		};

		/**
		 * A band's outline in plan, clockwise from the portal's north front corner (G1): along the portal block's north side
		 * to the disc, meeting its circle or, within the sector, the sector's radial edge; round the circle by the north, east
		 * and south; back to the south front corner; then, when Notch is more than nothing, in along the portal's front and
		 * round the base's face r 14.800 by the west, leaving out the notch of that half width. Without the portal block, the
		 * disc alone.
		 */
		TArray<FVector2D> BandOutline(const FBandPlan& Band, double Notch)
		{
			TArray<FVector2D> Outline;
			if (!Band.bPortal)
			{
				Outline.Add(OnCircle(Band.Radius, 0.0));
				AddArc(Outline, Band.Radius, 0.0, 360.0);
				Outline.Pop();
				return Outline;
			}
			const double Half = Plan::PortalHalfWidth;
			const double FromWest = FMath::RadiansToDegrees(FMath::Asin(Half / Band.Radius));
			// The side meets the circle outside the sector, or else the sector's radial edge, which it follows out to the circle.
			const double Leaves = FMath::Max(FromWest, Band.SectorDeg);
			Outline.Add(FVector2D(Plan::PortalFrontX, -Half));
			if (FromWest < Band.SectorDeg)
			{
				Outline.Add(FVector2D(-Half / FMath::Tan(FMath::DegreesToRadians(Band.SectorDeg)), -Half));
			}
			Outline.Add(OnCircle(Band.Radius, 180.0 + Leaves));
			AddArc(Outline, Band.Radius, 180.0 + Leaves, 540.0 - Leaves);
			if (FromWest < Band.SectorDeg)
			{
				Outline.Add(FVector2D(-Half / FMath::Tan(FMath::DegreesToRadians(Band.SectorDeg)), Half));
			}
			Outline.Add(FVector2D(Plan::PortalFrontX, Half));
			if (Notch > 0.0)
			{
				const double Face = FMath::RadiansToDegrees(FMath::Asin(Notch / Plan::BaseOuter));
				Outline.Append({FVector2D(Plan::PortalFrontX, Notch), OnCircle(Plan::BaseOuter, 180.0 - Face)});
				AddArc(Outline, Plan::BaseOuter, 180.0 - Face, 180.0 + Face);
				Outline.Add(FVector2D(Plan::PortalFrontX, -Notch));
			}
			return Outline;
		}

		/** The prisms' plan from the height of a band's bottom: P1, P1c, P2 or P3. */
		FBandPlan BandAt(double Bottom)
		{
			constexpr double Level = 1.0e-9;
			if (Bottom >= Plan::PortalTop - Level)
			{
				return {Plan::EntablatureRadius, 0.0, false};
			}
			if (Bottom >= Plan::WashTop - Level)
			{
				return {Plan::FinRadius, Plan::FinSectorHalfDeg, true};
			}
			return {Bottom >= Plan::CoronaUnderside - Level ? Plan::FootprintRadius : Plan::SocleRadius, Plan::WestSectorHalfDeg, true};
		}

		/**
		 * The footprint (§1): the plan outline at the ground, G1 (-15.000, -6.050), G2, the arc R 15.400 by the north, east and
		 * south, G3, G4; and the stacked prisms P1, P1c, P2 and P3, each less the notch the Hall of Light's end stands in, its
		 * arched head in level steps.
		 */
		void Footprint(FSiteDescription& Out)
		{
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			Out.Footprint.OutlineMeters = BandOutline({Plan::FootprintRadius, Plan::WestSectorHalfDeg, true}, 0.0);
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;
			const double Arch = Plan::PortalArchRadius;
			const double JambTop = Plan::PortalArchCentreZ + FMath::Sqrt(Arch * Arch - FMath::Square(Plan::PortalJamb));
			const double Crown = Plan::PortalArchCentreZ + Arch;
			TArray<double> Levels = {Plan::FootprintBottom, Plan::CoronaUnderside, Plan::WashTop, Plan::PortalTop, Plan::FootprintTop, Crown};
			for (double Level = JambTop; Level < Crown; Level += Plan::NotchStepMeters)
			{
				Levels.Add(Level);
			}
			Levels.Sort();
			for (int32 Index = 0; Index + 1 < Levels.Num(); ++Index)
			{
				const double Bottom = Levels[Index];
				const double Top = Levels[Index + 1];
				if (Top - Bottom < UE_DOUBLE_KINDA_SMALL_NUMBER)
				{
					continue;
				}
				// The notch under the soffit, as wide as the soffit at the band's top; none above the crown.
				const double Notch = Top <= JambTop ? Plan::PortalJamb : (Bottom < Crown ? FMath::Sqrt(FMath::Max(0.0, Arch * Arch
					- FMath::Square(Top - Plan::PortalArchCentreZ))) : 0.0);
				FSitePrism& Prism = Out.Footprint.Prisms.AddDefaulted_GetRef();
				Prism.OutlineMeters = BandOutline(BandAt(Bottom), Notch);
				Prism.BottomMeters = Bottom;
				Prism.TopMeters = Top;
			}
			TurnedVolume(Out);
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
		 * shut, the iris's blades shut), the car's leaves and the mast's head riding the car; the two controls; the stele's
		 * wall and the eleven bays' walls. Anchors on the car are declared where they stand with the car at the Atrium stop,
		 * whose frame is the stage's.
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
			// The car carries its two leaves and the Cube mast's head (§3.6): each rides the car, hung from it where drawn.
			for (const TCHAR* Leaf : {TEXT("car.leaf.north"), TEXT("car.leaf.south")})
			{
				Out.BodyParents.Add(Key(Leaf), CarBody());
			}
			Out.BodyParents.Add(MastHeadBody(), CarBody());
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
		// The stele's barrier (§3.7): the box of the foot's plan up to the glass's top stops the body only; the look passes.
		const double (&Foot)[2][3] = Plan::SteleFoot;
		const FVector Low(Foot[0][0], Foot[0][1], Foot[0][2]);
		const FVector High(Foot[1][0], Foot[1][1], Plan::SteleGlass[1][2]);
		Out.Barriers.Add({Key(TEXT("stele.barrier")), FMuseeFrame(0.5 * (Low + High)), 0.5 * (High - Low)});
		Footprint(Out);
		Ports(Out);
		Openings(Out);
		Anchors(Out);
		Enclosures(Out);
	}
}
