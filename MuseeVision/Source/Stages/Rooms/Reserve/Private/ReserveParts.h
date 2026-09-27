#pragma once

#include "CoreMinimal.h"
#include "FormAxes.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"

struct FSiteDescription;

/**
 * The undercroft's parts by trade, each drawn in Form's language into the description's pieces: the shell (the walls
 * with the west door and its architrave, the floor's bed and the box's top, §3.1), the floor (the bluestone flags and
 * the flush tracks, §3.2), the piers and responds (§3.3), the vaults (§3.4), the overhead tracks (§3.6), the 36 racks
 * (§3.7), the luminaires' bodies (§3.8), and the fittings (the easel and the plan chests, §3.9). The site's
 * declarations (footprint, port, opening, anchors, enclosure) are ReserveSite's.
 */
namespace Musee::Reserve
{
	/** Frames set out by their axes are Form's (FormAxes.h). */
	using Musee::Form::FrameOfAxes;

	/** A key under the stage: reserve.<Local>. */
	FMuseeKey Key(const TCHAR* Local);

	/** A surface of the stage's finish schedule: reserve.<Local>. */
	FName Surface(const TCHAR* Local);

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

	/** A box between two corners, as a plan extrusion. */
	FFormExtrusion Block(const FVector& Min, const FVector& Max);

	/** A round bar's section of a diameter, about its path. */
	FFormProfile Round(double Diameter);

	/** A closed section turned on a lathe from points (radius, height), the last joined back to the first. */
	FFormLathe Turned(TConstArrayView<FVector2D> Points, int32 Sides = 0);

	/** A round rod or disc: a cylinder of a diameter about the frame's z, from one height to another. */
	FFormLathe Cylinder(double Diameter, double From, double To);

	/** The frame at a point whose z runs along a direction (for a turned or extruded part set along it). */
	FMuseeFrame AlongAxis(const FVector& Origin, const FVector& Direction);

	/** The piers' lines, west to east: x -22.50 + 7.50 i, i from 0 to 6. */
	double PierX(int32 Line);

	/** Bay b's clear span (0 to 7, west to east): from the end wall or a pier line's band to the next. */
	double BayFrom(int32 Bay);
	double BayTo(int32 Bay);
	double BayCentre(int32 Bay);
	double BayHalf(int32 Bay);

	/** One of the three rows of vaults: the north aisle, the nave, the south aisle, by its middle, half-width and rise. */
	struct FRow
	{
		const TCHAR* Name;
		double Middle;
		double Half;
		double Rise;
	};
	TConstArrayView<FRow> Rows();

	/**
	 * The vault's soffit above a plan point inside the walls (§3.4): a web's intrados, 4.06 plus the higher of its two
	 * barrels; a cross band's or an arcade band's, 4.00 plus its barrel; the springing plane over a pier.
	 */
	double Soffit(double X, double Y);

	/**
	 * The highest point of the soffit over a disc: where a canopy's flat top is seated, so it bears all round and is let
	 * into the brick where the soffit falls away under it.
	 */
	double SeatOver(double X, double Y, double Radius);

	/** One rack (§3.7): its letter or number as its keys name it (a, n07, s01), its x, and its side (-1 north, +1 south). */
	struct FRack
	{
		FString Name;
		double X = 0.0;
		int32 Side = 0;

		/** Its body key, reserve.screen-<name>, and a key or piece name under it. */
		FMuseeKey Body() const;
		FString Piece(const TCHAR* Part) const;

		/** Its home centre on the aisle's side, and the direction it is drawn out: towards the nave. */
		double HomeY() const;
		double TowardsNave() const { return double(-Side); }
	};
	TArray<FRack> Racks();

	/** The names of the pieces that bound the undercroft (the site's list reads them). */
	namespace Pieces
	{
		inline const TCHAR* const Walls = TEXT("walls");
		inline const TCHAR* const Bed = TEXT("floor.bed");
		inline const TCHAR* const Top = TEXT("top");

		/** The vaults' masonry: the webs, the cross and arcade bands and the springing blocks over the piers. */
		TArray<FName> Vaults();
	}

	void DrawShell(TArray<FFormDrawing>& Out);
	void DrawFloors(TArray<FFormDrawing>& Out);
	void DrawPiers(TArray<FFormDrawing>& Out);
	void DrawVaults(TArray<FFormDrawing>& Out);
	void DrawTracks(TArray<FFormDrawing>& Out);
	void DrawRacks(TArray<FFormDrawing>& Out);
	void DrawLuminaires(TArray<FFormDrawing>& Out);
	void DrawFittings(TArray<FFormDrawing>& Out);
	void DescribeSite(FSiteDescription& Out);
}
