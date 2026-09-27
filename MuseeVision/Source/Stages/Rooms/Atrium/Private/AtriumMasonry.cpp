#include "AtriumParts.h"

/**
 * The masonry (§3.2): the travertine base with its cove, the door's rough opening cut straight through it, the coping
 * band between the ribs, the exterior dress round the plinth, the entablature at the equator notched round the fins, and
 * the portal over the Hall of Light's end with its cornice.
 *
 * Form lays no ashlar yet (Lay, the kit's backlog): the base is drawn as the stones its door needs (the long ring, the two
 * returns beside the jambs, the lintel and the sill), not coursed. A turned solid ends on a radial plane, so the dress
 * stops where its bed meets the portal's side and the entablature's notches are radial; and Form has no passage of a
 * segmental arch through a block to a curved face, so the portal is its front slab to the base's face on the axis, with
 * piers, haunches and attic behind it drawn exactly where the plan allows. Each is the nearest honest form.
 */
namespace Musee::Atrium
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/** The bearing offset from west of a point on a circle whose |y| is given. */
		double OffsetFromWest(double HalfY, double Radius)
		{
			return FMath::RadiansToDegrees(FMath::Asin(HalfY / Radius));
		}

		/** A point on a circle at a y, on the west side. */
		FVector2D WestAt(double Radius, double Y)
		{
			return FVector2D(-FMath::Sqrt(Radius * Radius - Y * Y), Y);
		}

		/** An arc of a circle about the axis from one point on it to another, the short way round. */
		FFormProfile& ArcThrough(FFormProfile& Outline, double Radius, const FVector2D& From, const FVector2D& To)
		{
			double A = PlanAngle(BearingOf(From));
			double B = PlanAngle(BearingOf(To));
			if (B - A > 180.0) { B -= 360.0; }
			if (A - B > 180.0) { B += 360.0; }
			return Outline.ArcAbout(FVector2D::ZeroVector, Radius, Radius, A, B, EFormJoint::Crisp);
		}

		/**
		 * The plan of the stone between the base's inner and outer faces, from one side (its points on the two faces) to
		 * the other: a jamb's straight line, or a radial joint. The returns beside the door, its lintel and its sill.
		 */
		FFormProfile BetweenFaces(const FVector2D& InnerFrom, const FVector2D& OuterFrom, const FVector2D& OuterTo, const FVector2D& InnerTo)
		{
			FFormProfile Outline = FFormProfile::From(InnerFrom);
			Outline.LineTo(OuterFrom, EFormJoint::Crisp);
			ArcThrough(Outline, Plan::BaseOuter, OuterFrom, OuterTo);
			Outline.LineTo(InnerTo, EFormJoint::Crisp);
			ArcThrough(Outline, Plan::BaseInner, InnerTo, InnerFrom);
			return Outline.Close();
		}

		/**
		 * The base: the long ring with the cove in its inner face, from one end of the cove round to the other; the two
		 * returns between the cove's square ends and the jambs; the lintel over the rough opening and the sill under it.
		 * The rough opening's jambs are cut straight through, parallel, as the class has them.
		 */
		void Base(TArray<FFormDrawing>& Out)
		{
			const double CoveEnd = OffsetFromWest(Plan::CoveEndHalf, Plan::BaseInner);
			const Plan::FStep Ring[] = {Plan::To(Plan::BaseInner, Plan::BaseBottom), Plan::To(Plan::BaseOuter, Plan::BaseBottom),
				Plan::To(Plan::BaseOuter, Plan::WallTop), Plan::To(Plan::BaseInner, Plan::WallTop), Plan::To(Plan::BaseInner, Plan::CoveTop),
				Plan::To(Plan::CoveBack, Plan::CoveTop), Plan::To(Plan::CoveBack, 0.0), Plan::To(Plan::BaseInner, 0.0)};
			Out.Add(Drawn(Pieces::Base, TEXT("base"), Turned(Section(Ring, true), Plan::DoorBearing + CoveEnd, Plan::DoorBearing - CoveEnd), Solid));

			for (const double Side : {-1.0, 1.0})
			{
				const double Jamb = Side * Plan::RoughHalf;
				const double End = Plan::DoorBearing - Side * CoveEnd;
				const FFormProfile Return = BetweenFaces(WestAt(Plan::BaseInner, Jamb), WestAt(Plan::BaseOuter, Jamb), Polar(Plan::BaseOuter, End),
					Polar(Plan::BaseInner, End));
				Out.Add(Drawn(FString::Printf(TEXT("base.return.%s"), Side < 0.0 ? TEXT("north") : TEXT("south")), TEXT("base"),
					Prism(Return, Plan::BaseBottom, Plan::WallTop), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::BaseBottom))));
			}
			const FFormProfile Across = BetweenFaces(WestAt(Plan::BaseInner, -Plan::RoughHalf), WestAt(Plan::BaseOuter, -Plan::RoughHalf),
				WestAt(Plan::BaseOuter, Plan::RoughHalf), WestAt(Plan::BaseInner, Plan::RoughHalf));
			Out.Add(Drawn(TEXT("base.lintel"), TEXT("base"), Prism(Across, Plan::RoughHead, Plan::WallTop), Solid,
				FMuseeFrame(FVector(0.0, 0.0, Plan::RoughHead))));
			Out.Add(Drawn(TEXT("base.sill"), TEXT("base"), Prism(Across, Plan::BaseBottom, Plan::RoughSill), Solid,
				FMuseeFrame(FVector(0.0, 0.0, Plan::BaseBottom))));
		}

		/** The coping band, lathed between each pair of ribs, 5 mm short of their side faces (measured at its inner face). */
		void CopingBand(TArray<FFormDrawing>& Out)
		{
			const double Inner = Plan::CopingBand[0].A;
			const double Stop = FMath::RadiansToDegrees(FMath::Asin((Plan::RibHalf + Plan::SteelClearance) / Inner));
			for (int32 Rib = 0; Rib < Plan::Ribs; ++Rib)
			{
				Out.Add(Drawn(FString::Printf(TEXT("band.%02d"), Rib + 1), TEXT("base"),
					Turned(Section(Plan::CopingBand, true), RibBearing(Rib) + Stop, RibBearing(Rib + 1) - Stop), Solid));
			}
		}

		/** The exterior dress round the plinth, outside the portal: its bed on the base's face meets the portal's sides. */
		void Dress(TArray<FFormDrawing>& Out)
		{
			const double End = OffsetFromWest(Plan::PortalHalfWidth, Plan::DressBed);
			Out.Add(Drawn(TEXT("dress.plinth"), TEXT("dress"), Turned(Section(Plan::Dress, true), Plan::DoorBearing + End, Plan::DoorBearing - End), Solid));
		}

		/** The entablature: its frieze and ovolo between the fins, and the corona, bead and wash all round over them. */
		void Entablature(TArray<FFormDrawing>& Out)
		{
			const double Notch = FMath::RadiansToDegrees(FMath::Asin(Plan::FinNotchHalf / Plan::EntablatureLower[0].A));
			for (int32 Fin = 0; Fin < Plan::Ribs; ++Fin)
			{
				Out.Add(Drawn(FString::Printf(TEXT("entablature.frieze.%02d"), Fin + 1), TEXT("dress"),
					Turned(Section(Plan::EntablatureLower, true), RibBearing(Fin) + Notch, RibBearing(Fin + 1) - Notch), Solid));
			}
			Out.Add(Drawn(TEXT("entablature.corona"), TEXT("dress"), TurnedAround(Section(Plan::EntablatureUpper, true)), Solid));
		}

		/** The arch's half-width at a height, and its springing from the jambs. */
		double ArchHalf(double Z)
		{
			const double Rise = Z - Plan::PortalArchCentreZ;
			return FMath::Sqrt(FMath::Max(0.0, Plan::PortalArchRadius * Plan::PortalArchRadius - Rise * Rise));
		}

		double Springing()
		{
			return Plan::PortalArchCentreZ + FMath::Sqrt(Plan::PortalArchRadius * Plan::PortalArchRadius - Plan::PortalJamb * Plan::PortalJamb);
		}

		/** A plan block from the front slab's back (x -14.800) to a circle about the axis, between |y| From and To on one side. */
		FFormProfile Behind(double Radius, double Side, double FromY, double ToY)
		{
			const double Back = -Plan::BaseOuter;
			FFormProfile Outline = FFormProfile::From(FVector2D(Back, Side * ToY));
			Outline.LineTo(FVector2D(Back, Side * FromY), EFormJoint::Crisp).LineTo(WestAt(Radius, Side * FromY), EFormJoint::Crisp);
			return ArcThrough(Outline, Radius, WestAt(Radius, Side * FromY), WestAt(Radius, Side * ToY)).Close();
		}

		/**
		 * The portal: the front slab with its opening (jambs ±4.950, the segmental arch R 5.800, crown 8.000) from the front
		 * plane to the base's face on the axis; behind it the piers to the base's face, the haunches over the arch and the
		 * attic over its crown to the drum's outer face; the cornice round its front and returns.
		 */
		void Portal(TArray<FFormDrawing>& Out)
		{
			const double Half = Plan::PortalHalfWidth;
			const double Jamb = Plan::PortalJamb;
			const double Spring = Springing();
			const double SpringDeg = FMath::RadiansToDegrees(FMath::Atan2(Spring - Plan::PortalArchCentreZ, Jamb));
			FFormProfile Front = FFormProfile::From(FVector2D(-Half, Plan::PortalBottom));
			Front.LineTo(FVector2D(-Jamb, Plan::PortalBottom), EFormJoint::Crisp).LineTo(FVector2D(-Jamb, Spring), EFormJoint::Crisp)
				.ArcAbout(FVector2D(0.0, Plan::PortalArchCentreZ), Plan::PortalArchRadius, Plan::PortalArchRadius, 180.0 - SpringDeg, SpringDeg, EFormJoint::Crisp)
				.LineTo(FVector2D(Jamb, Plan::PortalBottom), EFormJoint::Crisp).LineTo(FVector2D(Half, Plan::PortalBottom), EFormJoint::Crisp)
				.LineTo(FVector2D(Half, Plan::PortalTop), EFormJoint::Crisp).LineTo(FVector2D(-Half, Plan::PortalTop), EFormJoint::Crisp);
			const FMuseeFrame Elevation = FrameOfAxes(FVector(Plan::PortalFrontX, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector::UpVector);
			Out.Add(Drawn(TEXT("portal.front"), TEXT("dress"), Prism(Front.Close(), 0.0, -Plan::BaseOuter - Plan::PortalFrontX), Solid, Elevation));

			const double Crown = Plan::PortalArchCentreZ + Plan::PortalArchRadius;
			const double HaunchFoot = ArchHalf(Plan::WallTop);
			for (const double Side : {-1.0, 1.0})
			{
				const TCHAR* Name = Side < 0.0 ? TEXT("north") : TEXT("south");
				Out.Add(Drawn(FString::Printf(TEXT("portal.pier.%s"), Name), TEXT("dress"), Prism(Behind(Plan::BaseOuter, Side, Jamb, Half),
					Plan::PortalBottom, Plan::WallTop), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::PortalBottom))));
				Out.Add(Drawn(FString::Printf(TEXT("portal.haunch.%s"), Name), TEXT("dress"), Prism(Behind(Plan::PortalBackAbove, Side, HaunchFoot, Half),
					Plan::WallTop, Crown), Solid, FMuseeFrame(FVector(0.0, 0.0, Plan::WallTop))));
			}
			FFormProfile Attic = FFormProfile::From(FVector2D(-Plan::BaseOuter, Half));
			Attic.LineTo(FVector2D(-Plan::BaseOuter, -Half), EFormJoint::Crisp).LineTo(WestAt(Plan::PortalBackAbove, -Half), EFormJoint::Crisp);
			ArcThrough(Attic, Plan::PortalBackAbove, WestAt(Plan::PortalBackAbove, -Half), WestAt(Plan::PortalBackAbove, Half));
			Out.Add(Drawn(TEXT("portal.attic"), TEXT("dress"), Prism(Attic.Close(), Crown, Plan::PortalTop), Solid, FMuseeFrame(FVector(0.0, 0.0, Crown))));

			FFormSweep Cornice;
			Cornice.Section = Section(Plan::Cornice, true);
			Cornice.Path = FFormProfile::Polyline({FVector2D(Plan::CorniceReturnX, -Half), FVector2D(Plan::PortalFrontX, -Half),
				FVector2D(Plan::PortalFrontX, Half), FVector2D(Plan::CorniceReturnX, Half)}, false);
			Out.Add(Drawn(TEXT("portal.cornice"), TEXT("dress"), MoveTemp(Cornice), Solid));
		}
	}

	void DrawMasonry(TArray<FFormDrawing>& Out)
	{
		Base(Out);
		CopingBand(Out);
		Dress(Out);
		Entablature(Out);
		Portal(Out);
	}
}
