#include "Reserve/ReserveRacks.h"

#include "Catalog/MuseeCatalog.h"
#include "Components/CapsuleComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Furniture/FurnitureKit.h"
#include "Geometry/MuseeBake.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInterface.h"
#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"
#include "Visitor/MuseeCharacter.h"
#include "Visitor/MuseeWorld.h"

#define LOCTEXT_NAMESPACE "ReserveRacks"

/**
 * The picture screens' geometry, in metres. A screen is built about its floor centre in its carriage's frame: x across
 * it (its faces at ±x), y along it (its nave end towards S · y: S = +1 for the north screens), z up from the floor.
 * The tracks are built in the plan's own frame (the actor stands at the origin), heights above the Reserve's floor.
 */
namespace ReserveRackBuild
{
	namespace R = MuseePlan::Reserve;
	namespace FK = FurnitureKit;
	using FK::FMeshData;

	constexpr double L = R::RackLength, H = R::RackHeight, Z0 = R::ScreenBottom;
	constexpr double Half = R::ScreenHalf, PostHalf = R::PostHalf;
	constexpr double PostLength = 0.09;              // the end posts along the screen
	constexpr double TopRail = 0.11, KickRail = 0.15;
	constexpr double LinenFace = 0.043;              // the linen 7 mm behind the oak's faces
	constexpr double Ease = 0.005;                   // the oak's eased arrises
	/** The picture rail under the top rail on each face, and the hanging rods (in front of it, behind the frames). */
	constexpr double RailZ0 = H - TopRail - 0.035, RailZ1 = H - TopRail - 0.004, RailOut = 0.056;
	constexpr double RodX = 0.0585, RodRadius = 0.0022;
	/** The trolleys' hangers (at ±HangerY along the screen) and the guide fins under it (±FinY), which run in the floor's slot. */
	constexpr double HangerY = 2.1, FinY = 2.2;
	constexpr double TZ0 = R::TrackZ0, TZ1 = R::TrackZ1, TrackHalf = 0.03, SlotHalf = 0.01;
	/** The picture light over a face with works: its trough's centre out from the screen's plane and its height. */
	constexpr double LampOut = 0.42, LampZ = 3.30;
	/** The pull on the nave end post, and the letter plate over it (the plate's centre, and its width once scaled). */
	constexpr double PullZ0 = 0.95, PullZ1 = 1.60;
	constexpr double PlateZ = 1.86, PlateWidth = 0.12;
	/** The tracks: from the side wall's face to near the axis; hung from the vault at these |y|, bolted to the wall. */
	constexpr double TrackWallY = R::HalfWidth - 0.002, TrackEndY = 0.90;
	constexpr double RodYs[3] = {7.6, 5.0, 1.2};
	constexpr float CarrySeconds = 5.f;
	const FString Base = TEXT("/Museum/Reserve/The_Reserve/");

	/** A member: a rounded-rectangle bar from A to B, half sizes HA, HB across it (FurnitureKit's frame: B is Up squared to the path, A = B × T), U along it. */
	void Member(FMeshData& M, const FVector& A, const FVector& B, double HA, double HB, const FVector& Up, double Radius = Ease)
	{
		FK::Bar(M, FK::FPath::Line(A, B, Up), FK::RectSection(0, 0, HA, HB, Radius, 3, 1));
	}

	/** A round rod from A to B. */
	void Rod(FMeshData& M, const FVector& A, const FVector& B, double Radius, int32 Sides = 12)
	{
		const FVector T = (B - A).GetSafeNormal();
		const FVector Up = FMath::Abs(T.Z) > 0.9 ? FVector(1, 0, 0) : FVector(0, 0, 1);
		FK::Bar(M, FK::FPath::Line(A, B, Up), [Radius, Sides](double, double) { return FK::Circle(0, 0, Radius, Sides); });
	}

	/** A closed box (metres), UVs in metres on each face. */
	void Box(FMeshData& M, double X0, double X1, double Y0, double Y1, double Z0_, double Z1_)
	{
		M.Box(FVector(FMath::Min(X0, X1), FMath::Min(Y0, Y1), Z0_), FVector(FMath::Max(X0, X1), FMath::Max(Y0, Y1), Z1_), FMeshData::AllFaces);
	}

	/** The linen on one face (F = ±1): a flat field in cells, UV0 = (y, z) as the fabric's own projection would lay it. */
	void LinenField(FMeshData& M, double F, double Y0, double Y1, double ZLo, double ZHi)
	{
		const int32 NY = 10, NZ = 7;
		const FVector N(F, 0, 0);
		const int32 First = M.Positions.Num();
		for (int32 i = 0; i <= NY; ++i)
		{
			for (int32 j = 0; j <= NZ; ++j)
			{
				const double Y = FMath::Lerp(Y0, Y1, double(i) / NY), Z = FMath::Lerp(ZLo, ZHi, double(j) / NZ);
				M.Vertex(FVector(F * LinenFace, Y, Z), N, FVector2D(Y, Z));
			}
		}
		for (int32 i = 0; i < NY; ++i)
		{
			for (int32 j = 0; j < NZ; ++j)
			{
				const int32 A = First + i * (NZ + 1) + j;
				M.Quad(A, A + NZ + 1, A + NZ + 2, A + 1);
			}
		}
	}

	struct FFaceWorks { double Y0 = 1e9, Y1 = -1e9; int32 Count = 0; };

	/** Where a work hangs on its lettered screen (m, in the carriage's frame): its face's works in a row centred on the screen. */
	FVector WorkSlot(const FString& RackLetter, const FString& Id)
	{
		bool bEast = false;
		for (const R::FRackWork& W : R::RackWorks)
		{
			if (Id == W.Id) { bEast = W.bEast; }
		}
		double Total = 0;
		int32 N = 0;
		for (const R::FRackWork& W : R::RackWorks)
		{
			if (RackLetter == W.Letter && W.bEast == bEast) { Total += W.W + 2 * R::WorkFrame; ++N; }
		}
		Total += FMath::Max(0, N - 1) * R::WorkGap;
		double Y = -Total / 2;
		const double X = bEast ? R::WorkOff : -R::WorkOff;
		for (const R::FRackWork& W : R::RackWorks)
		{
			if (RackLetter != W.Letter || W.bEast != bEast) { continue; }
			const double Width = W.W + 2 * R::WorkFrame;
			if (Id == W.Id) { return FVector(X, Y + Width / 2, R::WorkHeight); }
			Y += Width + R::WorkGap;
		}
		return FVector(X, 0, R::WorkHeight);
	}

	FString Letter(int32 K) { return K < R::LetteredRacks ? FString::Chr(TEXT('A') + K) : FString(); }

	/** The works on screen K's face F (−1 west, +1 east): their extent along the screen. */
	FFaceWorks FaceWorks(int32 K, double F)
	{
		FFaceWorks Out;
		const FString Let = Letter(K);
		if (Let.IsEmpty()) { return Out; }
		for (const R::FRackWork& W : R::RackWorks)
		{
			if (Let != W.Letter || (W.bEast ? 1.0 : -1.0) != F) { continue; }
			const FVector At = WorkSlot(Let, W.Id);
			const double Hw = W.W / 2 + R::WorkFrame;
			Out.Y0 = FMath::Min(Out.Y0, At.Y - Hw);
			Out.Y1 = FMath::Max(Out.Y1, At.Y + Hw);
			++Out.Count;
		}
		return Out;
	}

	/** The picture lights: one per face that carries works, (screen, face) in the order they are made. */
	TArray<TPair<int32, double>> LitFaces()
	{
		TArray<TPair<int32, double>> Out;
		for (int32 K = 0; K < R::LetteredRacks; ++K)
		{
			for (const double F : {-1.0, 1.0})
			{
				if (FaceWorks(K, F).Count > 0) { Out.Add({K, F}); }
			}
		}
		return Out;
	}

	/** A picture light's trough along the face (y0 … y1 in the carriage's frame). */
	FVector2D LampSpan(int32 K, double F)
	{
		const FFaceWorks W = FaceWorks(K, F);
		const double C = (W.Y0 + W.Y1) / 2, HalfLen = FMath::Clamp((W.Y1 - W.Y0) / 2 + 0.10, 0.35, 1.9);
		return FVector2D(C - HalfLen, C + HalfLen);
	}

	/** One screen about its floor centre: S = +1 when its nave end is towards +y (the north screens), −1 for the south ones. */
	void BuildScreen(FMeshData& Oak, FMeshData& Linen, FMeshData& Bronze, FMeshData& Patina, int32 K, double S)
	{
		const FVector Up(0, 0, 1), Across(1, 0, 0);
		const double L2 = L / 2, Inner = L2 - PostLength;
		// The frame: two end posts (the grain up them), the top rail and the kick rail between them (into the posts 1 cm).
		for (const double E : {-1.0, 1.0})
		{
			const double Y = E * (L2 - PostLength / 2);
			Member(Oak, FVector(0, Y, Z0), FVector(0, Y, H), PostLength / 2, PostHalf, Across, 0.008);
		}
		Member(Oak, FVector(0, -Inner - 0.01, H - TopRail / 2), FVector(0, Inner + 0.01, H - TopRail / 2), Half, TopRail / 2, Up);
		Member(Oak, FVector(0, -Inner - 0.01, Z0 + KickRail / 2), FVector(0, Inner + 0.01, Z0 + KickRail / 2), Half, KickRail / 2, Up);
		// The linen: the board inside (closed, behind the frame) and its two faces.
		const double LZ0 = Z0 + KickRail - 0.004, LZ1 = H - TopRail + 0.004;
		Box(Oak, -LinenFace + 0.002, LinenFace - 0.002, -Inner - 0.002, Inner + 0.002, LZ0, LZ1);
		for (const double F : {-1.0, 1.0}) { LinenField(Linen, F, -Inner - 0.002, Inner + 0.002, LZ0, LZ1); }

		// The picture rail on each face under the top rail, its ends returned into the posts.
		for (const double F : {-1.0, 1.0})
		{
			const double Hx = (RailOut - LinenFace) / 2 + 0.002;
			Member(Bronze, FVector(F * (LinenFace + RailOut) / 2, -Inner - 0.004, (RailZ0 + RailZ1) / 2),
				   FVector(F * (LinenFace + RailOut) / 2, Inner + 0.004, (RailZ0 + RailZ1) / 2), Hx, (RailZ1 - RailZ0) / 2, Up, 0.003);
		}

		// The works' rods and hooks (screens A–E): two rods per work, from a hook on the rail's lip down behind its frame.
		const FString Let = Letter(K);
		for (const R::FRackWork& W : R::RackWorks)
		{
			if (Let.IsEmpty() || Let != W.Letter) { continue; }
			const double F = W.bEast ? 1.0 : -1.0;
			const FVector At = WorkSlot(Let, W.Id);
			const double FrameTop = At.Z + W.H / 2 + R::WorkFrame;
			const double Spread = W.W / 2 + R::WorkFrame - 0.07;
			for (const double E : {-1.0, 1.0})
			{
				const double Y = At.Y + E * Spread;
				// The rod, its top bent into a crook over the rail (up in front of it, back over its top, down behind its lip).
				const double Crook = RailZ1 + 0.009, Back = F * (LinenFace + 0.005);
				Rod(Patina, FVector(F * RodX, Y, FrameTop - 0.09), FVector(F * RodX, Y, Crook), RodRadius, 8);
				Rod(Patina, FVector(F * RodX, Y, Crook), FVector(Back, Y, Crook), RodRadius, 8);
				Rod(Patina, FVector(Back, Y, Crook), FVector(Back, Y, RailZ1 - 0.006), RodRadius, 8);
				// The rod's grip, a short sleeve where it meets the frame's hanger (behind the frame).
				Rod(Patina, FVector(F * RodX, Y, FrameTop - 0.12), FVector(F * RodX, Y, FrameTop - 0.075), RodRadius * 2.2, 8);
			}
		}

		// The picture lights over the faces with works: a bronze trough on two arms from the top rail.
		for (const double F : {-1.0, 1.0})
		{
			if (FaceWorks(K, F).Count == 0) { continue; }
			const FVector2D Span = LampSpan(K, F);
			const double X = F * LampOut;
			Member(Bronze, FVector(X, Span.X, LampZ + 0.02), FVector(X, Span.Y, LampZ + 0.02), 0.032, 0.022, Up, 0.012);
			for (const double Y : {Span.X + 0.12, Span.Y - 0.12})
			{
				Rod(Bronze, FVector(F * (Half - 0.004), Y, H - TopRail / 2), FVector(X - F * 0.02, Y, LampZ + 0.03), 0.007, 10);
				Box(Bronze, F * (Half - 0.002), F * (Half + 0.006), Y - 0.025, Y + 0.025, H - TopRail / 2 - 0.03, H - TopRail / 2 + 0.03);
			}
		}

		// The pull on the nave end post: a bar on two stand-offs.
		const double EndY = S * L2;
		for (const double Z : {PullZ0 + 0.05, PullZ1 - 0.05})
		{
			Rod(Bronze, FVector(0, EndY - S * 0.004, Z), FVector(0, EndY + S * 0.05, Z), 0.007, 10);
		}
		Rod(Bronze, FVector(0, EndY + S * 0.058, PullZ0), FVector(0, EndY + S * 0.058, PullZ1), 0.011, 14);

		// The trolleys' hangers: a saddle on the top rail, a stem up into the track's slot, a nut.
		for (const double E : {-1.0, 1.0})
		{
			const double Y = E * HangerY;
			Box(Bronze, -0.036, 0.036, Y - 0.07, Y + 0.07, H - 0.002, H + 0.012);
			Rod(Bronze, FVector(0, Y, H + 0.01), FVector(0, Y, TZ0 + 0.03), 0.008, 10);
			Rod(Bronze, FVector(0, Y, H + 0.012), FVector(0, Y, H + 0.028), 0.016, 6);
		}
		// The guide fins under the kick rail, into the floor's slot.
		for (const double E : {-1.0, 1.0})
		{
			const double Y = E * FinY;
			Box(Bronze, -0.022, 0.022, Y - 0.07, Y + 0.07, Z0 - 0.02, Z0 + 0.004);
			Box(Bronze, -0.0045, 0.0045, Y - 0.05, Y + 0.05, -0.03, Z0 - 0.018);
		}
	}

	/** The lens under a picture light's trough (lit opal): a strip facing down. */
	void BuildLens(FMeshData& M, int32 K, double F)
	{
		const FVector2D Span = LampSpan(K, F);
		const double X = F * LampOut, Z = LampZ - 0.0025;
		const double W = 0.011, Xc = X - F * 0.008;   // a narrow slit, set towards the screen (the lamp faces it)
		M.Rect(FVector(Xc - W, Span.X + 0.03, Z), FVector(Xc + W, Span.X + 0.03, Z), FVector(Xc + W, Span.Y - 0.03, Z),
			   FVector(Xc - W, Span.Y - 0.03, Z), FVector(0, 0, -1));
	}

	/** The vault's underside over plan (x, y), height above the floor (as AReserveStructure builds it: max of the two arches + the webs' step; the arcade bands follow the aisles' arch). */
	double VaultZ(double X, double Y)
	{
		double Bx0 = R::X0, Bx1 = R::X1;
		for (int32 P = 0; P < R::PierLines; ++P)
		{
			const double Px = R::PierX(P);
			if (X < Px) { Bx1 = FMath::Min(Bx1, Px - R::PierHalf); break; }
			Bx0 = Px + R::PierHalf;
		}
		auto Arch = [](double V, double Lo, double Hi, double Rise)
		{
			const double C = (Lo + Hi) / 2, Hw = (Hi - Lo) / 2;
			const double T = FMath::Clamp((V - C) / Hw, -1.0, 1.0);
			return R::Spring + Rise * FMath::Sqrt(FMath::Max(0.0, 1.0 - T * T));
		};
		const double AY = FMath::Abs(Y);
		const double Band = R::PierY + R::PierHalf, BandIn = R::PierY - R::PierHalf;
		if (AY >= BandIn && AY <= Band) { return Arch(X, Bx0, Bx1, R::AisleRise); }
		const double Lo = AY < BandIn ? -BandIn : Band, Hi = AY < BandIn ? BandIn : R::HalfWidth;
		const double Rise = AY < BandIn ? R::NaveRise : R::AisleRise;
		return FMath::Max(Arch(AY < BandIn ? Y : AY, Lo, Hi, Rise), Arch(X, Bx0, Bx1, Rise)) + 0.06;
	}

	/** The tracks overhead, in the plan's frame (heights above the floor): one per screen, from the side wall to near the axis. */
	void BuildTracks(FMeshData& Bronze, FMeshData& Patina)
	{
		const FVector Up(0, 0, 1);
		for (int32 K = 0; K < R::RackCount; ++K)
		{
			const R::FRackSlot Slot = R::Rack(K);
			const double X = Slot.X, Sg = Slot.bNorth ? -1.0 : 1.0;   // the aisle's side of the axis
			const double YW = Sg * TrackWallY, YE = Sg * TrackEndY;
			const double Ya = FMath::Min(YW, YE), Yb = FMath::Max(YW, YE);
			// The track: a bronze channel, open underneath in a slot the trolleys' stems run in.
			Member(Bronze, FVector(X, Ya, TZ1 - 0.006), FVector(X, Yb, TZ1 - 0.006), TrackHalf, 0.006, Up, 0.002);
			for (const double E : {-1.0, 1.0})
			{
				Member(Bronze, FVector(X + E * (TrackHalf - 0.004), Ya, (TZ0 + TZ1) / 2), FVector(X + E * (TrackHalf - 0.004), Yb, (TZ0 + TZ1) / 2),
					   0.004, (TZ1 - TZ0) / 2 - 0.0005, Up, 0.0015);
				const double Lc = E * (SlotHalf + TrackHalf) / 2, Lh = (TrackHalf - SlotHalf) / 2;
				Member(Bronze, FVector(X + Lc, Ya, TZ0 + 0.005), FVector(X + Lc, Yb, TZ0 + 0.005), Lh, 0.005, Up, 0.0015);
			}
			// The nave end's stop, and the wall plate (bolted into the brick).
			Box(Bronze, X - TrackHalf - 0.004, X + TrackHalf + 0.004, YE - Sg * 0.012, YE, TZ0 - 0.006, TZ1 + 0.004);
			Box(Bronze, X - 0.075, X + 0.075, Sg * (TrackWallY - 0.012), Sg * (R::HalfWidth + 0.03), TZ0 - 0.07, TZ1 + 0.06);
			for (const double Bx : {-0.05, 0.05})
			{
				for (const double Bz : {TZ0 - 0.045, TZ1 + 0.035})
				{
					Rod(Bronze, FVector(X + Bx, Sg * (TrackWallY - 0.010), Bz), FVector(X + Bx, Sg * (TrackWallY - 0.022), Bz), 0.011, 6);
				}
			}
			// The hanger rods up to the vault: a clamp on the track, the rod, a canopy against the brick.
			for (const double RY : RodYs)
			{
				const double Y = Sg * RY;
				const double Top = VaultZ(X, Y);
				Box(Bronze, X - TrackHalf - 0.006, X + TrackHalf + 0.006, Y - 0.03, Y + 0.03, TZ1 - 0.03, TZ1 + 0.012);
				Rod(Patina, FVector(X, Y, TZ1 + 0.01), FVector(X, Y, Top + 0.02), 0.007, 10);
				Rod(Patina, FVector(X, Y, TZ1 + 0.012), FVector(X, Y, TZ1 + 0.04), 0.014, 6);
				Rod(Patina, FVector(X, Y, Top - 0.02), FVector(X, Y, Top + 0.03), 0.038, 16);
			}
		}
	}

	/** Rack K's home, the plan (x, y) of its floor centre, and the way it glides out (+1 towards +y). */
	FVector2D Home(int32 K)
	{
		const R::FRackSlot Slot = R::Rack(K);
		return FVector2D(Slot.X, R::RackHomeY(Slot.bNorth));
	}
	double OutDir(int32 K) { return R::Rack(K).bNorth ? 1.0 : -1.0; }

	FString PrimName(const FString& Id) { return TEXT("reserve_") + Id.Replace(TEXT("-"), TEXT("_")); }

	int32 RackOfLetter(const FString& RackLetter)
	{
		for (int32 K = 0; K < R::LetteredRacks; ++K)
		{
			if (Letter(K) == RackLetter) { return K; }
		}
		return INDEX_NONE;
	}

	/** Every actor attached under Actor, and Actor. */
	void Collect(AActor* Actor, TArray<AActor*>& Out)
	{
		if (!Actor) { return; }
		Out.Add(Actor);
		TArray<AActor*> Children;
		Actor->GetAttachedActors(Children);
		for (AActor* Child : Children) { Collect(Child, Out); }
	}

	/** True when a work (its back at its origin) faces west: its frame and canvas lie west of its origin. */
	bool FacesWest(AActor* Work)
	{
		TArray<AActor*> All;
		Collect(Work, All);
		FBox Box(ForceInit);
		for (AActor* A : All)
		{
			if (A != Work) { Box += A->GetComponentsBoundingBox(true); }
		}
		return !Box.IsValid || Box.GetCenter().X <= Work->GetActorLocation().X;
	}

	double Ease01(double T) { T = FMath::Clamp(T, 0.0, 1.0); return T * T * (3 - 2 * T); }

	/** A title short enough for a prompt: up to its first bracket. */
	FText Short(const FString& Title)
	{
		FString Left;
		return FText::FromString(Title.Split(TEXT(" ("), &Left, nullptr) ? Left : Title);
	}
}

namespace RRB = ReserveRackBuild;

AReserveRacks::AReserveRacks()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);   // the tracks are fixed; the carriages (movable) glide on it
	// The screens and tracks bake (Lumen needs their cards); what moves is the carriages, plain scene components the
	// baked meshes hang from. Only the lenses stay procedural.
	Tags.AddUnique(MuseeBake::BakeableTag());
	for (int32 K = 0; K < MuseePlan::Reserve::RackCount; ++K)
	{
		USceneComponent* Carriage = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Carriage_%s"), *RackName(K)));
		Carriage->SetupAttachment(RootComponent);
		Carriage->SetMobility(EComponentMobility::Movable);
		const FVector2D Home = RRB::Home(K);
		Carriage->SetRelativeLocation(MuseePlan::At(Home.X, Home.Y, MuseePlan::Reserve::FloorZ));
		Carriages.Add(Carriage);

		UProceduralMeshComponent* Screen = CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("Screen_%s"), *RackName(K)));
		Screen->SetupAttachment(Carriage);
		Screen->bUseAsyncCooking = true;
		Screen->bUseComplexAsSimpleCollision = true;
		Screen->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Screen->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Screen->SetMobility(EComponentMobility::Movable);
		Screen->CanCharacterStepUpOn = ECB_No;
		Screens.Add(Screen);
	}
	Tracks = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Tracks"));
	Tracks->SetupAttachment(RootComponent);
	Tracks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tracks->SetMobility(EComponentMobility::Static);

	const TArray<TPair<int32, double>> Faces = RRB::LitFaces();
	for (const TPair<int32, double>& Face : Faces)
	{
		const FString Name = RackName(Face.Key) + (Face.Value > 0 ? TEXT("_East") : TEXT("_West"));
		URectLightComponent* Lamp = CreateDefaultSubobject<URectLightComponent>(*(TEXT("PictureLight_") + Name));
		Lamp->SetupAttachment(Carriages[Face.Key]);
		Lamp->SetMobility(EComponentMobility::Movable);
		Lamp->SetVisibility(false);
		PictureLights.Add(Lamp);
		UProceduralMeshComponent* Lens = CreateDefaultSubobject<UProceduralMeshComponent>(*(TEXT("Lens_") + Name));
		Lens->SetupAttachment(Carriages[Face.Key]);
		Lens->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Lens->SetMobility(EComponentMobility::Movable);
		Lens->SetCastShadow(false);
		MuseeBake::NoBake(Lens);
		Lenses.Add(Lens);
		LightRack.Add(Face.Key);
	}
	LightLevel.Init(0.f, LightRack.Num());
	InitialRack = MuseePlan::Reserve::InitialRack;

	auto Path = [](const TCHAR* Folder, const TCHAR* Name)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name)));
	};
	const TCHAR* Materials = TEXT("/Game/Museum/Materials");
	OakMaterial = Path(Materials, TEXT("M_Oak_Fumed"));
	LinenMaterial = Path(Materials, TEXT("MI_Reserve_Linen"));          // Scripts/reserve_materials.py
	LinenFallbackMaterial = Path(Materials, TEXT("M_Frame_Linen"));
	BronzeMaterial = Path(Materials, TEXT("M_Bronze_Brushed"));
	PatinaMaterial = Path(Materials, TEXT("M_Bronze_Patina"));
	LensMaterial = Path(Materials, TEXT("M_PictureLens"));              // Scripts/reserve_materials.py
}

FString AReserveRacks::RackName(int32 K)
{
	const MuseePlan::Reserve::FRackSlot Slot = MuseePlan::Reserve::Rack(K);
	if (Slot.bNorth && K < MuseePlan::Reserve::LetteredRacks) { return RRB::Letter(K); }
	return FString::Printf(TEXT("%s%02d"), Slot.bNorth ? TEXT("N") : TEXT("S"), Slot.Index + 1);
}

TArray<FString> AReserveRacks::GetReplacedImportPrims()
{
	TArray<FString> Out;
	for (int32 K = 0; K < MuseePlan::Reserve::LetteredRacks; ++K)
	{
		Out.Add(RRB::Base + TEXT("Rack_") + RRB::Letter(K) + TEXT("/rack_frame"));
		Out.Add(RRB::Base + TEXT("Rack_") + RRB::Letter(K) + TEXT("/rack_mesh"));
	}
	// The export's unlettered racks: slots 1 … 32 but 10, 20 and 30; north to 25 (A–F took the rest), south from 3.
	for (int32 Slot = 1; Slot <= 32; ++Slot)
	{
		if (Slot % 10 == 0) { continue; }
		if (Slot <= 25) { Out.Add(RRB::Base + FString::Printf(TEXT("Rack_N%02d"), Slot)); }
		if (Slot >= 3) { Out.Add(RRB::Base + FString::Printf(TEXT("Rack_S%02d"), Slot)); }
	}
	return Out;
}

FVector AReserveRacks::EaselPlacement(const FString& WorkId)
{
	namespace R = MuseePlan::Reserve;
	double H = 1.0;
	for (const R::FRackWork& W : R::RackWorks)
	{
		if (WorkId == W.Id) { H = W.H; }
	}
	return MuseePlan::At(R::EaselWorkX, 0, R::FloorZ + R::EaselWorkHeight(H));
}

void AReserveRacks::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	BuildLenses();
}

void AReserveRacks::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }
	UMaterialInterface* Oak = OakMaterial.LoadSynchronous();
	UMaterialInterface* Linen = LinenMaterial.LoadSynchronous();
	if (!Linen) { Linen = LinenFallbackMaterial.LoadSynchronous(); }
	UMaterialInterface* Bronze = BronzeMaterial.LoadSynchronous();
	UMaterialInterface* Patina = PatinaMaterial.LoadSynchronous();
	auto Write = [](UProceduralMeshComponent* C, int32 Section, const RRB::FMeshData& Data, UMaterialInterface* Material, bool bCollision)
	{
		Data.Write(C, Section, bCollision);
		if (Material && Data.Indices.Num()) { C->SetMaterial(Section, Material); }
	};
	for (int32 K = 0; K < Screens.Num(); ++K)
	{
		UProceduralMeshComponent* Screen = Screens[K];
		if (!Screen) { continue; }
		RRB::FMeshData O, Li, B, P;
		RRB::BuildScreen(O, Li, B, P, K, RRB::OutDir(K));
		Screen->ClearAllMeshSections();
		Write(Screen, 0, O, Oak, true);
		Write(Screen, 1, Li, Linen, true);
		Write(Screen, 2, B, Bronze, false);
		Write(Screen, 3, P, Patina, false);
	}
	if (Tracks)
	{
		RRB::FMeshData B, P;
		RRB::BuildTracks(B, P);
		// (Salon interior fix: the tracks are built in heights above the Reserve's floor, so the component stands on that
		// floor; at the origin they hung 3.6 m over the Salon's.)
		Tracks->SetRelativeLocation(MuseePlan::At(0, 0, MuseePlan::Reserve::FloorZ));
		Tracks->ClearAllMeshSections();
		Write(Tracks, 0, B, Bronze, false);
		Write(Tracks, 1, P, Patina, false);
	}
	for (int32 K = 0; K < Carriages.Num(); ++K)
	{
		const FVector2D Home = RRB::Home(K);
		if (Carriages[K]) { Carriages[K]->SetRelativeLocation(MuseePlan::At(Home.X, Home.Y, MuseePlan::Reserve::FloorZ)); }
	}
}

void AReserveRacks::BuildLenses()
{
	// The lenses and lamps are never baked (they are shown and hidden), so they are built whether or not the rest is.
	UMaterialInterface* Lit = LensMaterial.LoadSynchronous();
	const TArray<TPair<int32, double>> Faces = RRB::LitFaces();
	for (int32 I = 0; I < Lenses.Num() && I < Faces.Num(); ++I)
	{
		const int32 K = Faces[I].Key;
		const double F = Faces[I].Value;
		if (UProceduralMeshComponent* Lens = Lenses[I])
		{
			RRB::FMeshData M;
			RRB::BuildLens(M, K, F);
			Lens->ClearAllMeshSections();
			M.Write(Lens, 0, false);
			if (Lit) { Lens->SetMaterial(0, Lit); }
			Lens->SetVisibility(false);
		}
		if (URectLightComponent* Lamp = PictureLights.IsValidIndex(I) ? PictureLights[I].Get() : nullptr)
		{
			const FVector2D Span = RRB::LampSpan(K, F);
			// Aimed at the works' centres (1.55 m up on the face), from the trough.
			const FVector From(F * RRB::LampOut, (Span.X + Span.Y) / 2, RRB::LampZ - 0.01);
			const FVector To(F * MuseePlan::Reserve::WorkOff, (Span.X + Span.Y) / 2, MuseePlan::Reserve::WorkHeight + 0.15);
			const FVector Dir = (To - From).GetSafeNormal();
			Lamp->SetRelativeLocationAndRotation(From * MuseePlan::Cm, FRotationMatrix::MakeFromXY(Dir, FVector(0, 1, 0)).Rotator());
			const double Length = Span.Y - Span.X - 0.04;
			Lamp->SetSourceWidth(static_cast<float>(Length * MuseePlan::Cm));
			Lamp->SetSourceHeight(3.f);
			Lamp->SetBarnDoorAngle(72.f);
			Lamp->SetBarnDoorLength(3.f);
			Lamp->SetIntensityUnits(ELightUnits::Lumens);
			Lamp->SetIntensity(0.f);
			Lamp->SetUseTemperature(true);
			Lamp->SetTemperature(2900.f);
			Lamp->SetLightColor(FLinearColor::White);
			Lamp->SetAttenuationRadius(420.f);
			Lamp->SetCastShadows(true);
			Lamp->SetVisibility(false);
		}
	}
}

int32 AReserveRacks::ArrangeImported()
{
	namespace R = MuseePlan::Reserve;
	UWorld* World = GetWorld();
	if (!World) { return 0; }
	int32 Placed = 0;
	auto Move = [&Placed](AActor* Actor, const FVector& To)
	{
		if (!Actor) { return; }
		Actor->Modify();
		Actor->SetActorLocation(To, false, nullptr, ETeleportType::TeleportPhysics);
		++Placed;
	};
	const FQuat HalfTurn(FVector::UpVector, UE_DOUBLE_PI);
	auto Face = [&HalfTurn](AActor* Work, bool bWest)
	{
		if (Work && RRB::FacesWest(Work) != bWest)
		{
			Work->Modify();
			Work->SetActorRotation(HalfTurn * Work->GetActorQuat(), ETeleportType::TeleportPhysics);
		}
	};
	for (int32 K = 0; K < R::LetteredRacks; ++K)
	{
		const FString Letter = RRB::Letter(K);
		const FString RackPath = RRB::Base + TEXT("Rack_") + Letter;
		AActor* Root = MuseeWorld::FindPrim(World, RackPath);
		if (!Root) { UE_LOG(LogMusee, Warning, TEXT("Reserve: no %s to arrange."), *RackPath); continue; }
		const FVector2D Home = RRB::Home(K);
		const FVector HomeAt = MuseePlan::At(Home.X, Home.Y, R::FloorZ);
		Move(Root, HomeAt);
		// The letter plate on the nave end post, facing the nave (it came facing west: turned once, a quarter turn), 12 cm wide.
		if (AActor* Plate = MuseeWorld::FindPrim(World, RackPath + TEXT("/Letter_plate")))
		{
			FVector Origin, Extent;
			Plate->GetActorBounds(false, Origin, Extent);
			if (Extent.X < Extent.Y)
			{
				Plate->Modify();
				Plate->SetActorRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(-90.0 * RRB::OutDir(K))) * Plate->GetActorQuat());
				Plate->GetActorBounds(false, Origin, Extent);
			}
			const double Width = 2.0 * FMath::Max(Extent.X, Extent.Y) / MuseePlan::Cm;
			if (Width > 0.01 && FMath::Abs(Width - RRB::PlateWidth) > 0.003)
			{
				Plate->Modify();
				Plate->SetActorScale3D(Plate->GetActorScale3D() * (RRB::PlateWidth / Width));
			}
			Move(Plate, HomeAt + MuseePlan::At(0, RRB::OutDir(K) * (RRB::L / 2 + 0.003), RRB::PlateZ));
		}
	}
	for (const R::FRackWork& W : R::RackWorks)
	{
		const int32 K = RRB::RackOfLetter(W.Letter);
		const FString RackPath = RRB::Base + TEXT("Rack_") + W.Letter;
		AActor* Work = MuseeWorld::FindPrim(World, RackPath + TEXT("/") + RRB::PrimName(W.Id));
		AActor* Root = MuseeWorld::FindPrim(World, RackPath);
		if (!Work || K == INDEX_NONE) { UE_LOG(LogMusee, Warning, TEXT("Reserve: no work %s to arrange."), W.Id); continue; }
		if (FString(W.Id) == R::EaselWork)
		{
			Work->Modify();
			Work->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			Face(Work, true);
			Move(Work, EaselPlacement(W.Id));
			continue;
		}
		if (Root && Work->GetAttachParentActor() != Root)
		{
			Work->Modify();
			Work->AttachToActor(Root, FAttachmentTransformRules::KeepWorldTransform);
		}
		Face(Work, !W.bEast);
		const FVector2D Home = RRB::Home(K);
		Move(Work, MuseePlan::At(Home.X, Home.Y, R::FloorZ) + RRB::WorkSlot(W.Letter, W.Id) * MuseePlan::Cm);
	}
	// The Degas pastels on the south islands of plan chests (The Tub west, The Star east), on their felt.
	for (int32 I = 0; I < 2; ++I)
	{
		AActor* Pastel = MuseeWorld::FindPrim(World, R::ChestWorks[I]);
		Move(Pastel, MuseePlan::At(R::BayCentreX(R::ChestBays[I]), R::AisleCentreY, R::FloorZ + R::ChestTop + 0.004));
	}
	UE_LOG(LogMusee, Log, TEXT("Reserve: %d imported pieces arranged for the screens."), Placed);
	return Placed;
}

void AReserveRacks::BeginPlay()
{
	Super::BeginPlay();
	Build();
	BuildLenses();
	FindParts();
	for (int32 K = 0; K < State.Num(); ++K)
	{
		if (!InitialRack.IsEmpty() && RackName(K) == InitialRack) { State[K].Offset = State[K].Target = MuseePlan::Reserve::RackGlide; }
		ApplyRack(K);
	}
	// The screen out at the start has its light on already.
	for (int32 I = 0; I < LightRack.Num(); ++I)
	{
		if (State.IsValidIndex(LightRack[I]) && State[LightRack[I]].Offset >= MuseePlan::Reserve::RackGlide) { LightLevel[I] = 1.f; }
	}
	UpdateLights();
}

void AReserveRacks::FindParts()
{
	namespace R = MuseePlan::Reserve;
	UWorld* World = GetWorld();
	State.SetNum(Carriages.Num());
	const UMuseeCatalog* Catalog = GetGameInstance() ? GetGameInstance()->GetSubsystem<UMuseeCatalog>() : nullptr;
	int32 Attached = 0;
	for (int32 K = 0; K < R::LetteredRacks && K < Carriages.Num(); ++K)
	{
		AActor* Root = MuseeWorld::FindPrim(World, RRB::Base + TEXT("Rack_") + RRB::Letter(K));
		if (!Root || !Carriages[K]) { continue; }
		if (USceneComponent* ImportedRoot = Root->GetRootComponent()) { ImportedRoot->SetMobility(EComponentMobility::Movable); }
		Root->AttachToComponent(Carriages[K], FAttachmentTransformRules::KeepWorldTransform);
		State[K].Imported = Root;
		TArray<AActor*> All;
		RRB::Collect(Root, All);
		for (AActor* A : All) { MuseeWorld::RegisterUse(A, this); }
		++Attached;
	}
	const FQuat HalfTurn(FVector::UpVector, UE_DOUBLE_PI);
	for (const R::FRackWork& W : R::RackWorks)
	{
		const int32 K = RRB::RackOfLetter(W.Letter);
		AActor* Actor = MuseeWorld::FindPrim(World, RRB::Base + TEXT("Rack_") + W.Letter + TEXT("/") + RRB::PrimName(W.Id));
		if (!Actor || K == INDEX_NONE) { continue; }
		FWork Work;
		Work.Actor = Actor;
		Work.Rack = K;
		Work.bEast = W.bEast;
		Work.HomeLocal = RRB::WorkSlot(W.Letter, W.Id) * MuseePlan::Cm;
		const FQuat Now = Actor->GetActorQuat();
		const FQuat West = RRB::FacesWest(Actor) ? Now : HalfTurn * Now;
		Work.HomeRotation = W.bEast ? HalfTurn * West : West;
		if (FString(W.Id) == R::EaselWork || Works.Num() == 0) { WestFacing = West; }
		Work.Id = W.Id;
		Work.Height = W.H;
		const FMuseeArtwork* Art = Catalog ? Catalog->Find(W.Id) : nullptr;
		Work.Title = Art ? RRB::Short(Art->Title) : FText::FromString(W.Id);
		if (USceneComponent* WorkRoot = Actor->GetRootComponent()) { WorkRoot->SetMobility(EComponentMobility::Movable); }
		TArray<AActor*> All;
		RRB::Collect(Actor, All);
		for (AActor* A : All) { MuseeWorld::RegisterUse(A, this); }
		// Not on its screen: on the easel (ArrangeImported put it there).
		if (State[K].Imported.IsValid() && Actor->GetAttachParentActor() != State[K].Imported.Get()) { EaselWork = Works.Num(); }
		Works.Add(Work);
	}
	UE_LOG(LogMusee, Log, TEXT("Reserve: %d screens, %d lettered with their imported parts, %d works (%s on the easel), %d picture lights."), Carriages.Num(),
		   Attached, Works.Num(), EaselWork != INDEX_NONE ? *Works[EaselWork].Id : TEXT("none"), PictureLights.Num());
}

void AReserveRacks::ApplyRack(int32 K)
{
	if (!Carriages.IsValidIndex(K) || !Carriages[K] || !State.IsValidIndex(K)) { return; }
	const double Glide = MuseePlan::Reserve::RackGlide;
	const FVector2D Home = RRB::Home(K);
	const double Y = Home.Y + RRB::OutDir(K) * RRB::Ease01(State[K].Offset / Glide) * Glide;
	Carriages[K]->SetRelativeLocation(MuseePlan::At(Home.X, Y, MuseePlan::Reserve::FloorZ), false, nullptr, ETeleportType::None);
}

bool AReserveRacks::Blocked(int32 K, double Offset) const
{
	// A screen at Offset against the visitor's capsule (in plan, inflated by its radius): it waits rather than push anyone.
	const UWorld* World = GetWorld();
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const AMuseeCharacter* Visitor = PC ? Cast<AMuseeCharacter>(PC->GetPawn()) : nullptr;
	if (!Visitor || !InReserve(Visitor)) { return false; }
	const double Radius = Visitor->GetCapsuleComponent()->GetScaledCapsuleRadius() / MuseePlan::Cm + 0.03;
	const FVector Feet = Visitor->FeetLocation() / MuseePlan::Cm;
	const double Glide = MuseePlan::Reserve::RackGlide;
	const FVector2D Home = RRB::Home(K);
	auto Depth = [&](double O)
	{
		const double Cy = Home.Y + RRB::OutDir(K) * RRB::Ease01(O / Glide) * Glide + RRB::OutDir(K) * 0.035;   // the pull stands 7 cm off the nave end
		const double Dx = 0.14 + Radius - FMath::Abs(Feet.X - Home.X);   // the frames stand 13.5 cm off the screen's plane
		const double Dy = RRB::L / 2 + 0.035 + Radius - FMath::Abs(Feet.Y - Cy);
		return (Dx > 0 && Dy > 0) ? Dy : 0.0;
	};
	const double Next = Depth(Offset), Now = Depth(State[K].Offset);
	return Next > 0 && Next > Now - 1e-6;
}

void AReserveRacks::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Glide = MuseePlan::Reserve::RackGlide;
	const double Step = DeltaSeconds * Glide / FMath::Max(0.1f, GlideSeconds);
	for (int32 K = 0; K < State.Num(); ++K)
	{
		FRackState& S = State[K];
		if (S.Offset == S.Target) { S.bWaiting = false; continue; }
		const double Next = S.Target > S.Offset ? FMath::Min(S.Target, S.Offset + Step) : FMath::Max(S.Target, S.Offset - Step);
		if (Blocked(K, Next))
		{
			if (!S.bWaiting)
			{
				S.bWaiting = true;
				const UWorld* World = GetWorld();
				const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
				if (AMuseeCharacter* Visitor = PC ? Cast<AMuseeCharacter>(PC->GetPawn()) : nullptr)
				{
					Visitor->SetMessage(FText::Format(LOCTEXT("Waiting", "Screen {0} waits: you are in its way"), RackTitle(K)));
				}
			}
			continue;
		}
		if (S.bWaiting)
		{
			S.bWaiting = false;
			const UWorld* World = GetWorld();
			const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			AMuseeCharacter* Visitor = PC ? Cast<AMuseeCharacter>(PC->GetPawn()) : nullptr;
			if (Visitor && Visitor->Message().ToString().StartsWith(TEXT("Screen "))) { Visitor->SetMessage(FText::GetEmpty()); }
		}
		S.Offset = Next;
		ApplyRack(K);
	}
	// The picture lights: on when their screen reaches its stop out, a lamp's half-second warm-up and fade.
	bool bChanged = false;
	for (int32 I = 0; I < LightLevel.Num(); ++I)
	{
		const int32 K = LightRack[I];
		const float Want = (State.IsValidIndex(K) && State[K].Target > 0 && State[K].Offset >= Glide) ? 1.f : 0.f;
		if (LightLevel[I] == Want) { continue; }
		LightLevel[I] = Want > LightLevel[I] ? FMath::Min(Want, LightLevel[I] + DeltaSeconds / 0.6f) : FMath::Max(Want, LightLevel[I] - DeltaSeconds / 0.3f);
		bChanged = true;
	}
	if (bChanged) { UpdateLights(); }
	UpdateCarry(DeltaSeconds);
}

void AReserveRacks::UpdateLights()
{
	const TArray<TPair<int32, double>> Faces = RRB::LitFaces();
	for (int32 I = 0; I < PictureLights.Num() && I < Faces.Num(); ++I)
	{
		const float Level = LightLevel.IsValidIndex(I) ? LightLevel[I] : 0.f;
		const FVector2D Span = RRB::LampSpan(Faces[I].Key, Faces[I].Value);
		if (URectLightComponent* Lamp = PictureLights[I])
		{
			// A warm filament's light comes up faster than its colour: level squared.
			Lamp->SetIntensity(PictureLightLumensPerMetre * static_cast<float>(Span.Y - Span.X) * Level * Level);
			Lamp->SetVisibility(Level > 0.001f);
		}
		if (Lenses.IsValidIndex(I) && Lenses[I]) { Lenses[I]->SetVisibility(Level > 0.35f); }
	}
}

bool AReserveRacks::IsOut(int32 K) const { return State.IsValidIndex(K) && State[K].Target > 0; }

void AReserveRacks::Toggle(int32 K)
{
	if (!State.IsValidIndex(K)) { return; }
	if (Carry.IsSet() && Works.IsValidIndex(Carry->Work) && Works[Carry->Work].Rack == K) { return; }   // a painting is on its way to it
	const bool bOut = !IsOut(K);
	for (FRackState& S : State) { S.Target = 0; }
	State[K].Target = bOut ? MuseePlan::Reserve::RackGlide : 0.0;
}

bool AReserveRacks::InReserve(const AMuseeCharacter* Visitor) const
{
	if (!Visitor) { return false; }
	const FVector Feet = Visitor->FeetLocation() / MuseePlan::Cm;
	return Feet.Z < -4.0 && Feet.X > MuseePlan::Reserve::X0 - 1.0 && Feet.X < MuseePlan::Reserve::X1;
}

int32 AReserveRacks::NearestRack(const AMuseeCharacter* Visitor, double MaxDistance) const
{
	if (!Visitor) { return INDEX_NONE; }
	const FVector Feet = Visitor->FeetLocation() / MuseePlan::Cm;
	int32 Best = INDEX_NONE;
	double BestD = MaxDistance;
	for (int32 K = 0; K < Carriages.Num(); ++K)
	{
		if (!Carriages[K]) { continue; }
		const FVector C = Carriages[K]->GetComponentLocation() / MuseePlan::Cm;
		const double DX = FMath::Max(0.0, FMath::Abs(Feet.X - C.X) - RRB::PostHalf);
		const double DY = FMath::Max(0.0, FMath::Abs(Feet.Y - C.Y) - RRB::L / 2);
		const double D = FMath::Sqrt(DX * DX + DY * DY);
		if (D < BestD) { BestD = D; Best = K; }
	}
	return Best;
}

int32 AReserveRacks::RackOf(const FHitResult& Hit) const
{
	// The screen's own mesh, or a baked one: both hang from its carriage.
	for (const USceneComponent* C = Hit.GetComponent(); C; C = C->GetAttachParent())
	{
		for (int32 K = 0; K < Carriages.Num(); ++K)
		{
			if (Carriages[K] && C == Carriages[K]) { return K; }
		}
	}
	for (const AActor* A = Hit.GetActor(); A; A = A->GetAttachParentActor())
	{
		for (int32 K = 0; K < State.Num(); ++K)
		{
			if (State[K].Imported.Get() == A) { return K; }
		}
		for (const FWork& W : Works)
		{
			if (W.Actor.Get() == A) { return W.Rack; }
		}
	}
	return INDEX_NONE;
}

int32 AReserveRacks::WorkOf(const FHitResult& Hit) const
{
	for (const AActor* A = Hit.GetActor(); A; A = A->GetAttachParentActor())
	{
		for (int32 I = 0; I < Works.Num(); ++I)
		{
			if (Works[I].Actor.Get() == A) { return I; }
		}
	}
	return INDEX_NONE;
}

FText AReserveRacks::RackTitle(int32 K) const { return FText::FromString(RackName(K)); }

bool AReserveRacks::CanInteract(const AMuseeCharacter* Visitor, const FHitResult& Hit) const
{
	return InReserve(Visitor) && !Carry.IsSet() && (WorkOf(Hit) != INDEX_NONE || RackOf(Hit) != INDEX_NONE);
}

void AReserveRacks::Interact(AMuseeCharacter* Visitor, const FHitResult& Hit)
{
	CarryVisitor = Visitor;
	const int32 Work = WorkOf(Hit);
	if (Work != INDEX_NONE && (Work == EaselWork || State[Works[Work].Rack].Offset >= MuseePlan::Reserve::RackGlide))
	{
		Send(Work);
		return;
	}
	Toggle(RackOf(Hit));
}

FText AReserveRacks::InteractHint(const AMuseeCharacter* Visitor, const FHitResult& Hit) const
{
	const int32 Work = WorkOf(Hit);
	if (Work == EaselWork && Work != INDEX_NONE)
	{
		return FText::Format(LOCTEXT("ReturnHint", "Hang it back on screen {0}"), RackTitle(Works[Work].Rack));
	}
	if (Work != INDEX_NONE && State[Works[Work].Rack].Offset >= MuseePlan::Reserve::RackGlide)
	{
		return LOCTEXT("SendHint", "Send it to the viewing easel");
	}
	const int32 K = RackOf(Hit);
	if (K == INDEX_NONE) { return FText::GetEmpty(); }
	return FText::Format(IsOut(K) ? LOCTEXT("PushHint", "Slide screen {0} home") : LOCTEXT("PullHint", "Draw out screen {0}"), RackTitle(K));
}

void AReserveRacks::GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const
{
	if (!InReserve(Visitor) || Carry.IsSet()) { return; }
	const FVector Feet = Visitor->FeetLocation() / MuseePlan::Cm;
	const int32 K = NearestRack(Visitor, 1.6);
	if (K != INDEX_NONE)
	{
		Out.Add({TEXT("rack.near"), FText::Format(IsOut(K) ? LOCTEXT("Push", "Slide screen {0} home") : LOCTEXT("Pull", "Draw out screen {0}"), RackTitle(K))});
	}
	// Standing before a face of a screen that is out: that face's paintings, to the easel.
	for (int32 I = 0; I < Works.Num() && Out.Num() < 3; ++I)
	{
		const FWork& W = Works[I];
		if (I == EaselWork || !Carriages.IsValidIndex(W.Rack) || !Carriages[W.Rack] || State[W.Rack].Offset < MuseePlan::Reserve::RackGlide) { continue; }
		const FVector C = Carriages[W.Rack]->GetComponentLocation() / MuseePlan::Cm;
		const double Side = W.bEast ? Feet.X - C.X : C.X - Feet.X;
		if (Side > 0 && Side < 7.0 && FMath::Abs(Feet.Y - C.Y) < 4.0)
		{
			Out.Add({FName(*(TEXT("easel.") + W.Id)), FText::Format(LOCTEXT("Send", "Send {0} to the easel"), W.Title)});
		}
	}
	if (Works.IsValidIndex(EaselWork))
	{
		const FVector Easel = EaselPlacement(Works[EaselWork].Id) / MuseePlan::Cm;
		if (FVector2D(Feet.X - Easel.X, Feet.Y - Easel.Y).Size() < 6.0)
		{
			Out.Add({TEXT("easel.return"), FText::Format(LOCTEXT("Return", "Hang {0} back on screen {1}"), Works[EaselWork].Title, RackTitle(Works[EaselWork].Rack))});
		}
	}
}

void AReserveRacks::RunPrompt(AMuseeCharacter* Visitor, FName Id)
{
	FString S = Id.ToString();
	if (S.StartsWith(TEXT("screen."))) { S = TEXT("rack.") + S.RightChop(7); }
	if (!S.StartsWith(TEXT("rack.")) && !S.StartsWith(TEXT("easel."))) { return; }
	CarryVisitor = Visitor;
	if (S == TEXT("rack.near")) { Toggle(NearestRack(Visitor, 1.6)); return; }
	if (S == TEXT("easel.return")) { Return(); return; }
	if (S.StartsWith(TEXT("rack.")))
	{
		const FString Name = S.RightChop(5);
		for (int32 K = 0; K < Carriages.Num(); ++K)
		{
			if (RackName(K) == Name) { Toggle(K); return; }
		}
		return;
	}
	const FString WorkId = S.RightChop(6);
	for (int32 I = 0; I < Works.Num(); ++I)
	{
		if (Works[I].Id == WorkId) { Send(I); return; }
	}
}

void AReserveRacks::Send(int32 Index)
{
	if (!Works.IsValidIndex(Index) || Carry.IsSet()) { return; }
	if (Index == EaselWork) { Return(); return; }
	if (EaselWork != INDEX_NONE)
	{
		// The easel's painting goes home first; this one follows.
		PendingEasel = Index;
		Return();
		return;
	}
	AActor* Actor = Works[Index].Actor.Get();
	if (!Actor) { return; }
	Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Actor->SetActorEnableCollision(false);
	Carry = FCarry{Index, true, 0.f, Actor->GetActorTransform()};
	if (AMuseeCharacter* Visitor = CarryVisitor.Get()) { Visitor->SetMessage(LOCTEXT("OnItsWay", "On its way to the viewing easel…")); }
}

void AReserveRacks::Return()
{
	if (!Works.IsValidIndex(EaselWork) || Carry.IsSet()) { return; }
	AActor* Actor = Works[EaselWork].Actor.Get();
	if (!Actor) { return; }
	Actor->SetActorEnableCollision(false);
	Carry = FCarry{EaselWork, false, 0.f, Actor->GetActorTransform()};
	EaselWork = INDEX_NONE;
}

void AReserveRacks::UpdateCarry(float DeltaSeconds)
{
	if (!Carry.IsSet()) { return; }
	FCarry& C = Carry.GetValue();
	FWork& W = Works[C.Work];
	AActor* Actor = W.Actor.Get();
	if (!Actor) { Carry.Reset(); return; }
	C.T = FMath::Min(1.f, C.T + DeltaSeconds / RRB::CarrySeconds);
	const FVector To = C.bToEasel ? EaselPlacement(W.Id) : Carriages[W.Rack]->GetComponentTransform().TransformPosition(W.HomeLocal);
	const FQuat ToRotation = C.bToEasel ? (W.bEast ? FQuat(FVector::UpVector, UE_DOUBLE_PI) * W.HomeRotation : W.HomeRotation) : W.HomeRotation;
	const double S = RRB::Ease01(C.T);
	FVector P = FMath::Lerp(C.From.GetLocation(), To, S);
	P.Z += FMath::Sin(C.T * UE_DOUBLE_PI) * 40.0;   // it lifts a little on its way
	Actor->SetActorLocationAndRotation(P, FQuat::Slerp(C.From.GetRotation(), ToRotation, S));
	if (C.T < 1.f) { return; }
	const bool bToEasel = C.bToEasel;
	const int32 Index = C.Work;
	Carry.Reset();
	Actor->SetActorEnableCollision(true);
	AMuseeCharacter* Visitor = CarryVisitor.Get();
	if (bToEasel)
	{
		EaselWork = Index;
		if (Visitor && Visitor->Message().ToString().StartsWith(TEXT("On its way"))) { Visitor->SetMessage(FText::GetEmpty()); }
		return;
	}
	AActor* Root = State[W.Rack].Imported.Get();
	if (Root) { Actor->AttachToActor(Root, FAttachmentTransformRules::KeepWorldTransform); }
	else { Actor->AttachToComponent(Carriages[W.Rack], FAttachmentTransformRules::KeepWorldTransform); }
	if (PendingEasel != INDEX_NONE)
	{
		const int32 Next = PendingEasel;
		PendingEasel = INDEX_NONE;
		Send(Next);
	}
}

/** musee.ReserveScreens 0|1: hide or show the picture screens, their tracks and picture lights (their GPU cost, for perf A/B runs). */
static FAutoConsoleCommandWithWorldAndArgs GReserveScreensCommand(
	TEXT("musee.ReserveScreens"),
	TEXT("musee.ReserveScreens 0|1: hide or show the Reserve's picture screens, tracks and picture lights (a perf check)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World) { return; }
		const bool bShow = Args.Num() == 0 || Args[0] != TEXT("0");
		for (TActorIterator<AReserveRacks> It(World); It; ++It) { It->SetActorHiddenInGame(!bShow); }
		UE_LOG(LogMusee, Log, TEXT("Reserve: picture screens %s."), bShow ? TEXT("shown") : TEXT("hidden"));
	}));

#undef LOCTEXT_NAMESPACE
