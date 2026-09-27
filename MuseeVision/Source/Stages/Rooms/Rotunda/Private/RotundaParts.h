#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"
#include "RotundaPlan.h"

struct FSiteDescription;

namespace Musee::Time
{
	class FMuseumDial;
}

/**
 * The Rotunda's parts by trade, each drawn in Form's language into the description's pieces: the drum (§3.2), the order
 * (§3.3), the dome (§3.4), the eye (§3.5), the floors (§3.1), the sun clock's lifting drum (§3.6), the shaft and its
 * landing (§3.7) and the stair (§3.8). The site's declarations (footprint, ports, openings, anchors, enclosures) are
 * RotundaSite's. Everything is set out on circles about the stage's vertical axis, so the helpers here speak in radii,
 * plan angles φ (degrees from +x towards +y) and heights.
 */
namespace Musee::Rotunda
{
	/** A key under the stage: rotunda.<Local>. */
	FMuseeKey Key(const TCHAR* Local);

	/** A surface of the stage's finish schedule: rotunda.<Local>. */
	FName Surface(const TCHAR* Local);

	/** The body of the sun clock's lifting drum, which carries the dial, its drum and the soffit. */
	FMuseeKey DrumBody();

	/** The collision classes the pieces take: solid masonry and fittings, and the floors and treads a visitor walks. */
	inline constexpr const Musee::Collision::FClass* Solid = &Musee::Collision::Solid;
	inline constexpr const Musee::Collision::FClass* Walkable = &Musee::Collision::Walkable;

	/** A drawing of the stage, finished in one of its surfaces, colliding as given, standing in a frame. */
	template <typename TShape>
	FFormDrawing Drawn(const FString& Name, const TCHAR* LocalSurface, TShape&& Shape, const Musee::Collision::FClass* Collision,
		const FMuseeFrame& Frame = FMuseeFrame())
	{
		FFormDrawing Drawing = FFormDrawing::Of(FName(*Name), Surface(LocalSurface), Forward<TShape>(Shape));
		Drawing.Collision = Collision ? Collision->Profile() : NAME_None;
		Drawing.Frame = Frame;
		return Drawing;
	}

	/** A drawing added to the pieces riding on the lifting drum. */
	template <typename TShape>
	void OnDrum(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, TShape&& Shape,
		const Musee::Collision::FClass* Collision, const FMuseeFrame& Frame = FMuseeFrame())
	{
		Out.Add_GetRef(Drawn(Name, LocalSurface, Forward<TShape>(Shape), Collision, Frame)).Body = DrumBody();
	}

	/** A profile from the sheet's steps (points and arcs), closed back to its first point or left open. */
	FFormProfile Profile(TConstArrayView<Plan::FStep> Steps, bool bClosed);

	/** The point at a radius and plan angle. */
	FVector2D Polar(double Radius, double Degrees);

	/** The stage's frame turned about its vertical axis by a plan angle, at an origin: its x points along φ. */
	FMuseeFrame Turned(double Degrees, const FVector& Origin = FVector::ZeroVector);

	/** The stage's frame raised to a height. */
	FMuseeFrame Raised(double Z);

	/** The frame whose x and y are the given axes (and z their cross product), at an origin. */
	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y);

	/** A turned ring of rectangular section, r Inner to Outer and z Bottom to Top, over a turn (a full one by default). */
	FFormLathe Ring(double Inner, double Outer, double Bottom, double Top, double FromDeg = 0.0, double ToDeg = 360.0);

	/** A plan extrusion of an outline and its holes, Depth up from its frame. */
	FFormExtrusion Slab(FFormProfile Outline, TArray<FFormProfile> Holes, double Depth);

	/** A circle about the axis as a closed profile. */
	FFormProfile Circle(double Radius, const FVector2D& Centre = FVector2D::ZeroVector);

	/** The plan region between two radii and two plan angles (From below To), as a closed profile. */
	FFormProfile AnnularSector(double Inner, double Outer, double FromDeg, double ToDeg);

	/** The half-angle, seen from the axis, of a width's half on a circle of a radius: where a chord meets it. */
	double HalfAngleDeg(double HalfWidth, double Radius);

	/** The unit vector along a plan angle, and the one a quarter turn on (the way φ rises). */
	FVector Radial(double Degrees);
	FVector Tangent(double Degrees);

	/** A point (r, z) of a sphere about the dome's centre (0, 0, 10.00), at its radius and an elevation. */
	FVector2D OnDome(double Radius, double ElevationDeg);

	/** The elevation at which a sphere about the dome's centre reaches a height. */
	double ElevationAt(double Radius, double Z);

	/** The elevation where the dome's inner sphere springs from the cornice's top ledge, z 10.50: 2.866°. */
	double CorniceDeg();

	/** The elevation of the upper plain band's end at the eye's curb, r 3.000: 72.54°. */
	double EyeCurbDeg();

	/**
	 * A moulding run round an arched opening on a cylindrical face (the drum's inner face, the sun clock's outer one): up
	 * the left jamb from its foot, over the semicircular head, down the right jamb. The face has no plane, so the run is
	 * laid as voussoirs (Plan::VoussoirMeters), each a sweep in the plane tangent to the face at its middle, the section's
	 * A out from the arch's line and its B proud of the face.
	 */
	struct FFaceArch
	{
		/** The face: its radius, the opening's axis (a plan angle), and whether the face is seen from outside (convex). */
		double FaceRadius = 0.0;
		double AxisDeg = 0.0;
		bool bConvex = false;
		/** The line the section is set out from: the arch's radius about its springing, and the jambs' foot. */
		double ArchRadius = 0.0;
		double Springing = 0.0;
		/** The jambs' foot, or the springing itself for a run that goes round the head alone. */
		double JambFoot = 0.0;
	};
	void RunRoundArch(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FFormProfile& Section,
		const FFaceArch& Arch, const FMuseeKey& Body = FMuseeKey());

	/** The names of the pieces that bound the enclosures (the site's list reads them). */
	namespace Pieces
	{
		inline const TCHAR* const DrumWall = TEXT("drum.wall");
		inline const TCHAR* const DrumFooting = TEXT("drum.footing");
		inline const TCHAR* const DrumTop = TEXT("drum.top");
		inline const TCHAR* const DomeBandLower = TEXT("dome.band.lower");
		inline const TCHAR* const DomeShell = TEXT("dome.shell");
		inline const TCHAR* const DomeBandUpper = TEXT("dome.band.upper");
		inline const TCHAR* const EyeCurb = TEXT("dome.eye-curb");
		inline const TCHAR* const EyeGlass = TEXT("eye.glass");
		inline const TCHAR* const FloorCore = TEXT("floor.core");
		inline const TCHAR* const SunDeck = TEXT("sun-clock.deck");
		inline const TCHAR* const SunWall = TEXT("sun-clock.wall");
		inline const TCHAR* const Sleeve = TEXT("stair.wall");
		inline const TCHAR* const MasonryInner = TEXT("shaft.masonry.inner");
		inline const TCHAR* const MasonryTympanum = TEXT("shaft.masonry.tympanum");
		inline const TCHAR* const MasonryOuter = TEXT("shaft.masonry.outer");
		inline const TCHAR* const Foundation = TEXT("stair.foundation");
	}

	void DescribeSite(FSiteDescription& Out);
	void DrawDrum(TArray<FFormDrawing>& Out);
	void DrawOrder(TArray<FFormDrawing>& Out);
	void DrawDome(TArray<FFormDrawing>& Out);
	void DrawEye(TArray<FFormDrawing>& Out);
	void DrawFloors(TArray<FFormDrawing>& Out);

	/** The dial's inlay laid from Time's dial: the hour ring, the hour lines, the gilt meridian and the minute ticks. */
	void DrawDialInlay(TArray<FFormDrawing>& Out, const Time::FMuseumDial& Dial);
	void DrawSunClock(TArray<FFormDrawing>& Out);
	void DrawShaft(TArray<FFormDrawing>& Out);
	void DrawStair(TArray<FFormDrawing>& Out);
}
