#include "Algo/AnyOf.h"
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

		/** Adds an arc's stations after its start, to its end, at Form's chord rule. */
		void AppendArc(TArray<FVector2D>& Points, const FVector2D& Centre, double Radius, double FromDegrees, double ToDegrees)
		{
			const double Step = 2.0 * FMath::Acos(1.0 - Plan::ChordToleranceMeters / Radius);
			const double Sweep = FMath::DegreesToRadians(ToDegrees - FromDegrees);
			const int32 Count = FMath::Max(1, FMath::CeilToInt32(FMath::Abs(Sweep) / Step));
			for (int32 Index = 1; Index <= Count; ++Index)
			{
				const double Angle = FMath::DegreesToRadians(FromDegrees) + Sweep * Index / Count;
				Points.Add(Centre + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
			}
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
		void Footprint(FSiteDescription& Out)
		{
			const FVector2D Atrium(Plan::AtriumAxisX, 0.0);
			const FVector2D Rotunda(Plan::RotundaAxisX, 0.0);
			const double Socle = Plan::SocleRadius;
			const double SocleAtEdge = FMath::Sqrt(Socle * Socle - FMath::Square(Plan::AtriumAxisX - Plan::FootprintEastX));
			const double SocleAtPortal = Plan::AtriumAxisX - FMath::Sqrt(Socle * Socle - Plan::PortalHalf * Plan::PortalHalf);
			const FEndFace EastPort = PortFace(EHallEnd::East);
			const FEndFace WestPort = PortFace(EHallEnd::West);
			const double Hall = Plan::HallHalf;
			TArray<FVector2D> Points = {{Plan::FootprintWestX, -Plan::FootprintHalf}, {Plan::FootprintEastX, -Plan::FootprintHalf},
				{Plan::FootprintEastX, -SocleAtEdge}};
			AppendArc(Points, Atrium, Socle, AngleAbout(Atrium, Points.Last(), 180.0), AngleAbout(Atrium, {SocleAtPortal, -Plan::PortalHalf}, 180.0));
			Points.Append({{Plan::PortalFrontX, -Plan::PortalHalf}, {Plan::PortalFrontX, -Hall}, EastPort.At(-Hall)});
			AppendArc(Points, Atrium, EastPort.Radius, EastPort.AngleDegrees(-Hall), EastPort.AngleDegrees(Hall));
			Points.Append({{Plan::PortalFrontX, Hall}, {Plan::PortalFrontX, Plan::PortalHalf}, {SocleAtPortal, Plan::PortalHalf}});
			AppendArc(Points, Atrium, Socle, AngleAbout(Atrium, Points.Last(), 180.0), AngleAbout(Atrium, {Plan::FootprintEastX, SocleAtEdge}, 180.0));
			Points.Append({{Plan::FootprintEastX, Plan::FootprintHalf}, {Plan::FootprintWestX, Plan::FootprintHalf}, {Plan::FootprintWestX, Hall},
				WestPort.At(Hall)});
			AppendArc(Points, Rotunda, WestPort.Radius, WestPort.AngleDegrees(Hall), WestPort.AngleDegrees(-Hall));
			Points.Add({Plan::FootprintWestX, -Hall});
			Out.Footprint.Kind = ESiteFootprintKind::Building;
			Out.Footprint.OutlineMeters = MoveTemp(Points);
			Out.Footprint.BottomMeters = Plan::FootprintBottom;
			Out.Footprint.TopMeters = Plan::FootprintTop;
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
			WestPort.FaceRadiusMeters = Plan::WestFaceRadius;
			WestPort.Mates = FMuseeKey(TEXT("rotunda.east"));

			FSitePort& EastPort = Out.Ports.AddDefaulted_GetRef();
			EastPort.Key = Key(TEXT("east"));
			EastPort.Kind = ESitePortKind::Doorway;
			EastPort.Class = FMuseeKey(Plan::EastClass);
			EastPort.Frame = FMuseeFrame(FVector(Plan::EastPortX, 0.0, 0.0));
			EastPort.WallDepthMeters = Plan::LiningMeters;
			EastPort.FaceRadiusMeters = Plan::EastFaceRadius;
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
