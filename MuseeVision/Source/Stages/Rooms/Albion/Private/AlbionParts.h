#pragma once

#include "AlbionPlan.h"
#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"

struct FSiteDescription;

/**
 * The court's parts by trade, each drawn in Form's language into the description's pieces: the masonry inside
 * (§3.1) and outside with the porch (§3.2), the floors (§3.3), the columns (§3.4), the iron and lead (§3.5), the glass
 * (§3.6), the porch's ceiling and roof (§3.7) and the fittings (§3.8). The site's declarations (footprint, port,
 * openings, anchors, enclosure) are AlbionSite's.
 */
namespace Musee::Albion
{
	inline const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;
	inline const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;
	/** Nothing collides with it: the iron, glass and lead overhead, out of reach (§3.5). */
	inline const Musee::Collision::FClass* const Overhead = nullptr;

	/** A key under the stage: albion.<Local>. */
	FMuseeKey Key(const FString& Local);

	/** A surface of the stage's finish schedule: albion.<Local>. */
	FName Surface(const FString& Local);

	/** A drawing of the stage, finished in one of its surfaces, colliding as given, standing in a frame. */
	template <typename TShape>
	FFormDrawing Drawn(const FString& Name, const FString& LocalSurface, TShape&& Shape, const Musee::Collision::FClass* Collision,
		const FMuseeFrame& Frame = FMuseeFrame())
	{
		FFormDrawing Drawing = FFormDrawing::Of(FName(*Name), Surface(LocalSurface), Forward<TShape>(Shape));
		Drawing.Collision = Collision ? Collision->Profile() : NAME_None;
		Drawing.Frame = Frame;
		return Drawing;
	}

	/** The same drawing, cast as glass: Nanite can't render it. */
	FFormDrawing Glass(FFormDrawing Drawing);

	// ---- frames ---------------------------------------------------------------------------------------------------------------
	/** The stage's frame whose x and y are the given axes (and z their cross product), at an origin. */
	FMuseeFrame FrameOfAxes(const FVector& Origin, const FVector& X, const FVector& Y);

	/** The vertical plane x = X, drawn in (y, z) and pushed east: a side wall's leaf, an arcade's arch. */
	FMuseeFrame AcrossX(double X);

	/** The vertical plane y = Y, drawn in (x, z) and pushed north: an end wall's leaf, a vault's section. */
	FMuseeFrame AcrossY(double Y);

	/** A frame at From whose z runs towards To and whose y is as near Up as it can be: a bar's section is drawn in its (x, y). */
	FMuseeFrame Along(const FVector& From, const FVector& To, const FVector& Up = FVector::UpVector);

	/** A frame on a face: its origin on the face, its z the face's outward normal, its y up, its x = up × normal. */
	FMuseeFrame OnFace(const FVector& Origin, const FVector& Normal);

	// ---- profiles --------------------------------------------------------------------------------------------------------------
	/** A closed polygon, crisp at every corner. */
	FFormProfile Polygon(TConstArrayView<FVector2D> Points);

	/** A closed section through points, run anticlockwise whichever way they were given, so its solid is on its left. */
	FFormProfile Section(TConstArrayView<FVector2D> Points);

	/** A section from the plan's (a, b) pairs, run anticlockwise. */
	template <SIZE_T N>
	FFormProfile SectionOf(const double (&Points)[N][2])
	{
		TArray<FVector2D> Out;
		for (const auto& Point : Points)
		{
			Out.Add(FVector2D(Point[0], Point[1]));
		}
		return Section(Out);
	}

	/** A circle, drawn anticlockwise (t increasing), so a section's solid is inside it. */
	FFormProfile Circle(const FVector2D& Centre, double Radius);

	/** A turned section (r, z) through the plan's points, smooth where the sheet marks them, raised by Lift. */
	FFormProfile Turned(TConstArrayView<Plan::FTurnPoint> Points, double Lift = 0.0);

	// ---- simple solids -----------------------------------------------------------------------------------------------------------
	/** A box between two corners of the stage, as a plan extrusion. */
	FFormDrawing Box(const FString& Name, const FString& LocalSurface, const FVector& Min, const FVector& Max,
		const Musee::Collision::FClass* Collision);

	/** A straight member from one point to another, its section drawn in the Along frame's (x, y). */
	FFormDrawing Bar(const FString& Name, const FString& LocalSurface, const FVector& From, const FVector& To, FFormProfile Profile,
		const Musee::Collision::FClass* Collision, const FVector& Up = FVector::UpVector);

	/** A rectangular member, Wide across (the frame's x) and High (its y, as near Up as it can be), centred on the line. */
	FFormDrawing Beam(const FString& Name, const FString& LocalSurface, const FVector& From, const FVector& To, double Wide, double High,
		const Musee::Collision::FClass* Collision, const FVector& Up = FVector::UpVector);

	/** A round bar from one point to another. */
	FFormDrawing Rod(const FString& Name, const FString& LocalSurface, const FVector& From, const FVector& To, double Radius,
		const Musee::Collision::FClass* Collision);

	/** A turned solid on a vertical axis at a plan point. */
	FFormDrawing Lathe(const FString& Name, const FString& LocalSurface, const FVector& Foot, FFormProfile Profile,
		const Musee::Collision::FClass* Collision, double FromDegrees = 0.0, double ToDegrees = 360.0);

	/** A ball. */
	FFormDrawing Ball(const FString& Name, const FString& LocalSurface, const FVector& Centre, double Radius,
		const Musee::Collision::FClass* Collision);

	/** A ring of round bar in the (x, y) plane of a frame, about its origin. */
	FFormDrawing Ring(const FString& Name, const FString& LocalSurface, const FMuseeFrame& Frame, double RingRadius, double Tube,
		const Musee::Collision::FClass* Collision);

	// ---- openings --------------------------------------------------------------------------------------------------------------
	/**
	 * An opening's outline on a face, in the face's (u, z): straight jambs Half either side of the axis from the sill to
	 * the springing, then a head of two arcs of Radius whose centres lie Offset across the axis (a two-centred pointed
	 * arch; Offset zero is a semicircle), or a flat head at the springing when Radius is zero. It draws itself into a
	 * profile piece by piece, so the masonry round it can follow it exactly.
	 */
	struct FArch
	{
		double Centre = 0.0;
		double Half = 0.0;
		double Sill = 0.0;
		double Springing = 0.0;
		double Radius = 0.0;
		double Offset = 0.0;

		/** A two-centred arch through its springing points and a crown. */
		static FArch Pointed(double InCentre, double InHalf, double InSill, double InSpringing, double InCrown);
		/** A two-centred arch of a radius to a head. */
		static FArch PointedToHead(double InCentre, double InHalf, double InSill, double InHead, double InRadius);
		/** A semicircular head. */
		static FArch Round(double InCentre, double InHalf, double InSill, double InSpringing);

		double Crown() const;
		/** The half-width at a height between the sill and the crown. */
		double HalfAt(double Z) const;
		/** The same outline grown by a distance round its jambs and head (the arcs about the same centres). */
		FArch Grown(double By) const;

		/** From the left jamb at Z0 up it to Z1. */
		void UpLeft(FFormProfile& P, double Z0, double Z1) const;
		/** From the left side at Z1 down it to Z0. */
		void DownLeft(FFormProfile& P, double Z1, double Z0) const;
		/** From the right jamb at Z0 up it to Z1. */
		void UpRight(FFormProfile& P, double Z0, double Z1) const;
		/** From the right side at Z1 down it to Z0. */
		void DownRight(FFormProfile& P, double Z1, double Z0) const;
		/** From the left side at ZLeft up over the head and down the right side to ZRight. */
		void OverHead(FFormProfile& P, double ZLeft, double ZRight) const;
		/** The whole outline, closed: a hole. */
		FFormProfile Outline() const;
		/** The head and jambs as an open path from the left jamb at Z round to the right jamb at Z (a hood's line). */
		FFormProfile Head(double Z) const;
	};

	/** A side wall's lancet in bay 1 to 6, on the wall's face in (y, z) (§3.1). */
	FArch Lancet(int32 Bay);

	/** The court door in the north wall, in (x, z) (§3.1). */
	FArch CourtDoor();

	/** The port's rough opening through the porch's end lining, in (x, z): the class's opening grown by its allowance (§2). */
	FArch RoughOpening();

	// ---- the vaults --------------------------------------------------------------------------------------------------------------
	/**
	 * One half of a vault in the transverse section (x, z): an arc from its springing on the arcade line or the wall up to
	 * the crown, whose centre lies across the crown's axis. The glass lies on the arc; everything under it is set out by
	 * its depth under the arc along the arc's radius, and by the arc length s from the springing.
	 */
	struct FVaultHalf
	{
		/** Its name among the six: nave-w, nave-e, west-w, west-e, east-w, east-e. */
		const TCHAR* Name = TEXT("");
		/** Which vault it is: the nave's or an aisle's (their glass differs, §3.6). */
		bool bNave = false;
		double SpringX = 0.0;
		double CrownX = 0.0;
		double CrownZ = 0.0;

		double Radius() const;
		double CentreX() const;
		/** +1 when the crown lies east of the springing, -1 when west. */
		double Sign() const;
		double SpringDeg() const;
		double CrownDeg() const;
		/** The arc's length from the springing to the crown. */
		double Length() const;
		/** The angle, degrees, s along the arc from the springing. */
		double DegAt(double S) const;
		/** The arc length where the arc reaches a height. */
		double SAtHeight(double Z) const;
		/** The point s along the arc, Depth under it along its radius. */
		FVector2D At(double S, double Depth = 0.0) const;
		/** The outward normal s along the arc. */
		FVector2D Normal(double S) const;
		/** Where the panes break: from 10.10 to the crown in equal lengths of about 0.60 of arc (17 a nave half, 12 an aisle half). */
		TArray<double> PaneStations() const;
		/** The arc Depth under the glass from S0 to S1, run clockwise about its centre so its centre lies to its right. */
		FFormProfile Arc(double Depth, double S0, double S1) const;
		/** A path through points s along the arc (or under it), ordered so the arc's centre lies to its right; smooth or crisp. */
		FFormProfile Path(TArray<FVector2D> Points, bool bSmooth = false) const;
	};

	/** The six halves: the nave's west and east, the west aisle's, the east aisle's. */
	TConstArrayView<FVaultHalf> VaultHalves();

	/** The height of the vaults' glass at x (the Lighting schedule's glass(x)). */
	double GlassZ(double X);

	void DescribeSite(FSiteDescription& Out);
	void DrawMasonry(TArray<FFormDrawing>& Out);
	void DrawOutside(TArray<FFormDrawing>& Out);
	void DrawPorch(TArray<FFormDrawing>& Out);
	void DrawFloors(TArray<FFormDrawing>& Out);
	void DrawColumns(TArray<FFormDrawing>& Out);
	void DrawArcade(TArray<FFormDrawing>& Out);
	void DrawVaults(TArray<FFormDrawing>& Out);
	void DrawRoof(TArray<FFormDrawing>& Out);
	void DrawScreens(TArray<FFormDrawing>& Out);
	void DrawLancets(TArray<FFormDrawing>& Out);
	void DrawRainwater(TArray<FFormDrawing>& Out);
	void DrawFittings(TArray<FFormDrawing>& Out);
}
