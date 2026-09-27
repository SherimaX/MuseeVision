#include "RotundaParts.h"
#include "RotundaPlan.h"

/**
 * The sun clock's lifting drum (§3.6), the moving body rotunda.sun-clock.drum, drawn lowered (its frame z_d is the
 * stage's): raised 3.50, it stands as a tempietto 13.98 across with its door to the west. Its wall of Crema Marfil
 * stands on a foot that bears on the rams, dressed with a base moulding, twelve fluted pilasters and an entablature
 * whose corona is the dial's edge; inside, its deck carries the dial and is coffered beneath as the stair's ceiling,
 * with a bronze rim at its edge and a gilt pendant over the well.
 */
namespace Musee::Rotunda
{
	namespace
	{
		/** The door's half-width on the drum's face, measured along it: its radial jambs at ±10.860°. */
		double DoorHalf() { return Plan::SunWallFace * FMath::DegreesToRadians(Plan::SunDoorHalfDeg); }

		/**
		 * The wall, its face r 6.900 drawn clockwise from the east so its 0.355 lies inside it, from the door's sill to
		 * under the dial's finish; the door half-way round, arched from z_d -1.85.
		 */
		void Tempietto(TArray<FFormDrawing>& Out)
		{
			FFormWall Shape;
			Shape.Plan = FFormProfile::Ellipse(FVector2D::ZeroVector, Plan::SunWallFace, Plan::SunWallFace, 0.0, true);
			Shape.ThicknessMeters = Plan::SunWallFace - Plan::SunWallInner;
			Shape.BaseMeters = Plan::SunSill;
			Shape.HeightMeters = -Plan::FinishDepth - Plan::SunSill;
			FFormWallOpening& Door = Shape.Openings.AddDefaulted_GetRef();
			Door.CenterMeters = Plan::SunWallFace * FMath::DegreesToRadians(Plan::SunDoorDeg);
			Door.WidthMeters = 2.0 * DoorHalf();
			Door.HeadMeters = Plan::SunDoorSpringing - Plan::SunSill;
			Door.bArched = true;
			FFormDrawing Drawing = Drawn(Pieces::SunWall, TEXT("sun-clock.wall"), MoveTemp(Shape), Solid);
			Drawing.PartSurfaces.Add(FFormWall::BackPart, Surface(TEXT("sun-clock.wall.inner")));
			Drawing.PartSurfaces.Add(FFormWall::RevealPart, Surface(TEXT("sun-clock.door")));
			Drawing.Body = DrumBody();
			Out.Add(MoveTemp(Drawing));

			// The foot and plinth, all round under the wall and the door, whose sill it is.
			OnDrum(Out, TEXT("sun-clock.foot"), TEXT("sun-clock.door.sill"), Ring(Plan::SunWallInner, Plan::DialRadius, Plan::SunFoot, Plan::SunSill), Solid);

			FFormLathe Base;
			Base.Section = Profile(Plan::SunBase, true);
			Base.FromDegrees = Plan::SunDoorDeg + Plan::SunDoorHalfDeg;
			Base.ToDegrees = Plan::SunDoorDeg + 360.0 - Plan::SunDoorHalfDeg;
			OnDrum(Out, TEXT("sun-clock.base"), TEXT("sun-clock.wall"), MoveTemp(Base), Solid);

			FFormLathe Cornice;
			Cornice.Section = Profile(Plan::SunEntablature, true);
			OnDrum(Out, TEXT("sun-clock.entablature"), TEXT("sun-clock.wall"), MoveTemp(Cornice), Solid);

			FFaceArch Arch;
			Arch.FaceRadius = Plan::SunWallFace;
			Arch.AxisDeg = Plan::SunDoorDeg;
			Arch.bConvex = true;
			Arch.ArchRadius = Plan::SunArchivoltRadius;
			Arch.Springing = Plan::SunDoorSpringing;
			Arch.JambFoot = Plan::SunArchivoltFoot;
			RunRoundArch(Out, TEXT("sun-clock.door.archivolt"), TEXT("sun-clock.door"), Profile(Plan::SunArchivolt, true), Arch, DrumBody());
		}

		/**
		 * A pilaster part's plan in its own frame (x out along its axis): its back on the drum's convex face, its front
		 * flat and proud on the axis; a shaft's front cut with five flutes, each a circular arc sagging 0.018.
		 */
		FFormProfile PilasterPlan(const Plan::FPilasterPart& Part)
		{
			const double Half = 0.5 * Part.Width;
			const double FrontFace = Plan::SunWallFace + Part.Proud;
			const double Side = HalfAngleDeg(Half, Plan::SunWallFace);
			FFormProfile Out = FFormProfile::From(Polar(Plan::SunWallFace, -Side));
			Out.LineTo(FVector2D(FrontFace, -Half), EFormJoint::Crisp);
			if (Part.bFluted)
			{
				const double Chord = 0.5 * Plan::SunFluteWidth;
				const double Radius = (Chord * Chord + Plan::SunFluteSag * Plan::SunFluteSag) / (2.0 * Plan::SunFluteSag);
				const double Spread = FMath::RadiansToDegrees(FMath::Atan2(Chord, Radius - Plan::SunFluteSag));
				double Y = -Half + Plan::SunFluteMargin;
				for (int32 Flute = 0; Flute < Plan::SunFlutes; ++Flute)
				{
					Out.LineTo(FVector2D(FrontFace, Y), EFormJoint::Crisp);
					const FVector2D Centre(FrontFace + Radius - Plan::SunFluteSag, Y + Chord);
					Out.ArcAbout(Centre, Radius, Radius, -180.0 + Spread, -180.0 - Spread, EFormJoint::Crisp);
					Y += Plan::SunFluteWidth + Plan::SunFluteFillet;
				}
			}
			Out.LineTo(FVector2D(FrontFace, Half), EFormJoint::Crisp)
				.LineTo(Polar(Plan::SunWallFace, Side), EFormJoint::Crisp)
				.ArcAbout(FVector2D::ZeroVector, Plan::SunWallFace, Plan::SunWallFace, Side, -Side, EFormJoint::Crisp);
			return Out.Close();
		}

		/** The twelve pilasters at 15° + 30°·k, the door between the two at 165° and 195°: each a stack of five parts. */
		void Pilasters(TArray<FFormDrawing>& Out)
		{
			for (int32 Index = 0; Index < Plan::SunPilasters; ++Index)
			{
				const double Deg = Plan::SunPilasterFirstDeg + 360.0 * Index / Plan::SunPilasters;
				for (const Plan::FPilasterPart& Part : Plan::SunPilasterParts)
				{
					OnDrum(Out, FString::Printf(TEXT("sun-clock.pilaster.%02d.%s"), Index + 1, Part.Name), TEXT("sun-clock.pilaster"),
						Slab(PilasterPlan(Part), {}, Part.Top - Part.Bottom), Solid, Turned(Deg, FVector(0.0, 0.0, Part.Bottom)));
				}
			}
		}

		/** A coffer's plan: between a ring's radii and its radial ribs, the whole of it inset by a margin from the ribs. */
		FFormProfile Coffer(const Plan::FCofferRing& Band, int32 Index, double Margin)
		{
			const double Each = 360.0 / Band.Coffers;
			const double Low = Each * Index + 0.5 * Band.RibDeg;
			const double High = Each * (Index + 1) - 0.5 * Band.RibDeg;
			const double Inner = Band.Inner + Margin;
			const double Outer = Band.Outer - Margin;
			// The inset sides run parallel to the ribs' edges, so at a radius they stand off by the angle the margin subtends.
			auto Off = [Margin](double Radius) { return HalfAngleDeg(Margin, Radius); };
			FFormProfile Out = FFormProfile::From(Polar(Outer, Low + Off(Outer)));
			Out.ArcAbout(FVector2D::ZeroVector, Outer, Outer, Low + Off(Outer), High - Off(Outer), EFormJoint::Crisp)
				.LineTo(Polar(Inner, High - Off(Inner)), EFormJoint::Crisp)
				.ArcAbout(FVector2D::ZeroVector, Inner, Inner, High - Off(Inner), Low + Off(Inner), EFormJoint::Crisp);
			return Out.Close();
		}

		/**
		 * The deck under the dial, whose underside is the coffers' panels at z_d -0.100; and the soffit in two plates
		 * under it: the ribs' plate (-0.130 to -0.115), pierced by every coffer between its ribs, and the frames' plate
		 * (-0.115 to -0.100), pierced by every coffer inset 40 mm, so each coffer steps up through its frame to its panel.
		 * Then the bronze rim under the dial's edge and the gilt pendant over the well.
		 */
		void Soffit(TArray<FFormDrawing>& Out)
		{
			FFormLathe Deck;
			Deck.Section = FFormProfile::Polyline({FVector2D(0.0, Plan::DeckPanel), FVector2D(Plan::RimInner, Plan::DeckPanel),
				FVector2D(Plan::RimInner, Plan::RimTopInner), FVector2D(Plan::SunWallInner, Plan::RimGroove), FVector2D(Plan::SunWallInner, -Plan::FinishDepth),
				FVector2D(0.0, -Plan::FinishDepth)}, false);
			OnDrum(Out, Pieces::SunDeck, TEXT("sun-clock.soffit"), MoveTemp(Deck), Solid);

			const TPair<const TCHAR*, double> Plates[] = {{TEXT("ribs"), 0.0}, {TEXT("frames"), Plan::SoffitFrameWidth}};
			const double Levels[] = {Plan::SoffitRib, Plan::SoffitFrame, Plan::DeckPanel};
			for (int32 Plate = 0; Plate < 2; ++Plate)
			{
				TArray<FFormProfile> Holes;
				for (const Plan::FCofferRing& Band : Plan::SoffitCoffers)
				{
					for (int32 Index = 0; Index < Band.Coffers; ++Index)
					{
						Holes.Add(Coffer(Band, Index, Plates[Plate].Value));
					}
				}
				OnDrum(Out, FString::Printf(TEXT("sun-clock.soffit.%s"), Plates[Plate].Key), TEXT("sun-clock.soffit"),
					Slab(Circle(Plan::SoffitOuter), MoveTemp(Holes), Levels[Plate + 1] - Levels[Plate]), nullptr, Raised(Levels[Plate]));
			}

			FFormLathe Rim;
			Rim.Section = Profile(Plan::SunRim, true);
			OnDrum(Out, TEXT("sun-clock.rim"), TEXT("sun-clock.rim"), MoveTemp(Rim), nullptr);
			FFormLathe Pendant;
			Pendant.Section = Profile(Plan::SunPendant, false);
			OnDrum(Out, TEXT("sun-clock.pendant"), TEXT("sun-clock.pendant"), MoveTemp(Pendant), nullptr);
		}
	}

	void DrawSunClock(TArray<FFormDrawing>& Out)
	{
		Tempietto(Out);
		Pilasters(Out);
		Soffit(Out);
	}
}
