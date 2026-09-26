#include "Facade/MuseeLandscape.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Curves/CurveFloat.h"
#include "Ground/MuseeGround.h"
#include "Kismet/GameplayStatics.h"
#include "Nature/MuseeNatureActor.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreMisc.h"
#include "Nature/MuseeHedge.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Facade/FacadeKit.h"
#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "MuseeVision.h"
#include "Nature/NatureMesh.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"

/** The grounds' layout, in plan metres (x east, plan y south, up); the ground plane lies at −0.03. */
namespace GroundsBuild
{
	namespace G = MuseePlan::Grounds;
	namespace FP = MuseePlan::Facade;
	using namespace FacadeKit;

	constexpr double kGravel = 0.018, kPave = 0.024, kSlabBottom = -0.12;
	constexpr double kPanelY0 = G::CanalY0, kPanelY1 = G::CanalY1 - 2.0;

	struct FParts
	{
		FMeshData Paving, Gravel, Stone, Basin, Water, Core, Posts, Globes;
	};

	/** The lamp standards: along the canal's walks every 8 m, and at the parvis's corners. */
	TArray<FVector2D> LampPositions()
	{
		const double Ax = G::AxisX, Walk = G::CanalHalf + 0.45;   // at the walk's canal side, clear of the coping
		TArray<FVector2D> P;
		for (const double Y : {29.0, 37.0, 45.0, 53.0})
		{
			for (const double Side : {-1.0, 1.0}) { P.Add(FVector2D(Ax + Side * Walk, Y)); }
		}
		for (const double Side : {-1.0, 1.0}) { P.Add(FVector2D(Ax + Side * (G::ParvisHalf - 1.5), G::ParvisY1 - 1.5)); }
		return P;
	}

	constexpr double kGlobeZ = 3.62, kGlobeR = 0.24;

	/** A cast bronze lamp standard (a moulded base, a slender tapering post, a collar and a cup) with an opal globe. */
	void Lamp(FParts& P, const FVector2D& At)
	{
		const double Pts[][2] = {{0, 0}, {0.27, 0}, {0.27, 0.07}, {0.23, 0.11}, {0.21, 0.40}, {0.17, 0.46}, {0.12, 0.52}, {0.10, 0.60},
								 {0.085, 0.70}, {0.062, 3.05}, {0.095, 3.10}, {0.095, 3.17}, {0.06, 3.21}, {0.07, 3.30}, {0.15, 3.38},
								 {0.16, 3.41}, {0, 3.41}};
		FProfile Post;
		for (int32 i = 0; i < UE_ARRAY_COUNT(Pts); ++i) { Post.Add(Pts[i][0], Pts[i][1], i == 4 || i == 5 || i == 6 || i == 13); }
		const FVector Foot(At.X, At.Y, 0.0);
		Lathe(P.Posts, Foot, Post, 20);
		FProfile Globe;
		constexpr int32 Steps = 12;
		for (int32 i = 0; i <= Steps; ++i)
		{
			const double T = Pi * i / Steps;
			Globe.Add(kGlobeR * FMath::Sin(T), kGlobeZ - kGlobeR * FMath::Cos(T), i > 0 && i < Steps);
		}
		Lathe(P.Globes, Foot, Globe, 24);
		FProfile Cap;
		Cap.Add(0.0, kGlobeZ + kGlobeR - 0.03).Add(0.09, kGlobeZ + kGlobeR - 0.03).Add(0.06, kGlobeZ + kGlobeR + 0.04).Add(0.025, kGlobeZ + kGlobeR + 0.07)
			.Add(0.035, kGlobeZ + kGlobeR + 0.11).Add(0.0, kGlobeZ + kGlobeR + 0.14);
		Lathe(P.Posts, Foot, Cap, 16);
	}

	void Slab(FMeshData& M, double X0, double X1, double Y0, double Y1, double Top)
	{
		if (X1 - X0 < 1e-3 || Y1 - Y0 < 1e-3) { return; }
		WBox(M, X0, X1, Y0, Y1, kSlabBottom, Top, FMeshData::AllFaces & ~FMeshData::NegZ);
	}

	/** A flat ring (or a sector of one) about C, its top at Top, its outer edge dropping into the ground. */
	void Annulus(FMeshData& M, const FVector2D& C, double R0, double R1, double Deg0, double Deg1, double Top)
	{
		const int32 N = FMath::Max(8, FMath::CeilToInt32(FMath::DegreesToRadians(FMath::Abs(Deg1 - Deg0)) * R1 / 0.3));
		const FVector Up(0, 0, 1);
		for (int32 i = 0; i < N; ++i)
		{
			const double A0 = FMath::DegreesToRadians(Deg0 + (Deg1 - Deg0) * i / N), A1 = FMath::DegreesToRadians(Deg0 + (Deg1 - Deg0) * (i + 1) / N);
			const FVector2D D0(FMath::Cos(A0), FMath::Sin(A0)), D1(FMath::Cos(A1), FMath::Sin(A1));
			M.Rect(Flat(C + D0 * R0, Top), Flat(C + D1 * R0, Top), Flat(C + D1 * R1, Top), Flat(C + D0 * R1, Top), Up);
			const FVector Out = Flat((D0 + D1).GetSafeNormal());
			M.Rect(Flat(C + D0 * R1, kSlabBottom), Flat(C + D1 * R1, kSlabBottom), Flat(C + D1 * R1, Top), Flat(C + D0 * R1, Top), Out);
		}
	}

	FProfile KerbProfile()
	{
		FProfile P;
		P.bClosed = true;
		P.Add(-0.06, -0.16).Add(0.06, -0.16).Add(0.06, 0.03).Add(0.045, 0.045).Add(-0.045, 0.045).Add(-0.06, 0.03);
		return P;
	}

	void Kerb(FParts& P, const TArray<FVector2D>& Path, bool bClosed = false)
	{
		const FProfile K = KerbProfile();
		const TArray<TArray<FSweepFrame>> Runs = PlanSweep(P.Stone, Path, bClosed, K);
		if (!bClosed) { CapEnds(P.Stone, Runs, K); }
	}

	/** A basin's coping (A out from the water's edge): a rounded nose over the water, a rounded outer arris; its walls down to the floor. */
	FProfile CopingProfile()
	{
		const double Top = G::CopingTop, W = G::CopingWidth, Foot = G::BasinFloor - 0.06;
		FProfile P;
		P.bClosed = true;
		P.Add(0.04, Foot).Add(W, Foot).Add(W, Top - 0.05);
		P.Arc(W - 0.05, Top - 0.05, 0.05, 0.05, 0, 90, 4);
		P.Add(0.03, Top);
		P.Arc(0.03, Top - 0.03, 0.03, 0.03, 90, 180, 4);
		P.Add(0.0, Top - 0.09).Add(0.04, Top - 0.09);
		return P;
	}

	/** A basin round Inner (its left normal pointing away from the water): coping, dark floor, still water. */
	void Basin(FParts& P, const TArray<FVector2D>& Inner)
	{
		PlanSweep(P.Stone, Inner, true, CopingProfile());
		PlanPoly(P.Basin, Inner, G::BasinFloor, true);
		PlanPoly(P.Water, Inner, G::WaterLevel, true);
	}

	// ================================================================================================ the hedges

	/** One hedge module (MuseeHedge) in place. */
	struct FFoliage
	{
		int32 Mesh = 0;
		FTransform Transform;
	};

	/** Where the hedges' foliage goes; and their cores (collision, and the dark interior under the leaves). */
	struct FHedgeSink
	{
		FParts* Parts = nullptr;
		TArray<FFoliage>* Foliage = nullptr;
	};

	constexpr double kGround = -0.03;

	/** The clipped surface's slow drift in height along a hedge (a fraction; a few millimetres, never a step). */
	double HeightDrift(const FVector2D& At)
	{
		return 1.0 + 0.012 * 2.0 * (MuseeNature::Noise(FVector(At.X / 5.5, At.Y / 5.5, 0.7), 91) - 0.5);
	}

	void Place(FHedgeSink& Sink, MuseeHedge::EKind Kind, const FVector2D& At, const FVector2D& XAxis, double ScaleX, double ScaleY, uint32 Key,
			   double Z = kGround)
	{
		if (!Sink.Foliage) { return; }
		const int32 Variants = MuseeHedge::Spec(Kind).Variants;
		const int32 V = FMath::Min(Variants - 1, int32(MuseeNature::Hash(int32(Key), 17, int32(Kind), 5) * Variants));
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(XAxis.Y, XAxis.X));
		FFoliage F;
		F.Mesh = MuseeHedge::MeshIndex(Kind, V);
		F.Transform = FTransform(FRotator(0.0, Yaw, 0.0), FVector(At.X, At.Y, Z) * MuseePlan::Cm, FVector(ScaleX, ScaleY, HeightDrift(At)));
		Sink.Foliage->Add(F);
	}

	/** Run modules along a straight line from A to B (their ends meet corners or ends there). */
	void Runs(FHedgeSink& Sink, MuseeHedge::EKind Kind, const FVector2D& A, const FVector2D& B, uint32& Key)
	{
		const double Len = FVector2D::Distance(A, B);
		if (Len < 0.05) { return; }
		const FVector2D D = (B - A) / Len;
		const int32 N = FMath::Max(1, FMath::RoundToInt32(Len / MuseeHedge::Length));
		const double SX = Len / (N * MuseeHedge::Length);
		// (Never mirrored: a mirrored instance turns its leaves' faces inside out, their pale undersides to the light.)
		for (int32 k = 0; k < N; ++k) { Place(Sink, Kind, A + D * (k * MuseeHedge::Length * SX), D, SX, 1.0, Key++); }
	}

	/** The dark core of a hedge along a plan path: Inset inside the clipped surface, a little sunk into the ground. */
	void Core(FHedgeSink& Sink, const TArray<FVector2D>& Path, bool bClosed, const MuseeHedge::FSpec& S)
	{
		if (!Sink.Parts) { return; }
		const double W = S.Width - 2.0 * S.Inset, H = S.Height - S.Inset + kGround;
		const double R = FMath::Min(0.06, 0.3 * W);
		FProfile P;
		P.bClosed = true;
		P.Add(-0.5 * W, kGround - 0.06).Add(0.5 * W, kGround - 0.06).Add(0.5 * W, H - R);
		P.Arc(0.5 * W - R, H - R, R, R, 0, 90, 4);
		P.Add(-0.5 * W + R, H);
		P.Arc(-0.5 * W + R, H - R, R, R, 90, 180, 4);
		const TArray<TArray<FSweepFrame>> Swept = PlanSweep(Sink.Parts->Core, Path, bClosed, P);
		if (!bClosed) { CapEnds(Sink.Parts->Core, Swept, P); }
	}

	/** A closed clipped box hedge round a rectangle (its corners in order): corner modules, runs between. */
	void BoxLoop(FHedgeSink& Sink, const TArray<FVector2D>& Corners, uint32& Key)
	{
		using MuseeHedge::EKind;
		const MuseeHedge::FSpec& S = MuseeHedge::Spec(EKind::BoxRun);
		Core(Sink, Corners, true, S);
		const int32 N = Corners.Num();
		const double A = 0.5 * S.Width;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D P = Corners[i];
			const FVector2D Next = (Corners[(i + 1) % N] - P).GetSafeNormal(), Prev = (Corners[(i + N - 1) % N] - P).GetSafeNormal();
			// The corner module's arms run along its +x and +y (+y to the left of +x).
			const bool bNextFirst = FVector2D::CrossProduct(Next, Prev) > 0.0;
			Place(Sink, EKind::BoxCorner, P, bNextFirst ? Next : Prev, 1.0, 1.0, Key++);
			Runs(Sink, EKind::BoxRun, P + Next * A, Corners[(i + 1) % N] - Next * A, Key);
		}
	}

	/** An open hedge along a straight line: an end module at each end, runs between. */
	void OpenLine(FHedgeSink& Sink, MuseeHedge::EKind RunKind, MuseeHedge::EKind EndKind, const FVector2D& A, const FVector2D& B, uint32& Key)
	{
		const MuseeHedge::FSpec& S = MuseeHedge::Spec(RunKind);
		const FVector2D D = (B - A).GetSafeNormal();
		Core(Sink, {A + D * S.Inset, B - D * S.Inset}, false, S);   // (its ends inside the clipped ends too)
		Place(Sink, EndKind, A, -D, 1.0, 1.0, Key++);
		Place(Sink, EndKind, B, D, 1.0, 1.0, Key++);
		Runs(Sink, RunKind, A + D * MuseeHedge::EndLength, B - D * MuseeHedge::EndLength, Key);
	}

	/** An open hedge along an arc about C (degrees From … To): ends, and runs bent to the curve. */
	void OpenArc(FHedgeSink& Sink, MuseeHedge::EKind RunKind, MuseeHedge::EKind EndKind, const FVector2D& C, double R, double From, double To,
				 uint32& Key)
	{
		const MuseeHedge::FSpec& S = MuseeHedge::Spec(RunKind);
		const double InsetDegrees = FMath::RadiansToDegrees(S.Inset / R) * (To > From ? 1.0 : -1.0);
		Core(Sink, ArcPoints(C, R, From + InsetDegrees, To - InsetDegrees, 0.4), false, S);
		auto Point = [&](double Rad) { return C + FVector2D(FMath::Cos(Rad), FMath::Sin(Rad)) * R; };
		auto Tangent = [&](double Rad) { return FVector2D(-FMath::Sin(Rad), FMath::Cos(Rad)); };
		const double T0 = FMath::DegreesToRadians(From), T1 = FMath::DegreesToRadians(To);
		const double Sign = T1 > T0 ? 1.0 : -1.0;
		Place(Sink, EndKind, Point(T0), -Tangent(T0) * Sign, 1.0, 1.0, Key++);
		Place(Sink, EndKind, Point(T1), Tangent(T1) * Sign, 1.0, 1.0, Key++);
		// The runs are bent to this curve (MuseeHedge::FSpec::Bend, the centre to their left: From < To): each starts
		// where the last ends, along the tangent there.
		ensure(FMath::IsNearlyEqual(S.Bend, R, 0.01) && Sign > 0.0);
		const double A = T0 + Sign * MuseeHedge::EndLength / R, B = T1 - Sign * MuseeHedge::EndLength / R;
		const double Arc = FMath::Abs(B - A) * R;
		const int32 N = FMath::Max(1, FMath::RoundToInt32(Arc / MuseeHedge::Length));
		const double Step = (B - A) / N;
		const double SX = FMath::Abs(Step) * R / MuseeHedge::Length;
		for (int32 k = 0; k < N; ++k)
		{
			const double Start = A + k * Step;
			Place(Sink, RunKind, Point(Start), Tangent(Start) * Sign, SX, 1.0, Key++);
		}
	}

	/** A clipped ball of box resting on the ground at At. */
	void BoxBall(FHedgeSink& Sink, const FVector2D& At, uint32& Key)
	{
		const MuseeHedge::FSpec& S = MuseeHedge::Spec(MuseeHedge::EKind::BoxBall);
		const double Yaw = MuseeNature::Hash(int32(Key), 9, 0, 7) * 2.0 * Pi;
		Place(Sink, MuseeHedge::EKind::BoxBall, At, FVector2D(FMath::Cos(Yaw), FMath::Sin(Yaw)), 1.0, 1.0, Key++);
		if (!Sink.Parts) { return; }
		const double R = S.Width - S.Inset;
		const double CZ = kGround + S.Width - MuseeHedge::BallSink;
		FProfile Ball;
		constexpr int32 Steps = 10;
		for (int32 i = 0; i <= Steps; ++i)
		{
			const double T = Pi * i / Steps;
			Ball.Add(R * FMath::Sin(T), CZ - R * FMath::Cos(T), i > 0 && i < Steps);
		}
		Lathe(Sink.Parts->Core, FVector(At.X, At.Y, 0.0), Ball, 20);
	}

	/**
	 * Every hedge of the grounds: box round the parterre's lawn panels with clipped balls at their corners; the yew
	 * exedra closing the cross walk; the Hall of Light's garden hedges along its long sides (yew, 1.5 m, replacing the
	 * import's plain boxes).
	 */
	void BuildHedges(FHedgeSink& Sink)
	{
		const double Ax = G::AxisX;
		uint32 Key = 9100;
		for (const double Side : {-1.0, 1.0})
		{
			const double In = 0.45;
			const double XI = Ax + Side * (G::PanelInner + In), XO = Ax + Side * (G::PanelOuter - In);
			BoxLoop(Sink, {FVector2D(XI, kPanelY0 + In), FVector2D(XO, kPanelY0 + In), FVector2D(XO, kPanelY1 - In), FVector2D(XI, kPanelY1 - In)}, Key);
			for (const double CY : {kPanelY0 + 1.35, kPanelY1 - 1.35})
			{
				for (const double CX : {XI + Side * 0.9, XO - Side * 0.9}) { BoxBall(Sink, FVector2D(CX, CY), Key); }
			}
		}
		{
			const FVector2D C(Ax, G::CourY1);
			const double R = 14.0, Gap = FMath::RadiansToDegrees(FMath::Asin((G::AvenueHalf + 1.0) / R));
			OpenArc(Sink, MuseeHedge::EKind::YewRun, MuseeHedge::EKind::YewEnd, C, R, 0.0, 90.0 - Gap, Key);
			OpenArc(Sink, MuseeHedge::EKind::YewRun, MuseeHedge::EKind::YewEnd, C, R, 90.0 + Gap, 180.0, Key);
		}
		for (const double Side : {-1.0, 1.0})
		{
			OpenLine(Sink, MuseeHedge::EKind::NarrowYewRun, MuseeHedge::EKind::NarrowYewEnd, FVector2D(11.0, Side * 14.0), FVector2D(41.0, Side * 14.0), Key);
		}
	}
	// ================================================================================================ the forecourt

	void BuildForecourt(FParts& P)
	{
		const double Ax = G::AxisX;
		const double Y0 = FP::SalonHalf + 1.56;   // the edge of the podium's gravel strip

		// The parvis before the steps, the allées' walks beside it.
		Slab(P.Paving, Ax - G::ParvisHalf, Ax + G::ParvisHalf, Y0, G::ParvisY1, kPave);
		Slab(P.Gravel, Ax - G::WalkOuter, Ax - G::WalkInner, Y0, G::ParvisY1, kGravel);
		Slab(P.Gravel, Ax + G::WalkInner, Ax + G::WalkOuter, Y0, G::ParvisY1, kGravel);

		// The cour: gravel round the canal and the two lawn panels.
		const double XB[] = {-G::WalkOuter, -G::PanelOuter, -G::PanelInner, -G::CanalHalf, G::CanalHalf, G::PanelInner, G::PanelOuter, G::WalkOuter};
		const double YB[] = {G::ParvisY1, G::CanalY0, kPanelY1, G::CanalY1, G::CourY1};
		for (int32 i = 0; i + 1 < UE_ARRAY_COUNT(XB); ++i)
		{
			for (int32 j = 0; j + 1 < UE_ARRAY_COUNT(YB); ++j)
			{
				const double MX = 0.5 * (XB[i] + XB[i + 1]), MY = 0.5 * (YB[j] + YB[j + 1]);
				const bool bCanal = FMath::Abs(MX) < G::CanalHalf && MY > G::CanalY0 && MY < G::CanalY1;
				const bool bPanel = FMath::Abs(MX) > G::PanelInner && FMath::Abs(MX) < G::PanelOuter && MY > kPanelY0 && MY < kPanelY1;
				if (bCanal || bPanel) { continue; }
				Slab(P.Gravel, Ax + XB[i], Ax + XB[i + 1], YB[j], YB[j + 1], kGravel);
			}
		}

		// The canal.
		{
			const double H = G::CanalHalf - G::CopingWidth;
			const double Ya = G::CanalY0 + G::CopingWidth, Yb = G::CanalY1 - G::CopingWidth;
			Basin(P, {FVector2D(Ax + H, Ya), FVector2D(Ax - H, Ya), FVector2D(Ax - H, Yb), FVector2D(Ax + H, Yb)});
		}

		// Kerbs: round the lawn panels, between the parvis and the gravel, and round the forecourt (open to the avenue
		// and to the walk east).
		for (const double Side : {-1.0, 1.0})
		{
			const double XI = Ax + Side * G::PanelInner, XO = Ax + Side * G::PanelOuter;
			Kerb(P, {FVector2D(XI, kPanelY0), FVector2D(XO, kPanelY0), FVector2D(XO, kPanelY1), FVector2D(XI, kPanelY1)}, true);
			Kerb(P, {FVector2D(Ax + Side * G::WalkInner, Y0), FVector2D(Ax + Side * G::WalkInner, G::ParvisY1)});
		}
		Kerb(P, {FVector2D(Ax - G::WalkOuter, G::ParvisY1), FVector2D(Ax + G::WalkOuter, G::ParvisY1)});
		Kerb(P, {FVector2D(Ax - G::WalkOuter, Y0), FVector2D(Ax - G::WalkOuter, G::CourY1), FVector2D(Ax - G::AvenueHalf, G::CourY1)});
		Kerb(P, {FVector2D(Ax + G::AvenueHalf, G::CourY1), FVector2D(Ax + G::WalkOuter, G::CourY1), FVector2D(Ax + G::WalkOuter, G::EastWalkY1)});
		Kerb(P, {FVector2D(Ax + G::WalkOuter, G::EastWalkY0), FVector2D(Ax + G::WalkOuter, Y0)});

		// (The hedges: BuildHedges.)

		// The avenue south.
		Slab(P.Gravel, Ax - G::AvenueHalf, Ax + G::AvenueHalf, G::CourY1, G::AvenueY1, kGravel + 0.001);
		for (const double Side : {-1.0, 1.0}) { Kerb(P, {FVector2D(Ax + Side * G::AvenueHalf, G::CourY1), FVector2D(Ax + Side * G::AvenueHalf, G::AvenueY1)}); }
	}

	// ================================================================================================ the walk east and the Élan's terrace

	void BuildEastWalk(FParts& P)
	{
		const double X0 = G::AxisX + G::WalkOuter;
		const FVector2D RP(G::EastWalkX1, G::RondPointY);
		const double RR = G::RondPointRadius, Half = 0.5 * (G::EastWalkY1 - G::EastWalkY0);
		const double WalkEnd = RP.X - FMath::Sqrt(RR * RR - Half * Half) + 0.3;
		const double PathHalf = 2.0;
		const double PathTop = RP.Y - FMath::Sqrt(RR * RR - PathHalf * PathHalf) + 0.3;
		const FVector2D EC(MuseePlan::Elan::CentreX, MuseePlan::Elan::CentreY);

		// The walk, up to the rond-point; the path from it north to the terrace. Albion: the walk bends round Albion's south
		// end (the Outside board's site plan): west of it at x -19.5, south of it at y 60, east of it at x 19.5, 4 m wide.
		const double BW = 19.5, BS = 60.0, BH = 2.0;
		Slab(P.Gravel, X0 - 0.01, -BW + BH, G::EastWalkY0, G::EastWalkY1, kGravel);
		Slab(P.Gravel, -BW - BH, -BW + BH, G::EastWalkY1, BS + BH, kGravel);
		Slab(P.Gravel, -BW + BH, BW - BH, BS - BH, BS + BH, kGravel);
		Slab(P.Gravel, BW - BH, BW + BH, G::EastWalkY1, BS + BH, kGravel);
		Slab(P.Gravel, BW - BH, WalkEnd, G::EastWalkY0, G::EastWalkY1, kGravel);
		Slab(P.Gravel, RP.X - PathHalf, RP.X + PathHalf, EC.Y + G::ElanTerraceOut - 0.3, PathTop, kGravel);
		// The rond-point: a gravel ring round a round basin.
		Annulus(P.Gravel, RP, G::RondBasinRadius - 0.1, RR, 0.0, 360.0, kGravel + 0.002);
		{
			TArray<FVector2D> Inner = ArcPoints(RP, G::RondBasinRadius - G::CopingWidth, 360.0, 0.0, 0.2);
			Inner.Pop();
			Basin(P, Inner);
		}
		// The terrace round the Élan's plinth.
		Annulus(P.Gravel, EC, G::ElanTerraceIn, G::ElanTerraceOut, G::ElanTerraceFromDegrees, G::ElanTerraceToDegrees, kGravel + 0.003);

		// Kerbs.
		Kerb(P, {FVector2D(X0, G::EastWalkY0), FVector2D(-BW + BH, G::EastWalkY0), FVector2D(-BW + BH, BS - BH), FVector2D(BW - BH, BS - BH),
				 FVector2D(BW - BH, G::EastWalkY0), FVector2D(WalkEnd - 0.25, G::EastWalkY0)});
		Kerb(P, {FVector2D(X0, G::EastWalkY1), FVector2D(-BW - BH, G::EastWalkY1), FVector2D(-BW - BH, BS + BH), FVector2D(BW + BH, BS + BH),
				 FVector2D(BW + BH, G::EastWalkY1), FVector2D(WalkEnd - 0.25, G::EastWalkY1)});
		for (const double Side : {-1.0, 1.0}) { Kerb(P, {FVector2D(RP.X + Side * PathHalf, PathTop - 0.25), FVector2D(RP.X + Side * PathHalf, EC.Y + G::ElanTerraceOut)}); }
		{
			// Round the rond-point, open to the walk (west) and the path (north).
			const double GapW = FMath::RadiansToDegrees(FMath::Asin(Half / RR)), GapN = FMath::RadiansToDegrees(FMath::Asin(PathHalf / RR));
			Kerb(P, ArcPoints(RP, RR, 180.0 + GapW, 270.0 - GapN, 0.3));
			Kerb(P, ArcPoints(RP, RR, 270.0 + GapN, 360.0 + 180.0 - GapW, 0.3));
		}
		{
			const double GapS = FMath::RadiansToDegrees(FMath::Asin(PathHalf / G::ElanTerraceOut));
			Kerb(P, ArcPoints(EC, G::ElanTerraceOut, G::ElanTerraceFromDegrees, 90.0 - GapS, 0.3));
			Kerb(P, ArcPoints(EC, G::ElanTerraceOut, 90.0 + GapS, G::ElanTerraceToDegrees, 0.3));
			for (const double Deg : {G::ElanTerraceFromDegrees, G::ElanTerraceToDegrees})
			{
				const FVector2D D(FMath::Cos(FMath::DegreesToRadians(Deg)), FMath::Sin(FMath::DegreesToRadians(Deg)));
				Kerb(P, {EC + D * (G::ElanTerraceIn + 0.05), EC + D * G::ElanTerraceOut});
			}
		}
	}

	FParts BuildAll()
	{
		FParts P;
		BuildForecourt(P);
		FHedgeSink Sink;
		Sink.Parts = &P;
		BuildHedges(Sink);
		BuildEastWalk(P);
		for (const FVector2D& At : LampPositions()) { Lamp(P, At); }
		return P;
	}
}

// ==================================================================================================== the actor

AMuseeLandscape::AMuseeLandscape()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* Name, bool bCollide)
	{
		UProceduralMeshComponent* M = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		M->SetupAttachment(RootComponent);
		M->bUseAsyncCooking = true;
		M->bUseComplexAsSimpleCollision = true;
		if (bCollide)
		{
			M->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			M->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return M;
	};
	Ground = Make(TEXT("Ground"), true);
	Water = Make(TEXT("Water"), false);
	Water->SetCastShadow(false);
	Hedges = Make(TEXT("Hedges"), true);
	Lamps = Make(TEXT("Lamps"), true);
	// The sun (channel 0) and the exterior's own lights (channel 1: the lamps, the facade's floodlights).
	for (UProceduralMeshComponent* C : {Ground.Get(), Water.Get(), Hedges.Get(), Lamps.Get()}) { C->SetLightingChannels(true, true, false); }
	for (int32 k = 0; k < GroundsBuild::LampPositions().Num(); ++k)
	{
		UPointLightComponent* L = CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("Lamp%02d"), k + 1));
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		LampLights.Add(L);
	}
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
	// Outdoors: EV100 → stops (instead of the museum's bias of 0.9 and its high-key curve, 1.7 stops at EV100 12.5).
	ExteriorExposureCurve = CreateDefaultSubobject<UCurveFloat>(TEXT("ExteriorExposureCurve"));
	for (const FVector2f Key : {FVector2f(6.f, 0.6f), FVector2f(11.5f, 0.7f), FVector2f(12.5f, 0.7f), FVector2f(13.5f, 0.55f), FVector2f(14.5f, 0.35f),
								FVector2f(15.5f, 0.15f), FVector2f(17.f, 0.f)})
	{
		ExteriorExposureCurve->FloatCurve.AddKey(Key.X, Key.Y);
	}
	ExteriorExposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("ExteriorExposure"));
	ExteriorExposure->SetupAttachment(RootComponent);
	ExteriorExposure->bUnbound = true;
	ExteriorExposure->Priority = 5.f;   // over the museum's volume (0), under the wings' zones (10)
	ExteriorExposure->BlendWeight = 0.f;
	ExteriorExposure->Settings.bOverride_AutoExposureBias = true;
	ExteriorExposure->Settings.AutoExposureBias = 0.f;
	ExteriorExposure->Settings.bOverride_AutoExposureBiasCurve = true;
	ExteriorExposure->Settings.AutoExposureBiasCurve = ExteriorExposureCurve;
	Parameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/MPC_Musee.MPC_Musee")));

	auto Soft = [](const TCHAR* Path) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path)); };
	// The exterior's weathered stone and the Paris gardens' pale gravel (Scripts/exterior_materials.py).
	PavingMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Paving.MI_Ext_Paving"));
	GravelMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Gravel.MI_Ext_Gravel"));
	KerbMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Kerb.MI_Ext_Kerb"));
	BasinMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_BasinBed.MI_Ext_BasinBed"));
	WaterMaterial = Soft(TEXT("/Game/Museum/Materials/MI_Water_Canal.MI_Water_Canal"));   // Single Layer Water (setup_project.make_water)
	HedgeCoreMaterial = Soft(MuseeHedge::CoreMaterialPath());
	LampMaterial = Soft(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_LampBronze.MI_Ext_LampBronze"));
	GlobeMaterial = Soft(TEXT("/Game/Museum/Materials/M_OpalGlobe.M_OpalGlobe"));

	AddTags();
}

void AMuseeLandscape::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
	PlaceLights();
	SetNight(0.f, 1.f);
	AddTags();
	UWorld* World = GetWorld();
	if (bPreviewHedgesInEditor && GIsEditor && !IsRunningCommandlet() && World && !World->IsGameWorld()) { PlantHedges(); }
}

void AMuseeLandscape::BeginPlay()
{
	Super::BeginPlay();
	Build();
	ApplyMaterials();
	PlaceLights();
	PlantHedges();
	LoadedParameters = Parameters.LoadSynchronous();
	LastDaylight = -1.f;
	float Daylight = 1.f;
	if (LoadedParameters) { Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, LoadedParameters, TEXT("Daylight")); }
	SetNight(1.f - FMath::SmoothStep(0.05f, 0.35f, Daylight), Daylight);
}

void AMuseeLandscape::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateExteriorExposure(DeltaSeconds);
	if (!LoadedParameters) { return; }
	const float Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, LoadedParameters, TEXT("Daylight"));
	if (FMath::Abs(Daylight - LastDaylight) < 0.005f) { return; }
	SetNight(1.f - FMath::SmoothStep(0.05f, 0.35f, Daylight), Daylight);
}

void AMuseeLandscape::UpdateExteriorExposure(float DeltaSeconds)
{
	if (!ExteriorExposure) { return; }
	float Target = 0.f;
	UWorld* World = GetWorld();
	APlayerCameraManager* Camera = World ? UGameplayStatics::GetPlayerCameraManager(this, 0) : nullptr;
	if (bExteriorExposure && Camera)
	{
		const FVector At = Camera->GetCameraLocation();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(MuseeOutdoors), false);
		Params.AddIgnoredActor(Camera->GetViewTarget());
		FHitResult Down;
		const bool bOnGrounds = World->LineTraceSingleByChannel(Down, At, At - FVector(0.0, 0.0, 1000.0), ECC_Visibility, Params) && Down.GetActor()
								&& (Down.GetActor()->IsA<AMuseeLandscape>() || Down.GetActor()->IsA<AMuseeGround>());
		bool bRoof = false;
		if (bOnGrounds)
		{
			// Open sky above: the trees' boughs aren't a roof.
			for (int32 Try = 0; Try < 4; ++Try)
			{
				FHitResult Up;
				if (!World->LineTraceSingleByChannel(Up, At, At + FVector(0.0, 0.0, 6000.0), ECC_Visibility, Params)) { break; }
				if (Up.GetActor() && Up.GetActor()->IsA<AMuseeNatureActor>())
				{
					Params.AddIgnoredActor(Up.GetActor());
					continue;
				}
				bRoof = true;
				break;
			}
		}
		Target = bOnGrounds && !bRoof ? 1.f : 0.f;
	}
	OutdoorWeight = FMath::FInterpConstantTo(OutdoorWeight, Target, DeltaSeconds, 1.5f);
	ExteriorExposure->BlendWeight = OutdoorWeight;
}

void AMuseeLandscape::PlaceLights()
{
	const TArray<FVector2D> At = GroundsBuild::LampPositions();
	for (int32 k = 0; k < LampLights.Num() && k < At.Num(); ++k)
	{
		UPointLightComponent* L = LampLights[k];
		if (!L) { continue; }
		L->SetRelativeLocation(FVector(At[k].X, At[k].Y, GroundsBuild::kGlobeZ) * MuseePlan::Cm);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetUseTemperature(true);
		L->SetTemperature(LampKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetAttenuationRadius(1600.f);
		L->SetSourceRadius(GroundsBuild::kGlobeR * MuseePlan::Cm);
		L->SetCastShadows(false);   // (the globe would shade itself)
		L->SetLightingChannels(false, true, false);
	}
}

void AMuseeLandscape::SetNight(float Level, float Daylight)
{
	const float N = bNightLights ? FMath::Clamp(Level, 0.f, 1.f) : 0.f;
	LastDaylight = Daylight;
	for (UPointLightComponent* L : LampLights)
	{
		if (!L) { continue; }
		L->SetIntensity(LampCandela * N);
		L->SetVisibility(N > 0.001f);
	}
}

void AMuseeLandscape::AddTags()
{
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Exterior")));
	Tags.AddUnique(FName(TEXT("musee.exterior")));
	Tags.AddUnique(MuseeBake::BakeableTag());
}

TArray<FString> AMuseeLandscape::GetReplacedImportPrims()
{
	return {};
}

void AMuseeLandscape::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	const GroundsBuild::FParts P = GroundsBuild::BuildAll();
	for (UProceduralMeshComponent* C : {Ground.Get(), Water.Get(), Hedges.Get()}) { C->ClearAllMeshSections(); }
	P.Paving.Write(Ground, 0, true);
	P.Gravel.Write(Ground, 1, true);
	P.Stone.Write(Ground, 2, true);
	P.Basin.Write(Ground, 3, true);
	P.Water.Write(Water, 0, false);
	P.Core.Write(Hedges, 0, true);
	Lamps->ClearAllMeshSections();
	P.Posts.Write(Lamps, 0, true);
	P.Globes.Write(Lamps, 1, false);
	TriangleCount = (P.Paving.Indices.Num() + P.Gravel.Indices.Num() + P.Stone.Indices.Num() + P.Basin.Indices.Num() + P.Water.Indices.Num()
					 + P.Core.Indices.Num()) / 3;
	UE_LOG(LogMusee, Log, TEXT("Grounds: %d triangles (the hedges' leaves are instanced: PlantHedges)."), TriangleCount);
}

void AMuseeLandscape::ApplyMaterials()
{
	auto Load = [](std::initializer_list<TSoftObjectPtr<UMaterialInterface>> Chain) -> UMaterialInterface*
	{
		for (const TSoftObjectPtr<UMaterialInterface>& Ref : Chain)
		{
			if (!Ref.IsNull())
			{
				if (UMaterialInterface* M = Ref.LoadSynchronous()) { return M; }
			}
		}
		return nullptr;
	};
	auto Path = [](const TCHAR* P) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(P)); };
	auto Set = [](UProceduralMeshComponent* C, int32 Section, UMaterialInterface* M)
	{
		if (C && M && Section < C->GetNumSections()) { C->SetMaterial(Section, M); }
	};
	const TSoftObjectPtr<UMaterialInterface> Honed = Path(TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed"));
	Set(Ground, 0, Load({PavingMaterial, Honed}));
	Set(Ground, 1, Load({GravelMaterial, Path(TEXT("/Game/Museum/ChineseWing/ChineseWing/Materials/gravel.gravel")), Honed}));
	Set(Ground, 2, Load({KerbMaterial, Honed}));
	Set(Ground, 3, Load({BasinMaterial, Path(TEXT("/Game/Museum/Materials/USD/MI_stone_grey.MI_stone_grey")), Honed}));
	Set(Water, 0, Load({WaterMaterial, Path(TEXT("/Game/Museum/ChineseWing/ChineseWing/Materials/water_garden.water_garden"))}));
	Set(Hedges, 0, Load({HedgeCoreMaterial, Path(TEXT("/Game/Museum/Nature/MI_Soil.MI_Soil"))}));
	Set(Lamps, 0, Load({LampMaterial, Path(TEXT("/Game/Museum/Materials/USD/MI_bronze.MI_bronze"))}));
	Set(Lamps, 1, Load({GlobeMaterial, Path(TEXT("/Game/Museum/Materials/M_Glass.M_Glass"))}));
}

// ---------------------------------------------------------------------------------------------------- the hedges' leaves

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GHedgesCommand(
		TEXT("musee.Hedges"), TEXT("musee.Hedges 0|1: hide or show the grounds' clipped hedges' leaves (their dark cores stay)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World || Args.Num() == 0) { return; }
			for (TActorIterator<AMuseeLandscape> It(World); It; ++It) { It->SetHedgesShown(FCString::Atoi(*Args[0]) != 0); }
		}));
}

void AMuseeLandscape::ClearHedges()
{
	for (UInstancedStaticMeshComponent* C : HedgeLeaves)
	{
		if (IsValid(C)) { C->DestroyComponent(); }
	}
	HedgeLeaves.Reset();
}

void AMuseeLandscape::PlantHedges()
{
	ClearHedges();
	TArray<GroundsBuild::FFoliage> All;
	GroundsBuild::FHedgeSink Sink;
	Sink.Foliage = &All;
	GroundsBuild::BuildHedges(Sink);
	const int32 Num = MuseeHedge::NumMeshes();
	TArray<TArray<FTransform>> Per;
	Per.SetNum(Num);
	for (const GroundsBuild::FFoliage& F : All)
	{
		if (Per.IsValidIndex(F.Mesh)) { Per[F.Mesh].Add(F.Transform); }
	}
	int32 Total = 0, Missing = 0;
	for (int32 m = 0; m < Num; ++m)
	{
		if (Per[m].Num() == 0) { continue; }
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MuseeHedge::MeshPath(m), nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Mesh)
		{
			++Missing;
			continue;
		}
		const FName Name = MakeUniqueObjectName(this, UInstancedStaticMeshComponent::StaticClass(), FName(*(TEXT("HedgeLeaves_") + MuseeHedge::MeshName(m))));
		UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this, Name, RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
		ISM->SetupAttachment(RootComponent);
		ISM->SetMobility(EComponentMobility::Static);
		ISM->SetStaticMesh(Mesh);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // the cores collide
		ISM->SetCanEverAffectNavigation(false);
		ISM->SetGenerateOverlapEvents(false);
		// Real leaves in the ray-traced scene: the sun's shadows fall through the shell (its depth), and onto the lawn.
		ISM->SetCastShadow(true);
		ISM->bVisibleInRayTracing = true;
		ISM->bAffectDistanceFieldLighting = false;
		ISM->bVisibleInReflectionCaptures = false;
		ISM->SetReceivesDecals(false);
		ISM->SetLightingChannels(true, true, false);
		ISM->ComponentTags.Add(MuseeBake::NoBakeTag());
		ISM->RegisterComponent();
		ISM->AddInstances(Per[m], false, false, false);
		HedgeLeaves.Add(ISM);
		Total += Per[m].Num();
	}
	if (Missing > 0) { UE_LOG(LogMusee, Warning, TEXT("Grounds: %d hedge module meshes are missing (run Scripts/apply_all.py lawn:hedges)."), Missing); }
	UE_LOG(LogMusee, Log, TEXT("Grounds: %d hedge modules planted in %d components."), Total, HedgeLeaves.Num());
}

void AMuseeLandscape::SetHedgesShown(bool bShow)
{
	for (UInstancedStaticMeshComponent* C : HedgeLeaves)
	{
		if (IsValid(C)) { C->SetVisibility(bShow); }
	}
}

void AMuseeLandscape::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearHedges();
	Super::EndPlay(Reason);
}
// ---------------------------------------------------------------------------------------------------- the trees

TArray<FMuseeGroundsTree> AMuseeLandscape::GetTreePlacements()
{
	namespace G = MuseePlan::Grounds;
	TArray<FMuseeGroundsTree> Out;
	int32 Seed = 700;
	auto Add = [&Out, &Seed](const FString& Name, double X, double Y, EMuseeTreeSpecies Species, float Height, float Crown, FVector2D Radii, float Density)
	{
		FMuseeGroundsTree T;
		T.Name = Name;
		T.Position = FVector2D(X, Y);
		T.Species = Species;
		T.TreeHeight = Height;
		T.CrownHeight = Crown;
		T.CrownRadii = Radii;
		T.LeafDensity = Density;
		T.Seed = Seed++;
		Out.Add(T);
	};
	const double Ax = G::AxisX;
	const double WalkMid = 0.5 * (G::WalkInner + G::WalkOuter);
	// The parterre's allées: round-headed osmanthus in the walks either side of the lawn panels, every 6 m.
	for (int32 i = 0; i < 6; ++i)
	{
		for (const double Side : {-1.0, 1.0})
		{
			Add(FString::Printf(TEXT("Grounds_Allee_%s_%02d"), Side < 0 ? TEXT("W") : TEXT("E"), i + 1), Ax + Side * WalkMid, 26.0 + 6.0 * i,
				EMuseeTreeSpecies::Osmanthus, 6.4f, 4.3f, FVector2D(2.1, 1.9), 1.4f);
		}
	}
	// The avenue: birches every 12 m beyond the exedra.
	for (int32 i = 0; i < 4; ++i)
	{
		for (const double Side : {-1.0, 1.0})
		{
			Add(FString::Printf(TEXT("Grounds_Avenue_%s_%02d"), Side < 0 ? TEXT("W") : TEXT("E"), i + 1), Ax + Side * (G::AvenueHalf + 2.5), 80.0 + 12.0 * i,
				EMuseeTreeSpecies::Birch, 11.5f, 7.6f, FVector2D(2.3, 3.2), 0.8f);
		}
	}
	// The walk east: birches either side, every 15 m.
	for (int32 i = 0; i < 4; ++i)
	{
		for (const double Side : {-1.0, 1.0})
		{
			// Albion: where the straight walk ran through its court the birches stand along the bend's south leg instead.
			const double X = -12.0 + 15.0 * i;
			const bool bInAlbion = X > -23.0 && X < 23.0;
			const double Y = bInAlbion ? (Side < 0 ? 57.2 - 1.2 : 62.0 + 1.8) : (Side < 0 ? G::EastWalkY0 - 1.8 : G::EastWalkY1 + 1.8);
			if (bInAlbion && Side < 0) { continue; }   // no room between the bend and Albion's buttresses
			Add(FString::Printf(TEXT("Grounds_Walk_%s_%02d"), Side < 0 ? TEXT("N") : TEXT("S"), i + 1), X,  Y,
				EMuseeTreeSpecies::Birch, 10.5f, 7.0f, FVector2D(2.2, 3.0), 0.8f);
		}
	}
	// Groves round the site, well clear of the walls: birches, and dark evergreen osmanthus among them.
	struct FGrove
	{
		double X, Y;
		bool bEvergreen;
	};
	const FGrove Groves[] = {
		{-40, -44, false}, {-33, -49, false}, {-47, -51, true},   // north of the Salon, over its roofline
		{-112, -8, false}, {-115, 7, false},                      // beyond the oval, on the axis's ends
		{-92, 44, false}, {-98, 56, true},                        // west of the parterre
		{88, -18, false}, {92, 10, true},                         // east of the Élan
		{30, 66, false},                                          // south of the walk east
		{-20.5, 18, true}, {-20.5, 30.5, true},                   // between the parterre and Albion (Albion: back from its buttresses)
	};
	// Bamboo against the Chinese Wing's white outer walls (the wall the paper, the plants the painting), in clumps along
	// its west, east and south faces, clear of the orchard's hedge (y 14.2). Each clump stands 1.6 m out (every culm's
	// foot 1.05 m and more from the wall) and keeps its culms, branches and leaves 0.15 m short of the wall's outer face
	// (AMuseeTree::Keep): leaning and branching bamboo would otherwise come through the wall into the court.
	{
		const double X = MuseePlan::ChineseWing::X1 + MuseePlan::ChineseWing::Wall, YS = MuseePlan::ChineseWing::Y1 + MuseePlan::ChineseWing::Wall;
		constexpr double Out1 = 1.6, Margin = 0.15, Far = 50.0;
		struct FClump
		{
			double X, Y;
			bool bAlongY;   // the clump runs along the wall (y) or across it
			int32 Face;     // the wall it stands against: 0 west, 1 east, 2 south
		};
		const FClump Clumps[] = {{-X - Out1, 17.5, true, 0}, {-X - Out1, 24.0, true, 0}, {-X - Out1, 30.5, true, 0},
								 {X + Out1, 19.0, true, 1}, {X + Out1, 30.0, true, 1}, {-5.5, YS + Out1, false, 2}};
		FRandomStream Rng(4242);
		for (int32 i = 0; i < 0 * int32(UE_ARRAY_COUNT(Clumps)); ++i)   // Albion: the Chinese Wing's bamboo went with it
		{
			const FClump& C = Clumps[i];
			TArray<FVector2D> Culms;
			for (int32 k = 0; k < 18; ++k)
			{
				// Denser at the clump's heart: a random point in an ellipse 3.2 × 1.1 m along the wall.
				const double A = Rng.FRandRange(0.0, 2.0 * UE_DOUBLE_PI), R = FMath::Sqrt(Rng.FRand());
				const double Along = 1.6 * R * FMath::Cos(A), Across = 0.55 * R * FMath::Sin(A);
				Culms.Add(C.bAlongY ? FVector2D(Across, Along) : FVector2D(Along, Across));
			}
			Add(FString::Printf(TEXT("Grounds_Bamboo_%02d"), i + 1), C.X, C.Y, EMuseeTreeSpecies::Bamboo, 5.6f, 0.0f, FVector2D::ZeroVector, 1.9f);   // a full screen of leaves (0.7 left the culms bare)
			Out.Last().Culms = Culms;
			// The keep box, relative to the clump: everything on the far side of the plane 0.15 m out from the outer face.
			FBox2D Keep(FVector2D(-Far, -Far), FVector2D(Far, Far));
			if (C.Face == 0) { Keep.Max.X = -(X + Margin) - C.X; }
			else if (C.Face == 1) { Keep.Min.X = (X + Margin) - C.X; }
			else { Keep.Min.Y = (YS + Margin) - C.Y; }
			Out.Last().Keep = Keep;
		}
	}
	for (int32 i = 0; i < UE_ARRAY_COUNT(Groves); ++i)
	{
		const FGrove& Gv = Groves[i];
		const FString Name = FString::Printf(TEXT("Grounds_Grove_%02d"), i + 1);
		if (Gv.bEvergreen) { Add(Name, Gv.X, Gv.Y, EMuseeTreeSpecies::Osmanthus, 7.0f, 4.4f, FVector2D(2.4, 2.1), 1.4f); }
		else { Add(Name, Gv.X, Gv.Y, EMuseeTreeSpecies::Birch, 12.0f, 8.0f, FVector2D(2.6, 3.4), 0.8f); }
	}
	return Out;
}
