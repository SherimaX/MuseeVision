#include "AtriumParts.h"

/**
 * The steelwork (§3.5): twelve pearl ribs that rise from bronze shoes, turn at 9 m and run under the Sphere to the
 * compression ring round the car's way; the ring and the hangers that carry the collar from it; and outdoors, the drum's
 * order of fins on pedestals, mullions and transom rings, rooted on the outer leaf's face.
 *
 * Each rib and each vertical of the order stands in a radial plane, so it is drawn exactly as its elevation pushed across
 * its width. The ribs' 30 mm chamfers on their room-side arrises wait for Form's sweeps along paths out of plan, which is
 * what a rib's spine is; their gilt lines wait for polish (§12).
 */
namespace Musee::Atrium
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/**
		 * A rib's elevation in its radial plane, (r, z): up the back line, round the turn and under the Sphere to the
		 * ring's outer face, and back along the room face 0.420 in from the back.
		 */
		FFormProfile RibElevation()
		{
			const double C = Plan::SphereCentreZ;
			const FVector2D Turn(Plan::RibTurnCentreR, Plan::RibTurnZ);
			const double Room = Plan::CanopyRadius + Plan::RibDepth;
			const double Under = Plan::RibTurnEndDeg - 180.0;
			auto ToRing = [](double Radius) { return -FMath::RadiansToDegrees(FMath::Acos(Plan::RingOuter / Radius)); };
			FFormProfile Rib = FFormProfile::From(FVector2D(Plan::RibBack, 0.0));
			Rib.LineTo(FVector2D(Plan::RibBack, Plan::RibTurnZ), EFormJoint::Crisp)
				.ArcAbout(Turn, Plan::RibTurnRadius, Plan::RibTurnRadius, 0.0, Plan::RibTurnEndDeg)
				.ArcAbout(FVector2D(0.0, C), Plan::CanopyRadius, Plan::CanopyRadius, Under, ToRing(Plan::CanopyRadius))
				.LineTo(FVector2D(Plan::RingOuter, C - FMath::Sqrt(Room * Room - Plan::RingOuter * Plan::RingOuter)), EFormJoint::Crisp)
				.ArcAbout(FVector2D(0.0, C), Room, Room, ToRing(Room), Under, EFormJoint::Crisp)
				.ArcAbout(Turn, Plan::RibTurnRadius - Plan::RibDepth, Plan::RibTurnRadius - Plan::RibDepth, Plan::RibTurnEndDeg, 0.0)
				.LineTo(FVector2D(Plan::RibBack - Plan::RibDepth, 0.0));
			return Rib.Close();
		}

		/** A shoe: the bronze sleeve round the rib's foot, open at its back where the rib stands 5 mm off the stone. */
		FFormProfile ShoePlan(double Bearing)
		{
			auto At = [Bearing](double D, double T) { return (Plan::RibBack - D) * Outward(Bearing) + T * Along(Bearing); };
			const double S = Plan::ShoeHalf;
			const double R = Plan::RibHalf;
			return FFormProfile::Polyline({At(0.0, -S), At(Plan::ShoeDepth, -S), At(Plan::ShoeDepth, S), At(0.0, S), At(0.0, R),
				At(Plan::RibDepth, R), At(Plan::RibDepth, -R), At(0.0, -R)}, true);
		}

		void Ribs(TArray<FFormDrawing>& Out)
		{
			for (int32 Rib = 0; Rib < Plan::Ribs; ++Rib)
			{
				const double Bearing = RibBearing(Rib);
				Out.Add(Drawn(FString::Printf(TEXT("rib.%02d"), Rib + 1), TEXT("rib"), Prism(RibElevation(), 0.0, 2.0 * Plan::RibHalf), Solid,
					RadialPlane(Bearing, Plan::RibHalf)));
				Out.Add(Drawn(FString::Printf(TEXT("rib.%02d.shoe"), Rib + 1), TEXT("rib.shoe"), Prism(ShoePlan(Bearing), 0.0, Plan::ShoeTop), Solid));
			}
		}

		/** The ring's sloping soffit, from its bore (3.000, 7.745) to its outer face (3.600, 7.890). */
		double RingSoffit(double Radius)
		{
			const Plan::FStep& Bore = Plan::CompressionRing[0];
			const Plan::FStep& Outer = Plan::CompressionRing[1];
			return Bore.B + (Radius - Bore.A) * (Outer.B - Bore.B) / (Outer.A - Bore.A);
		}

		/** The compression ring, and the twelve hanger plates at the rib bearings that carry the collar from its soffit. */
		void Ring(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(Pieces::Ring, TEXT("canopy.grid"), TurnedAround(Section(Plan::CompressionRing, true)), Solid));
			const FFormProfile Plate = FFormProfile::Polyline({FVector2D(Plan::HangerInner, Plan::UpPlane), FVector2D(Plan::HangerOuter, Plan::UpPlane),
				FVector2D(Plan::HangerOuter, RingSoffit(Plan::HangerOuter)), FVector2D(Plan::HangerInner, RingSoffit(Plan::HangerInner))}, true);
			for (int32 Rib = 0; Rib < Plan::Ribs; ++Rib)
			{
				Out.Add(Drawn(FString::Printf(TEXT("canopy.hanger.%02d"), Rib + 1), TEXT("canopy.grid"), Prism(Plate, 0.0, Plan::HangerThickness), Solid,
					RadialPlane(RibBearing(Rib), 0.5 * Plan::HangerThickness)));
			}
		}

		/** Where a vertical of the order stands: on the portal's roof within ±25° of west, on the wash elsewhere. */
		double Seat(double Bearing)
		{
			return FMath::Abs(Bearing - Plan::DoorBearing) <= Plan::PortalSector ? Plan::SeatOnPortal : Plan::SeatOnWash;
		}

		/** A fin's elevation from its foot on the pedestal, out and up to its elliptical head, cut level at the corona's soffit. */
		FFormProfile FinElevation(double Seat)
		{
			const double Foot = Seat + Plan::FinFoot;
			const double Cut = FMath::RadiansToDegrees(UE_DOUBLE_PI - FMath::Asin((Plan::FinHeadCut - Plan::FinWaistZ) / Plan::FinHeadRadiusZ));
			FFormProfile Fin = FFormProfile::From(FVector2D(Plan::OuterLeafOuter, Foot));
			Fin.LineTo(FVector2D(Plan::FinFootOut, Foot), EFormJoint::Crisp).LineTo(FVector2D(Plan::FinWaistR, Plan::FinWaistZ), EFormJoint::Crisp)
				.ArcAbout(FVector2D(Plan::FinHeadCentreR, Plan::FinWaistZ), Plan::FinHeadRadiusR, Plan::FinHeadRadiusZ, 180.0, Cut)
				.LineTo(FVector2D(Plan::OuterLeafOuter, Plan::FinHeadCut), EFormJoint::Crisp);
			return Fin.Close();
		}

		/** The half-width of the vertical at a bearing, at a height: a fin or its pedestal at 15° + 30°k, a mullion at 30°k. */
		double VerticalHalf(double Bearing, double Z)
		{
			const bool bFin = FMath::Fmod(Bearing - Plan::RibFirstBearing + 360.0, Plan::BayStep) < 1.0;
			if (!bFin)
			{
				return Plan::MullionHalf;
			}
			return Z < Seat(Bearing) + Plan::PedestalHeight ? Plan::PedestalHalf : Plan::FinHalf;
		}

		/** The order: twelve fins on their pedestals, twelve mullions, and the three transom rings between the verticals. */
		void Order(TArray<FFormDrawing>& Out)
		{
			for (int32 Rib = 0; Rib < Plan::Ribs; ++Rib)
			{
				const double Fin = RibBearing(Rib);
				const double Mullion = Plan::BayStep * Rib;
				Out.Add(Drawn(FString::Printf(TEXT("order.fin.%02d"), Rib + 1), TEXT("order"), Prism(FinElevation(Seat(Fin)), 0.0, 2.0 * Plan::FinHalf),
					Solid, RadialPlane(Fin, Plan::FinHalf)));
				Out.Add(Drawn(FString::Printf(TEXT("order.pedestal.%02d"), Rib + 1), TEXT("order"), Prism(Section(Plan::Pedestal, true, Seat(Fin)), 0.0,
					2.0 * Plan::PedestalHalf), Solid, RadialPlane(Fin, Plan::PedestalHalf)));
				const FFormProfile Shaft = Box(Plan::OuterLeafOuter, Seat(Mullion), Plan::MullionOut, Plan::MullionTop);
				Out.Add(Drawn(FString::Printf(TEXT("order.mullion.%02d"), Rib + 1), TEXT("order"), Prism(Shaft, 0.0, 2.0 * Plan::MullionHalf), Solid,
					RadialPlane(Mullion, Plan::MullionHalf)));
			}
			const double Root = Plan::OuterLeafOuter;
			const int32 Verticals = int32(360.0 / Plan::PaneBearingStep);
			for (int32 Level = 0; Level < int32(UE_ARRAY_COUNT(Plan::Transoms)); ++Level)
			{
				const double Z = Plan::Transoms[Level];
				for (int32 Index = 0; Index < Verticals; ++Index)
				{
					const double From = Plan::PaneBearingStep * Index;
					const double To = From + Plan::PaneBearingStep;
					auto Clear = [Z, Root](double Bearing) { return FMath::RadiansToDegrees(FMath::Asin(VerticalHalf(Bearing, Z) / Root)); };
					Out.Add(Drawn(FString::Printf(TEXT("order.transom.%d.%02d"), Level + 1, Index + 1), TEXT("order"),
						Band(Root, Plan::TransomOut, Z - Plan::TransomHalfHeight, Z + Plan::TransomHalfHeight, From + Clear(From), To - Clear(To)), Solid));
				}
			}
		}
	}

	void DrawSteel(TArray<FFormDrawing>& Out)
	{
		Ribs(Out);
		Ring(Out);
		Order(Out);
	}
}
