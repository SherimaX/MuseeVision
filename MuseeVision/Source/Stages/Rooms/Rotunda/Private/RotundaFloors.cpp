#include "FormCatalogue.h"
#include "RotundaParts.h"
#include "RotundaPlan.h"
#include "TimeMuseumDial.h"

/**
 * The floors (§3.1): the fixed ring of radial Crema Marfil slabs in three courses, laid 30 mm over its core; and the
 * dial, the lifting drum's top, in seven bands with bronze fillets on their joints and the gilt sun at its centre,
 * the arrival point; the motto in bronze capitals on the ring's middle course. The dial's hour ring, hour lines,
 * meridian and minute ticks are laid from Time's dial (ﾂｧ5), in the groove its ring cuts in the outer band
 * (RotundaDialInlay). Nothing is typed by hand here.
 */
namespace Musee::Rotunda
{
	namespace
	{
		/**
		 * The ring: its core from the floor's bed, outside the shaft's masonry (r 7.60), and its slabs in three courses,
		 * toned by the parity of slab and course; the outer course is offset half a slab.
		 */
		void FixedRing(TArray<FFormDrawing>& Out)
		{
			Out.Add(Drawn(Pieces::FloorCore, TEXT("floor.ring.a"), Ring(Plan::WellRadius, Plan::RingOuter, Plan::FloorBed, -Plan::FinishDepth), Solid));
			for (int32 Course = 0; Course < int32(UE_ARRAY_COUNT(Plan::Courses)); ++Course)
			{
				const Plan::FCourse& C = Plan::Courses[Course];
				const double Each = 360.0 / C.Slabs;
				for (int32 Stone = 0; Stone < C.Slabs; ++Stone)
				{
					const double From = C.OffsetDeg + Each * Stone;
					const TCHAR* Tone = (Stone + Course) % 2 == 0 ? TEXT("floor.ring.a") : TEXT("floor.ring.b");
					Out.Add(Drawn(FString::Printf(TEXT("floor.ring.course-%d.slab-%02d"), Course + 1, Stone + 1), Tone,
						Ring(C.Inner, C.Outer, -Plan::FinishDepth, 0.0, From, From + Each), Walkable));
				}
			}
		}

		/**
		 * The motto (§3.1): its capitals cast in bronze and set on the middle course, its words parted by gilt stops, lettered
		 * in the catalogue's Centaur. When the catalogue lacks the face, the empty face leaves every letter undrawn and Form's
		 * making says so.
		 */
		void Motto(TArray<FFormDrawing>& Out)
		{
			const TValueOrError<FFormTypeface, FString> Face = Musee::Form::FindTypeface(FMuseeKey(Plan::MottoFace));
			FFormLettering Shape;
			for (const TCHAR* Word : Plan::MottoWords)
			{
				if (!Shape.Text.IsEmpty())
				{
					Shape.Text.AppendChar(FFormLettering::Interpunct);
				}
				Shape.Text += Word;
			}
			Shape.Typeface = Face.HasValue() ? Face.GetValue() : FFormTypeface();
			Shape.CapMeters = Plan::MottoCap;
			Shape.TrackingCaps = Plan::MottoTracking;
			Shape.WordSpaceCaps = Plan::MottoWordSpace;
			Shape.StopGapCaps = Plan::MottoStopGap;
			Shape.StopHalfWidthCaps = Plan::MottoStopHalfWidth;
			Shape.StopTopCaps = Plan::MottoStopTop;
			Shape.StopPointCaps = Plan::MottoStopPoint;
			Shape.StopApexMeters = Plan::MottoStopApex;
			Shape.BaselineRadiusMeters = Plan::MottoBaseline;
			Shape.CentreDegrees = Plan::MottoCentreDeg;
			Shape.bTopsInward = true;
			Shape.ProudMeters = Plan::MottoProud;
			Shape.BevelMeters = Plan::MottoBevel;
			FFormDrawing& Drawing = Out.Add_GetRef(Drawn(TEXT("floor.motto"), TEXT("floor.motto"), MoveTemp(Shape), nullptr));
			Drawing.PartSurfaces.Add(FFormLettering::StopPart, Surface(TEXT("floor.motto.stop")));
		}

		bool HasFillet(double Radius)
		{
			for (const double At : Plan::FilletRadii)
			{
				if (FMath::IsNearlyEqual(At, Radius, UE_KINDA_SMALL_NUMBER))
				{
					return true;
				}
			}
			return false;
		}

		/**
		 * A band of the dial, its section cut with the groove its fillet sits in at each edge that carries one, the groove
		 * the hour ring sits in where the shadow circle (RingRadius) crosses it, and the centre band with the groove the
		 * gilt sun's boss sits in. The centre band's section ends on the axis.
		 */
		FFormProfile BandSection(double Inner, double Outer, double RingRadius)
		{
			const double Half = 0.5 * Plan::FilletWidth;
			const double Groove = -Plan::InlayGroove;
			const double Bed = -Plan::FinishDepth;
			FFormProfile Out = FFormProfile::From(FVector2D(Inner, Bed));
			Out.LineTo(FVector2D(Outer, Bed), EFormJoint::Crisp);
			if (HasFillet(Outer))
			{
				Out.LineTo(FVector2D(Outer, Groove), EFormJoint::Crisp)
					.LineTo(FVector2D(Outer - Half, Groove), EFormJoint::Crisp)
					.LineTo(FVector2D(Outer - Half, 0.0), EFormJoint::Crisp);
			}
			else
			{
				Out.LineTo(FVector2D(Outer, 0.0), EFormJoint::Crisp);
			}
			const double RingHalf = 0.5 * Plan::HourRingWidth;
			if (RingRadius - RingHalf > Inner && RingRadius + RingHalf < Outer)
			{
				Out.LineTo(FVector2D(RingRadius + RingHalf, 0.0), EFormJoint::Crisp)
					.LineTo(FVector2D(RingRadius + RingHalf, Groove), EFormJoint::Crisp)
					.LineTo(FVector2D(RingRadius - RingHalf, Groove), EFormJoint::Crisp)
					.LineTo(FVector2D(RingRadius - RingHalf, 0.0), EFormJoint::Crisp);
			}
			if (Inner <= 0.0)
			{
				Out.LineTo(FVector2D(Plan::BossRadius, 0.0), EFormJoint::Crisp)
					.LineTo(FVector2D(Plan::BossRadius, Groove), EFormJoint::Crisp)
					.LineTo(FVector2D(0.0, Groove), EFormJoint::Crisp);
				return Out;
			}
			if (HasFillet(Inner))
			{
				Out.LineTo(FVector2D(Inner + Half, 0.0), EFormJoint::Crisp)
					.LineTo(FVector2D(Inner + Half, Groove), EFormJoint::Crisp)
					.LineTo(FVector2D(Inner, Groove), EFormJoint::Crisp);
			}
			else
			{
				Out.LineTo(FVector2D(Inner, 0.0), EFormJoint::Crisp);
			}
			return Out.Close();
		}

		/**
		 * The dial's bands, grooved for the hour ring on the shadow circle, and a bronze fillet in the groove on each joint
		 * that carries one: 10 wide, 2 proud, bevelled.
		 */
		void Dial(TArray<FFormDrawing>& Out, double RingRadius)
		{
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::DialBands)); ++Index)
			{
				const Plan::FBand& Band = Plan::DialBands[Index];
				FFormLathe Shape;
				Shape.Section = BandSection(Band.Inner, Band.Outer, RingRadius);
				OnDrum(Out, FString::Printf(TEXT("dial.band.%d"), Index + 1), Band.Surface, MoveTemp(Shape), Walkable);
			}
			const double Half = 0.5 * Plan::FilletWidth;
			const double Crest = Plan::FilletProud;
			const double Bevel = Plan::FilletBevel;
			for (const double At : Plan::FilletRadii)
			{
				FFormLathe Fillet;
				Fillet.Section = FFormProfile::Polyline({FVector2D(At - Half, -Plan::InlayGroove), FVector2D(At + Half, -Plan::InlayGroove),
					FVector2D(At + Half, Crest - Bevel), FVector2D(At + Half - Bevel, Crest), FVector2D(At - Half + Bevel, Crest), FVector2D(At - Half, Crest - Bevel)},
					true);
				OnDrum(Out, FString::Printf(TEXT("dial.fillet.r%04d"), FMath::RoundToInt32(At * 1000.0)), TEXT("dial.inlay"), MoveTemp(Fillet), nullptr);
			}
		}

		/**
		 * One of the sun's rays, lofted as the founder casts it: long ones (i even, one to XII) to r 1.25, 72 wide at the root
		 * tapering as (1 - t)^0.62; short flames between them to r 0.86, 60 wide tapering as (1 - t)^0.75, their crests
		 * weaving. Across it, each flank rises from its foot 2 mm down in the dial through a hollow to the ridge; the crest
		 * falls towards the tip, where the flanks close on an edge.
		 */
		void Ray(TArray<FFormDrawing>& Out, int32 Index)
		{
			const bool bLong = Index % 2 == 0;
			const double Tip = bLong ? Plan::LongRayTip : Plan::ShortRayTip;
			const double HalfWidth = 0.5 * (bLong ? Plan::LongRayWidth : Plan::ShortRayWidth);
			const double Taper = bLong ? Plan::LongRayTaper : Plan::ShortRayTaper;
			const double Crest = bLong ? Plan::LongRayCrest : Plan::ShortRayCrest;
			FFormLoft Shape;
			for (int32 Station = 0; Station < Plan::RayStations; ++Station)
			{
				const double T = double(Station) / (Plan::RayStations - 1);
				const double W = HalfWidth * FMath::Pow(1.0 - T, Taper);
				const double H = Plan::RayTipCrest + (Crest - Plan::RayTipCrest) * FMath::Pow(1.0 - T, Plan::RayCrestFall);
				const double Weave = bLong ? 0.0 : Plan::FlameWeave * FMath::Sin(2.0 * Plan::FlameWaves * UE_DOUBLE_PI * T) * FMath::Sqrt(T) * (1.0 - T);
				// x across the ray, y up: the foot, the hollow and the ridge of one flank, then the other's, and the flat bed.
				FFormLoftSection& Section = Shape.Sections.AddDefaulted_GetRef();
				Section.Outline = FFormProfile::From(FVector2D(Weave - W, -Plan::InlayGroove));
				Section.Outline.LineTo(FVector2D(Weave + W, -Plan::InlayGroove), EFormJoint::Crisp)
					.LineTo(FVector2D(Weave + 0.5 * W, Plan::RayHollow * H), EFormJoint::Crisp)
					.LineTo(FVector2D(Weave, H), EFormJoint::Smooth)
					.LineTo(FVector2D(Weave - 0.5 * W, Plan::RayHollow * H), EFormJoint::Crisp)
					.LineTo(FVector2D(Weave - W, -Plan::InlayGroove), EFormJoint::Smooth)
					.Close();
				Section.AtMeters = FMath::Lerp(Plan::RayRoot, Tip, T);
			}
			// The ray's frame: z out along the ray, x across it (the way φ rises), y up.
			const FMuseeFrame Frame = FrameOfAxes(FVector::ZeroVector, Tangent(360.0 * Index / Plan::Rays), FVector::UpVector);
			OnDrum(Out, FString::Printf(TEXT("dial.sun.ray-%02d"), Index + 1), TEXT("dial.sun"), MoveTemp(Shape), Walkable, Frame);
		}

		/**
		 * The gilt sun: its boss (a foot in its groove, a rolled shoulder, a low dome), the plain bead ring on the dome and
		 * the plain torus at its foot, and its 32 rays. The visitor arrives standing on it and walks off across it, so the whole
		 * casting is walkable, as the dial round it is.
		 */
		void Sun(TArray<FFormDrawing>& Out)
		{
			const double Groove = -Plan::InlayGroove;
			const double DomeCentre = Plan::BossTop - Plan::BossDomeRadius;
			const double DomeFoot = Plan::BossRadius - Plan::BossRoll;
			const double DomeFromDeg = FMath::RadiansToDegrees(FMath::Acos(DomeFoot / Plan::BossDomeRadius));
			FFormLathe Boss;
			Boss.Section = FFormProfile::From(FVector2D(0.0, Groove));
			Boss.Section.LineTo(FVector2D(Plan::BossRadius, Groove), EFormJoint::Crisp)
				.LineTo(FVector2D(Plan::BossRadius, Plan::BossShoulder), EFormJoint::Crisp)
				.ArcAbout(FVector2D(DomeFoot, Plan::BossShoulder), Plan::BossRoll, Plan::BossRoll, 0.0, 90.0)
				.ArcAbout(FVector2D(0.0, DomeCentre), Plan::BossDomeRadius, Plan::BossDomeRadius, DomeFromDeg, 90.0, EFormJoint::Crisp);
			OnDrum(Out, TEXT("dial.sun.boss"), TEXT("dial.sun"), MoveTemp(Boss), Walkable);

			const double BeadZ = DomeCentre + FMath::Sqrt(FMath::Square(Plan::BossDomeRadius) - FMath::Square(Plan::BeadAt));
			FFormLathe Bead;
			Bead.Section = Circle(Plan::BeadRadius, FVector2D(Plan::BeadAt, BeadZ));
			OnDrum(Out, TEXT("dial.sun.bead"), TEXT("dial.sun"), MoveTemp(Bead), Walkable);
			FFormLathe Torus;
			Torus.Section = Circle(Plan::FootTorusRadius, FVector2D(Plan::FootTorusAt, Plan::FootTorusZ));
			OnDrum(Out, TEXT("dial.sun.foot"), TEXT("dial.sun"), MoveTemp(Torus), Walkable);

			for (int32 Index = 0; Index < Plan::Rays; ++Index)
			{
				Ray(Out, Index);
			}
		}
	}

	void DrawFloors(TArray<FFormDrawing>& Out)
	{
		FixedRing(Out);
		Motto(Out);
		// The dial is Time's: where its ring runs and its marks fall is where the museum's sun casts the node's shadow.
		// Without it the dial is drawn bare, and the missing inlay is Rotunda.DialLaid's to report.
		const TValueOrError<Time::FMuseumDial, FString> TimeDial = Time::FMuseumDial::Read();
		ensureMsgf(TimeDial.HasValue(), TEXT("The Rotunda's dial needs Time's: %s"), TimeDial.HasError() ? *TimeDial.GetError() : TEXT(""));
		Dial(Out, TimeDial.HasValue() ? TimeDial.GetValue().ShadowRadiusMeters() : 0.0);
		if (TimeDial.HasValue())
		{
			DrawDialInlay(Out, TimeDial.GetValue());
		}
		Sun(Out);
	}
}
