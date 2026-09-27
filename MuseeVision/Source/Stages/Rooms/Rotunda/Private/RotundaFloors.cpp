#include "FormCatalogue.h"
#include "RotundaParts.h"
#include "RotundaPlan.h"

/**
 * The floors (§3.1): the fixed ring of radial Crema Marfil slabs in three courses, laid 30 mm over its core; and the
 * dial, the lifting drum's top, in seven bands with bronze fillets on their joints and the gilt sun at its centre,
 * the arrival point; the motto in bronze capitals on the ring's middle course. The dial's hour ring, hour lines, meridian,
 * minute ticks and numerals are Time's to lay from its dial (§5).
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
		 * A band of the dial, its section cut with the groove its fillet sits in at each edge that carries one, and the
		 * centre band with the groove the gilt sun's boss sits in. The centre band's section ends on the axis.
		 */
		FFormProfile BandSection(double Inner, double Outer)
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

		/** The dial's bands, and a bronze fillet in the groove on each joint that carries one: 10 wide, 2 proud, bevelled. */
		void Dial(TArray<FFormDrawing>& Out)
		{
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::DialBands)); ++Index)
			{
				const Plan::FBand& Band = Plan::DialBands[Index];
				FFormLathe Shape;
				Shape.Section = BandSection(Band.Inner, Band.Outer);
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
		 * One of the sun's rays: long ones (i even, one to XII) to r 1.25, 72 wide at the root tapering as (1 - t)^0.62;
		 * short flames between them to r 0.86, 60 wide tapering as (1 - t)^0.75. Form has no lofted solid, so the nearest
		 * honest ray is round-backed: a half turn about its own axis, laid 2 mm down in its groove, its crest its half
		 * width. A hair of a bore runs along the axis inside the dial's stone, where the half turn's ends close the ray's
		 * flat underside.
		 */
		void Ray(TArray<FFormDrawing>& Out, int32 Index)
		{
			const bool bLong = Index % 2 == 0;
			const double Tip = bLong ? Plan::LongRayTip : Plan::ShortRayTip;
			const double HalfWidth = 0.5 * (bLong ? Plan::LongRayWidth : Plan::ShortRayWidth);
			const double Taper = bLong ? Plan::LongRayTaper : Plan::ShortRayTaper;
			const double Bore = 0.5 * Plan::InlayGroove;
			const double Deg = 360.0 * Index / Plan::Rays;
			FFormProfile Section = FFormProfile::From(FVector2D(Bore, Plan::RayRoot));
			for (int32 Station = 0; Station < Plan::RayStations; ++Station)
			{
				const double T = double(Station) / (Plan::RayStations - 1);
				const double Radius = Plan::InlayGroove + HalfWidth * FMath::Pow(1.0 - T, Taper);
				Section.LineTo(FVector2D(Radius, FMath::Lerp(Plan::RayRoot, Tip, T)), Station == 0 ? EFormJoint::Crisp : EFormJoint::Smooth);
			}
			Section.LineTo(FVector2D(Bore, Tip), EFormJoint::Crisp).Close();
			FFormLathe Shape;
			Shape.Section = MoveTemp(Section);
			Shape.FromDegrees = 0.0;
			Shape.ToDegrees = 180.0;
			// The ray's frame: z out along the ray, x across it (the way φ rises), y up; the half turn runs over the top.
			const FMuseeFrame Frame = FrameOfAxes(FVector(0.0, 0.0, -Plan::InlayGroove), Tangent(Deg), FVector::UpVector);
			OnDrum(Out, FString::Printf(TEXT("dial.sun.ray-%02d"), Index + 1), TEXT("dial.sun"), MoveTemp(Shape), nullptr, Frame);
		}

		/**
		 * The gilt sun: its boss (a foot in its groove, a rolled shoulder, a low dome), the plain bead ring on the dome and
		 * the plain torus at its foot, and its 32 rays.
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
			OnDrum(Out, TEXT("dial.sun.bead"), TEXT("dial.sun"), MoveTemp(Bead), nullptr);
			FFormLathe Torus;
			Torus.Section = Circle(Plan::FootTorusRadius, FVector2D(Plan::FootTorusAt, Plan::FootTorusZ));
			OnDrum(Out, TEXT("dial.sun.foot"), TEXT("dial.sun"), MoveTemp(Torus), nullptr);

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
		Dial(Out);
		Sun(Out);
	}
}
