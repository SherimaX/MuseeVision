#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "GroundsPlan.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"

struct FSiteDescription;

/**
 * The Grounds' parts by trade, each drawn in Form's language into the description's pieces: the terrain (§3.2), the
 * parvis's paving (§3.3), the gravel walks (§3.4), the strip round the stone complex and its edge (§3.5), the kerbs
 * (§3.5), the basins' copings and floors (§3.6) and the lamp standards (§3.7). The lawn, trees, hedges and water are
 * Nature's, from Planting.json; the lamps' light is Light's, from Lighting.json. The site's declarations (footprint,
 * anchors, enclosure) are GroundsSite's.
 */
namespace Musee::Grounds
{
	/** A key under the stage: grounds.<Local>. */
	[[nodiscard]] FMuseeKey Key(const FString& Local);

	/** A surface of the stage's finish schedule: grounds.<Local>. */
	[[nodiscard]] FName Surface(const TCHAR* Local);

	/** A drawing of the stage, finished in one of its surfaces, colliding as given, standing in a frame. */
	template <typename TShape>
	[[nodiscard]] FFormDrawing Drawn(const FString& Name, const TCHAR* LocalSurface, TShape&& Shape, const Musee::Collision::FClass* Collision,
		const FMuseeFrame& Frame = FMuseeFrame())
	{
		FFormDrawing Drawing = FFormDrawing::Of(FName(*Name), Surface(LocalSurface), Forward<TShape>(Shape));
		Drawing.Collision = Collision ? Collision->Profile() : NAME_None;
		Drawing.Frame = Frame;
		return Drawing;
	}

	/** A point of the plan, as a vector. */
	[[nodiscard]] inline FVector2D At(const Plan::FPlanPoint& Point) { return FVector2D(Point.X, Point.Y); }

	/** The stage's frame whose x and y are the given axes (and z their cross product), at an origin. */
	[[nodiscard]] FMuseeFrame FrameOfAxes(const FVector& OriginMeters, const FVector& X, const FVector& Y);

	/** A closed polygon of points, as a profile with a crisp corner at each. */
	[[nodiscard]] FFormProfile Polygon(TConstArrayView<FVector2D> Points);

	/** The closed section drawn through a sheet's points, crisp at each. */
	[[nodiscard]] FFormProfile Section(TConstArrayView<Plan::FPlanPoint> Points);

	/** The rectangle between two corners in plan. */
	[[nodiscard]] FFormProfile Rectangle(const FVector2D& Min, const FVector2D& Max);

	/** The circle about a centre in plan. */
	[[nodiscard]] FFormProfile Circle(const FVector2D& Centre, double Radius);

	/** The point at a plan angle (degrees, from east towards south) on a circle about a centre. */
	[[nodiscard]] FVector2D OnCircle(const FVector2D& Centre, double Radius, double Degrees);

	/** The plan angle (degrees) of a point seen from a centre, in (-180, 180]. */
	[[nodiscard]] double AngleOf(const FVector2D& Centre, const FVector2D& Point);

	/**
	 * A laid piece: an outline and its holes in plan, pushed from one level up to another, in a frame raised to the lower:
	 * the soil, a slab, a walk's gravel, a basin's floor.
	 */
	[[nodiscard]] FFormDrawing Laid(const FString& Name, const TCHAR* LocalSurface, FFormProfile Outline, TArray<FFormProfile> Holes,
		double Bottom, double Top, const Musee::Collision::FClass* Collision);

	/** The lamp standards' bases (r 0.27) in plan: the holes a walk's gravel leaves round them. */
	[[nodiscard]] TArray<FFormProfile> LampBasesWithin(const FBox2D& Region);

	void DrawTerrain(TArray<FFormDrawing>& Out);
	void DrawPaving(TArray<FFormDrawing>& Out);
	void DrawGravel(TArray<FFormDrawing>& Out);
	void DrawStrip(TArray<FFormDrawing>& Out);
	void DrawKerbs(TArray<FFormDrawing>& Out);
	void DrawBasins(TArray<FFormDrawing>& Out);
	void DrawLamps(TArray<FFormDrawing>& Out);
	void DescribeSite(FSiteDescription& Out);

	/** The pieces the site's declarations name. */
	namespace Pieces
	{
		inline const TCHAR* const Terrain = TEXT("terrain");
	}
}
