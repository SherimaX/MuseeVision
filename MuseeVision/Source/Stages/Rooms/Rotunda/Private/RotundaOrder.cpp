#include "RotundaParts.h"
#include "RotundaPlan.h"

/**
 * The order (§3.3): sixteen fluted Roman Doric pilasters on the drum's inner face, the entablature ring that carries the
 * dome, the archivolts round the four doors, and each niche's surround and sill ledge.
 */
namespace Musee::Rotunda
{
	namespace
	{
		double PilasterHalf() { return 0.5 * Plan::PilasterWidth; }

		/** Where a pilaster's return meets the drum's inner face, in the pilaster's frame (x out along its axis). */
		FVector2D ReturnAtWall(double Side)
		{
			return Polar(Plan::DrumInner, Side * HalfAngleDeg(PilasterHalf(), Plan::DrumInner));
		}

		/** The shaft's plan: the flat face at r 9.57 and the two returns back to the curved wall, which its back follows. */
		FFormProfile ShaftPlan(bool bFluted)
		{
			const double Half = PilasterHalf();
			const double Face = Plan::PilasterFace;
			FFormProfile Out = FFormProfile::From(FVector2D(Face, -Half));
			if (bFluted)
			{
				// Along the face from the -y return: a margin, then each flute's sine sunk into the face and a fillet.
				double Y = -Half + Plan::FluteMargin;
				Out.LineTo(FVector2D(Face, Y), EFormJoint::Crisp);
				for (int32 Flute = 0; Flute < Plan::Flutes; ++Flute)
				{
					for (int32 Station = 1; Station < Plan::FluteStations; ++Station)
					{
						const double T = double(Station) / (Plan::FluteStations - 1);
						// The flute's arris with the face (its first joint) is crisp; within the flute the sine runs smooth.
						Out.LineTo(FVector2D(Face + Plan::FluteDepth * FMath::Sin(UE_DOUBLE_PI * T), Y + Plan::FluteWidth * T),
							Station == 1 ? EFormJoint::Crisp : EFormJoint::Smooth);
					}
					Y += Plan::FluteWidth;
					if (Flute + 1 < Plan::Flutes)
					{
						Y += Plan::FluteFillet;
						Out.LineTo(FVector2D(Face, Y), EFormJoint::Crisp);
					}
				}
			}
			const double Side = HalfAngleDeg(Half, Plan::DrumInner);
			Out.LineTo(FVector2D(Face, Half), EFormJoint::Crisp)
				.LineTo(ReturnAtWall(1.0), EFormJoint::Crisp)
				.ArcAbout(FVector2D::ZeroVector, Plan::DrumInner, Plan::DrumInner, Side, -Side, EFormJoint::Crisp);
			return Out.Close();
		}

		/** The path the base and capital run along: out of the wall along one return, across the face, back along the other. */
		FFormProfile RoundThePilaster()
		{
			const double Half = PilasterHalf();
			return FFormProfile::Polyline({ReturnAtWall(-1.0), FVector2D(Plan::PilasterFace, -Half), FVector2D(Plan::PilasterFace, Half),
				ReturnAtWall(1.0)}, false);
		}

		/** A moulding swept round the pilaster, its section's height absolute. */
		FFormSweep Moulding(TConstArrayView<Plan::FStep> Section)
		{
			FFormSweep Shape;
			Shape.Section = Profile(Section, true);
			Shape.Path = RoundThePilaster();
			return Shape;
		}

		/** One pilaster: the torus base, the shaft's plain foot, its fluted length stopped square, its plain head, the capital. */
		void Pilaster(TArray<FFormDrawing>& Out, int32 Index)
		{
			const double Deg = Plan::PilasterDegrees[Index];
			const FString Name = FString::Printf(TEXT("pilaster.%02d"), Index + 1);
			const TCHAR* Finish = TEXT("pilaster");
			Out.Add(Drawn(Name + TEXT(".base"), Finish, Moulding(Plan::PilasterBase), Solid, Turned(Deg)));
			Out.Add(Drawn(Name + TEXT(".shaft.foot"), Finish, Slab(ShaftPlan(false), {}, Plan::FlutesFrom), Solid, Turned(Deg)));
			Out.Add(Drawn(Name + TEXT(".shaft.fluted"), Finish, Slab(ShaftPlan(true), {}, Plan::FlutesTo - Plan::FlutesFrom), Solid,
				Turned(Deg, FVector(0.0, 0.0, Plan::FlutesFrom))));
			Out.Add(Drawn(Name + TEXT(".shaft.head"), Finish, Slab(ShaftPlan(false), {}, Plan::CapitalTop - Plan::FlutesTo), Solid,
				Turned(Deg, FVector(0.0, 0.0, Plan::FlutesTo))));
			Out.Add(Drawn(Name + TEXT(".astragal"), Finish, Moulding(Plan::PilasterAstragal), nullptr, Turned(Deg)));
			Out.Add(Drawn(Name + TEXT(".capital"), Finish, Moulding(Plan::PilasterCapital), nullptr, Turned(Deg)));
		}

		/** The entablature, turned whole: its cornice's top ledge (r 9.22 to 9.99 at z 10.50) carries the cove lights. */
		void Entablature(TArray<FFormDrawing>& Out)
		{
			FFormLathe Shape;
			Shape.Section = Profile(Plan::Entablature, true);
			Out.Add(Drawn(TEXT("entablature"), TEXT("entablature"), MoveTemp(Shape), nullptr));
		}

		/** The archivolts, from springing to springing round each door's clear arch, on the drum's inner face. */
		void Archivolts(TArray<FFormDrawing>& Out)
		{
			const FFormProfile Section = Profile(Plan::DoorArchivolt, true);
			for (const Plan::FDoor& Door : Plan::Doors)
			{
				FFaceArch Arch;
				Arch.FaceRadius = Plan::DrumInner;
				Arch.AxisDeg = Door.Deg;
				Arch.ArchRadius = 0.5 * Door.Clear;
				Arch.Springing = Plan::DoorSpringing;
				Arch.JambFoot = Plan::DoorSpringing;
				RunRoundArch(Out, FString::Printf(TEXT("door.%s.archivolt"), Door.Name), TEXT("door.archivolt"), Section, Arch);
			}
		}

		/**
		 * Each niche's surround, up its jambs and round its head on the drum's face; and its sill ledge, swept along the
		 * chord plane at r 10.00 so it fills the sliver at the niche's foot and runs into the face beside it.
		 */
		void NicheDressings(TArray<FFormDrawing>& Out)
		{
			const FFormProfile Surround = Profile(Plan::NicheSurround, true);
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::NicheDegrees)); ++Index)
			{
				const double Deg = Plan::NicheDegrees[Index];
				const TCHAR* Name = Plan::NicheNames[Index];
				FFaceArch Arch;
				Arch.FaceRadius = Plan::DrumInner;
				Arch.AxisDeg = Deg;
				Arch.ArchRadius = Plan::NicheHalfWidth;
				Arch.Springing = Plan::NicheSpringing;
				Arch.JambFoot = Plan::NicheSill;
				RunRoundArch(Out, FString::Printf(TEXT("niche.%s.surround"), Name), TEXT("niche.surround"), Surround, Arch);

				// The ledge's frame: x across the niche (the way φ rises), y into the room, z up; its section's A to the path's right.
				FFormSweep Ledge;
				Ledge.Section = Profile(Plan::NicheSillLedge, true);
				Ledge.Path = FFormProfile::Polyline({FVector2D(-Plan::SillHalfLength, 0.0), FVector2D(Plan::SillHalfLength, 0.0)}, false);
				const FVector2D Chord = Polar(Plan::DrumInner, Deg);
				Out.Add(Drawn(FString::Printf(TEXT("niche.%s.sill"), Name), TEXT("niche.sill"), MoveTemp(Ledge), Solid,
					FrameOfAxes(FVector(Chord.X, Chord.Y, 0.0), Tangent(Deg), -Radial(Deg))));
			}
		}
	}

	void DrawOrder(TArray<FFormDrawing>& Out)
	{
		for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::PilasterDegrees)); ++Index)
		{
			Pilaster(Out, Index);
		}
		Entablature(Out);
		Archivolts(Out);
		NicheDressings(Out);
	}
}
