#include "FormCatalogue.h"
#include "ReserveParts.h"
#include "ReservePlan.h"

/**
 * The undercroft's shell (§3.1): the four brick walls on their footings with the arched west door (the port) and its
 * travertine architrave on the hall face, the flat brick bed the flags are laid on, and the box's top where it rises
 * under the Salon's floor.
 */
namespace Musee::Reserve
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
		 * The four walls as one, their inner faces drawn anticlockwise on the plan from the north-west corner (down the west
		 * wall first), so the 0.60 of brick lies outside the hall; from the footings to 5.470, where the top and the lawn
		 * bear. The west door is the port's class opening through the west wall on the axis, its sill at the floor.
		 */
		FFormDrawing Walls()
		{
			FFormWallOpening Door;
			Door.CenterMeters = Plan::InnerY;
			Door.WidthMeters = Plan::DoorWidth;
			Door.SillMeters = -Plan::Footing;
			Door.HeadMeters = Plan::DoorSpringing - Plan::Footing;
			Door.bArched = true;
			FFormWall Shape;
			Shape.Plan = FFormProfile::Polygon({FVector2D(-Plan::InnerX, -Plan::InnerY), FVector2D(-Plan::InnerX, Plan::InnerY),
				FVector2D(Plan::InnerX, Plan::InnerY), FVector2D(Plan::InnerX, -Plan::InnerY)});
			Shape.ThicknessMeters = Plan::WallMeters;
			Shape.BaseMeters = Plan::Footing;
			Shape.HeightMeters = Plan::TopLow - Plan::Footing;
			Shape.Openings.Add(Door);
			FFormDrawing Drawing = Drawn(Pieces::Walls, TEXT("wall.brick"), MoveTemp(Shape), Solid);
			for (const TCHAR* Part : {FFormWall::BackPart, FFormWall::TopPart, FFormWall::BedPart})
			{
				Drawing.PartSurfaces.Add(Part, Surface(TEXT("exterior.buried")));
			}
			Drawing.PartSurfaces.Add(FFormWall::RevealPart, Surface(TEXT("door.reveal")));
			return Drawing;
		}

		/**
		 * P6 round the door on the hall face (x -30.00, looking east), drawn in the face's development from the north jamb's
		 * foot on the flags, up and over the semicircular head and down the south jamb, its section out from the opening.
		 */
		FFormDrawing Architrave()
		{
			const double Half = 0.5 * Plan::DoorWidth;
			const double Middle = Plan::InnerY;
			FFormSweep Shape;
			Shape.Section = Moulding(Plan::ArchitraveProfile);
			Shape.Face = FFormProfile::Polyline({FVector2D(-Plan::InnerX, -Plan::InnerY), FVector2D(-Plan::InnerX, Plan::InnerY)}, false);
			Shape.Path = FFormProfile::From(FVector2D(Middle - Half, 0.0));
			Shape.Path.LineTo(FVector2D(Middle - Half, Plan::DoorSpringing), EFormJoint::Crisp)
				.ArcAbout(FVector2D(Middle, Plan::DoorSpringing), Half, Half, 180.0, 0.0, EFormJoint::Smooth)
				.LineTo(FVector2D(Middle + Half, 0.0), EFormJoint::Smooth);
			return Drawn(TEXT("door.architrave"), TEXT("door.architrave"), MoveTemp(Shape), Solid);
		}

		/**
		 * The top where it rises from 5.470 to 5.750 under the Salon's floor: over the aisles from the arcade bands' faces to
		 * |y| 7.550 and over the end walls, round the nave's and the arcade bands' masonry, which rises to 5.750 itself.
		 */
		FFormDrawing Top()
		{
			FFormExtrusion Shape;
			Shape.Outline = FFormProfile::Rectangle(FVector2D(-Plan::SalonHalfX, -Plan::SalonHalfY), FVector2D(Plan::SalonHalfX, Plan::SalonHalfY));
			Shape.Holes.Add(FFormProfile::Rectangle(FVector2D(-Plan::InnerX, -Plan::AisleInner), FVector2D(Plan::InnerX, Plan::AisleInner)));
			Shape.DepthMeters = Plan::TopHigh - Plan::TopLow;
			return Drawn(Pieces::Top, TEXT("exterior.buried"), MoveTemp(Shape), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::TopLow)));
		}
	}

	void DrawShell(TArray<FFormDrawing>& Out)
	{
		Out.Add(Walls());
		Out.Add(Architrave());
		Out.Add(Drawn(Pieces::Bed, TEXT("exterior.buried"), Block(FVector(-Plan::InnerX, -Plan::InnerY, Plan::Footing),
			FVector(Plan::InnerX, Plan::InnerY, -Plan::FlagThickness)), Solid));
		Out.Add(Top());
	}
}
