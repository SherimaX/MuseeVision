#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"

struct FSiteDescription;

/**
 * The hall's parts by trade, each drawn in Form's language into the description's pieces: the walls and the passage
 * (§3.1, §2), the niches (§3.1), the floors (§3.2), the gallery's barrel and lanterns and the tribune's vaults (§3.3),
 * the order (§3.4), the mouldings, panels and consoles (§3.4), and the carriers of the works (§3.5). The site's
 * declarations (footprint, port, openings, anchors, enclosure) are ClassicalHallSite's.
 */
namespace Musee::ClassicalHall
{
	/** A key under the stage: classical-hall.<Local>. */
	FMuseeKey Key(const FString& Local);

	/** A surface of the stage's finish schedule: classical-hall.<Local>. */
	FName Surface(const TCHAR* Local);

	/** The collision classes the hall's pieces take: walls, columns and carriers block; floors are walked on. */
	inline const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;
	inline const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;

	/** A drawing of the stage, finished in one of its surfaces, colliding as given (none for what is out of reach), in a frame. */
	template <typename TShape>
	FFormDrawing Drawn(const FString& Name, const TCHAR* LocalSurface, TShape&& Shape, const Musee::Collision::FClass* Collision,
		const FMuseeFrame& Frame = FMuseeFrame())
	{
		FFormDrawing Drawing = FFormDrawing::Of(FName(*Name), Surface(LocalSurface), Forward<TShape>(Shape));
		Drawing.Collision = Collision ? Collision->Profile() : NAME_None;
		Drawing.Frame = Frame;
		return Drawing;
	}

	/** The structural masonry's pieces are named shell.<...>: they bound the hall's one enclosure. */
	inline const TCHAR* const ShellPrefix = TEXT("shell.");

	// ---- frames -------------------------------------------------------------------------------------------------------

	/** The stage's frame whose x and y are the given axes (and z their cross product x × y), at an origin. */
	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y);

	/** A frame on the plan at a point, turned about z so its x points along the plan angle φ (degrees from +x towards +y). */
	FMuseeFrame Turned(const FVector& Origin, double Deg);

	/** The unit plan direction of φ (degrees from +x towards +y). */
	FVector Direction(double Deg);

	/** A point at radius R and plan angle φ about the stage's origin. */
	FVector2D Polar(double Radius, double Deg);

	// ---- profiles -----------------------------------------------------------------------------------------------------

	/** A closed polygon, crisp at each corner. */
	FFormProfile Polygon(TConstArrayView<FVector2D> Points);

	/** A closed circle, run clockwise on the plan (so a sweep along it lies inside). */
	FFormProfile Circle(const FVector2D& Centre, double Radius);

	/** The closed rectangle between two corners. */
	FFormProfile Rect(double MinA, double MinB, double MaxA, double MaxB);

	/** The same closed rectangle drawn the other way round, so a moulding swept along it faces out. */
	FFormProfile RectOutward(double MinA, double MinB, double MaxA, double MaxB);

	/** An annular sector between two radii about a centre, from one angle to another (degrees, increasing). */
	FFormProfile AnnularSector(const FVector2D& Centre, double Inner, double Outer, double FromDeg, double ToDeg);

	/**
	 * The pen the sheet's profiles are written with: straight lines, quarter rounds and cymas, from point to point.
	 * "Round to P" is a quarter circle on the chord (convex bulges to the right of the travel, away from the solid on
	 * its left; concave to the left); "cyma to P" is two such rounds meeting at the chord's midpoint, the first convex
	 * for a cyma reversa ("convex first"), concave for a cyma recta.
	 */
	class FPen
	{
	public:
		explicit FPen(const FVector2D& Start);
		FPen(double A, double B) : FPen(FVector2D(A, B)) {}

		FPen& To(double A, double B);
		FPen& Round(double A, double B, bool bConvex);
		FPen& Cyma(double A, double B, bool bConvexFirst);
		/** An arc about a centre, from one angle to another (degrees), joined by a line where it starts elsewhere. */
		FPen& Arc(const FVector2D& Centre, double Radius, double FromDeg, double ToDeg);
		/** An elliptical arc about a centre, radii along A and B, from one parametric angle to another (degrees). */
		FPen& Ellipse(const FVector2D& Centre, double RadiusA, double RadiusB, double FromDeg, double ToDeg);

		FVector2D At() const { return Here; }
		FFormProfile Closed() const;
		FFormProfile Open() const;

	private:
		FFormProfile Profile;
		FVector2D Here;
	};

	// ---- solids -------------------------------------------------------------------------------------------------------

	/** An extrusion of an outline (and its holes) pushed Depth along its frame's z. */
	FFormExtrusion Prism(FFormProfile Outline, double Depth, TArray<FFormProfile> Holes = {});

	/** A plan prism between two heights: the outline in the stage's plan, its frame raised to the lower. */
	FFormDrawing PlanPrism(const FString& Name, const TCHAR* LocalSurface, FFormProfile Outline, TArray<FFormProfile> Holes, double Bottom,
		double Top, const Musee::Collision::FClass* Collision);

	/** A plan prism in a frame: the outline in the frame's (x, y), from Bottom to Top along its z. */
	FFormDrawing PrismIn(const FString& Name, const TCHAR* LocalSurface, const FMuseeFrame& Frame, FFormProfile Outline, double Bottom,
		double Top, const Musee::Collision::FClass* Collision);

	// ---- the gallery's barrel -----------------------------------------------------------------------------------------

	/**
	 * The barrel's section, (x, z): a point s along its face (from the crown, + east) at radius Rho about its axis
	 * (x 0, z 1.3216). A frame whose x is the stage's x and whose y is up draws in this section, its z pointing north.
	 */
	FVector2D Barrel(double S, double Rho);

	/** The angle (degrees from +x towards up) of the point s along the face. */
	double BarrelDeg(double S);

	/** The frame of the barrel's section at a y: x east, y up, z north, so a prism pushed along it runs north from there. */
	FMuseeFrame BarrelFrame(double Y);

	// ---- the rosettes -------------------------------------------------------------------------------------------------

	/**
	 * How far a rosette of a radius stands above the coffer's back at a distance from its centre and an angle round it:
	 * the turned section of §3.3 at the rosette's size, its eight petals swelling the outer rows; zero off the rosette.
	 */
	double RosetteHeight(double Distance, double Around, double Radius);

	/** A cast rosette on a coffer's back: a plane relief on its own square bed, 2 mm behind the back, facing the room. */
	FFormDrawing Rosette(const FString& Name, const TCHAR* LocalSurface, const FVector& Back, const FVector& Facing, const FVector& Across,
		double Radius);

	// ---- the niches ---------------------------------------------------------------------------------------------------

	/**
	 * A niche, set out on its mouth plane: the mouth's centre on the floor, the direction into the wall, the half-round's
	 * radius and its springing. The gallery's stand on the wall's plane x ±3.00, the tribune's on the plane tangent to
	 * r 6.60 on their axes.
	 */
	struct FNiche
	{
		FString Name;
		FVector Origin;
		FVector In;
		double Radius = 0.0;
		double Springing = 0.0;
		bool bTribune = false;

		/** Along the mouth plane, to the right looking into the niche from above: Up × In. */
		FVector Lateral() const;
		/** The niche's plan frame: x into the wall, y lateral, z up, at the mouth's centre on the floor. */
		FMuseeFrame PlanFrame() const;
		/** The niche's elevation frame at a depth into the wall: x lateral, y up, z into the wall. */
		FMuseeFrame WallFrame(double Depth) const;
		/** The key of its Mount anchor's local part: gallery.niche.e1, tribune.niche.n. */
		FString AnchorLocal() const;
		/** The flat bay it is cut in reaches this far either side of its axis. */
		double BayHalf() const;
	};

	/** The thirteen niches: E1…E4, W1…W4 south to north, then the tribune's N, NE, NW, SE, SW. */
	const TArray<FNiche>& Niches();

	/** The niche a carrier's anchor stands in, or null. */
	const FNiche* NicheOf(const TCHAR* AnchorLocal);

	// ---- the trades ---------------------------------------------------------------------------------------------------

	void DrawWalls(TArray<FFormDrawing>& Out);
	void DrawNiches(TArray<FFormDrawing>& Out);
	void DrawFloors(TArray<FFormDrawing>& Out);
	void DrawGalleryVault(TArray<FFormDrawing>& Out);
	void DrawTribuneVaults(TArray<FFormDrawing>& Out);
	void DrawOrder(TArray<FFormDrawing>& Out);
	void DrawEntablatures(TArray<FFormDrawing>& Out);
	void DrawMouldings(TArray<FFormDrawing>& Out);
	void DrawCarriers(TArray<FFormDrawing>& Out);
	void DescribeSite(FSiteDescription& Out);
}
