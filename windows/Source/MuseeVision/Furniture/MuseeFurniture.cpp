#include "Furniture/MuseeFurniture.h"

#include "Chenghuai/ChenghuaiPlan.h"

#include "Furniture/FurnitureKit.h"
#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"

/**
 * Plan metres throughout (x east, plan y south, z up). The pieces' numbers are the materials spec's (Part 4) and the
 * plan's (MuseePlan.h; Plan.Oval.benches for the oval, as ASalonStructure had them).
 */
namespace MuseeFurnitureBuild
{
	using namespace FurnitureKit;

	enum ESection : int32 { FumedOak = 0, Leather, PatinaBronze, NaturalOak, Travertine, Granite, HitBoxes, SectionCount };

	struct FParts
	{
		FMeshData S[SectionCount];
		FMeshData Bronze;   // the thresholds
	};

	constexpr double kSeat = 0.45;   // every seat's top

	// ---------------------------------------------------------------- Helpers

	/** The part of a plan polyline between arc lengths S0 and S1. */
	TArray<FVector2D> SubPath(const TArray<FVector2D>& Pts, double S0, double S1)
	{
		TArray<FVector2D> Result;
		double Acc = 0.0;
		for (int32 i = 0; i + 1 < Pts.Num(); ++i)
		{
			const double L = FVector2D::Distance(Pts[i], Pts[i + 1]);
			const double A = Acc, B = Acc + L;
			Acc = B;
			if (L < 1e-12 || B <= S0 || A >= S1) { continue; }
			if (Result.Num() == 0) { Result.Add(FMath::Lerp(Pts[i], Pts[i + 1], (FMath::Max(S0, A) - A) / L)); }
			if (B < S1 - 1e-9) { Result.Add(Pts[i + 1]); }
			else
			{
				Result.Add(FMath::Lerp(Pts[i], Pts[i + 1], (S1 - A) / L));
				break;
			}
		}
		return Result;
	}

	double PathLength(const TArray<FVector2D>& Pts)
	{
		double L = 0.0;
		for (int32 i = 0; i + 1 < Pts.Num(); ++i) { L += FVector2D::Distance(Pts[i], Pts[i + 1]); }
		return L;
	}

	/** A level bar's section: across ±Half, from Z0 to Z1, every arris eased to Rad (shrunk by the ends' rounding). */
	FSectionFn Level(double Half, double Z0, double Z1, double Rad, int32 Seg = 4, int32 Sub = 1)
	{
		return RectSection(0.0, 0.5 * (Z0 + Z1), Half, 0.5 * (Z1 - Z0), Rad, Seg, Sub);
	}

	FBarEnds Rounded(double E, int32 Steps = 4)
	{
		FBarEnds Ends;
		Ends.E0 = Ends.E1 = E;
		Ends.RoundSteps = Steps;
		return Ends;
	}

	/**
	 * A seat of a slab on two plinths along a plan path (the oval's, the Sculpture Hall's): the slab SlabHalf across,
	 * 0.10 m thick, 10 mm round every edge; the plinths set back 60 mm from its long faces, PlinthLength long and
	 * PlinthFromEnd in from its ends, with a toe recess (8 × 12 mm) at the floor and a 20 mm shadow gap under the slab.
	 */
	void SlabOnPlinths(FParts& Out, const TArray<FVector2D>& Line, double SlabHalf, ESection SlabSection, double PlinthFromEnd, double PlinthLength)
	{
		const double SlabBottom = kSeat - 0.10, Ease = 0.010;
		const double SetBack = 0.06, Gap = 0.020, GapInset = 0.030, Toe = 0.012, ToeInset = 0.008;
		const double PlinthHalf = SlabHalf - SetBack, PlinthTop = SlabBottom - Gap;
		// The slab: stations every path vertex (a curve) so the grain and the eased arrises follow it.
		const FPath Path(AtHeight(Line, 0.0));
		Bar(Out.S[SlabSection], Path, Level(SlabHalf, SlabBottom, kSeat, Ease, 5, 2), Rounded(Ease, 5));
		const double Len = PathLength(Line);
		for (int32 End = 0; End < 2; ++End)
		{
			const double S0 = End == 0 ? PlinthFromEnd : Len - PlinthFromEnd - PlinthLength;
			const double S1 = S0 + PlinthLength;
			const FPath Plinth(AtHeight(SubPath(Line, S0, S1), 0.0));
			const FPath Neck(AtHeight(SubPath(Line, S0 + GapInset, S1 - GapInset), 0.0));
			const FPath Foot(AtHeight(SubPath(Line, S0 + ToeInset, S1 - ToeInset), 0.0));
			// The body, its arrises eased 6 mm; the shadow gap (a neck set in 30 mm) up into the slab; the toe.
			Bar(Out.S[Travertine], Plinth, Level(PlinthHalf, Toe, PlinthTop, 0.006), Rounded(0.006, 3));
			Bar(Out.S[Travertine], Neck, Level(PlinthHalf - GapInset, PlinthTop - 0.003, SlabBottom + 0.003, 0.002), Rounded(0.002, 2));
			Bar(Out.S[Travertine], Foot, Level(PlinthHalf - ToeInset, -0.003, Toe + 0.003, 0.002), Rounded(0.002, 2));
		}
		Prism(Out.S[HitBoxes], Path, -SlabHalf, SlabHalf, -0.01, kSeat);
	}

	// ---------------------------------------------------------------- The Salon's banquettes

	/**
	 * Salon interior update: none. The axis stays clear; the Salon's seats are now the velvet benches beside the line
	 * (ASalonBenches, Salon/SalonInterior.h: a pair in each of bays 2–5). The banquette's detail stays here, unused.
	 */
	const TArray<FVector2D>& BanquetteCentres()
	{
		static const TArray<FVector2D> List;
		return List;
	}

	void Banquette(FParts& Out, const FVector2D& C)
	{
		constexpr double HalfL = 0.90, HalfW = 0.30, Cushion = 0.08, CushionBottom = kSeat - Cushion;
		constexpr double Crown = 0.008, CushionTop = kSeat - Crown;          // 0.442 at the arris, 0.45 on the crown
		constexpr double RTop = 0.022, RBottom = 0.010, EndR = 0.022;
		constexpr double LegIn = 0.04, LegTop = 0.050, LegFoot = 0.035, Apron = 0.060, AproneThick = 0.025, Reveal = 0.003;
		constexpr double ApronBottom = CushionBottom - Apron;               // 0.31
		const FVector O(C.X, C.Y, 0.0);

		// The cushion: an upholstered box, crowned, its arrises soft (22 mm over the top, 10 mm under), rounded at the ends.
		{
			const double CB = 0.5 * (CushionBottom + CushionTop), HB = 0.5 * (CushionTop - CushionBottom);
			const FPath Path = FPath::Line(O + FVector(-HalfL, 0, 0), O + FVector(HalfL, 0, 0));
			Bar(Out.S[Leather], Path, [=](double, double Inset)
			{
				return RoundRect(0.0, CB, HalfW - Inset, HB - Inset, RBottom - Inset, RTop - Inset, 5, 8, Crown * FMath::Clamp(1.0 - Inset / EndR, 0.0, 1.0));
			}, Rounded(EndR, 6));
			// The piping round its top and bottom seams: a 4 mm cord, 3 mm proud, on the 45° line of each rounding.
			const double S45 = 1.0 - FMath::Sqrt(0.5);
			for (const bool bTop : {true, false})
			{
				const double R = bTop ? RTop : RBottom;
				const double Out45 = 0.0021;   // the cord's centre, 3 mm out along the 45° normal
				const double Z = bTop ? CushionTop - R * S45 + Out45 : CushionBottom + R * S45 - Out45;
				const double HX = HalfL - EndR * S45 + Out45, HY = HalfW - R * S45 + Out45;
				const TArray<FVector2D> Loop = RoundRect(C.X, C.Y, HX, HY, EndR * 0.8, EndR * 0.8, 6, 12);
				const FPath Cord(AtHeight(Loop, Z), FVector::UpVector, true);
				Bar(Out.S[Leather], Cord, [](double, double) { return Circle(0.0, 0.0, 0.004, 10); });
			}
		}

		// The frame: four legs, tapered on their inner faces (their outer faces plumb), 40 mm in from the cushion's edges.
		const double LX = HalfL - LegIn, LY = HalfW - LegIn;   // the legs' outer faces
		auto LegWidth = [=](double Z) { return FMath::Lerp(LegFoot, LegTop, FMath::Clamp(Z / ApronBottom, 0.0, 1.0)); };
		for (const double SX : {-1.0, 1.0})
		{
			for (const double SY : {-1.0, 1.0})
			{
				const FVector Top = O + FVector(SX * (LX - LegTop / 2), SY * (LY - LegTop / 2), 0.0);
				// A vertical path: B along x, A along −y (the frame's A = B × T).
				const FPath Leg = FPath::Line(Top, Top + FVector(0, 0, CushionBottom + 0.002), FVector::ForwardVector);
				auto Section = [=](double Z, double Inset)
				{
					const double W = LegWidth(Z);
					const double DX = SX * (LegTop - W) / 2, DY = SY * (LegTop - W) / 2;   // the centre moves out: outer faces plumb
					return RoundRect(-DY, DX, W / 2 - Inset, W / 2 - Inset, 0.004 - Inset, 0.004 - Inset, 3, 1);
				};
				FBarEnds Ends;
				Ends.E0 = 0.002;
				Ends.E1 = 0.0;
				Ends.RoundSteps = 2;
				Bar(Out.S[FumedOak], Leg, Section, Ends, {0.0, ApronBottom, CushionBottom + 0.002});
				// Its bronze shoe: a sleeve 40 mm high, 1.5 mm proud of the leg, its top arris eased.
				const double ShoeW = LegWidth(0.02) + 0.003;
				const double DX = SX * (LegTop - LegWidth(0.02)) / 2, DY = SY * (LegTop - LegWidth(0.02)) / 2;
				FBarEnds ShoeEnds;
				ShoeEnds.E1 = 0.0015;
				ShoeEnds.RoundSteps = 2;
				Bar(Out.S[PatinaBronze], FPath::Line(Top + FVector(0, 0, -0.001), Top + FVector(0, 0, 0.040), FVector::ForwardVector),
					[=](double, double Inset) { return RoundRect(-DY, DX, ShoeW / 2 - Inset, ShoeW / 2 - Inset, 0.005 - Inset, 0.005 - Inset, 3, 1); }, ShoeEnds);
			}
		}
		// The aprons, 60 × 25 mm, set 3 mm back from the legs' outer faces, their ends 5 mm into the legs.
		const double AZ0 = ApronBottom, AZ1 = CushionBottom;
		const double AlongX = LX - LegTop + 0.005, AlongY = LY - LegTop + 0.005;
		for (const double SY : {-1.0, 1.0})
		{
			const double Y = SY * (LY - Reveal - AproneThick / 2);
			Bar(Out.S[FumedOak], FPath::Line(O + FVector(-AlongX, Y, 0), O + FVector(AlongX, Y, 0)), Level(AproneThick / 2, AZ0, AZ1, 0.003));
		}
		for (const double SX : {-1.0, 1.0})
		{
			const double X = SX * (LX - Reveal - AproneThick / 2);
			Bar(Out.S[FumedOak], FPath::Line(O + FVector(X, -AlongY, 0), O + FVector(X, AlongY, 0)), Level(AproneThick / 2, AZ0, AZ1, 0.003));
		}
		// The H-stretcher, low (its top 0.12 m): across between each pair of legs, and along the middle.
		const double SZ0 = 0.090, SZ1 = 0.120, SHalf = 0.011;
		const double LegCentreX = LX - LegWidth(0.105) / 2;
		const double AcrossY = LY - LegWidth(0.105) + 0.005;
		for (const double SX : {-1.0, 1.0})
		{
			const double X = SX * LegCentreX;
			Bar(Out.S[FumedOak], FPath::Line(O + FVector(X, -AcrossY, 0), O + FVector(X, AcrossY, 0)), Level(SHalf, SZ0, SZ1, 0.003));
		}
		Bar(Out.S[FumedOak], FPath::Line(O + FVector(-LegCentreX + SHalf - 0.004, 0, 0), O + FVector(LegCentreX - SHalf + 0.004, 0, 0)),
			Level(SHalf, SZ0 + 0.002, SZ1 - 0.002, 0.003));

		Prism(Out.S[HitBoxes], FPath::Line(O + FVector(-HalfL, 0, 0), O + FVector(HalfL, 0, 0)), -HalfW, HalfW, -0.01, kSeat);
	}

	// ---------------------------------------------------------------- The oval's benches

	const TArray<TArray<FVector2D>>& OvalBenchLines()
	{
		// Plan.Oval.benches, moved out for the pond's new coping (the pond-edge redesign, 2026-09-26): the same arcs,
		// now 1.975 m out from the water's edge (the ellipse 3.9 × 2 about (−86.5, 0)): 1.26 m clear of the coping's face.
		static const TArray<TArray<FVector2D>> Lines = {
			{FVector2D(-92.04, 1.39), FVector2D(-91.79, 1.81), FVector2D(-91.50, 2.17), FVector2D(-91.18, 2.48), FVector2D(-90.84, 2.76), FVector2D(-90.48, 2.99),
			 FVector2D(-90.10, 3.20), FVector2D(-89.71, 3.38), FVector2D(-89.31, 3.53), FVector2D(-88.90, 3.66), FVector2D(-88.48, 3.76)},
			{FVector2D(-84.52, 3.76), FVector2D(-84.10, 3.66), FVector2D(-83.69, 3.53), FVector2D(-83.29, 3.38), FVector2D(-82.90, 3.20), FVector2D(-82.52, 2.99),
			 FVector2D(-82.16, 2.76), FVector2D(-81.82, 2.48), FVector2D(-81.50, 2.17), FVector2D(-81.21, 1.81), FVector2D(-80.96, 1.39)},
			{FVector2D(-80.96, -1.39), FVector2D(-81.21, -1.81), FVector2D(-81.50, -2.17), FVector2D(-81.82, -2.48), FVector2D(-82.16, -2.76), FVector2D(-82.52, -2.99),
			 FVector2D(-82.90, -3.20), FVector2D(-83.29, -3.38), FVector2D(-83.69, -3.53), FVector2D(-84.10, -3.66), FVector2D(-84.52, -3.76)},
			{FVector2D(-88.48, -3.76), FVector2D(-88.90, -3.66), FVector2D(-89.31, -3.53), FVector2D(-89.71, -3.38), FVector2D(-90.10, -3.20), FVector2D(-90.48, -2.99),
			 FVector2D(-90.84, -2.76), FVector2D(-91.18, -2.48), FVector2D(-91.50, -2.17), FVector2D(-91.79, -1.81), FVector2D(-92.04, -1.39)},
		};

		return Lines;
	}

	void OvalBenches(FParts& Out)
	{
		for (const TArray<FVector2D>& Line : OvalBenchLines())
		{
			SlabOnPlinths(Out, CatmullRom(Line, 8), 0.275, NaturalOak, 0.30, 0.45);
		}
	}

	// ---------------------------------------------------------------- The Sculpture Hall's bench

	void SculptureBench(FParts& Out)
	{
		// Opposite The Dance: x −4.9 … −4.4, y −22.3 … −19.9 (the plan's), along y.
		SlabOnPlinths(Out, {FVector2D(-4.65, -22.3), FVector2D(-4.65, -19.9)}, 0.25, Travertine, 0.25, 0.30);
	}

	// ---------------------------------------------------------------- The Chinese Wing's terrace benches

	void ChineseBenches(FParts& Out)
	{
		namespace CW = MuseePlan::ChineseWing;
		const double Y = 0.5 * (CW::BenchY0 + CW::BenchY1), Half = 0.5 * (CW::BenchY1 - CW::BenchY0);
		const double SlabBottom = CW::BenchHeight - 0.09;   // 0.36: the path light under it (0.335) stays clear
		for (const double X0 : CW::BenchX0)
		{
			const double X1 = X0 + CW::BenchLength;
			const FPath Slab = FPath::Line(FVector(X0, Y, 0), FVector(X1, Y, 0));
			Bar(Out.S[Granite], Slab, Level(Half, SlabBottom, CW::BenchHeight, 0.012, 4, 2), Rounded(0.012, 4));
			// Two waisted legs (束腰): a hoof foot, a waist, a bead, the shaft, a flared cap under the slab.
			const double HX = 0.15, HY = Half - 0.035;
			struct FStep { double Z, Inset; };
			static const FStep Profile[] = {
				{0.000, 0.000}, {0.030, 0.000}, {0.040, 0.003}, {0.048, 0.009}, {0.055, 0.018}, {0.062, 0.024}, {0.084, 0.024},
				{0.090, 0.017}, {0.097, 0.012}, {0.104, 0.017}, {0.112, 0.020}, {0.320, 0.020}, {0.330, 0.014}, {0.340, 0.008},
				{0.348, 0.006}, {SlabBottom + 0.004, 0.006}};
			TArray<double> Stations;
			for (const FStep& P : Profile) { Stations.Add(P.Z); }
			auto InsetAt = [](double Z)
			{
				const int32 N = UE_ARRAY_COUNT(Profile);
				for (int32 i = 0; i + 1 < N; ++i)
				{
					if (Z <= Profile[i + 1].Z) { return FMath::Lerp(Profile[i].Inset, Profile[i + 1].Inset, FMath::Clamp((Z - Profile[i].Z) / (Profile[i + 1].Z - Profile[i].Z), 0.0, 1.0)); }
				}
				return Profile[N - 1].Inset;
			};
			for (const double XL : {X0 + 0.35, X1 - 0.35})
			{
				const FPath Leg = FPath::Line(FVector(XL, Y, 0.0), FVector(XL, Y, SlabBottom + 0.004), FVector::ForwardVector);
				FBarEnds Ends;
				Bar(Out.S[Granite], Leg, [=](double Z, double)
				{
					const double D = InsetAt(Z);
					return RoundRect(0.0, 0.0, HY - D, HX - D, 0.010, 0.010, 3, 1);
				}, Ends, Stations);
			}
			Prism(Out.S[HitBoxes], Slab, -Half, Half, -0.01, CW::BenchHeight);
		}
	}

	// ---------------------------------------------------------------- The thresholds

	/** The strip's section: 60 mm wide, 12 mm deep, its top 0.5 mm proud of the floor at the path's height. */
	FSectionFn Strip() { return Level(0.030, -0.0115, 0.0005, 0.0008, 2, 1); }

	void ArcStrip(FParts& Out, const FVector2D& Centre, double Radius, double Bearing, double HalfWidth, double Z)
	{
		const double Reach = FMath::Asin(FMath::Min(1.0, (HalfWidth + 0.015) / Radius));
		Bar(Out.Bronze, FPath(FurnitureKit::Arc(Centre, Radius, Bearing - Reach, Bearing + Reach, Z, 48)), Strip());
	}

	void Straight(FParts& Out, const FVector& A, const FVector& B)
	{
		Bar(Out.Bronze, FPath::Line(A, B), Strip());
	}

	void BuildThresholds(FParts& Out)
	{
		namespace R = MuseePlan::Rotunda;
		const double J = 0.015;   // into the jambs
		// The Rotunda's four doors, on the sun clock's edge (r 10.3): west to the Salon's passage, north to the Sculpture
		// Hall, south to the Chinese vestibule, east to the Hall of Light. Plan bearings from east towards south.
		const double Clock = MuseePlan::HallOfLight::SunClockRadius;
		ArcStrip(Out, FVector2D::ZeroVector, Clock, kPi, R::DoorWidthWestEast / 2, 0.0);
		ArcStrip(Out, FVector2D::ZeroVector, Clock, 0.0, R::DoorWidthWestEast / 2, 0.0);
		ArcStrip(Out, FVector2D::ZeroVector, Clock, -0.5 * kPi, R::DoorWidthNorthSouth / 2, 0.0);
		ArcStrip(Out, FVector2D::ZeroVector, Clock, 0.5 * kPi, R::DoorWidthNorthSouth / 2, 0.0);
		// The Salon's door from the passage: over the joint where the passage's stone meets the Salon's (x −13.4).
		Straight(Out, FVector(-13.43, -1.5 - J, 0.0), FVector(-13.43, 1.5 + J, 0.0));
		// The Manet cabinet's door (x −20, 3 m) in the Salon's north wall (y −7.0 … −7.6): mid-reveal.
		Straight(Out, FVector(-20.0 - 1.5 - J, -7.3, 0.0), FVector(-20.0 + 1.5 + J, -7.3, 0.0));
		// The Nymphéas oval's arch: on the oval's inner face (a 11 × 7.5 m about x −86.5), where its jambs end.
		{
			const double Q = 1.5 / 7.5;
			const double X = -86.5 + 11.0 * FMath::Sqrt(1.0 - Q * Q) + 0.03;
			Straight(Out, FVector(X, -1.5 - J, 0.0), FVector(X, 1.5 + J, 0.0));
		}
		// The Atrium's west door (4 m): where the Hall of Light's floor meets the Atrium's (its 96-gon, r 14.3).
		ArcStrip(Out, FVector2D(MuseePlan::Elan::CentreX, MuseePlan::Elan::CentreY), MuseePlan::Atrium::FloorRadius, kPi, MuseePlan::Elan::DoorWidth / 2, 0.0);
		// The Reserve's arch: where the long stair's travertine landing meets the concrete (x −73.7), across its notch.
		{
			const double Z = MuseePlan::Reserve::FloorZ, H = MuseePlan::LongStair::HalfWidth;
			Straight(Out, FVector(MuseePlan::LongStair::LandingEnd - 0.03, -H, Z), FVector(MuseePlan::LongStair::LandingEnd - 0.03, H, Z));
		}
		// The portico's bronze door (x −44, 3 m, its leaves 5–12 cm out from the Salon's south face at y 7.6, sill on the podium).
		{
			namespace FP = MuseePlan::Facade;
			const double Y = FP::SalonHalf + FP::WindowBack + 0.06 + 0.03;   // just in front of the leaves
			const double Z = FP::Podium + 0.004;   // the portico's floor stands 4 mm over the podium's cap
			Straight(Out, FVector(FP::PorticoAxisX - 1.5 - J, Y, Z), FVector(FP::PorticoAxisX + 1.5 + J, Y, Z));
		}
		// The Classical Hall: a 10 mm bronze angle on the nosing of its 1 cm porphyry sill (the lip, r 7.55, on the landing).
		{
			namespace CH = MuseePlan::ClassicalHall;
			const double Half = CH::OpeningHalfWidth - CH::PassageInset + 0.005;
			const double Reach = FMath::Asin(Half / CH::LipRadius);
			const FPath Lip(FurnitureKit::Arc(FVector2D::ZeroVector, CH::LipRadius, -0.5 * kPi - Reach, -0.5 * kPi + Reach, CH::FloorZ, 32));
			// On this path A points towards the centre (the landing): the angle's top runs 10 mm onto the porphyry
			// (A −0.010) and its face stands 0.5 mm proud of the riser (A +0.0005); 0.5 mm over the sill's top.
			Bar(Out.Bronze, Lip, RectSection(-0.00475, 0.00425, 0.00525, 0.00625, 0.0008, 2, 1));
		}
	}
}

AMuseeFurniture::AMuseeFurniture()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);

	Pieces = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Pieces"));
	Pieces->SetupAttachment(RootComponent);
	Pieces->bUseAsyncCooking = true;
	Pieces->bUseComplexAsSimpleCollision = true;
	Pieces->SetCollisionProfileName(TEXT("BlockAll"));
	Pieces->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	Thresholds = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Thresholds"));
	Thresholds->SetupAttachment(RootComponent);
	Thresholds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Thresholds->SetCastShadow(false);

	auto Soft = [](const TCHAR* Path) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path)); };
	FumedOakMaterial = Soft(TEXT("/Game/Museum/Materials/M_Oak_Fumed.M_Oak_Fumed"));
	LeatherMaterial = Soft(TEXT("/Game/Museum/Materials/M_Leather_Cognac.M_Leather_Cognac"));
	PatinaBronzeMaterial = Soft(TEXT("/Game/Museum/Materials/M_Bronze_Patina.M_Bronze_Patina"));
	NaturalOakMaterial = Soft(TEXT("/Game/Museum/Materials/M_Oak_Natural.M_Oak_Natural"));
	TravertineMaterial = Soft(TEXT("/Game/Museum/Materials/M_Travertine_Solid.M_Travertine_Solid"));   // honed, jointless
	GraniteMaterial = Soft(TEXT("/Game/Museum/Materials/USD/MI_stone_plinth_r75.MI_stone_plinth_r75"));
	ThresholdMaterial = Soft(TEXT("/Game/Museum/Materials/M_Bronze_Brushed.M_Bronze_Brushed"));
	FallbackMaterials = {
		Soft(TEXT("/Game/Museum/Materials/M_Wood.M_Wood")), Soft(TEXT("/Game/Museum/Materials/M_Fabric.M_Fabric")),
		Soft(TEXT("/Game/Museum/Materials/USD/MI_bronze_dark.MI_bronze_dark")), Soft(TEXT("/Game/Museum/Materials/M_Wood.M_Wood")),
		Soft(TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed")), Soft(TEXT("/Game/Museum/Materials/USD/MI_stone_grey.MI_stone_grey")),
		Soft(TEXT("/Game/Museum/Materials/M_Metal.M_Metal"))};

	Tags.AddUnique(FName(TEXT("musee.building")));
}

void AMuseeFurniture::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
}

void AMuseeFurniture::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never shows an older build.
	Build();
	ApplyMaterials();
}

TArray<FVector2D> AMuseeFurniture::GetBanquetteCentres()
{
	return MuseeFurnitureBuild::BanquetteCentres();
}

void AMuseeFurniture::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	using namespace MuseeFurnitureBuild;
	FParts Parts;
	for (const FVector2D& C : BanquetteCentres()) { Banquette(Parts, C); }
	OvalBenches(Parts);
	// Chenghuai: once it has the north door the Sculpture Hall is retired (its bench would stand in Chenghuai's front row).
	if (!Chenghuai::bNorthDoorOpen) { SculptureBench(Parts); }
	// ChineseBenches(Parts);   // Albion: the Chinese Wing is retired from the south door; its benches with it
	BuildThresholds(Parts);

	Pieces->ClearAllMeshSections();
	Thresholds->ClearAllMeshSections();
	for (int32 s = 0; s < HitBoxes; ++s) { Parts.S[s].Write(Pieces, s, false); }
	Parts.S[HitBoxes].Write(Pieces, HitBoxes, true);
	Pieces->SetMeshSectionVisible(HitBoxes, false);
	Parts.Bronze.Write(Thresholds, 0, false);
}

void AMuseeFurniture::ApplyMaterials()
{
	auto Pick = [this](const TSoftObjectPtr<UMaterialInterface>& Wanted, int32 Fallback) -> UMaterialInterface*
	{
		UMaterialInterface* M = Wanted.IsNull() ? nullptr : Wanted.LoadSynchronous();
		if (!M && FallbackMaterials.IsValidIndex(Fallback)) { M = FallbackMaterials[Fallback].LoadSynchronous(); }
		return M;
	};
	const TSoftObjectPtr<UMaterialInterface>* Wanted[] = {&FumedOakMaterial, &LeatherMaterial, &PatinaBronzeMaterial, &NaturalOakMaterial,
														 &TravertineMaterial, &GraniteMaterial};
	for (int32 s = 0; s < UE_ARRAY_COUNT(Wanted); ++s)
	{
		if (UMaterialInterface* M = Pick(*Wanted[s], s)) { Pieces->SetMaterial(s, M); }
	}
	if (UMaterialInterface* M = Pick(TravertineMaterial, 4)) { Pieces->SetMaterial(MuseeFurnitureBuild::HitBoxes, M); }
	if (UMaterialInterface* M = Pick(ThresholdMaterial, 6)) { Thresholds->SetMaterial(0, M); }
}
