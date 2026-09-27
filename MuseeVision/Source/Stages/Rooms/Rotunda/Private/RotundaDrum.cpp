#include "RotundaParts.h"
#include "RotundaPlan.h"

/**
 * The drum (§3.2): the wall of 1.20 between the inner face r 10.00 and the outer face r 11.20, pierced by the four
 * doorways' rough openings and the four niches; its footing and the plinth course under the shadow gap; the drum's top
 * course under the dome's springing; and each niche's half-round back, its quarter-sphere head and its laid floor.
 *
 * The doorways run straight through the drum (§2, §3.2): their jambs are the planes parallel to the door's axis at the
 * rough half-width, and their heads the half-cylinder about the springing line, as Form cuts a straight passage through a
 * curved wall; the niches' jambs are radial.
 */
namespace Musee::Rotunda
{
	namespace
	{
		/** The drum's face is drawn from φ 22.5°, between the east door and the south-east niche, so no opening spans its start. */
		constexpr double FaceStartDeg = 22.5;

		/** The distance along the drum's inner face, drawn anticlockwise from its start, to a plan angle. */
		double AlongFace(double Degrees)
		{
			const double Swept = FMath::Fmod(FaceStartDeg - Degrees + 720.0, 360.0);
			return Plan::DrumInner * FMath::DegreesToRadians(Swept);
		}

		/** A doorway's rough half-width across its axis: the clear half and the lining's allowance. */
		double DoorHalf(const Plan::FDoor& Door) { return 0.5 * Door.Clear + Plan::LiningAllowance; }

		/**
		 * The ring between two radii from one doorway to the next round the drum (anticlockwise), cut square to each doorway
		 * on its jamb plane, half its rough opening off its axis, as the wall's straight passages are.
		 */
		FFormProfile BetweenDoors(double Inner, double Outer, const Plan::FDoor& From, const Plan::FDoor& To)
		{
			const double End = To.Deg + (To.Deg <= From.Deg ? 360.0 : 0.0);
			auto Leaving = [&From](double Radius) { return From.Deg + HalfAngleDeg(DoorHalf(From), Radius); };
			auto Reaching = [&To, End](double Radius) { return End - HalfAngleDeg(DoorHalf(To), Radius); };
			FFormProfile Out = FFormProfile::From(Polar(Outer, Leaving(Outer)));
			Out.ArcAbout(FVector2D::ZeroVector, Outer, Outer, Leaving(Outer), Reaching(Outer), EFormJoint::Crisp);
			Out.LineTo(Polar(Inner, Reaching(Inner)), EFormJoint::Crisp);
			Out.ArcAbout(FVector2D::ZeroVector, Inner, Inner, Reaching(Inner), Leaving(Inner), EFormJoint::Crisp);
			return Out.Close();
		}

		/** A niche's half-angle: its half-width on the inner face, where its front meets the chord plane. */
		double NicheHalfDeg() { return FMath::RadiansToDegrees(Plan::NicheHalfWidth / Plan::DrumInner); }

		/** The niche's opening in the wall starts under its laid floor. */
		double NicheOpeningSill() { return Plan::NicheSill - Plan::FinishDepth; }

		/**
		 * The drum's wall, from the shadow gap's soffit to z 10.00 behind the entablature, its face drawn anticlockwise so
		 * its thickness lies outside the room. The doorways run to the wall's bed, where the plinth course stops for them;
		 * the niches open from under their laid floors to their springing, arched.
		 */
		FFormDrawing DrumWallDrawing()
		{
			FFormWall Shape;
			Shape.Plan = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::DrumInner, Plan::DrumInner, FaceStartDeg, false);
			Shape.ThicknessMeters = Plan::DrumOuter - Plan::DrumInner;
			Shape.BaseMeters = Plan::ShadowGapTop;
			Shape.HeightMeters = Plan::DrumWallTop - Plan::ShadowGapTop;
			for (const Plan::FDoor& Door : Plan::Doors)
			{
				FFormWallOpening& Doorway = Shape.Openings.AddDefaulted_GetRef();
				Doorway.CenterMeters = AlongFace(Door.Deg);
				Doorway.WidthMeters = 2.0 * DoorHalf(Door);
				Doorway.HeadMeters = Plan::DoorSpringing - Plan::ShadowGapTop;
				Doorway.bArched = true;
				Doorway.bStraight = true;
			}
			for (const double Deg : Plan::NicheDegrees)
			{
				FFormWallOpening& Recess = Shape.Openings.AddDefaulted_GetRef();
				Recess.CenterMeters = AlongFace(Deg);
				Recess.WidthMeters = 2.0 * Plan::NicheHalfWidth;
				Recess.SillMeters = NicheOpeningSill() - Plan::ShadowGapTop;
				Recess.HeadMeters = Plan::NicheSpringing - Plan::ShadowGapTop;
				Recess.bArched = true;
			}
			FFormDrawing Drawing = Drawn(Pieces::DrumWall, TEXT("drum.wall.inner"), MoveTemp(Shape), Solid);
			Drawing.PartSurfaces.Add(FFormWall::BackPart, Surface(TEXT("drum.wall.outer")));
			Drawing.PartSurfaces.Add(FFormWall::RevealPart, Surface(TEXT("port.rough-opening")));
			Drawing.PartSurfaces.Add(FFormWall::BedPart, Surface(TEXT("drum.shadow-gap")));
			return Drawing;
		}

		/**
		 * Under the wall: the footing, r 10.00 to 11.20 from the floor's bed to the doorways' rough sill; and between the
		 * doorways the plinth course, which carries the wall over the shadow gap (its back r 10.04, its soffit the wall's
		 * bed at 0.15) and gives the gap its 40 mm floor: its bed, r 10.00 to 11.20 from the rough sill to the floor, and over
		 * it the course proper from the gap's back. Each stops on the jamb planes of the doorways either side, as the wall
		 * over it is cut, so the joint's threshold and lining stand against it all the way through the drum.
		 */
		void Footing(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(Pieces::DrumFooting, TEXT("port.rough-opening"), Ring(Plan::DrumInner, Plan::DrumOuter, Plan::FloorBed, Plan::RoughSill), Solid));
			const double Gap = Plan::DrumInner + Plan::ShadowGapDepth;
			const int32 Count = int32(UE_ARRAY_COUNT(Plan::Doors));
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const Plan::FDoor& From = Plan::Doors[Index];
				const Plan::FDoor& To = Plan::Doors[(Index + 1) % Count];
				const FString Name = FString::Printf(TEXT("drum.plinth.%s-%s"), From.Name, To.Name);
				Out.Add(Drawn(Name + TEXT(".bed"), TEXT("drum.shadow-gap"), Slab(BetweenDoors(Plan::DrumInner, Plan::DrumOuter, From, To), {}, -Plan::RoughSill),
					Solid, Raised(Plan::RoughSill)));
				Out.Add(Drawn(Name, TEXT("drum.shadow-gap"), Slab(BetweenDoors(Gap, Plan::DrumOuter, From, To), {}, Plan::ShadowGapTop), Solid));
			}
		}

		/**
		 * The drum's top course: from the wall's top at 10.00 to the drum's top at 11.00 and out to the outer face, under
		 * the dome's springing. Inside, it meets the entablature's back and the lower plain band, and carries the shell's
		 * foot on the line from the shell's outer sphere at 11.00 to the panels' sphere at 4°.
		 */
		void TopCourse(TArray<FFormDrawing>& Out)
		{
			const double Cornice = CorniceDeg();
			FFormLathe Shape;
			Shape.Section = FFormProfile::From(FVector2D(Plan::DrumInner, Plan::DrumWallTop));
			Shape.Section.LineTo(FVector2D(Plan::DrumOuter, Plan::DrumWallTop), EFormJoint::Crisp)
				.LineTo(FVector2D(Plan::DrumOuter, Plan::DrumTop), EFormJoint::Crisp)
				.LineTo(OnDome(Plan::ShellRadius, ElevationAt(Plan::ShellRadius, Plan::DrumTop)), EFormJoint::Crisp)
				.LineTo(OnDome(Plan::PanelRadius, Plan::CofferFromDeg), EFormJoint::Crisp)
				.ArcAbout(FVector2D(0.0, Plan::DomeCentreZ), Plan::PanelRadius, Plan::PanelRadius, Plan::CofferFromDeg, Cornice, EFormJoint::Crisp)
				.LineTo(OnDome(Plan::DomeRadius, Cornice), EFormJoint::Crisp)
				.Close();
			Out.Add(Drawn(Pieces::DrumTop, TEXT("drum.top"), MoveTemp(Shape), Solid));
		}

		/**
		 * A niche (§3.2), drawn in its own frame: x out along its axis, y the way φ rises. Its back is the wall's opening
		 * less the half-round (r 1.00 about the axis point on r 10.00) and the sliver between the chord plane and the
		 * curved face, from under the laid floor to the arch's crown, where it runs on into the wall above the arch. Its
		 * head is the quarter sphere's spandrel over the springing, a half turn about the niche's vertical axis.
		 */
		void Niche(TArray<FFormDrawing>& Out, int32 Index)
		{
			const double Deg = Plan::NicheDegrees[Index];
			const TCHAR* Name = Plan::NicheNames[Index];
			const double Half = NicheHalfDeg();
			const double Radius = Plan::NicheHalfWidth;
			const FVector2D Axis(Plan::DrumInner, 0.0);
			const double Floor = NicheOpeningSill();
			const double Crown = Plan::NicheSpringing + Radius;

			FFormProfile Back = FFormProfile::From(Polar(Plan::DrumInner, Half));
			Back.LineTo(Polar(Plan::DrumOuter, Half), EFormJoint::Crisp)
				.ArcAbout(FVector2D::ZeroVector, Plan::DrumOuter, Plan::DrumOuter, Half, -Half, EFormJoint::Crisp)
				.LineTo(Polar(Plan::DrumInner, -Half), EFormJoint::Crisp)
				.LineTo(Axis + FVector2D(0.0, -Radius), EFormJoint::Crisp)
				.ArcAbout(Axis, Radius, Radius, -90.0, 90.0, EFormJoint::Crisp)
				.Close();
			Out.Add(Drawn(FString::Printf(TEXT("niche.%s.back"), Name), TEXT("niche.back"), Slab(MoveTemp(Back), {}, Crown - Floor), Solid,
				Turned(Deg, FVector(0.0, 0.0, Floor))));

			FFormLathe Head;
			Head.Section = FFormProfile::From(FVector2D(0.0, Radius));
			Head.Section.ArcAbout(FVector2D::ZeroVector, Radius, Radius, 90.0, 0.0, EFormJoint::Crisp)
				.LineTo(FVector2D(Radius, Radius), EFormJoint::Crisp)
				.Close();
			Head.FromDegrees = -90.0;
			Head.ToDegrees = 90.0;
			const FVector2D Centre = Polar(Plan::DrumInner, Deg);
			Out.Add(Drawn(FString::Printf(TEXT("niche.%s.head"), Name), TEXT("niche.head"), MoveTemp(Head), Solid,
				Turned(Deg, FVector(Centre.X, Centre.Y, Plan::NicheSpringing))));

			FFormProfile Floorplate = FFormProfile::From(Axis + FVector2D(0.0, -Radius));
			Floorplate.ArcAbout(Axis, Radius, Radius, -90.0, 90.0, EFormJoint::Crisp).Close();
			Out.Add(Drawn(FString::Printf(TEXT("niche.%s.floor"), Name), TEXT("niche.sill"), Slab(MoveTemp(Floorplate), {}, Plan::FinishDepth), Solid,
				Turned(Deg, FVector(0.0, 0.0, Floor))));
		}
	}

	void DrawDrum(TArray<FFormDrawing>& Out)
	{
		Out.Add(DrumWallDrawing());
		Footing(Out);
		TopCourse(Out);
		for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::NicheDegrees)); ++Index)
		{
			Niche(Out, Index);
		}
	}
}
