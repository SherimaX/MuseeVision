#include "RotundaParts.h"
#include "RotundaPlan.h"

/**
 * The shaft under the drum (§3.7), in the well the Rotunda serves: the stair's wall (the sleeve), the pocket the lifting
 * drum's wall slides into with the eight ram bores in its floor and the rams standing in them, the masonry out to the
 * port face r 7.60 with the port's arched reveal and the tympanum over the portal, the foundation, and the landing with
 * its rosette.
 *
 * The portal and the port's reveal are one passage through curved masonry, which Form cuts radially: its jambs are set
 * out on the stair's face (x = ±1.80 at r 6.48) and run radially out through every layer to the port face, so the layers'
 * jambs meet flush (the known gap of a straight passage through a curved wall).
 */
namespace Musee::Rotunda
{
	namespace
	{
		/** The passage's half-angle about the north axis. */
		double PortalHalfDeg() { return HalfAngleDeg(Plan::PortalHalfWidth, Plan::SleeveFace); }

		/** Everything in the well stands on the foundation, under the landing's laid finish. */
		double WellBed() { return Plan::LandingZ - Plan::FinishDepth; }

		/** The distance along a face of a radius, drawn from the south round either way, to the north axis: half its length. */
		double NorthAlong(double Radius) { return UE_DOUBLE_PI * Radius; }

		/** A face circle drawn from the south, anticlockwise (its thickness outside) or clockwise (inside). */
		FFormProfile FaceFromSouth(double Radius, bool bClockwise)
		{
			return FFormProfile::Ellipse(FVector2D::ZeroVector, Radius, Radius, Plan::SouthDeg, bClockwise);
		}

		/** The portal through a wall whose face has a radius: the passage's half-angle there, from the wall's bed. */
		FFormWallOpening Portal(double Radius, double Head, bool bArched)
		{
			FFormWallOpening Passage;
			Passage.CenterMeters = NorthAlong(Radius);
			Passage.WidthMeters = 2.0 * Radius * FMath::DegreesToRadians(PortalHalfDeg());
			Passage.HeadMeters = Head;
			Passage.bArched = bArched;
			return Passage;
		}

		/**
		 * The sleeve, face r 6.48, from the landing's bed to its top under the rim's groove, the portal cut through it with
		 * its flat head at the lintel; its back and top are the pocket's.
		 */
		void Sleeve(TArray<FFormDrawing>& Out)
		{
			FFormWall Shape;
			Shape.Plan = FaceFromSouth(Plan::SleeveFace, false);
			Shape.ThicknessMeters = Plan::SleeveBack - Plan::SleeveFace;
			Shape.BaseMeters = WellBed();
			Shape.HeightMeters = Plan::SleeveTop - WellBed();
			Shape.Openings.Add(Portal(Plan::SleeveFace, Plan::PortalHead - WellBed(), false));
			FFormDrawing Drawing = Drawn(Pieces::Sleeve, TEXT("stair.wall"), MoveTemp(Shape), Solid);
			Drawing.PartSurfaces.Add(FFormWall::BackPart, Surface(TEXT("shaft.pocket")));
			Drawing.PartSurfaces.Add(FFormWall::TopPart, Surface(TEXT("shaft.pocket")));
			Out.Add(MoveTemp(Drawing));
		}

		/**
		 * The masonry between the sleeve and the reveal, r 6.54 to 7.05, up to the pocket's floor: its foot under the
		 * bores, the bored course beside the portal, and the lintel course over it; each pierced by the eight bores
		 * (Ø 0.36 on r 6.77) that stop on the foot.
		 */
		void PocketMasonry(TArray<FFormDrawing>& Out)
		{
			const double Half = PortalHalfDeg();
			const FFormProfile BesidePortal = AnnularSector(Plan::SleeveBack, Plan::RevealInner, Plan::NorthDeg + Half, Plan::NorthDeg - Half + 360.0);
			TArray<FFormProfile> Bores;
			for (int32 Bore = 0; Bore < Plan::Bores; ++Bore)
			{
				Bores.Add(Circle(0.5 * Plan::BoreDiameter, Polar(Plan::BoreRadius, Plan::BoreFirstDeg + 360.0 * Bore / Plan::Bores)));
			}
			Out.Add(Drawn(TEXT("shaft.masonry.foot"), TEXT("shaft.pocket"), Slab(BesidePortal, {}, Plan::BoreBottom - WellBed()), Solid, Raised(WellBed())));
			Out.Add(Drawn(Pieces::MasonryInner, TEXT("shaft.pocket"), Slab(BesidePortal, Bores, Plan::PortalHead - Plan::BoreBottom), Solid,
				Raised(Plan::BoreBottom)));
			TArray<FFormProfile> LintelHoles = Bores;
			LintelHoles.Add(Circle(Plan::SleeveBack));
			Out.Add(Drawn(TEXT("shaft.masonry.lintel"), TEXT("stair.wall"), Slab(Circle(Plan::RevealInner), MoveTemp(LintelHoles),
				Plan::PocketFloor - Plan::PortalHead), Solid, Raised(Plan::PortalHead)));
		}

		/**
		 * Over the pocket's floor, the tympanum course r 7.00 to 7.05, its outer face closing the port's arch; and the outer
		 * masonry r 7.05 to the port face r 7.60, drawn from the port face inwards, cut by the port's arched reveal.
		 */
		void OuterMasonry(TArray<FFormDrawing>& Out)
		{
			FFormWall Tympanum;
			Tympanum.Plan = FaceFromSouth(Plan::PocketOuter, false);
			Tympanum.ThicknessMeters = Plan::RevealInner - Plan::PocketOuter;
			Tympanum.BaseMeters = Plan::PocketFloor;
			Tympanum.HeightMeters = -Plan::FinishDepth - Plan::PocketFloor;
			FFormDrawing Inner = Drawn(Pieces::MasonryTympanum, TEXT("shaft.pocket"), MoveTemp(Tympanum), Solid);
			Inner.PartSurfaces.Add(FFormWall::BackPart, Surface(TEXT("stair.wall")));
			Out.Add(MoveTemp(Inner));

			FFormWall Reveal;
			Reveal.Plan = FaceFromSouth(Plan::WellRadius, true);
			Reveal.ThicknessMeters = Plan::WellRadius - Plan::RevealInner;
			Reveal.BaseMeters = WellBed();
			Reveal.HeightMeters = -Plan::FinishDepth - WellBed();
			Reveal.Openings.Add(Portal(Plan::WellRadius, Plan::LandingZ + Plan::StairPortSpringing - WellBed(), true));
			Out.Add(Drawn(Pieces::MasonryOuter, TEXT("stair.port.reveal"), MoveTemp(Reveal), Solid));
		}

		/** The rams' barrels, standing in their bores with their heads under the lowered drum's foot. */
		void Rams(TArray<FFormDrawing>& Out)
		{
			for (int32 Bore = 0; Bore < Plan::Bores; ++Bore)
			{
				const FVector2D At = Polar(Plan::BoreRadius, Plan::BoreFirstDeg + 360.0 * Bore / Plan::Bores);
				Out.Add(Drawn(FString::Printf(TEXT("shaft.ram.%d"), Bore + 1), TEXT("sun-clock.ram"),
					Ring(0.0, 0.5 * Plan::RamDiameter, 0.0, Plan::SunFoot - Plan::BoreBottom), nullptr, FMuseeFrame(FVector(At.X, At.Y, Plan::BoreBottom))));
			}
		}

		/** The landing's rosette: its 16-point star, each point split on its axis into black and red halves. */
		void Star(TArray<FFormDrawing>& Out, const FMuseeFrame& Bed)
		{
			const double Valley = 180.0 / Plan::RosettePoints;
			for (int32 Point = 0; Point < Plan::RosettePoints; ++Point)
			{
				const double Deg = 2.0 * Valley * Point;
				const bool bLong = Point % 2 == 0;
				const double Tip = bLong ? Plan::RosetteLongPoint : Plan::RosetteShortPoint;
				const FVector2D Root = Polar(Plan::RosetteDisc, Deg);
				const FVector2D Apex = Polar(Tip, Deg);
				// The long points are black on the side of lower φ and red on the other; the short ones the other way round.
				const TCHAR* Lower = bLong ? TEXT("stair.landing.black") : TEXT("stair.landing.red");
				const TCHAR* Upper = bLong ? TEXT("stair.landing.red") : TEXT("stair.landing.black");
				Out.Add(Drawn(FString::Printf(TEXT("stair.landing.star.%02d.lower"), Point + 1), Lower,
					Slab(FFormProfile::Polyline({Root, Polar(Plan::RosetteDisc, Deg - Valley), Apex}, true), {}, Plan::FinishDepth), Walkable, Bed));
				Out.Add(Drawn(FString::Printf(TEXT("stair.landing.star.%02d.upper"), Point + 1), Upper,
					Slab(FFormProfile::Polyline({Root, Apex, Polar(Plan::RosetteDisc, Deg + Valley)}, true), {}, Plan::FinishDepth), Walkable, Bed));
			}
		}

		/**
		 * The foundation under the well, and the landing laid on it: the pale field from the rosette to the stair's wall
		 * and out through the portal to the port face; and the rosette, from its pale eye and black 32-gon disc through
		 * the star in its pale field to the black and red bands.
		 */
		void Landing(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(Pieces::Foundation, TEXT("stair.landing.floor"), Ring(0.0, Plan::WellRadius, Plan::FoundationBottom, WellBed()), Solid));
			const double Half = PortalHalfDeg();
			const FMuseeFrame Bed = Raised(WellBed());
			FFormProfile Field = FFormProfile::From(Polar(Plan::SleeveFace, Plan::NorthDeg + Half));
			Field.ArcAbout(FVector2D::ZeroVector, Plan::SleeveFace, Plan::SleeveFace, Plan::NorthDeg + Half, Plan::NorthDeg - Half + 360.0, EFormJoint::Crisp)
				.LineTo(Polar(Plan::WellRadius, Plan::NorthDeg - Half), EFormJoint::Crisp)
				.ArcAbout(FVector2D::ZeroVector, Plan::WellRadius, Plan::WellRadius, Plan::NorthDeg - Half, Plan::NorthDeg + Half, EFormJoint::Crisp)
				.Close();
			Out.Add(Drawn(TEXT("stair.landing"), TEXT("stair.landing.floor"), Slab(MoveTemp(Field), {Circle(Plan::RosetteRed)}, Plan::FinishDepth),
				Walkable, Bed));

			const double Level = Plan::LandingZ;
			Out.Add(Drawn(TEXT("stair.landing.rosette.eye"), TEXT("stair.landing.floor"), Ring(0.0, Plan::RosetteEye, WellBed(), Level), Walkable));
			TArray<FVector2D> Disc;
			TArray<FVector2D> StarOutline;
			const int32 Corners = 2 * Plan::RosettePoints;
			for (int32 Corner = 0; Corner < Corners; ++Corner)
			{
				const double Deg = 360.0 * Corner / Corners;
				Disc.Add(Polar(Plan::RosetteDisc, Deg));
				const bool bTip = Corner % 2 == 0;
				const bool bLong = Corner % 4 == 0;
				StarOutline.Add(Polar(bTip ? (bLong ? Plan::RosetteLongPoint : Plan::RosetteShortPoint) : Plan::RosetteDisc, Deg));
			}
			Out.Add(Drawn(TEXT("stair.landing.rosette.disc"), TEXT("stair.landing.black"),
				Slab(FFormProfile::Polyline(Disc, true), {Circle(Plan::RosetteEye)}, Plan::FinishDepth), Walkable, Bed));
			Star(Out, Bed);
			Out.Add(Drawn(TEXT("stair.landing.rosette.field"), TEXT("stair.landing.floor"),
				Slab(Circle(Plan::RosetteField), {FFormProfile::Polyline(StarOutline, true)}, Plan::FinishDepth), Walkable, Bed));
			Out.Add(Drawn(TEXT("stair.landing.rosette.band.black"), TEXT("stair.landing.black"), Ring(Plan::RosetteField, Plan::RosetteBlack, WellBed(), Level),
				Walkable));
			Out.Add(Drawn(TEXT("stair.landing.rosette.band.red"), TEXT("stair.landing.red"), Ring(Plan::RosetteBlack, Plan::RosetteRed, WellBed(), Level),
				Walkable));
		}
	}

	void DrawShaft(TArray<FFormDrawing>& Out)
	{
		Sleeve(Out);
		PocketMasonry(Out);
		OuterMasonry(Out);
		Rams(Out);
		Landing(Out);
	}
}
