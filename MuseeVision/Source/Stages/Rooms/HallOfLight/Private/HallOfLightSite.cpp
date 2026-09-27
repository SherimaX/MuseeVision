#include "Algo/AnyOf.h"
#include "Algo/Reverse.h"
#include "HallOfLightEnds.h"
#include "HallOfLightParts.h"
#include "HallOfLightPlan.h"
#include "SiteDescription.h"

/**
 * What the hall declares to the site and the systems (sec. 1, sec. 2): its footprint with the two gardens, its two
 * ports on its neighbours' cylinders, the openings in its shell, the anchors the systems furnish, and its three
 * enclosures with the pieces that bound them.
 */
namespace Musee::HallOfLight
{
	namespace
	{
		const FVector East(1.0, 0.0, 0.0);
		const FVector West(-1.0, 0.0, 0.0);
		const FVector North(0.0, -1.0, 0.0);
		const FVector South(0.0, 1.0, 0.0);
		const FVector Up(0.0, 0.0, 1.0);

		FVector2D OnCircle(const FVector2D& Centre, double Radius, double Degrees)
		{
			const double Angle = FMath::DegreesToRadians(Degrees);
			return Centre + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		}

		/**
		 * Adds an arc after its start, to its end, either way round: its neighbour's stations strictly between (PerTurn to the
		 * turn about the neighbour's axis, the first on +x), then the end itself.
		 */
		void AppendArc(TArray<FVector2D>& Points, const FVector2D& Centre, double Radius, double FromDegrees, double ToDegrees, int32 PerTurn)
		{
			const double Step = 360.0 / PerTurn;
			constexpr double Apart = UE_DOUBLE_KINDA_SMALL_NUMBER;
			if (ToDegrees > FromDegrees)
			{
				for (int32 K = FMath::FloorToInt32(FromDegrees / Step + Apart) + 1; K * Step < ToDegrees - Apart; ++K)
				{
					Points.Add(OnCircle(Centre, Radius, K * Step));
				}
			}
			else
			{
				for (int32 K = FMath::CeilToInt32(FromDegrees / Step - Apart) - 1; K * Step > ToDegrees + Apart; --K)
				{
					Points.Add(OnCircle(Centre, Radius, K * Step));
				}
			}
			Points.Add(OnCircle(Centre, Radius, ToDegrees));
		}

		/** A point's angle about a centre, degrees, taken near a given angle. */
		double AngleAbout(const FVector2D& Centre, const FVector2D& Point, double Near)
		{
			const double Angle = FMath::RadiansToDegrees(FMath::Atan2(Point.Y - Centre.Y, Point.X - Centre.X));
			return Angle + 360.0 * FMath::RoundToDouble((Near - Angle) / 360.0);
		}

		/**
		 * The footprint (sec. 1), clockwise on the north-up plan from F1: the meadow's north edge, the Atrium's socle and
		 * its portal's front, the east port face through the axis, the same mirrored round the orchard, and back along
		 * the orchard's south edge to the west port face on the Rotunda's drum.
		 */
		const FVector2D AtriumAxis(Plan::AtriumAxisX, 0.0);
		const FVector2D RotundaAxis(Plan::RotundaAxisX, 0.0);

		/**
		 * A garden's edge by the Atrium, from its outer east corner (29.90, Side 14.25) in to the portal's side (Side 6.05):
		 * down the east edge to a circle R about the Atrium's axis, round it, along the portal's side to its front, and in
		 * along the front to the hall band (Side 4.80). Side is -1 for the meadow (north), +1 for the orchard (south).
		 */
		void AddGardenEast(TArray<FVector2D>& Points, double Side, double Radius)
		{
			const double AtEdge = FMath::Sqrt(Radius * Radius - FMath::Square(Plan::AtriumAxisX - Plan::FootprintEastX));
			const double AtPortal = Plan::AtriumAxisX - FMath::Sqrt(Radius * Radius - Plan::PortalHalf * Plan::PortalHalf);
			Points.Add({Plan::FootprintEastX, Side * AtEdge});
			AppendArc(Points, AtriumAxis, Radius, AngleAbout(AtriumAxis, Points.Last(), 180.0), AngleAbout(AtriumAxis, {AtPortal, Side * Plan::PortalHalf}, 180.0),
				Plan::AtriumStations);
			Points.Append({{Plan::PortalFrontX, Side * Plan::PortalHalf}, {Plan::PortalFrontX, Side * Plan::HallHalf}});
		}

		/** One garden between two heights, its east edge on a circle about the Atrium's axis. */
		FSitePrism Garden(double Side, double Radius, double Bottom, double Top)
		{
			FSitePrism Prism;
			Prism.OutlineMeters = {{Plan::FootprintWestX, Side * Plan::FootprintHalf}, {Plan::FootprintEastX, Side * Plan::FootprintHalf}};
			AddGardenEast(Prism.OutlineMeters, Side, Radius);
			Prism.OutlineMeters.Add({Plan::FootprintWestX, Side * Plan::HallHalf});
			Prism.BottomMeters = Bottom;
			Prism.TopMeters = Top;
			return Prism;
		}

		/** The hall band west of the portal's front: |y| <= 4.80 from the west port face on the drum. */
		FSitePrism HallBand()
		{
			const FEndFace WestPort = PortFace(EHallEnd::West);
			FSitePrism Prism;
			Prism.OutlineMeters = {{Plan::PortalFrontX, -Plan::HallHalf}, {Plan::PortalFrontX, Plan::HallHalf}, WestPort.At(Plan::HallHalf)};
			AppendArc(Prism.OutlineMeters, RotundaAxis, WestPort.Radius, WestPort.AngleDegrees(Plan::HallHalf), WestPort.AngleDegrees(-Plan::HallHalf),
				Plan::RotundaStations);
			Prism.BottomMeters = Plan::LiningBottom;
			Prism.TopMeters = Plan::LiningCrown;
			return Prism;
		}

		/** The hall's end in the Atrium's portal between two heights, |y| <= Half, from the portal's front to the east port face. */
		FSitePrism EndInPortal(double Half, double Bottom, double Top)
		{
			const FEndFace EastPort = PortFace(EHallEnd::East);
			FSitePrism Prism;
			Prism.OutlineMeters = {{Plan::PortalFrontX, -Half}, EastPort.At(-Half)};
			AppendArc(Prism.OutlineMeters, AtriumAxis, EastPort.Radius, EastPort.AngleDegrees(-Half), EastPort.AngleDegrees(Half), Plan::AtriumStations);
			Prism.OutlineMeters.Add({Plan::PortalFrontX, Half});
			Prism.BottomMeters = Bottom;
			Prism.TopMeters = Top;
			return Prism;
		}

		/**
		 * The hall's end in the portal, stepped as the linings' outline (sec. 3.1): 4.80 wide to the kerbs' top, 4.68 to the
		 * plinths', 4.60 to the eaves' top, then the arc R 5.42 about z 2.20 in level steps up to the crown, each step as wide
		 * as the arc at its bottom.
		 */
		void AddEndInPortal(TArray<FSitePrism>& Out)
		{
			Out.Add(EndInPortal(Plan::LiningKerbHalf, Plan::LiningBottom, Plan::KerbTop));
			Out.Add(EndInPortal(Plan::LiningPlinthHalf, Plan::KerbTop, Plan::PlinthTop));
			Out.Add(EndInPortal(Plan::LiningEavesHalf, Plan::PlinthTop, Plan::EavesTop));
			const int32 Steps = FMath::CeilToInt32((Plan::LiningCrown - Plan::EavesTop) / Plan::NotchStepMeters);
			for (int32 Step = 0; Step < Steps; ++Step)
			{
				const double Bottom = Plan::EavesTop + Step * Plan::NotchStepMeters;
				const double Top = FMath::Min(Bottom + Plan::NotchStepMeters, Plan::LiningCrown);
				const double Half = FMath::Sqrt(FMath::Square(Plan::LiningArcRadius) - FMath::Square(Bottom - Plan::VaultAxisZ));
				Out.Add(EndInPortal(FMath::Min(Half, Plan::LiningEavesHalf), Bottom, Top));
			}
		}

		/**
		 * The footprint (sec. 1). Its outline, clockwise on the north-up plan from F1: the meadow's north edge, the Atrium's
		 * socle and its portal's front, the east port face through the axis, the same mirrored round the orchard, and back
		 * along the orchard's south edge to the west port face on the Rotunda's drum. Its prisms: the hall band, the hall's end
		 * in the Atrium's portal, and the two gardens under and over the Atrium's corona.
		 */
		void Footprint(FSiteDescription& Out)
		{
			const FEndFace EastPort = PortFace(EHallEnd::East);
			const FEndFace WestPort = PortFace(EHallEnd::West);
			const double Hall = Plan::HallHalf;
			TArray<FVector2D> Points = {{Plan::FootprintWestX, -Plan::FootprintHalf}, {Plan::FootprintEastX, -Plan::FootprintHalf}};
			AddGardenEast(Points, -1.0, Plan::SocleRadius);
			Points.Add(EastPort.At(-Hall));
			AppendArc(Points, AtriumAxis, EastPort.Radius, EastPort.AngleDegrees(-Hall), EastPort.AngleDegrees(Hall), Plan::AtriumStations);
			TArray<FVector2D> Orchard;
			AddGardenEast(Orchard, 1.0, Plan::SocleRadius);
			Algo::Reverse(Orchard);
			Points.Append(Orchard);
			Points.Append({{Plan::FootprintEastX, Plan::FootprintHalf}, {Plan::FootprintWestX, Plan::FootprintHalf}, {Plan::FootprintWestX, Hall},
				WestPort.At(Hall)});
			AppendArc(Points, RotundaAxis, WestPort.Radius, WestPort.AngleDegrees(Hall), WestPort.AngleDegrees(-Hall), Plan::RotundaStations);
			Points.Add({Plan::FootprintWestX, -Hall});
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			Out.Footprint.OutlineMeters = MoveTemp(Points);
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;

			TArray<FSitePrism>& Prisms = Out.Footprint.Prisms;
			Prisms.Add(HallBand());
			AddEndInPortal(Prisms);
			for (const double Side : {-1.0, 1.0})
			{
				Prisms.Add(Garden(Side, Plan::SocleRadius, Plan::FootprintBottom, Plan::AtriumCoronaUnderside));
				Prisms.Add(Garden(Side, Plan::AtriumCoronaRadius, Plan::AtriumCoronaUnderside, Plan::FootprintTop));
			}
		}

		/**
		 * The ports (sec. 2), each on its neighbour's cylinder at the threshold, facing out: the west on the Rotunda's drum,
		 * meeting rotunda.east; the east on the Atrium's base, meeting atrium.west. Each lining is the wall its opening
		 * passes through; the joints (thresholds, strips, surrounds) are Site's.
		 */
		void Ports(FSiteDescription& Out)
		{
			FSitePort& WestPort = Out.Ports.AddDefaulted_GetRef();
			WestPort.Key = Key(TEXT("west"));
			WestPort.Kind = ESitePortKind::Doorway;
			WestPort.Class = FMuseeKey(Plan::WestClass);
			WestPort.Frame = FrameOfAxes(FVector::ZeroVector, West, North);
			WestPort.WallDepthMeters = Plan::LiningMeters;
			// Each face wraps its neighbour's cylinder, whose axis stands ahead of it, outside the hall: a negative radius (FSitePort).
			WestPort.FaceRadiusMeters = -Plan::WestFaceRadius;
			WestPort.Mates = FMuseeKey(TEXT("rotunda.east"));

			FSitePort& EastPort = Out.Ports.AddDefaulted_GetRef();
			EastPort.Key = Key(TEXT("east"));
			EastPort.Kind = ESitePortKind::Doorway;
			EastPort.Class = FMuseeKey(Plan::EastClass);
			EastPort.Frame = FMuseeFrame(FVector(Plan::EastPortX, 0.0, 0.0));
			EastPort.WallDepthMeters = Plan::LiningMeters;
			EastPort.FaceRadiusMeters = -Plan::EastFaceRadius;
			EastPort.Mates = FMuseeKey(TEXT("atrium.west"));
		}

		FFormOpening& Opening(FSiteDescription& Out, const FString& Local, EFormOpeningKind Kind, const FMuseeFrame& Frame, const FVector2D& Size)
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
		 * The openings, all of the hall (sec. 1): the two ports' class openings on their faces; the glazed walls between
		 * their end posts; the vault over its springing plane; the lunette in the east lining; and the twenty step lights'
		 * housings in the plinths, each looking into its pocket.
		 */
		void Openings(FSiteDescription& Out)
		{
			const double WestCrown = Plan::WestSpringing + 0.5 * Plan::WestClearWidth;
			Opening(Out, TEXT("west"), EFormOpeningKind::Port, FrameOfAxes(FVector(0.0, 0.0, 0.5 * WestCrown), North, Up),
				FVector2D(Plan::WestClearWidth, WestCrown));
			Opening(Out, TEXT("east"), EFormOpeningKind::Port, FrameOfAxes(FVector(Plan::EastPortX, 0.0, 0.5 * Plan::EastHead), South, Up),
				FVector2D(Plan::EastClearWidth, Plan::EastHead));

			const double WallWest = InnerFace(EHallEnd::West, Plan::EndPostAlong).X(Plan::GlassLine);
			const double WallEast = InnerFace(EHallEnd::East, Plan::EndPostAlong).X(Plan::GlassLine);
			const double WallMiddle = 0.5 * (Plan::WallGlassFoot + Plan::WallGlassHead);
			const FVector2D WallSize(WallEast - WallWest, Plan::WallGlassHead - Plan::WallGlassFoot);
			for (const int32 Side : Sides)
			{
				const FVector At(0.5 * (WallWest + WallEast), Side * Plan::GlassLine, WallMiddle);
				Opening(Out, FString::Printf(TEXT("wall.%s"), SideName(Side)), EFormOpeningKind::Glazed, FrameOfAxes(At, Side < 0 ? East : West, Up),
					WallSize).Glazing = Surface(TEXT("wall.glass"));
			}

			const double VaultWest = InnerFace(EHallEnd::West).X(0.0);
			const double VaultEast = InnerFace(EHallEnd::East).X(0.0);
			const double Springing = 2.0 * Plan::VaultRadius * FMath::Sin(EavesAngle());
			Opening(Out, TEXT("vault"), EFormOpeningKind::Glazed, FMuseeFrame(FVector(0.5 * (VaultWest + VaultEast), 0.0, Plan::EavesTop)),
				FVector2D(VaultEast - VaultWest, Springing)).Glazing = Surface(TEXT("vault.glass"));

			const double Reach = FMath::Sqrt(FMath::Square(Plan::LunetteRadius) - FMath::Square(Plan::LunetteSill - Plan::VaultAxisZ));
			const double LunetteTop = Plan::VaultAxisZ + Plan::LunetteRadius;
			Opening(Out, TEXT("lunette"), EFormOpeningKind::Glazed,
				FrameOfAxes(FVector(VaultEast, 0.0, 0.5 * (Plan::LunetteSill + LunetteTop)), South, Up), FVector2D(2.0 * Reach, LunetteTop - Plan::LunetteSill))
				.Glazing = Surface(TEXT("lunette.glass"));

			for (const int32 Side : Sides)
			{
				for (int32 Post = 0; Post < Plan::Posts; ++Post)
				{
					const FVector Mouth(PostX(Post), WallY(Side, Plan::PlinthHalf), 0.5 * (Plan::HousingBottom + Plan::HousingTop));
					Opening(Out, FString::Printf(TEXT("plinth.%s.h%d"), SideName(Side), Post), EFormOpeningKind::Housing,
						FrameOfAxes(Mouth, Side < 0 ? East : West, Up), FVector2D(2.0 * Plan::HousingHalf, Plan::HousingTop - Plan::HousingBottom));
				}
			}
		}

		void Anchor(FSiteDescription& Out, const FString& Local, const TCHAR* Kind, const FMuseeFrame& Frame)
		{
			Out.Anchors.Add({Key(Local), FName(Kind), Frame});
		}

		/**
		 * The hanging walls and the stereo readers (sec. 2): origin at the viewer's left on the hanging plane, y towards the
		 * viewer; a reader's at the centre of its plate's face, where the card lies.
		 */
		void WorkAnchors(FSiteDescription& Out)
		{
			const TPair<const TCHAR*, double> Planes[] = {{TEXT("prints"), Plan::PrintsD}, {TEXT("autochromes"), Plan::AutochromesD}};
			for (const TPair<const TCHAR*, double>& Plane : Planes)
			{
				Anchor(Out, FString::Printf(TEXT("%s.north"), Plane.Key), TEXT("Wall"), FrameOfAxes(FVector(0.0, WallY(-1, Plane.Value), 0.0), East, South));
				Anchor(Out, FString::Printf(TEXT("%s.south"), Plane.Key), TEXT("Wall"),
					FrameOfAxes(FVector(Plan::EastPortX, WallY(1, Plane.Value), 0.0), West, North));
			}
			for (int32 Index = 0; Index < Plan::StereoStones; ++Index)
			{
				const FMuseeFrame Reader = StereoReader(Index);
				const FVector Face = Reader.OriginMeters + Reader.DirectionToParent(FVector::RightVector) * (0.5 * Plan::ReaderThickness);
				Anchor(Out, FString::Printf(TEXT("stereo.%d"), Index + 1), TEXT("Mount"), FMuseeFrame(Face, Reader.Rotation));
			}
		}

		/**
		 * The gardens' beds (sec. 2, sec. 7): each garden's ground, and its flower or bulb beds, at the ground's middle
		 * (their plants stand at the points Planting.json plots); each tree at its point; each hedge at its line's middle,
		 * x along it.
		 */
		void GardenAnchors(FSiteDescription& Out)
		{
			const double MiddleX = 0.5 * (Plan::GardenWestX + Plan::GardenEastX);
			const double MiddleY = 0.5 * (Plan::GardenInner + Plan::GardenOuter);
			const FMuseeFrame Meadow(FVector(MiddleX, -MiddleY, 0.0));
			const FMuseeFrame Orchard(FVector(MiddleX, MiddleY, 0.0));
			Anchor(Out, TEXT("meadow.ground"), TEXT("Bed"), Meadow);
			Anchor(Out, TEXT("orchard.ground"), TEXT("Bed"), Orchard);
			for (const TCHAR* Flowers : {TEXT("lilac"), TEXT("pink"), TEXT("copper"), TEXT("yellow")})
			{
				Anchor(Out, FString::Printf(TEXT("meadow.flowers.%s"), Flowers), TEXT("Bed"), Meadow);
			}
			for (const TCHAR* Bulbs : {TEXT("white"), TEXT("pink"), TEXT("yellow")})
			{
				Anchor(Out, FString::Printf(TEXT("orchard.bulbs.%s"), Bulbs), TEXT("Bed"), Orchard);
			}
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::Birches)); ++Index)
			{
				Anchor(Out, FString::Printf(TEXT("birch.%d"), Index + 1), TEXT("Bed"), FMuseeFrame(FVector(Plan::Birches[Index][0], Plan::Birches[Index][1], 0.0)));
			}
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::FruitTrees)); ++Index)
			{
				Anchor(Out, FString::Printf(TEXT("orchard.%d"), Index + 1), TEXT("Bed"),
					FMuseeFrame(FVector(Plan::FruitTrees[Index][0], Plan::FruitTrees[Index][1], 0.0)));
			}
			const double HedgeX = 0.5 * (Plan::HedgeWestX + Plan::HedgeEastX);
			Anchor(Out, TEXT("hedge.north"), TEXT("Bed"), FMuseeFrame(FVector(HedgeX, -Plan::HedgeLine, 0.0)));
			Anchor(Out, TEXT("hedge.south"), TEXT("Bed"), FMuseeFrame(FVector(HedgeX, Plan::HedgeLine, 0.0)));
		}

		/** The drawn pieces whose names start with any of the prefixes. */
		TArray<FName> PiecesNamed(const FSiteDescription& Out, TConstArrayView<const TCHAR*> Prefixes)
		{
			TArray<FName> Names;
			for (const FFormDrawing& Piece : Out.Pieces)
			{
				const FString Name = Piece.Name.ToString();
				if (Algo::AnyOf(Prefixes, [&Name](const TCHAR* Prefix) { return Name == Prefix || Name.StartsWith(FString(Prefix) + TEXT(".")); }))
				{
					Names.Add(Piece.Name);
				}
			}
			return Names;
		}

		/**
		 * The enclosures: the hall, bounded by its floor and viewing stones, the plinths, the glass walls with their steel,
		 * the eaves, the vault with its steel, the linings and the lunette; and the two gardens, outdoors, edged by the kerbs.
		 */
		void Enclosures(FSiteDescription& Out)
		{
			const TCHAR* const Shell[] = {TEXT("floor"), TEXT("viewing-stone"), TEXT("plinth"), TEXT("lining"), TEXT("post"), TEXT("end-post"),
				TEXT("shoe"), TEXT("wall.glass"), TEXT("eaves"), TEXT("arch"), TEXT("purlin"), TEXT("collar"), TEXT("vault.glass"), TEXT("lunette")};
			FFormEnclosure& Hall = Out.Enclosures.AddDefaulted_GetRef();
			Hall.Key = Key(TEXT("hall"));
			Hall.Kind = EFormEnclosureKind::Enclosed;
			Hall.Pieces = PiecesNamed(Out, Shell);

			FFormEnclosure& Meadow = Out.Enclosures.AddDefaulted_GetRef();
			Meadow.Key = Key(TEXT("meadow"));
			Meadow.Kind = EFormEnclosureKind::Outdoors;
			const TCHAR* const MeadowEdge[] = {TEXT("kerb.north")};
			Meadow.Pieces = PiecesNamed(Out, MeadowEdge);

			FFormEnclosure& Orchard = Out.Enclosures.AddDefaulted_GetRef();
			Orchard.Key = Key(TEXT("orchard"));
			Orchard.Kind = EFormEnclosureKind::Outdoors;
			const TCHAR* const OrchardEdge[] = {TEXT("kerb.south")};
			Orchard.Pieces = PiecesNamed(Out, OrchardEdge);
		}
	}

	void DescribeSite(FSiteDescription& Out)
	{
		Footprint(Out);
		Ports(Out);
		Openings(Out);
		WorkAnchors(Out);
		GardenAnchors(Out);
		Enclosures(Out);
	}
}
