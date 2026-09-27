#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"
#include "SalonPlan.h"

struct FSiteDescription;

/**
 * The Salon's parts by trade, each drawn in Form's language into the description's pieces: the walls and silk (§3.1),
 * the piers and arches (§3.2), the mouldings (§3.3), the vault and its rosettes (§3.4), the lanterns, roof and pergola
 * (§3.4), the Manet cabinet (§3.1, §3.5), the floors (§3.6) and the fittings (§3.7). The site's declarations (footprint,
 * ports, openings, anchors, enclosures) are SalonSite's.
 */
namespace Musee::Salon
{
	/** A key under the stage: salon.<Local>. */
	FMuseeKey Key(const TCHAR* Local);

	/** A surface of the stage's finish schedule: salon.<Local>. */
	FName Surface(const TCHAR* Local);
	FName Surface(const FString& Local);

	/** The collision classes the pieces take. */
	inline const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;
	inline const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;

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

	/** The stage's frame whose x and y are the given axes (and z their cross product), at an origin. */
	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y);

	/**
	 * The frame of a face whose normal N looks into the room: z along N, y up, x = up × N (along the face, to the left
	 * as the room sees it). A lathe in it turns from x over the top; an outline in it is drawn on the face.
	 */
	FMuseeFrame FaceFrame(const FVector& Origin, const FVector& N);

	/** A closed polygon through points, crisp at each. */
	FFormProfile Polygon(TConstArrayView<FVector2D> Points);

	/** A section of the plan's steps, closed, moved by an offset in (A, B). */
	FFormProfile Section(TConstArrayView<Plan::FStep> Steps, const FVector2D& Offset = FVector2D::ZeroVector);

	/** A box between two corners of the stage's frame. */
	FFormDrawing Box(const FString& Name, const TCHAR* LocalSurface, const FVector& Min, const FVector& Max,
		const Musee::Collision::FClass* Collision);

	/** A slab of an outline and its holes in plan, between two heights. */
	FFormDrawing PlanSlab(const FString& Name, const TCHAR* LocalSurface, FFormProfile Outline, TArray<FFormProfile> Holes, double Bottom,
		double Top, const Musee::Collision::FClass* Collision);

	/** A circle as a closed profile. */
	FFormProfile Circle(const FVector2D& Centre, double Radius);

	/**
	 * An arched opening's outline on a face, (u across, v up): jambs at ±Half from the bottom to the springing, a
	 * semicircular head over them; drawn anticlockwise.
	 */
	FFormProfile ArchedOutline(double Half, double Springing, double Bottom);

	/**
	 * A band round an arched opening on a face, between the opening (Inner half-width) and a parallel Outer half-width, from
	 * the bottom up round the head: a door's lining, or the notch the silk leaves round it.
	 */
	FFormProfile ArchedBand(double Inner, double Outer, double Springing, double Bottom);

	/** A slab drawn on a face: its outline in the face's frame, pushed along the face's normal by the depth. */
	FFormDrawing FaceSlab(const FString& Name, const TCHAR* LocalSurface, const FMuseeFrame& Face, FFormProfile Outline, TArray<FFormProfile> Holes,
		double Depth, const Musee::Collision::FClass* Collision);

	/**
	 * A moulding round an arched opening in a face (P5): an extrusion up each jamb from the floor to the springing and a
	 * lathe over the head about the springing's centre, the section's A out from the opening's edge and B out of the face.
	 */
	void ArchedTrim(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& FaceCentre, const FVector& N,
		double Half, double Springing, TConstArrayView<Plan::FStep> Steps);

	/** The x of coffer column line C (0 to 8) of a bay: the plain bands, then the columns at the bay's pitch. */
	double ColumnX(int32 Bay, int32 C);

	/** The angle of row line K (0 to 19) of the vault from the north springing, radians. */
	double RowAngle(int32 K);

	/** A point of the barrel at angle Phi from the north springing and radius R, at x. */
	FVector VaultPoint(double X, double Phi, double R);

	/** The lantern's rectangle in plan of a bay: its columns 3 to 5 across the crown row. */
	FBox2D Lantern(int32 Bay);

	/** Names of pieces that more than one part draws or reads (the site's enclosures name the rest as the parts draw them). */
	namespace Pieces
	{
		inline const TCHAR* const Roof = TEXT("roof");
		inline const TCHAR* const CabinetRoof = TEXT("cabinet.roof");
		inline const TCHAR* const CabinetCeiling = TEXT("cabinet.ceiling");
		inline const TCHAR* const CabinetWall = TEXT("cabinet.wall");
		inline const TCHAR* const CabinetFloor = TEXT("floor.cabinet");
		inline const TCHAR* const NorthWallCabinet = TEXT("wall.north.cabinet");
	}

	void DrawWalls(TArray<FFormDrawing>& Out);
	void DrawPiers(TArray<FFormDrawing>& Out);
	void DrawMouldings(TArray<FFormDrawing>& Out);
	void DrawVault(TArray<FFormDrawing>& Out);
	void DrawRosettes(TArray<FFormDrawing>& Out);
	void DrawLanterns(TArray<FFormDrawing>& Out);
	void DrawRoof(TArray<FFormDrawing>& Out);
	void DrawCabinet(TArray<FFormDrawing>& Out);
	void DrawFloors(TArray<FFormDrawing>& Out);
	void DrawFittings(TArray<FFormDrawing>& Out);
	void DescribeSite(FSiteDescription& Out);
}
