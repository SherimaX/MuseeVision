#include "SphereParts.h"
#include "SpherePlan.h"

/**
 * The shells about the centre C (§3.1-3.3): the hidden steel plate the Sphere is built on, the image surface inside it,
 * and the exterior dome over the equator: the pearl skin, the gilt drip ring bedded on the Atrium's wash, the boss and
 * the ball. All are turned about the Élan axis from their sections in (r, z), each drawn with its solid on its left.
 */
namespace Musee::Sphere
{
	namespace
	{
		/** The centre C in a section's (r, z) plane. */
		const FVector2D C(0.0, Plan::CentreZ);

		/** The angle about C, in a section's (r, z) plane, of a point. */
		double AboutC(const FVector2D& Point)
		{
			return FMath::RadiansToDegrees(FMath::Atan2(Point.Y - C.Y, Point.X - C.X));
		}

		/** The point of a sphere about C at a radius from the axis, below or above the equator. */
		FVector2D OnSphere(double SphereRadius, double Radius, bool bAbove)
		{
			return FVector2D(Radius, SphereZ(SphereRadius, Radius, bAbove));
		}

		/**
		 * A spherical shell about C between two radii, from where it meets the throat's outer face (r 2.980) below the
		 * equator up over the crown to the axis: from the crown down the inner face, across the throat's face, and up the
		 * outer face back to the crown.
		 */
		FFormLathe SphericalShell(double Inner, double Outer)
		{
			const FVector2D InnerEdge = OnSphere(Inner, Plan::ThroatOuter, false);
			const FVector2D OuterEdge = OnSphere(Outer, Plan::ThroatOuter, false);
			FFormProfile Section = FFormProfile::From(FVector2D(0.0, Plan::CentreZ + Inner));
			Section.ArcAbout(C, Inner, Inner, 90.0, AboutC(InnerEdge))
				.LineTo(OuterEdge, EFormJoint::Crisp)
				.ArcAbout(C, Outer, Outer, AboutC(OuterEdge), 90.0, EFormJoint::Crisp);
			return Turned(MoveTemp(Section));
		}

		/** The Atrium's entablature wash, which falls outwards from its inner edge: its height at a radius (atrium.md §3). */
		double WashZ(double Radius)
		{
			return Plan::WashInnerZ - Plan::WashFallPerMeter * (Radius - Plan::WashInnerR);
		}

		/** The skin's inner face at its foot: where the skin and the drip ring meet, r 14.2263. */
		FVector2D SkinFootInner()
		{
			return FVector2D(FMath::Sqrt(FMath::Square(Plan::SkinInner) - FMath::Square(Plan::SkinFoot - Plan::CentreZ)), Plan::SkinFoot);
		}

		/** The skin (§3.2): 60 mm between R 14.240 and 14.300 about C, from its foot on the ring to the crown hole. */
		FFormLathe Skin()
		{
			const FVector2D FootInner = SkinFootInner();
			const FVector2D FootOuter(FMath::Sqrt(FMath::Square(Plan::SkinOuter) - FMath::Square(Plan::SkinFoot - Plan::CentreZ)), Plan::SkinFoot);
			const FVector2D CrownOuter = OnSphere(Plan::SkinOuter, Plan::CrownHole, true);
			const FVector2D CrownInner = OnSphere(Plan::SkinInner, Plan::CrownHole, true);
			FFormProfile Section = FFormProfile::From(FootInner);
			Section.LineTo(FootOuter, EFormJoint::Crisp)
				.ArcAbout(C, Plan::SkinOuter, Plan::SkinOuter, AboutC(FootOuter), AboutC(CrownOuter), EFormJoint::Crisp)
				.LineTo(CrownInner, EFormJoint::Crisp)
				.ArcAbout(C, Plan::SkinInner, Plan::SkinInner, AboutC(CrownInner), AboutC(FootInner), EFormJoint::Crisp);
			return Turned(Section.Close());
		}

		/**
		 * The drip ring (§3.2): bedded face to face on the wash from the skin's inner face out to its flat's edge, up to a
		 * 50 mm half-round whose underside stands clear of the wash (the drip), and back along the skin's foot.
		 */
		FFormLathe DripRing()
		{
			const FVector2D FootInner = SkinFootInner();
			const FVector2D Round(Plan::RingFlatOuter, Plan::RingRoundZ);
			FFormProfile Section = FFormProfile::From(FVector2D(FootInner.X, WashZ(FootInner.X)));
			Section.LineTo(FVector2D(Plan::RingFlatOuter, WashZ(Plan::RingFlatOuter)), EFormJoint::Crisp)
				.LineTo(FVector2D(Plan::RingFlatOuter, Plan::RingRoundZ - Plan::RingRoundRadius), EFormJoint::Crisp)
				.ArcAbout(Round, Plan::RingRoundRadius, Plan::RingRoundRadius, -90.0, 90.0, EFormJoint::Crisp)
				.LineTo(FootInner, EFormJoint::Crisp);
			return Turned(Section.Close());
		}

		/**
		 * The boss at the crown (§3.2): its spigot through the skin's crown hole, its rim on the skin's outer face with a
		 * quarter-round, the cone up to the ball's seat, and the seat, a part of the ball's sphere, down to the axis.
		 */
		FFormLathe Boss()
		{
			const FVector2D SpigotTop = OnSphere(Plan::SkinOuter, Plan::SpigotRadius, true);
			const FVector2D RimOuter = OnSphere(Plan::SkinOuter, Plan::BossRim, true);
			const FVector2D RoundCentre(Plan::BossRim - Plan::BossRound, RimOuter.Y);
			const FVector2D Ball(0.0, Plan::BallCentreZ);
			const FVector2D SeatEdge(Plan::BossSeatR, Plan::BallCentreZ - FMath::Sqrt(FMath::Square(Plan::BallRadius) - FMath::Square(Plan::BossSeatR)));
			const double SeatEdgeDeg = FMath::RadiansToDegrees(FMath::Atan2(SeatEdge.Y - Ball.Y, SeatEdge.X - Ball.X));
			FFormProfile Section = FFormProfile::From(FVector2D(0.0, Plan::BossBottom));
			Section.LineTo(FVector2D(Plan::SpigotRadius, Plan::BossBottom), EFormJoint::Crisp)
				.LineTo(SpigotTop, EFormJoint::Crisp)
				.ArcAbout(C, Plan::SkinOuter, Plan::SkinOuter, AboutC(SpigotTop), AboutC(RimOuter), EFormJoint::Crisp)
				.ArcAbout(RoundCentre, Plan::BossRound, Plan::BossRound, 0.0, 90.0, EFormJoint::Crisp)
				.LineTo(FVector2D(Plan::BossShoulderR, Plan::BossShoulderZ), EFormJoint::Crisp)
				.LineTo(SeatEdge, EFormJoint::Crisp)
				.ArcAbout(Ball, Plan::BallRadius, Plan::BallRadius, SeatEdgeDeg, -90.0, EFormJoint::Crisp);
			return Turned(MoveTemp(Section));
		}

		/** The gilt ball in the boss's seat. */
		FFormLathe Ball()
		{
			const FVector2D Centre(0.0, Plan::BallCentreZ);
			FFormProfile Section = FFormProfile::From(Centre - FVector2D(0.0, Plan::BallRadius));
			Section.ArcAbout(Centre, Plan::BallRadius, Plan::BallRadius, -90.0, 90.0);
			return Turned(MoveTemp(Section));
		}
	}

	void DrawShells(TArray<FFormDrawing>& Out)
	{
		// §3.1: the 10 mm plate, welded to the throat's outer face; its stiffeners, absorber and standoffs are polish.
		Out.Add(Drawn(TEXT("shell.plate"), TEXT("steel"), SphericalShell(Plan::PlateInner, Plan::PlateOuter), Solid));

		// §3.3: the image surface, one continuous surface of the light-field panels' depth tonight; Light's image luminaire
		// shows the sky on it. Its faces are glass, which Nanite can't render.
		FFormDrawing& Image = Out.Add_GetRef(Drawn(Pieces::ImageSurface, TEXT("image"), SphericalShell(Plan::ImageInner, Plan::ImageOuter), Solid));
		Image.bNanite = false;

		// §3.2: the exterior dome, outdoors.
		Out.Add(Drawn(TEXT("dome.skin"), TEXT("skin"), Skin(), Solid));
		Out.Add(Drawn(TEXT("dome.drip-ring"), TEXT("gilt.outside"), DripRing(), Solid));
		Out.Add(Drawn(TEXT("dome.boss"), TEXT("gilt.outside"), Boss(), Solid));
		Out.Add(Drawn(TEXT("dome.ball"), TEXT("gilt.outside"), Ball(), Solid));
	}
}
