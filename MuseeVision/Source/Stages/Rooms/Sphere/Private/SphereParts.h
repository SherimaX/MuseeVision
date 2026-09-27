#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"

struct FSiteDescription;

/**
 * The Sphere's parts by trade, each drawn in Form's language into the description's pieces: the shells (the plate, the
 * image surface and the exterior dome, §3.1-3.3), the neck (§3.4), the platform (§3.5) and the mast's footing (§3.6).
 * The site's declarations (footprint, port, openings, anchors, enclosures) are SphereSite's. Everything is set out
 * about the Élan axis, so the helpers speak in radii, compass bearings (clockwise from north, as the sheet does) and
 * heights; a point at bearing b and radius r is (r sin b, -r cos b).
 */
namespace Musee::Sphere
{
	/** A key under the stage: sphere.<Local>. */
	FMuseeKey Key(const TCHAR* Local);

	/** A surface of the stage's finish schedule: sphere.<Local>. */
	FName Surface(const TCHAR* Local);

	/** The collision classes the pieces take: solid fittings and shells, and the floors a visitor walks. */
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

	// ---- setting out about the axis ------------------------------------------------------------------------------------

	/** The plan point at a radius and a bearing. */
	FVector2D AtBearing(double Radius, double BearingDeg);

	/** The horizontal unit vector out along a bearing, and the one along the way the bearing rises (clockwise on plan). */
	FVector Radial(double BearingDeg);
	FVector Tangent(double BearingDeg);

	/** The plan angle (degrees from +x towards +y, the lathe's and the iris pins' measure) of a bearing. */
	double PlanDeg(double BearingDeg);

	/** The frame whose x and y are the given axes (and z their cross product), at an origin. */
	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y);

	/** The stage's frame raised to a height. */
	FMuseeFrame Raised(double Z);

	/** A frame on a bearing at a radius and height: x out along the bearing, y the way it rises, z up. */
	FMuseeFrame OnBearing(double BearingDeg, double Radius, double Z);

	/** The height at which a sphere about the centre C has a radius from the axis, below or above the equator. */
	double SphereZ(double SphereRadius, double Radius, bool bAbove);

	/** The angle (degrees about C, in the section's (r, z) plane from +r towards +z) of a sphere's point at a radius. */
	double SphereDeg(double SphereRadius, double Radius, bool bAbove);

	// ---- shapes -------------------------------------------------------------------------------------------------------

	/** A turned ring of rectangular section, r Inner to Outer and z Bottom to Top, between two bearings (a full turn by default). */
	FFormLathe Ring(double Inner, double Outer, double Bottom, double Top, double FromBearing = 0.0, double ToBearing = 360.0);

	/** A turned solid about the axis from its closed or axis-ending section. */
	FFormLathe Turned(FFormProfile Section);

	/** A plan extrusion of an outline and its holes, Depth up from its frame. */
	FFormExtrusion Slab(FFormProfile Outline, TArray<FFormProfile> Holes, double Depth);

	/** A circle about a plan centre as a closed profile. */
	FFormProfile Circle(double Radius, const FVector2D& Centre = FVector2D::ZeroVector);

	/** A rectangle between two corners, closed. */
	FFormProfile Rectangle(const FVector2D& Min, const FVector2D& Max);

	/** The seats' bore (§3.4): the 96-gon of circumradius 2.460 whose sides are centred on bearings 3.75°k. */
	TArray<FVector2D> SeatPolygonPoints();
	FFormProfile SeatPolygon();

	/** The inscribed radius of the seats' 96-gon: where its sides are nearest the axis, 2.4587. */
	double SeatInscribed();

	/**
	 * A plan sector between two radii and two bearings (From below To), each end a straight line parallel to its radial
	 * line and set off it towards the sector: a slab between two joints of those half-widths, or a band stopped beside a
	 * rib or a housing whose half-width it is.
	 */
	FFormProfile Sector(double Inner, double Outer, double FromBearing, double ToBearing, double FromOffset, double ToOffset);
	FFormProfile Sector(double Inner, double Outer, double FromBearing, double ToBearing, double Offset);

	/**
	 * The seats' 96-gon between two bearings (From below To): where each bearing's radial line crosses it, and the corners
	 * between, in rising bearing.
	 */
	TArray<FVector2D> SeatPolygonBetween(double FromBearing, double ToBearing);

	/** The plan angle of a plan point, carried a turn on where it would fall below a given angle. */
	double AngleOf(const FVector2D& Point, double AtLeastDeg = -360.0);

	/** A round rod on the axis of its frame: a pin, a disc, an end plate. */
	FFormLathe Rod(double Radius, double Bottom, double Top);

	/**
	 * A blade of the class's iris in its open place, drawn with its pin at the origin and the iris's axis 3.745 along -x:
	 * a 110° arc of the middle circle about the axis, 1.250 wide with round ends, running from the pin's end towards +y.
	 */
	FFormProfile IrisBladeOpen();

	/**
	 * Where blade k's outline stands in its body's frame (the pin's, with the stage's axes), shut: turned to its pin's
	 * plan angle, 360°k/7, and on by 52° (towards rising plan angle, so Motion's turn about -z opens it), its top face on
	 * the frame's origin.
	 */
	FMuseeFrame IrisBladeShut(int32 BladeIndex);

	/** The pin frame of blade k of an iris whose top blade's top face is at TopZ: the body's frame (§2 Anchors). */
	FMuseeFrame IrisPinFrame(int32 BladeIndex, double TopZ);

	/** The body of blade k of an iris: sphere.<Iris>.iris.blade.<k+1>. */
	FMuseeKey IrisBladeBody(const TCHAR* Iris, int32 BladeIndex);

	/** The body of dog k and its frame: at its tip's top face, retracted, x inward along its slide, z up (§2 Anchors). */
	FMuseeKey DogBody(int32 DogIndex);
	FMuseeFrame DogFrame(int32 DogIndex);

	/** The names of the pieces that bound the enclosures (the site's list reads them). */
	namespace Pieces
	{
		inline const TCHAR* const ImageSurface = TEXT("image.surface");
		inline const TCHAR* const Throat = TEXT("neck.throat");
		inline const TCHAR* const FlangeBase = TEXT("neck.flange");
		inline const TCHAR* const FlangeWall = TEXT("neck.flange.pocket-wall");
		inline const TCHAR* const FlangePlate = TEXT("neck.flange.plate");

		/** The throat's band between the dogs' housings (0 to 3), each housing's channel and cap, and the neck iris's blades. */
		FString ThroatBand(int32 Index);
		FString HousingChannel(int32 DogIndex);
		FString HousingCap(int32 DogIndex);
		FString NeckBlade(int32 BladeIndex);

		/** The pieces between the neck and the interior: the throat, its band, the flange, the housings and the neck iris. */
		TArray<FName> NeckWall();
	}

	void DescribeSite(FSiteDescription& Out);
	void DrawShells(TArray<FFormDrawing>& Out);
	void DrawNeck(TArray<FFormDrawing>& Out);
	void DrawPlatform(TArray<FFormDrawing>& Out);
	void DrawMast(TArray<FFormDrawing>& Out);
}
