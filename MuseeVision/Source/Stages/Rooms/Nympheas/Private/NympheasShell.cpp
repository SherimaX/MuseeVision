#include "FormCatalogue.h"
#include "NympheasParts.h"
#include "NympheasPlan.h"

/**
 * The oval's shell (§3.1-3.3): the elliptical wall with its door at the east apex, the passage's masonry out to the
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
		 * with the door half-way round at the east apex, as wide on the face as the arc between its jambs.
		 */
		FFormDrawing OvalWall()
		{
			FFormWallOpening Door;
			Door.CenterMeters = 0.5 * Oval().Length(0.0);
			Door.WidthMeters = 2.0 * Oval().ArcAt(JambT(), 0.0);
			Door.HeadMeters = Plan::DoorSpringing;
			Door.bArched = true;
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
		 * The passage's masonry. The portal is the port face's slab, from the port face back to the plane that touches the
		 * oval's outer face on the axis, pierced by the passage (the port's class: 3.00 wide, arched from 3.30); the
		 * cheeks fill between that plane and the curved outer face beside the passage, floor to the block's top.
		 */
		void Passage(TArray<FFormDrawing>& Out)
		{
			const double Back = Plan::OvalA + Plan::WallMeters;
			const double Half = 0.5 * Plan::DoorWidth;
			const double Block = Plan::BlockHalfWidth;
			FFormExtrusion Portal;
			Portal.Outline = FFormProfile::From(FVector2D(-Block, 0.0));
			Portal.Outline.LineTo(FVector2D(-Half, 0.0), EFormJoint::Crisp).LineTo(FVector2D(-Half, Plan::DoorSpringing), EFormJoint::Crisp)
				.ArcAbout(FVector2D(0.0, Plan::DoorSpringing), Half, Half, 180.0, 0.0).LineTo(FVector2D(Half, 0.0))
				.LineTo(FVector2D(Block, 0.0), EFormJoint::Crisp).LineTo(FVector2D(Block, Plan::BlockTop), EFormJoint::Crisp)
				.LineTo(FVector2D(-Block, Plan::BlockTop), EFormJoint::Crisp).Close();
			Portal.DepthMeters = Plan::PortFaceX - Back;
			const FMuseeFrame Across = FrameOfAxes(FVector(Back, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector(0.0, 0.0, 1.0));
			Out.Add(Drawn(Pieces::Portal, TEXT("passage.reveal"), MoveTemp(Portal), Solid, Across));

			// The north cheek, between y -2.10 and -1.50: the plane, then back along the outer face.
			const double Near = Oval().TWhereY(-Half, Plan::WallMeters, -UE_DOUBLE_HALF_PI, 0.0);
			const double Far = Oval().TWhereY(-Block, Plan::WallMeters, -UE_DOUBLE_HALF_PI, 0.0);
			const int32 Stations = FMath::Max(2, FMath::CeilToInt32(Plan::OvalStations * (Near - Far) / (2.0 * UE_DOUBLE_PI)));
			TArray<FVector2D> North = {FVector2D(Back, -Block), FVector2D(Back, -Half)};
			North.Append(Oval().Stations(Plan::WallMeters, Near, Far, Stations));
			Out.Add(Drawn(Pieces::CheekNorth, TEXT("passage.reveal"), Slab(Polygon(North), {}, 0.0, Plan::BlockTop), Solid));
			Out.Add(Drawn(Pieces::CheekSouth, TEXT("passage.reveal"), Slab(Polygon(MirroredInY(North)), {}, 0.0, Plan::BlockTop), Solid));
		}

		/** The cove (P9) and skirting (P10), run round the inner face: the cove unbroken over the door, the skirting stopped either side of it. */
		void Mouldings(TArray<FFormDrawing>& Out)
		{
			FFormSweep Cove;
			Cove.Section = Moulding(Plan::CoveProfile);
			Cove.Path = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::OvalA, Plan::OvalB, 180.0, true);
			Cove.ElevationMeters = Plan::CoveFoot;
			Out.Add(Drawn(TEXT("oval.cove"), TEXT("cove"), MoveTemp(Cove), nullptr));

			const double From = FMath::Asin(Plan::SkirtingEndY / Plan::OvalB);
			FFormSweep Skirting;
			Skirting.Section = Moulding(Plan::SkirtingProfile);
			Skirting.Path = FFormProfile::From(Oval().Point(From)).ArcAbout(FVector2D::ZeroVector, Plan::OvalA, Plan::OvalB,
				FMath::RadiansToDegrees(From), 360.0 - FMath::RadiansToDegrees(From));
			Out.Add(Drawn(TEXT("oval.skirting"), TEXT("wall.skirting"), MoveTemp(Skirting), nullptr));
		}

		/** A dome springing from the rim line's ellipse at a height. */
		FFormDome RimDome(double Springing, double Rise, double Thickness, FName Outer)
		{
			FFormDome Dome;
			Dome.Frame = FMuseeFrame(FVector(0.0, 0.0, Springing));
			Dome.SemiAxisXMeters = Plan::RimA;
			Dome.SemiAxisYMeters = Plan::RimB;
			Dome.RiseMeters = Rise;
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
			Ring.Section = Polygon({FVector2D(0.0, 0.0), FVector2D(Leg, 0.0), FVector2D(Leg, Thick), FVector2D(Thick, Thick),
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
			Roof.Outline = Polygon(Outer);
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
		Passage(Out);
		Mouldings(Out);
		Ceiling(Out);
	}
}
