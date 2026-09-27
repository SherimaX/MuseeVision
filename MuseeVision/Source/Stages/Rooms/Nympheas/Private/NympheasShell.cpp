#include "FormCatalogue.h"
#include "NympheasParts.h"
#include "NympheasPlan.h"

/**
 * The oval's shell (§3.1-3.3): the elliptical wall with its door at the east apex, the passage block out to the
 * port face, the cove and skirting round the room, the linen velarium on its bronze rim ring, the attic's upstand, the
 * roof ring and the glass dome over it all.
 */
namespace Musee::Nympheas
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** A catalogue moulding; when the catalogue lacks it, an empty section, which Form's making reports as undrawn. */
		FFormProfile Moulding(const TCHAR* Profile)
		{
			const TValueOrError<FFormProfile, FString> Found = Musee::Form::FindProfile(FMuseeKey(Profile));
			return Found.HasValue() ? Found.GetValue() : FFormProfile();
		}

		/**
		 * The oval's wall: its inner face drawn anticlockwise from the west apex, so its thickness lies outside the room,
		 * with the door half-way round at the east apex: a straight passage 3.00 wide through the curved wall, its jambs on
		 * y ±1.50 from the inner face to the outer, running on into the passage's masonry.
		 */
		FFormDrawing OvalWall()
		{
			FFormWallOpening Door;
			Door.CenterMeters = 0.5 * Oval().Length(0.0);
			Door.WidthMeters = Plan::DoorWidth;
			Door.HeadMeters = Plan::DoorSpringing;
			Door.bArched = true;
			Door.bStraight = true;
			FFormWall Shape;
			Shape.Plan = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::OvalA, Plan::OvalB, 180.0, false);
			Shape.ThicknessMeters = Plan::WallMeters;
			Shape.HeightMeters = Plan::RingSoffit;
			Shape.Openings.Add(Door);
			FFormDrawing Drawing = Drawn(Pieces::OvalWall, TEXT("wall.plaster"), MoveTemp(Shape), Solid);
			Drawing.PartSurfaces.Add(FFormWall::BackPart, Surface(TEXT("exterior.wall")));
			Drawing.PartSurfaces.Add(FFormWall::TopPart, Surface(TEXT("exterior.wall")));
			Drawing.PartSurfaces.Add(FFormWall::BedPart, Surface(TEXT("exterior.wall")));
			Drawing.PartSurfaces.Add(FFormWall::RevealPart, Surface(TEXT("passage.reveal")));
			return Drawing;
		}

		/**
		 * The passage block (§3.1): masonry filling |y| ≤ 2.10 between the port face (x 11.90) and the oval's curved outer
		 * face, floor to its flat top, pierced by the passage: the same straight passage as the oval's door, 3.00 wide and
		 * arched from 3.30, running on from the wall's jambs to the port face. Drawn as a wall between two faces, its face on
		 * the port face (walked south, so the block lies to its right) and its back on the oval's outer face.
		 */
		FFormDrawing PassageBlock()
		{
			const double Block = Plan::BlockHalfWidth;
			const double Reach = Oval().TWhereY(Block, Plan::WallMeters, 0.0, UE_DOUBLE_HALF_PI);
			const int32 Stations = FMath::Max(2, FMath::CeilToInt32(Plan::OvalStations * Reach / UE_DOUBLE_PI));
			FFormWallOpening Passage;
			Passage.CenterMeters = Block;
			Passage.WidthMeters = Plan::DoorWidth;
			Passage.HeadMeters = Plan::DoorSpringing;
			Passage.bArched = true;
			Passage.bStraight = true;
			FFormWall Shape;
			Shape.Plan = FFormProfile::Polyline({FVector2D(Plan::PortFaceX, -Block), FVector2D(Plan::PortFaceX, Block)}, false);
			Shape.Back = SmoothPath(Oval().Stations(Plan::WallMeters, -Reach, Reach, Stations));
			Shape.HeightMeters = Plan::BlockTop;
			Shape.Openings.Add(Passage);
			FFormDrawing Drawing = Drawn(Pieces::PassageBlock, TEXT("exterior.wall"), MoveTemp(Shape), Solid);
			Drawing.PartSurfaces.Add(FFormWall::RevealPart, Surface(TEXT("passage.reveal")));
			return Drawing;
		}

		/** A distance along the oval's inner face from the west apex, anticlockwise (the wall's own U), where it stands at y. */
		double AlongFace(double Y)
		{
			const double T = FMath::Asin(Y / Plan::OvalB);
			return 0.5 * Oval().Length(0.0) - FMath::Sign(T) * Oval().ArcAt(FMath::Abs(T), 0.0);
		}

		/**
		 * P6, the slim architrave round the door (§3.2), drawn on the oval's inner face in its development: up the south
		 * jamb, over the arch at its stations and down the north jamb, its section out from the opening and proud of the face.
		 */
		FFormDrawing Architrave()
		{
			const double Half = 0.5 * Plan::DoorWidth;
			TArray<FVector2D> Points = {FVector2D(AlongFace(Half), 0.0)};
			for (int32 Station = 0; Station <= Plan::ArchStations; ++Station)
			{
				const double Angle = UE_DOUBLE_PI * Station / Plan::ArchStations;
				Points.Add(FVector2D(AlongFace(Half * FMath::Cos(Angle)), Plan::DoorSpringing + Half * FMath::Sin(Angle)));
			}
			Points.Add(FVector2D(AlongFace(-Half), 0.0));
			FFormSweep Shape;
			Shape.Section = Moulding(Plan::ArchitraveProfile);
			Shape.Face = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::OvalA, Plan::OvalB, 180.0, false);
			Shape.Path = SmoothPath(Points);
			return Drawn(TEXT("oval.door.architrave"), TEXT("door.architrave"), MoveTemp(Shape), Solid);
		}

		/**
		 * The cove (P9) and skirting (P10), run round the inner face, and the door's architrave (P6): the cove unbroken over
		 * the door, the skirting stopped against the architrave's outer edge either side of it.
		 */
		void Mouldings(TArray<FFormDrawing>& Out)
		{
			Out.Add(Architrave());

			FFormSweep Cove;
			Cove.Section = Moulding(Plan::CoveProfile);
			Cove.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::OvalA, Plan::OvalB, 180.0, true);
			Cove.ElevationMeters = Plan::CoveFoot;
			Out.Add(Drawn(TEXT("oval.cove"), TEXT("cove"), MoveTemp(Cove), nullptr));

			const double From = Oval().TAt(Oval().ArcAt(JambT(), 0.0) + Plan::ArchitraveWidth, 0.0);
			FFormSweep Skirting;
			Skirting.Section = Moulding(Plan::SkirtingProfile);
			Skirting.Path = FFormProfile::From(Oval().Point(From)).ArcAbout(FVector2D::ZeroVector, Plan::OvalA, Plan::OvalB,
				FMath::RadiansToDegrees(From), 360.0 - FMath::RadiansToDegrees(From));
			Out.Add(Drawn(TEXT("oval.skirting"), TEXT("wall.skirting"), MoveTemp(Skirting), nullptr));
		}

		/** A paraboloid springing from the rim line's ellipse at a height, as the cloth and the glass rise (§3.3). */
		FFormDome RimDome(double Springing, double Rise, double Thickness, FName Outer)
		{
			FFormDome Dome;
			Dome.Frame = FMuseeFrame(FVector(0.0, 0.0, Springing));
			Dome.SemiAxisXMeters = Plan::RimA;
			Dome.SemiAxisYMeters = Plan::RimB;
			Dome.RiseMeters = Rise;
			Dome.bParaboloid = true;
			Dome.ThicknessMeters = Thickness;
			Dome.Surface = Outer;
			return Dome;
		}

		/**
		 * The ceiling: the linen velarium laced to its bronze rim ring on the cove's top; the attic's upstand at the rim
		 * line carrying the roof ring, which spans from the outer face to the rim line; the glass dome over the attic.
		 */
		void Ceiling(TArray<FFormDrawing>& Out)
		{
			FFormDrawing Velarium = Drawn(TEXT("velarium"), TEXT("velarium"), RimDome(Plan::RimZ, Plan::VelariumRise, 0.0,
				Surface(TEXT("velarium"))), nullptr);
			Velarium.bNanite = false;
			Out.Add(MoveTemp(Velarium));

			// The rim ring: an angle whose upright leg stands against the upstand and whose flat leg runs in under the cloth's edge.
			const double Leg = Plan::RimAngleLeg;
			const double Thick = Plan::RimAngleThickness;
			FFormSweep Ring;
			Ring.Section = FFormProfile::Polygon({FVector2D(0.0, 0.0), FVector2D(Leg, 0.0), FVector2D(Leg, Thick), FVector2D(Thick, Thick),
				FVector2D(Thick, Leg), FVector2D(0.0, Leg)});
			Ring.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::RimA, Plan::RimB, 180.0, true);
			Ring.ElevationMeters = Plan::RimZ;
			Out.Add(Drawn(TEXT("velarium.frame.ring"), TEXT("velarium.frame"), MoveTemp(Ring), nullptr));

			FFormWall Upstand;
			Upstand.Plan = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::RimA, Plan::RimB, 180.0, false);
			Upstand.ThicknessMeters = Plan::UpstandMeters;
			Upstand.BaseMeters = Plan::RimZ;
			Upstand.HeightMeters = Plan::RingSoffit - Plan::RimZ;
			Out.Add(Drawn(Pieces::Upstand, TEXT("attic.upstand"), MoveTemp(Upstand), Solid));

			TArray<FVector2D> Outer = Oval().Stations(Plan::WallMeters, 0.0, 2.0 * UE_DOUBLE_PI, Plan::OvalStations);
			Outer.Pop();
			FFormCeiling Roof;
			Roof.Outline = FFormProfile::Polygon(Outer);
			Roof.Opening = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::RimA, Plan::RimB, 0.0, true);
			Roof.ThicknessMeters = Plan::RingTop - Plan::RingSoffit;
			Out.Add(Drawn(Pieces::RoofRing, TEXT("exterior.roof"), MoveTemp(Roof), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::RingSoffit))));

			FFormDome Glass = RimDome(Plan::RingTop, Plan::DomeRise, Plan::DomeGlassMeters, Surface(TEXT("roof.glass")));
			Glass.InnerSurface = Surface(TEXT("roof.glass"));
			FFormDrawing Dome = Drawn(Pieces::RoofGlass, TEXT("roof.glass"), MoveTemp(Glass), Solid);
			Dome.bNanite = false;
			Out.Add(MoveTemp(Dome));
		}
	}

	void DrawShell(TArray<FFormDrawing>& Out)
	{
		Out.Add(OvalWall());
		Out.Add(PassageBlock());
		Mouldings(Out);
		Ceiling(Out);
	}
}
