#include "HallOfLightEnds.h"
#include "HallOfLightParts.h"
#include "HallOfLightPlan.h"

/**
 * The two end linings (sec. 3.1) and the east lunette (sec. 3.2, 3.3): 50 mm of polished travertine laid on the port
 * faces, the Rotunda's drum at the west and the Atrium's base at the east, each a cylinder about its neighbour's axis,
 * with the rough opening of its port; every hall piece at an end bears on its lining's inner face. The east lining
 * stops at the lunette's sill, where six flat panes between five steel mullions look through to the Atrium's drum.
 *
 * Each lining's top follows the sheet's outline: level at the eaves' top out to the vault's arc (R 5.42), then over the arc
 * to its crown; its rough opening runs straight through, jambs parallel, as its class's do; and the east lining's lunette,
 * the arc R 5.19 above its sill, is cut through it above the door, leaving the band r 5.19 to 5.42 round it.
 */
namespace Musee::HallOfLight
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** A lining's port: its rough opening's half width, its sill and head above the lining's bed, and whether it is arched. */
		struct FRoughOpening
		{
			double Half;
			double Sill;
			double Head;
			bool bArched;
		};

		FRoughOpening RoughOpening(EHallEnd End)
		{
			const double Sill = -Plan::RoughAllowance - Plan::LiningBottom;
			if (End == EHallEnd::West)
			{
				return {0.5 * Plan::WestClearWidth + Plan::RoughAllowance, Sill, Plan::WestSpringing - Plan::LiningBottom, true};
			}
			return {0.5 * Plan::EastClearWidth + Plan::RoughAllowance, Sill, Plan::EastHead + Plan::RoughAllowance - Plan::LiningBottom, false};
		}

		/** A point on a circle about a face's centre, at an angle in degrees. */
		FVector2D OnCircle(const FEndFace& Face, double Radius, double Degrees)
		{
			const double Angle = FMath::DegreesToRadians(Degrees);
			return Face.Centre() + Radius * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
		}

		/**
		 * The lining's top along its face from its start at From (U metres along the face, Z above its bed): level at the eaves'
		 * top out to where the vault's arc R 5.42 rises from it, then over the arc on short chords, and level again.
		 */
		FFormProfile LiningTop(const FEndFace& Port, double From)
		{
			auto Along = [&Port, From](double Y) { return Port.Radius * FMath::DegreesToRadians(FMath::Abs(Port.AngleDegrees(Y) - Port.AngleDegrees(From))); };
			const double Bed = Plan::LiningBottom;
			const double Rise = Plan::EavesTop - Plan::VaultAxisZ;
			const double Springs = FMath::Asin(FMath::Sqrt(Plan::LiningArcRadius * Plan::LiningArcRadius - Rise * Rise) / Plan::LiningArcRadius);
			const double Sign = FMath::Sign(From);
			TArray<FVector2D> Line = {FVector2D(0.0, Plan::EavesTop - Bed)};
			const int32 Steps = FMath::CeilToInt32(2.0 * Springs * Plan::LiningArcRadius / Plan::LiningTopStep);
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				const double Angle = Sign * FMath::Lerp(Springs, -Springs, double(Step) / Steps);
				const double Y = Plan::LiningArcRadius * FMath::Sin(Angle);
				Line.Add(FVector2D(Along(Y), Plan::VaultAxisZ + Plan::LiningArcRadius * FMath::Cos(Angle) - Bed));
			}
			Line.Add(FVector2D(Along(-From), Plan::EavesTop - Bed));
			return FFormProfile::Polyline(Line, false);
		}

		/**
		 * The lining over the eaves' width: a wall on the port face (its face), 50 mm thick into the hall (its back, the inner
		 * face), from its bed at -0.120 up to its outline's top, pierced by the port's rough opening straight through, and at
		 * the east by the lunette above it.
		 */
		FFormDrawing LiningWall(EHallEnd End)
		{
			const FEndFace Port = PortFace(End);
			const double Reach = Plan::LiningEavesHalf;
			const double From = Port.Inward() * Reach;
			FFormWall Shape;
			// Walked with the neighbour's axis on its left, so the thickness lies towards the hall.
			Shape.Plan = FFormProfile::From(Port.At(From));
			Shape.Plan.ArcAbout(Port.Centre(), Port.Radius, Port.Radius, Port.AngleDegrees(From), Port.AngleDegrees(-From));
			Shape.ThicknessMeters = Plan::LiningMeters;
			Shape.BaseMeters = Plan::LiningBottom;
			Shape.Top = LiningTop(Port, From);
			const double Centre = Port.Radius * FMath::Asin(Reach / Port.Radius);
			const FRoughOpening Rough = RoughOpening(End);
			FFormWallOpening Door;
			Door.CenterMeters = Centre;
			Door.WidthMeters = 2.0 * Rough.Half;
			Door.SillMeters = Rough.Sill;
			Door.HeadMeters = Rough.Head;
			Door.bArched = Rough.bArched;
			Door.bStraight = true;
			Shape.Openings.Add(Door);
			if (End == EHallEnd::East)
			{
				// The lunette: the semicircle R 5.19 about the vault's axis, cut above the sill at 6.20.
				FFormWallOpening Lunette;
				Lunette.CenterMeters = Centre;
				Lunette.WidthMeters = 2.0 * Plan::LunetteRadius;
				Lunette.SillMeters = Plan::LunetteSill - Plan::LiningBottom;
				Lunette.HeadMeters = Plan::VaultAxisZ - Plan::LiningBottom;
				Lunette.bArched = true;
				Lunette.bStraight = true;
				Shape.Openings.Add(Lunette);
			}
			const TCHAR* Name = End == EHallEnd::West ? TEXT("lining.west") : TEXT("lining.east");
			FFormDrawing Drawing = Drawn(Name, Surface(TEXT("lining")), MoveTemp(Shape), Solid);
			Drawing.PartSurfaces.Add(FFormWall::RevealPart, Surface(TEXT("port.rough-opening")));
			return Drawing;
		}

		/**
		 * The lining's feet beyond the eaves' width, over the plinth's end (to |y| 4.68, z 0.30) and the kerb's (to 4.80,
		 * z 0.06): sectors of the lining between the port face and the inner face, their joints radial as the wall's ends.
		 */
		void LiningFeet(TArray<FFormDrawing>& Out, EHallEnd End)
		{
			const FEndFace Port = PortFace(End);
			const double Inner = Port.Radius + Plan::LiningMeters;
			const struct
			{
				double From;
				double To;
				double Top;
				const TCHAR* Name;
			} Feet[] = {{Plan::LiningEavesHalf, Plan::LiningPlinthHalf, Plan::PlinthTop, TEXT("plinth-end")},
				{Plan::LiningPlinthHalf, Plan::LiningKerbHalf, Plan::KerbTop, TEXT("kerb-end")}};
			for (const int32 Side : Sides)
			{
				for (const auto& Foot : Feet)
				{
					const double A0 = Port.AngleDegrees(Side * Foot.From);
					const double A1 = Port.AngleDegrees(Side * Foot.To);
					FFormExtrusion Shape;
					Shape.Outline = FFormProfile::From(OnCircle(Port, Port.Radius, A0));
					Shape.Outline.ArcAbout(Port.Centre(), Port.Radius, Port.Radius, A0, A1, EFormJoint::Crisp);
					Shape.Outline.LineTo(OnCircle(Port, Inner, A1), EFormJoint::Crisp);
					Shape.Outline.ArcAbout(Port.Centre(), Inner, Inner, A1, A0, EFormJoint::Crisp);
					Shape.Outline.Close();
					Shape.DepthMeters = Foot.Top - Plan::LiningBottom;
					const FString Name = FString::Printf(TEXT("lining.%s.%s.%s"), End == EHallEnd::West ? TEXT("west") : TEXT("east"),
						SideName(Side), Foot.Name);
					Out.Add(Drawn(Name, Surface(TEXT("lining")), MoveTemp(Shape), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::LiningBottom))));
				}
			}
		}

		/** The lunette's arc (the collar's underside) at a plan y, and the y where it meets the sill. */
		double LunetteArcZ(double Y, double Radius)
		{
			return Plan::VaultAxisZ + FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Y * Y));
		}

		double LunetteReach()
		{
			return FMath::Sqrt(FMath::Square(Plan::LunetteRadius) - FMath::Square(Plan::LunetteSill - Plan::VaultAxisZ));
		}

		/**
		 * The mullions: a T on the east lining's face at each purlin's y, from the sill up under the purlin that ends over
		 * it (R 5.18, 10 mm under the collar's underside, so the stem's end meets the purlin rather than running into it).
		 */
		void Mullions(TArray<FFormDrawing>& Out)
		{
			const FEndFace Face = InnerFace(EHallEnd::East);
			const double In = Face.Inward();
			const double Half = Plan::MullionHalf;
			const double Stem = Plan::MullionStemHalf;
			const double Flange = Plan::MullionFlange;
			const double Depth = Plan::MullionDepth;
			const double Under = Plan::VaultRadius + Plan::PurlinFromB;
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::MullionY)); ++Index)
			{
				const double Y = Plan::MullionY[Index];
				auto Off = [&Face, In](double AtY, double By) { return FVector2D(Face.X(AtY) + In * By, AtY); };
				FFormExtrusion Shape;
				Shape.Outline = FFormProfile::From(Face.At(Y - Half));
				AddFaceArc(Shape.Outline, Face, Y - Half, Y + Half);
				for (const FVector2D& Point : {Off(Y + Half, Flange), Off(Y + Stem, Flange), Off(Y + Stem, Depth), Off(Y - Stem, Depth),
					Off(Y - Stem, Flange), Off(Y - Half, Flange)})
				{
					Shape.Outline.LineTo(Point, EFormJoint::Crisp);
				}
				Shape.Outline.Close();
				const double Top = LunetteArcZ(FMath::Abs(Y) + Half, Under);
				Shape.DepthMeters = Top - Plan::LunetteSill;
				Out.Add(Drawn(FString::Printf(TEXT("lunette.mullion.%d"), Index + 1), Surface(TEXT("vault.steel")), MoveTemp(Shape), nullptr,
					FMuseeFrame(FVector(0.0, 0.0, Plan::LunetteSill))));
			}
		}

		/**
		 * One flat pane of the lunette, between two of its edges' y: its hall face on the chord of the east lining's inner
		 * face (within 15 mm of it), its thickness into the lining, from the sill up to the lunette's arc. In the pane's
		 * own plane the arc is an ellipse, the circle R 5.19 stretched along the chord.
		 */
		FFormDrawing LunettePane(int32 Index, double YA, double YB)
		{
			const FEndFace Face = InnerFace(EHallEnd::East);
			const FVector2D A = Face.At(YA);
			const FVector2D B = Face.At(YB);
			const double Length = FVector2D::Distance(A, B);
			const double Across = YB - YA;
			const FVector2D Chord = (B - A) / Length;
			const double Radius = Plan::LunetteRadius;
			const double Sill = Plan::LunetteSill;
			const double Rise = Plan::LunetteSill - Plan::VaultAxisZ;
			auto Angle = [Radius](double Y) { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Y / Radius, -1.0, 1.0))); };
			constexpr double Level = 1.0e-6;
			FFormExtrusion Shape;
			Shape.Outline = FFormProfile::From(FVector2D(0.0, 0.0));
			if (LunetteArcZ(YA, Radius) - Sill > Level)
			{
				Shape.Outline.LineTo(FVector2D(0.0, LunetteArcZ(YA, Radius) - Sill), EFormJoint::Crisp);
			}
			Shape.Outline.ArcAbout(FVector2D(-YA * Length / Across, -Rise), Radius * Length / Across, Radius, Angle(YA), Angle(YB), EFormJoint::Crisp);
			if (LunetteArcZ(YB, Radius) - Sill > Level)
			{
				Shape.Outline.LineTo(FVector2D(Length, 0.0), EFormJoint::Crisp);
			}
			Shape.Outline.Close();
			Shape.DepthMeters = Plan::GlassMeters;
			const FMuseeFrame Frame = FrameOfAxes(FVector(A.X, A.Y, Sill), FVector(Chord.X, Chord.Y, 0.0), FVector::UpVector);
			FFormDrawing Pane = Drawn(FString::Printf(TEXT("lunette.glass.%d"), Index + 1), Surface(TEXT("lunette.glass")), MoveTemp(Shape), nullptr, Frame);
			Pane.bNanite = false;
			return Pane;
		}

		void Lunette(TArray<FFormDrawing>& Out)
		{
			TArray<double> Edges = {-LunetteReach()};
			Edges.Append(Plan::MullionY, int32(UE_ARRAY_COUNT(Plan::MullionY)));
			Edges.Add(LunetteReach());
			for (int32 Index = 0; Index + 1 < Edges.Num(); ++Index)
			{
				Out.Add(LunettePane(Index, Edges[Index], Edges[Index + 1]));
			}
			Mullions(Out);
		}
	}

	void DrawLinings(TArray<FFormDrawing>& Out)
	{
		for (const EHallEnd End : {EHallEnd::West, EHallEnd::East})
		{
			Out.Add(LiningWall(End));
			LiningFeet(Out, End);
		}
		Lunette(Out);
	}
}
