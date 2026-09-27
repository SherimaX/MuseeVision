#include "AtriumParts.h"

/**
 * The glass car (§3.6), body atrium.car, drawn at the Atrium stop with its floor datum on the axis at the stage's origin:
 * its four panes and two leaves, bronze base and top rings, etched glass floor and clear roof, the glass handrail on its
 * bronze brackets, the roller guides on both rings, the gilt crown on its four arms, the Cube mast's head on the crown
 * (body atrium.car.mast.head), and the seat under the floor the Sphere's mast bears on. The light ring's slot is Light's.
 *
 * The mast's spiral-band column between crown and head is Motion's variable-length piece, of no length at rest; the
 * rack pinions and rollers are polish (§12); the handrail's ball ends wait for Form's shaped sweep ends.
 */
namespace Musee::Atrium
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;

		FFormProfile Disc(double Radius)
		{
			return FFormProfile::Ellipse(FVector2D::ZeroVector, Radius, Radius, 0.0, true);
		}

		/** A piece of the car: it rides the car's body. */
		FFormDrawing& OnCar(FFormDrawing& Drawing)
		{
			Drawing.Body = CarBody();
			return Drawing;
		}

		void Shell(TArray<FFormDrawing>& Out)
		{
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::CarPanes)); ++Index)
			{
				const Plan::FSpan& Span = Plan::CarPanes[Index];
				OnCar(AsGlass(Out.Add_GetRef(Drawn(FString::Printf(TEXT("car.pane.%d"), Index + 1), TEXT("car.glass"), Band(Plan::CarGlassIn,
					Plan::CarGlassOut, Plan::CarGlassBottom, Plan::CarGlassTop, FromDoor(Span.From), FromDoor(Span.To)), Solid))));
			}
			DrawLeafPair(Out, TEXT("car"), Plan::CarLeafIn, Plan::CarLeafOut, Plan::CarLeafBottom, Plan::CarGlassTop, TEXT("car.glass"), TEXT("car.bronze"));
			OnCar(Out.Add_GetRef(Drawn(TEXT("car.ring.base"), TEXT("car.bronze"), TurnedAround(Box(Plan::CarRingIn, Plan::CarBaseRing[0], Plan::CarRingOut,
				Plan::CarBaseRing[1])), Solid)));
			OnCar(Out.Add_GetRef(Drawn(TEXT("car.ring.top"), TEXT("car.bronze"), TurnedAround(Box(Plan::CarRingIn, Plan::CarTopRing[0], Plan::CarRingOut,
				Plan::CarTopRing[1])), Solid)));
			OnCar(AsGlass(Out.Add_GetRef(Drawn(TEXT("car.floor"), TEXT("car.floor"), Prism(Disc(Plan::CarFloorRadius), Plan::CarFloor[0], Plan::CarFloor[1]),
				Walkable, FMuseeFrame(FVector(0.0, 0.0, Plan::CarFloor[0]))))));
			OnCar(AsGlass(Out.Add_GetRef(Drawn(TEXT("car.roof"), TEXT("car.glass"), Prism(Disc(Plan::CarFloorRadius), Plan::CarRoofSoffit, Plan::CrownBottom),
				Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::CarRoofSoffit))))));
		}

		/** The handrail from 30° to 330° off the door, on its five brackets; and the roller guides on both rings. */
		void Fittings(TArray<FFormDrawing>& Out)
		{
			const double Rod = 0.5 * Plan::HandrailDiameter;
			FFormSweep Rail;
			Rail.Section = FFormProfile::Ellipse(FVector2D::ZeroVector, Rod, Rod, 0.0, false);
			Rail.Path = FFormProfile::From(Polar(Plan::HandrailRadius, FromDoor(Plan::HandrailFrom)));
			Rail.Path.ArcAbout(FVector2D::ZeroVector, Plan::HandrailRadius, Plan::HandrailRadius, PlanAngle(FromDoor(Plan::HandrailFrom)),
				PlanAngle(FromDoor(Plan::HandrailTo)));
			Rail.ElevationMeters = Plan::HandrailHeight;
			OnCar(AsGlass(Out.Add_GetRef(Drawn(TEXT("car.handrail"), TEXT("car.handrail"), MoveTemp(Rail), Solid))));

			const double Half = 0.5 * Plan::HandrailBracketSize;
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::HandrailBrackets)); ++Index)
			{
				const FFormProfile Arm = AtBearing(FromDoor(Plan::HandrailBrackets[Index]), Plan::HandrailRadius, Plan::HandrailBracketOut, -Half, Half);
				OnCar(Out.Add_GetRef(Drawn(FString::Printf(TEXT("car.handrail.bracket.%d"), Index + 1), TEXT("car.bronze"),
					Prism(Arm, Plan::HandrailHeight - Half, Plan::HandrailHeight + Half), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::HandrailHeight - Half)))));
			}

			const double* Rings[] = {Plan::CarBaseRing, Plan::CarTopRing};
			for (const double Bearing : Plan::RailBearings)
			{
				for (int32 Ring = 0; Ring < 2; ++Ring)
				{
					const FFormProfile Block = AtBearing(Bearing, Plan::GuideIn, Plan::GuideOut, -Plan::GuideHalf, Plan::GuideHalf);
					OnCar(Out.Add_GetRef(Drawn(FString::Printf(TEXT("car.guide.%s.%s"), Bearing < 90.0 ? TEXT("north") : TEXT("south"),
						Ring == 0 ? TEXT("base") : TEXT("top")), TEXT("car.bronze"), Prism(Block, Rings[Ring][0], Rings[Ring][1]), Solid,
						FMuseeFrame(FVector(0.0, 0.0, Rings[Ring][0])))));
				}
			}
		}

		/** The crown on its four arms over the roof, the mast's head riding on it, and the seat under the floor. */
		void Crown(TArray<FFormDrawing>& Out)
		{
			OnCar(Out.Add_GetRef(Drawn(TEXT("car.crown"), TEXT("car.bronze"), Prism(Disc(Plan::CrownRadius), Plan::CrownBottom, Plan::CrownTop), Solid,
				FMuseeFrame(FVector(0.0, 0.0, Plan::CrownBottom)))));
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::CrownArmBearings)); ++Index)
			{
				const FFormProfile Arm = AtBearing(Plan::CrownArmBearings[Index], Plan::CrownArmIn, Plan::CrownArmOut, -Plan::CrownArmHalf, Plan::CrownArmHalf);
				OnCar(Out.Add_GetRef(Drawn(FString::Printf(TEXT("car.crown.arm.%d"), Index + 1), TEXT("car.bronze"), Prism(Arm, Plan::CrownArmBottom,
					Plan::CrownArmTop), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::CrownArmBottom)))));
			}
			FFormDrawing& Head = Out.Add_GetRef(Drawn(TEXT("car.mast.head"), TEXT("car.mast"), TurnedAround(Section(Plan::MastHead, false)), Solid));
			Head.Body = MastHeadBody();
			OnCar(Out.Add_GetRef(Drawn(TEXT("car.seat"), TEXT("car.mast"), Prism(Disc(Plan::SeatRadius), Plan::SeatBottom, Plan::SeatTop), Solid,
				FMuseeFrame(FVector(0.0, 0.0, Plan::SeatBottom)))));
		}
	}

	FMuseeKey CarBody()
	{
		return Key(TEXT("car"));
	}

	FMuseeKey MastHeadBody()
	{
		return Key(TEXT("car.mast.head"));
	}

	FMuseeFrame MastHeadFrame()
	{
		return FMuseeFrame(FVector(0.0, 0.0, Plan::CrownTop));
	}

	void DrawCar(TArray<FFormDrawing>& Out)
	{
		Shell(Out);
		Fittings(Out);
		Crown(Out);
	}

	void DrawStele(TArray<FFormDrawing>& Out)
	{
		// The Starry Night's stele (§3.7): its glass set 20 mm into the bronze foot. The foot's eased top edges wait for Form.
		const double (&Glass)[2][3] = Plan::SteleGlass;
		const double (&Foot)[2][3] = Plan::SteleFoot;
		AsGlass(Out.Add_GetRef(Drawn(TEXT("stele.glass"), TEXT("stele.glass"), Prism(Box(Glass[0][0], Glass[0][1], Glass[1][0], Glass[1][1]), Glass[0][2],
			Glass[1][2]), Solid, FMuseeFrame(FVector(0.0, 0.0, Glass[0][2])))));
		Out.Add(Drawn(TEXT("stele.foot"), TEXT("stele.foot"), Prism(Box(Foot[0][0], Foot[0][1], Foot[1][0], Foot[1][1]), Foot[0][2], Foot[1][2]), Solid,
			FMuseeFrame(FVector(0.0, 0.0, Foot[0][2]))));
	}
}
