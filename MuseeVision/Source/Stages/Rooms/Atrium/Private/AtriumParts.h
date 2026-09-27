#pragma once

#include "AtriumPlan.h"
#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"

struct FSiteDescription;

/**
 * The Atrium's parts by trade, each drawn in Form's language into the description's pieces: the masonry (§3.2), the floor
 * and its structure (§3.1, §3.3), the glazing (§3.4), the steelwork and the canopy grid (§3.5), the lift installation and
 * the car (§3.6), and the stele (§3.7). The site's declarations (footprint, ports, openings, anchors, enclosures) are
 * AtriumSite's. Everything is set out on the Élan axis, by bearing and radius.
 */
namespace Musee::Atrium
{
	/** A key under the stage: atrium.<Local>. */
	FMuseeKey Key(const TCHAR* Local);

	/** A surface of the stage's finish schedule: atrium.<Local>. */
	FName Surface(const TCHAR* Local);

	/** The plan angle Form turns by (from +x towards +y) of a compass bearing: the bearing less 90 degrees. */
	double PlanAngle(double Bearing);

	/** The point at a bearing and a radius on the plan: (r sin b, -r cos b). */
	FVector2D Polar(double Radius, double Bearing);

	/** The unit vectors at a bearing: out from the axis, and along the circle towards increasing bearing. */
	FVector2D Outward(double Bearing);
	FVector2D Along(double Bearing);

	/** The bearing of a plan point seen from the axis, 0 to 360. */
	double BearingOf(const FVector2D& Point);

	/** The bearing of rib k (0 to 11): 15° + 30°k. */
	double RibBearing(int32 Rib);

	/** A profile from the plan's steps, (A, B) raised by Rise along B, closed or open. */
	FFormProfile Section(TConstArrayView<Plan::FStep> Steps, bool bClosed, double Rise = 0.0);

	/** The rectangle between (A0, B0) and (A1, B1), drawn anticlockwise so its solid is inside. */
	FFormProfile Box(double A0, double B0, double A1, double B1);

	/** A turned solid of a section between two bearings (clockwise, the way bearings increase), or a full turn. */
	FFormLathe Turned(FFormProfile Section, double FromBearing, double ToBearing);
	FFormLathe TurnedAround(FFormProfile Section);

	/** A band r0 → r1 by z0 → z1 turned between two bearings: a pane of a drum, a ring's segment. */
	FFormLathe Band(double R0, double R1, double Z0, double Z1, double FromBearing, double ToBearing);

	/** A plan extrusion of an outline between two heights (the drawing's frame is raised to the lower). */
	FFormExtrusion Prism(FFormProfile Outline, double Bottom, double Top, TArray<FFormProfile> Holes = {});

	/**
	 * The frame of a radial plane at a bearing: its x out from the axis, its y up and its z along the circle towards
	 * decreasing bearing, moved back by Half along z so a solid Half either side of the plane is drawn by pushing its
	 * elevation (r, z) 2 × Half along z.
	 */
	FMuseeFrame RadialPlane(double Bearing, double Half);

	/** The frame on the axis at a height, turned so its x points out at a bearing and its y along the circle (z up). */
	FMuseeFrame OnBearing(double Bearing, double Height = 0.0);

	/** A frame from two axes: x as given, y the given direction made square to it, z = x × y. */
	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y);

	/**
	 * The outline of an annular sector between two bearings (clockwise) and two radii, its radial sides moved in by
	 * HalfJoint so a joint of twice that width lies on each radial line: a floor slab, a pane.
	 */
	FFormProfile Sector(double R0, double R1, double FromBearing, double ToBearing, double HalfJoint = 0.0);

	/** A rectangle in plan at a bearing: U0 → U1 out from the axis along it, T0 → T1 across it (towards increasing bearing). */
	FFormProfile AtBearing(double Bearing, double U0, double U1, double T0, double T1);

	/** The bearing of a span's end round the landing or the car: degrees clockwise from the door's centre line. */
	double FromDoor(double Degrees);

	/** The angle, in degrees, that a chord's half-width subtends on a circle: where a notch's side meets it. */
	double HalfAngle(double HalfWidth, double Radius);

	/** A drawing of the stage, finished in one of its surfaces, colliding as given, standing in a frame. */
	template <typename TShape>
	FFormDrawing Drawn(const FString& Name, const TCHAR* LocalSurface, TShape&& Shape, const Musee::Collision::FClass* Collision,
		const FMuseeFrame& Frame = FMuseeFrame())
	{
		FFormDrawing Drawing = FFormDrawing::Of(FName(*Name), Surface(LocalSurface), Forward<TShape>(Shape));
		Drawing.Collision = Collision ? Collision->Profile() : NAME_None;
		Drawing.Frame = Frame;
		Drawing.Drawer = FName(Plan::Stage);
		return Drawing;
	}

	/** Glass is cast without Nanite, which can't render it. */
	FFormDrawing& AsGlass(FFormDrawing& Drawing);

	/** The names of the pieces that bound the hall (the site's list reads them by these prefixes and names). */
	namespace Pieces
	{
		inline const TCHAR* const Floor = TEXT("floor.");
		inline const TCHAR* const WaitingRing = TEXT("floor.waiting-ring");
		inline const TCHAR* const Base = TEXT("base");
		inline const TCHAR* const InnerLeaf = TEXT("drum.inner.");
		inline const TCHAR* const Closer = TEXT("canopy.closer");
		inline const TCHAR* const Canopy = TEXT("canopy.glass");
		inline const TCHAR* const Ring = TEXT("canopy.ring");
		inline const TCHAR* const Collar = TEXT("collar");
		inline const TCHAR* const LandingGlass = TEXT("landing.pane.");
		inline const TCHAR* const TubeGlass = TEXT("tube.pane.");
		inline const TCHAR* const LandingLeaf = TEXT("landing.leaf.");
		inline const TCHAR* const Sill = TEXT("landing.sill");
	}

	/** The car's body (its frame the floor datum on the axis at the Atrium stop, the stage's origin), and the Cube mast's head on its crown. */
	FMuseeKey CarBody();
	FMuseeKey MastHeadBody();
	FMuseeFrame MastHeadFrame();

	/** The moving body of blade k (0 to 6) of the collar iris, and its frame: at its pin, on its top face, axes of the stage. */
	FMuseeKey IrisBladeBody(int32 Blade);
	FMuseeFrame IrisPinFrame(int32 Blade);

	/**
	 * A pair of glass leaves closing a door on the axis's west side (the landing's, the car's): each turns about the axis
	 * from the door's centre line to 25°, north towards increasing bearing, south towards decreasing, with a gilt leading
	 * fin on the centre line. Each rides its body <Owner>.leaf.north|south, whose frame is the stage's own.
	 */
	void DrawLeafPair(TArray<FFormDrawing>& Out, const TCHAR* Owner, double R0, double R1, double Z0, double Z1, const TCHAR* GlassSurface,
		const TCHAR* FinSurface);

	void DrawMasonry(TArray<FFormDrawing>& Out);
	void DrawFloor(TArray<FFormDrawing>& Out);
	void DrawGlazing(TArray<FFormDrawing>& Out);
	void DrawSteel(TArray<FFormDrawing>& Out);
	void DrawCanopyGrid(TArray<FFormDrawing>& Out);
	void DrawLift(TArray<FFormDrawing>& Out);
	void DrawCar(TArray<FFormDrawing>& Out);
	void DrawStele(TArray<FFormDrawing>& Out);
	void DescribeSite(FSiteDescription& Out);
}
