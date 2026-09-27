#include "AtriumParts.h"

/**
 * The floor and its structure (§3.1, §3.3): the concrete slab under the marble; the Crema Marfil laid in eight rings of
 * radial slabs with 6 mm joints, a joint on every rib line; ring 1's lip notched for the rails and cut back for the
 * landing's sill; ring 8 running into the cove except in the door bay, where the marble stops on the base's inner face;
 * the bronze waiting ring and the waiting mark.
 *
 * Form cuts no rebate or recess in a laid slab: the landing's shoe and the waiting mark stand in the marble's thickness as
 * the sheet draws them, and the rebate and recesses under them wait for Form's laying.
 */
namespace Musee::Atrium
{
	namespace
	{
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		constexpr double HalfJoint = 0.5 * Plan::FloorJoint;

		FFormProfile Circle(const FVector2D& Centre, double Radius)
		{
			return FFormProfile::Ellipse(Centre, Radius, Radius, 0.0, true);
		}

		/** The bearing a joint's side stands at on a circle: the radial line moved HalfJoint into the slab. */
		double Inset(double Radius)
		{
			return FMath::RadiansToDegrees(FMath::Asin(HalfJoint / Radius));
		}

		/** A point in a rail's notch: U out along the rail's bearing, T across it (towards increasing bearing). */
		FVector2D AtRail(double Bearing, double U, double T)
		{
			return U * Outward(Bearing) + T * Along(Bearing);
		}

		/** The arc about the axis on the current circle from one bearing to another. */
		void ArcTo(FFormProfile& Outline, double Radius, double From, double To)
		{
			Outline.ArcAbout(FVector2D::ZeroVector, Radius, Radius, PlanAngle(From), PlanAngle(To), EFormJoint::Crisp);
		}

		/** The T-shaped notch a rail's blade and foot stand in, cut into the lip at the circle of radius Lip. */
		void RailNotch(FFormProfile& Outline, double Lip, double Bearing)
		{
			const double Blade = Plan::BladeSlotHalf;
			const double Foot = Plan::FootSlotHalf;
			const double OnLip = FMath::Sqrt(Lip * Lip - Blade * Blade);
			const FVector2D Points[] = {AtRail(Bearing, Plan::BladeSlotOut, -Blade), AtRail(Bearing, Plan::BladeSlotOut, -Foot),
				AtRail(Bearing, Plan::FootSlotOut, -Foot), AtRail(Bearing, Plan::FootSlotOut, Foot), AtRail(Bearing, Plan::BladeSlotOut, Foot),
				AtRail(Bearing, Plan::BladeSlotOut, Blade), AtRail(Bearing, OnLip, Blade)};
			for (const FVector2D& Point : Points)
			{
				Outline.LineTo(Point, EFormJoint::Crisp);
			}
		}

		/**
		 * A slab of ring 1, from its lip out to the waiting ring: its lip notched where a rail stands in it, and cut back to
		 * the sill's outer edge across the door (±26°), with radial steps at the sill's ends.
		 */
		FFormProfile RingOneSlab(double From, double To)
		{
			const double Lip = Plan::RingRadii[0];
			const double Outer = Plan::RingRadii[1];
			const double SillFrom = Plan::DoorBearing - Plan::SillHalf;
			const double SillTo = Plan::DoorBearing + Plan::SillHalf;
			auto InnerAt = [&](double Bearing) { return Bearing > SillFrom && Bearing < SillTo ? Plan::SillOut : Lip; };

			double Radius = InnerAt(From);
			double At = From + Inset(Radius);
			FFormProfile Outline = FFormProfile::From(Polar(Radius, At));
			for (const double Step : {SillFrom, SillTo})
			{
				if (Step > From && Step < To)
				{
					ArcTo(Outline, Radius, At, Step);
					Radius = Radius == Lip ? Plan::SillOut : Lip;
					Outline.LineTo(Polar(Radius, Step), EFormJoint::Crisp);
					At = Step;
				}
			}
			for (double Rail : Plan::RailBearings)
			{
				Rail = Rail < From ? Rail + 360.0 : Rail;
				if (Rail > From && Rail < To && Radius == Lip)
				{
					const double Side = FMath::RadiansToDegrees(FMath::Asin(Plan::BladeSlotHalf / Lip));
					ArcTo(Outline, Radius, At, Rail - Side);
					RailNotch(Outline, Lip, Rail);
					At = Rail + Side;
				}
			}
			ArcTo(Outline, Radius, At, To - Inset(Radius));
			Outline.LineTo(Polar(Outer, To - Inset(Outer)), EFormJoint::Crisp);
			ArcTo(Outline, Outer, To - Inset(Outer), From + Inset(Outer));
			return Outline.Close();
		}

		/**
		 * A slab of ring 8 in the door bay: it runs into the cove (r 14.090) as far as the cove's square end (|y| 2.350),
		 * and stops on the base's inner face (r 14.000) beyond it.
		 */
		FFormProfile RingEightDoorSlab(double Inner, double From, double To)
		{
			const double Cove = Plan::CoveBack;
			const double Face = Plan::BaseInner;
			auto EndOn = [](double Radius, double Side)
			{
				// The cove's square end on a circle: the point of |y| 2.350 on the west side.
				return FVector2D(-FMath::Sqrt(Radius * Radius - Plan::CoveEndHalf * Plan::CoveEndHalf), Side * Plan::CoveEndHalf);
			};
			FFormProfile Outline = FFormProfile::From(Polar(Inner, From + Inset(Inner)));
			ArcTo(Outline, Inner, From + Inset(Inner), To - Inset(Inner));
			// Walk the outer edge back from To to From, stepping out into the cove where it begins.
			const double NorthEnd = BearingOf(EndOn(Face, -1.0));
			const double SouthEnd = BearingOf(EndOn(Face, 1.0));
			const bool bToInCove = To - Inset(Face) > NorthEnd || To - Inset(Face) < SouthEnd;
			const bool bFromInCove = From + Inset(Face) > NorthEnd || From + Inset(Face) < SouthEnd;
			double Radius = bToInCove ? Cove : Face;
			double At = To - Inset(Radius);
			Outline.LineTo(Polar(Radius, At), EFormJoint::Crisp);
			if (bToInCove && !bFromInCove)
			{
				const FVector2D Out = EndOn(Cove, -1.0);
				ArcTo(Outline, Cove, At, BearingOf(Out));
				Outline.LineTo(EndOn(Face, -1.0), EFormJoint::Crisp);
				Radius = Face;
				At = NorthEnd;
			}
			else if (!bToInCove && bFromInCove)
			{
				ArcTo(Outline, Face, At, SouthEnd);
				Outline.LineTo(EndOn(Cove, 1.0), EFormJoint::Crisp);
				Radius = Cove;
				At = BearingOf(EndOn(Cove, 1.0));
			}
			ArcTo(Outline, Radius, At, From + Inset(Radius));
			return Outline.Close();
		}

		/** The marble: eight rings of radial slabs, the radial joints on bearings 15° + k·360°/n, 6 mm joints between rings. */
		void Marble(TArray<FFormDrawing>& Out)
		{
			const double CoveFrom = Plan::DoorBearing - 15.0;
			const double CoveTo = Plan::DoorBearing + 15.0;
			for (int32 Ring = 0; Ring < int32(UE_ARRAY_COUNT(Plan::RingSlabs)); ++Ring)
			{
				// Ring 1 lies between the lip and the waiting ring; ring 2 on starts outside the waiting ring.
				const int32 Edge = Ring == 0 ? 0 : Ring + 1;
				const double Inner = Plan::RingRadii[Edge] + (Ring > 1 ? HalfJoint : 0.0);
				const double Outer = Plan::RingRadii[Edge + 1] - (Ring > 0 && Ring + 1 < int32(UE_ARRAY_COUNT(Plan::RingSlabs)) ? HalfJoint : 0.0);
				const int32 Slabs = Plan::RingSlabs[Ring];
				const double Step = 360.0 / Slabs;
				for (int32 Slab = 0; Slab < Slabs; ++Slab)
				{
					const double From = Plan::RibFirstBearing + Step * Slab;
					const double To = From + Step;
					FFormProfile Outline;
					if (Ring == 0)
					{
						Outline = RingOneSlab(From, To);
					}
					else if (Ring + 1 == int32(UE_ARRAY_COUNT(Plan::RingSlabs)) && To > CoveFrom && From < CoveTo)
					{
						Outline = RingEightDoorSlab(Inner, From, To);
					}
					else
					{
						Outline = Sector(Inner, Outer, From, To, HalfJoint);
					}
					Out.Add(Drawn(FString::Printf(TEXT("%sring-%d.%02d"), Pieces::Floor, Ring + 1, Slab + 1), TEXT("floor"),
						Prism(MoveTemp(Outline), Plan::MarbleBottom, 0.0), Walkable, FMuseeFrame(FVector(0.0, 0.0, Plan::MarbleBottom))));
				}
			}
		}

		/** The bronze waiting ring, flush, and the waiting mark west of it: a half ring open towards the car, and a dot. */
		void Waiting(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(Pieces::WaitingRing, TEXT("waiting-ring"), Prism(Circle(FVector2D::ZeroVector, Plan::RingRadii[2]), Plan::MarbleBottom, 0.0,
				{Circle(FVector2D::ZeroVector, Plan::RingRadii[1])}), Walkable, FMuseeFrame(FVector(0.0, 0.0, Plan::MarbleBottom))));

			const FVector2D Centre(Plan::MarkCentreX, 0.0);
			FFormProfile Half = FFormProfile::From(Centre + FVector2D(0.0, Plan::MarkOuter));
			Half.ArcAbout(Centre, Plan::MarkOuter, Plan::MarkOuter, 90.0, 270.0, EFormJoint::Crisp)
				.LineTo(Centre + FVector2D(0.0, -Plan::MarkInner), EFormJoint::Crisp)
				.ArcAbout(Centre, Plan::MarkInner, Plan::MarkInner, 270.0, 90.0, EFormJoint::Crisp);
			const FMuseeFrame Bed(FVector(0.0, 0.0, Plan::MarkBottom));
			Out.Add(Drawn(TEXT("floor.waiting-mark.ring"), TEXT("waiting-ring"), Prism(Half.Close(), Plan::MarkBottom, Plan::MarkTop), Walkable, Bed));
			Out.Add(Drawn(TEXT("floor.waiting-mark.dot"), TEXT("waiting-ring"), Prism(Circle(FVector2D(Plan::DotX, 0.0), Plan::DotRadius),
				Plan::MarkBottom, Plan::MarkTop), Walkable, Bed));
		}

		/** The slab under the marble, from the lining to the base's inner face, bearing on the base's footing. */
		void Slab(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(TEXT("slab"), TEXT("slab"), Prism(Circle(FVector2D::ZeroVector, Plan::BaseInner), Plan::SlabBottom, Plan::SlabTop,
				{Circle(FVector2D::ZeroVector, Plan::SlabInner)}), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::SlabBottom))));
		}
	}

	void DrawFloor(TArray<FFormDrawing>& Out)
	{
		Slab(Out);
		Marble(Out);
		Waiting(Out);
	}
}
