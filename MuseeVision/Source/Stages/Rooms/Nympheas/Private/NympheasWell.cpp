#include "NympheasParts.h"
#include "NympheasPlan.h"
#include "SiteJoint.h"
#include "SitePortClass.h"

/**
 * The well and its stair (§3.7): the foundation slab, the brick west end wall and the head wall round the Reserve's
 * door, the long stone flight of 36 risers running east down from the top landing, its closed strings, the two
 * landings, and the bronze handrails on both walls with their brackets.
 */
namespace Musee::Nympheas
{
	namespace
	{
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		double Riser() { return Plan::StairRise / Plan::Risers; }

		/** The nosing line: the height of the line through every nosing at x, the top landing's edge at z 0. */
		double Nosing(double X) { return -(X - Plan::TopRiserX) * Riser() / Plan::Going; }

		/** Where the nosing line, offset by a height, meets a level. */
		double NosingReaches(double Offset, double Z) { return Plan::TopRiserX - (Z - Offset) * Plan::Going / Riser(); }

		/** The last (lowest) riser's x: the foot of the flight. */
		double FootX() { return Plan::TopRiserX + (Plan::Risers - 1) * Plan::Going; }

		/** A frame whose (x, y) plane is the stage's vertical (y, z) plane, facing east: for shapes drawn in elevation looking west. */
		FMuseeFrame Elevation(double X)
		{
			return FrameOfAxes(FVector(X, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector(0.0, 0.0, 1.0));
		}

		/** A frame whose (x, y) plane is the stage's vertical (x, z) plane, its z facing north: for shapes drawn in section along the flight. */
		FMuseeFrame Section(double Y)
		{
			return FrameOfAxes(FVector(0.0, Y, 0.0), FVector(1.0, 0.0, 0.0), FVector(0.0, 0.0, 1.0));
		}

		/**
		 * The rebate in the bottom landing's top at the port face (x across, z down): half the Reserve's door class's bronze
		 * strip, which the joint lays astride the mating plane, and the strip's depth (the joint's record). The class has no
		 * allowance, so the joint lays no threshold: the landing is the oval's, and the strip lies between it and the
		 * Reserve's own threshold, as the passage's floor stops for the east door's strip. None when a record can't be read.
		 */
		FVector2D StripRebate()
		{
			const TValueOrError<FSitePortClass, FString> Door = Musee::Site::FindPortClass(FMuseeKey(Plan::StairClass));
			const TValueOrError<FSiteJointRecord, FString> Joint = Musee::Site::ReadJoints();
			if (Door.HasError() || Joint.HasError() || Door.GetValue().StripMeters <= 0.0)
			{
				return FVector2D::ZeroVector;
			}
			return FVector2D(0.5 * Door.GetValue().StripMeters, Joint.GetValue().StripDepthMeters);
		}

		/** The foundation under the well, the west end wall, and the head wall over the Reserve's arched door. */
		void Masonry(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(Pieces::WellBase, TEXT("well.wall"), Block(FVector(Plan::EndWallWestX, -Plan::ShaftOuterHalf, Plan::FoundationUnderside),
				FVector(Plan::PortFaceX, Plan::ShaftOuterHalf, Plan::WallBed)), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::FoundationUnderside))));
			Out.Add(Drawn(Pieces::EndWall, TEXT("well.wall"), Block(FVector(Plan::EndWallWestX, -Plan::ShaftHalf, Plan::WallBed),
				FVector(Plan::OpeningWestX, Plan::ShaftHalf, -Plan::FloorStone)), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::WallBed))));

			// The head wall fills the shaft's width above the door's arch (the port's class: 2.40 clear, arched from 2.00).
			const double Half = Plan::ShaftHalf;
			const double Springing = Plan::WellFloor + Plan::ReserveDoorSpringing;
			FFormExtrusion Head;
			Head.Outline = FFormProfile::From(FVector2D(Half, Springing));
			Head.Outline.LineTo(FVector2D(Half, Plan::SlabSoffit), EFormJoint::Crisp).LineTo(FVector2D(-Half, Plan::SlabSoffit), EFormJoint::Crisp)
				.LineTo(FVector2D(-Half, Springing), EFormJoint::Crisp).ArcAbout(FVector2D(0.0, Springing), Half, Half, 180.0, 0.0, EFormJoint::Crisp).Close();
			Head.DepthMeters = Plan::PortFaceX - Plan::HeadWallX;
			Out.Add(Drawn(Pieces::HeadWall, TEXT("well.wall"), MoveTemp(Head), Solid, Elevation(Plan::HeadWallX)));
		}

		/**
		 * The flight, set out by Form from its foot: it climbs west from its first riser at the foot, one tread past its
		 * last riser at the top. Its soffit runs flush with the strings' lower edge, 0.35 under the nosing line.
		 */
		void Flight(TArray<FFormDrawing>& Out)
		{
			const double Pitch = FMath::Atan(Riser() / Plan::Going);
			FFormStair Stair;
			Stair.RiseMeters = Plan::StairRise;
			Stair.Risers = Plan::Risers;
			Stair.RunMeters = (Plan::Risers - 1) * Plan::Going;
			Stair.WidthMeters = 2.0 * Plan::StringFace;
			Stair.WaistMeters = Plan::StringUnderNosing * FMath::Cos(Pitch);
			const FMuseeFrame Foot(FVector(FootX(), 0.0, Plan::WellFloor), FRotator(0.0, 180.0, 0.0).Quaternion());
			Out.Add(Drawn(Pieces::Stair, TEXT("stair.tread"), MoveTemp(Stair), Walkable, Foot));

			// The top landing: flush with the floor, wall to wall, round the flight's top tread and over the strings' caps.
			const double TopTread = Plan::TopRiserX - Plan::Going;
			const double Face = Plan::StringFace;
			const double Wall = Plan::ShaftHalf;
			const TArray<FVector2D> Landing = {FVector2D(Plan::OpeningWestX, -Wall), FVector2D(Plan::TopRiserX, -Wall), FVector2D(Plan::TopRiserX, -Face),
				FVector2D(TopTread, -Face), FVector2D(TopTread, Face), FVector2D(Plan::TopRiserX, Face), FVector2D(Plan::TopRiserX, Wall),
				FVector2D(Plan::OpeningWestX, Wall)};
			Out.Add(Drawn(Pieces::TopLanding, TEXT("stair.tread"), Slab(FFormProfile::Polygon(Landing), {}, Plan::StringCapZ, 0.0), Walkable,
				FMuseeFrame(FVector(0.0, 0.0, Plan::StringCapZ))));

			// The bottom landing, from where the flight's soffit meets it, through the head wall's door to the port face, drawn
			// in section; its top rebated at the port face for the half of the joint's bronze strip on the oval's side.
			const double Start = NosingReaches(-Plan::StringUnderNosing, Plan::WellFloor);
			const FVector2D Rebate = StripRebate();
			const TArray<FVector2D> Profile = {FVector2D(Start, Plan::WallBed), FVector2D(Plan::PortFaceX, Plan::WallBed),
				FVector2D(Plan::PortFaceX, Plan::WellFloor - Rebate.Y), FVector2D(Plan::PortFaceX - Rebate.X, Plan::WellFloor - Rebate.Y),
				FVector2D(Plan::PortFaceX - Rebate.X, Plan::WellFloor), FVector2D(Start, Plan::WellFloor)};
			// The section frame's z faces north: pushed from the south wall to the north.
			Out.Add(Drawn(Pieces::BottomLanding, TEXT("stair.tread"), Slab(FFormProfile::Polygon(Profile), {}, 0.0, 2.0 * Wall), Walkable,
				Section(Wall)));
		}

		/**
		 * The strings, 25 mm proud of the brick: from the top landing's west end, capped under its nosing, down the flight
		 * 0.24 over and 0.35 under the nosing line, then a 0.15 skirting along the bottom landing to the head wall, where the
		 * Reserve's door keeps its whole class opening for its closure.
		 */
		void Strings(TArray<FFormDrawing>& Out)
		{
			const double Cap = Plan::StringCapZ;
			const double Over = Plan::StringOverNosing;
			const double Under = Plan::StringUnderNosing;
			const double SkirtingTop = Plan::WellFloor + Plan::LandingSkirting;
			const double West = Plan::OpeningWestX;
			const TArray<FVector2D> Outline = {FVector2D(West, Nosing(West) - Under), FVector2D(NosingReaches(-Under, Plan::WellFloor), Plan::WellFloor),
				FVector2D(Plan::HeadWallX, Plan::WellFloor), FVector2D(Plan::HeadWallX, SkirtingTop), FVector2D(NosingReaches(Over, SkirtingTop), SkirtingTop),
				FVector2D(NosingReaches(Over, Cap), Cap), FVector2D(West, Cap)};
			const double Thick = Plan::ShaftHalf - Plan::StringFace;
			for (const bool bNorth : {true, false})
			{
				FFormExtrusion String = Slab(FFormProfile::Polygon(Outline), {}, 0.0, Thick);
				// The section frame's z faces north: the north string is pushed from its face into the wall, the south from the wall to its face.
				const double From = bNorth ? -Plan::StringFace : Plan::ShaftHalf;
				Out.Add(Drawn(bNorth ? TEXT("stair.string.north") : TEXT("stair.string.south"), TEXT("stair.string"), MoveTemp(String), Solid, Section(From)));
			}
		}

		/** The handrail's axis in the vertical plane (x, z): down the flight 0.90 over the nosings, a knee onto the landing, level to its end. */
		FFormProfile HandrailLine()
		{
			const double Pitch = FMath::Atan(Riser() / Plan::Going);
			const double Level = Nosing(Plan::HandrailKneeX) + Plan::HandrailOverNosing;
			const double Radius = Plan::HandrailKneeRadius;
			const double Tangent = Radius * FMath::Tan(0.5 * Pitch);
			const FVector2D Knee(Plan::HandrailKneeX, Level);
			const FVector2D Down(FMath::Cos(Pitch), -FMath::Sin(Pitch));
			const FVector2D Centre(Knee.X + Tangent, Level + Radius);
			return FFormProfile::From(FVector2D(Plan::HandrailStartX, Nosing(Plan::HandrailStartX) + Plan::HandrailOverNosing))
				.LineTo(Knee - Down * Tangent)
				.ArcAbout(Centre, Radius, Radius, -90.0 - FMath::RadiansToDegrees(Pitch), -90.0, EFormJoint::Smooth)
				.LineTo(FVector2D(Plan::HandrailEndX, Level), EFormJoint::Smooth);
		}

		/** The rail's section: a Ø 50 round with its underside flat across the chord from 240° to 300°, to carry the strip. */
		FFormProfile HandrailSection()
		{
			const double Radius = 0.5 * Plan::HandrailDiameter;
			// A (the rail line's right in its vertical plane) is up; the flat faces down, about A's negative, 180°.
			const double From = 180.0 + Plan::HandrailFlatHalfAngleDeg;
			const double To = 180.0 - Plan::HandrailFlatHalfAngleDeg + 360.0;
			const FVector2D Start(Radius * FMath::Cos(FMath::DegreesToRadians(From)), Radius * FMath::Sin(FMath::DegreesToRadians(From)));
			return FFormProfile::From(Start).ArcAbout(FVector2D::ZeroVector, Radius, Radius, From, To, EFormJoint::Crisp).Close();
		}

		/** The height of the rail's axis at x. */
		double HandrailZ(double X)
		{
			const double Level = Nosing(Plan::HandrailKneeX) + Plan::HandrailOverNosing;
			return FMath::Max(Level, Nosing(X) + Plan::HandrailOverNosing);
		}

		/** A bracket: a rose on the wall, an arm out under the rail, and a stem up to the rail's flat underside. */
		void Bracket(TArray<FFormDrawing>& Out, const TCHAR* Side, int32 Index, double X, double Sign)
		{
			const double Wall = Sign * Plan::ShaftHalf;
			const double Axis = Sign * Plan::HandrailAxisY;
			const double ArmZ = HandrailZ(X) - Plan::ArmDrop;
			const double FlatBelowAxis = 0.5 * Plan::HandrailDiameter * FMath::Cos(FMath::DegreesToRadians(Plan::HandrailFlatHalfAngleDeg));
			// The lathe's axis points out of the wall into the shaft.
			const FQuat OutOfWall(FVector::ForwardVector, Sign < 0.0 ? -UE_DOUBLE_HALF_PI : UE_DOUBLE_HALF_PI);
			auto Drum = [](double Radius, double From, double To)
			{
				FFormLathe Shape;
				Shape.Section = FFormProfile::From(FVector2D(0.0, From));
				Shape.Section.LineTo(FVector2D(Radius, From), EFormJoint::Crisp).LineTo(FVector2D(Radius, To), EFormJoint::Crisp)
					.LineTo(FVector2D(0.0, To), EFormJoint::Crisp);
				return Shape;
			};
			const FString Name = FString::Printf(TEXT("stair.handrail.%s.bracket.%02d"), Side, Index);
			const double Reach = FMath::Abs(Wall - Axis);
			Out.Add(Drawn(*(Name + TEXT(".rose")), TEXT("stair.handrail"), Drum(0.5 * Plan::RoseDiameter, 0.0, Plan::RoseThickness), Solid,
				FMuseeFrame(FVector(X, Wall, ArmZ), OutOfWall)));
			Out.Add(Drawn(*(Name + TEXT(".arm")), TEXT("stair.handrail"), Drum(0.5 * Plan::ArmDiameter, Plan::RoseThickness, Reach), Solid,
				FMuseeFrame(FVector(X, Wall, ArmZ), OutOfWall)));
			Out.Add(Drawn(*(Name + TEXT(".stem")), TEXT("stair.handrail"), Drum(0.5 * Plan::ArmDiameter, 0.0, Plan::ArmDrop - FlatBelowAxis), Solid,
				FMuseeFrame(FVector(X, Axis, ArmZ))));
		}

		/** Both handrails, swept in the walls' vertical plane, with brackets evenly along them at about 1.30. */
		void Handrails(TArray<FFormDrawing>& Out)
		{
			const double Run = Plan::HandrailEndX - Plan::HandrailStartX;
			const int32 Brackets = FMath::Max(2, FMath::RoundToInt32(Run / Plan::BracketSpacing));
			const double Spacing = Run / Brackets;
			for (const bool bNorth : {true, false})
			{
				const double Sign = bNorth ? -1.0 : 1.0;
				const TCHAR* Side = bNorth ? TEXT("north") : TEXT("south");
				FFormSweep Rail;
				Rail.Section = HandrailSection();
				Rail.Path = HandrailLine();
				// The section frame's z faces north, so the rail's elevation is its distance north of the axis.
				Rail.ElevationMeters = -Sign * Plan::HandrailAxisY;
				Out.Add(Drawn(*FString::Printf(TEXT("stair.handrail.%s"), Side), TEXT("stair.handrail"), MoveTemp(Rail), Solid, Section(0.0)));
				for (int32 Index = 0; Index < Brackets; ++Index)
				{
					Bracket(Out, Side, Index, Plan::HandrailStartX + (Index + 0.5) * Spacing, Sign);
				}
			}
		}
	}

	void DrawWell(TArray<FFormDrawing>& Out)
	{
		Masonry(Out);
		Flight(Out);
		Strings(Out);
		Handrails(Out);
	}
}
