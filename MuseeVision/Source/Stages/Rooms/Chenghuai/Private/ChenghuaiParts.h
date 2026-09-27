#pragma once

#include "ChenghuaiFrame.h"
#include "ChenghuaiPlan.h"
#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "MuseeCollision.h"
#include "MuseeKey.h"

struct FSiteDescription;

/**
 * The words Chenghuai draws its pieces with, all in Form's language: keys and surfaces under the stage, sections as the
 * sheet writes them, and solids set square to a building's frame (a block, a section run along or across the building,
 * an outline raised on the plan, a turned piece, a rod or a bar between two points, a relief over a curve of the
 * building's section). Every piece is one closed solid, finished in one surface of the stage's finish schedule.
 */
namespace Musee::Chenghuai
{
	/** A key under the stage: chenghuai.<Local>. */
	FMuseeKey Key(const FString& Local);

	/** A surface of the stage's finish schedule: chenghuai.<Local>. */
	FName Surface(const TCHAR* Local);

	/**
	 * How a piece collides: as a wall (Solid), as a floor or a tread (Walkable), or not at all, for what the visitor can
	 * never reach (a roof, a rafter, a ridge overhead). Collision is the visible solid itself (§3.10: no invisible guards).
	 */
	enum class EHit : uint8
	{
		None,
		Solid,
		Walkable,
	};

	/** A section through the points, crisp at each unless marked smooth; closed or open; its A scaled (a drum's radii in
	 *  diameters of its column). */
	FFormProfile Section(TConstArrayView<FSectionPoint> Points, bool bClosed, double ScaleA = 1.0);

	/** An open path through points, smooth at every joint: a curve given by its stations. */
	FFormProfile SmoothPath(TConstArrayView<FVector2D> Points);

	/** A circle as a closed profile, drawn exactly. */
	FFormProfile Circle(const FVector2D& Centre, double Radius);

	/** The rectangle between two corners as four points, anticlockwise in (A, B). */
	TArray<FVector2D> Rectangle(double A0, double B0, double A1, double B1);

	/** A regular polygon of Sides corners on a circle, the first at PhaseDegrees from +A towards +B. */
	TArray<FVector2D> RegularPolygon(const FVector2D& Centre, double Radius, int32 Sides, double PhaseDegrees);

	/** The collision profile of a hit class, or none. */
	FName CollisionOf(EHit Hit);

	/** Adds a drawing: named, finished in a surface of the stage, colliding as given, standing in a frame. */
	template <typename TShape>
	FFormDrawing& Draw(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, TShape&& Shape, EHit Hit,
		const FMuseeFrame& Frame = FMuseeFrame())
	{
		FFormDrawing& Drawing = Out.Add_GetRef(FFormDrawing::Of(FName(*Name), Surface(LocalSurface), Forward<TShape>(Shape)));
		Drawing.Collision = CollisionOf(Hit);
		Drawing.Frame = Frame;
		return Drawing;
	}

	/** A block square to a building's frame between two corners in (u, v, z). */
	FFormDrawing& Block(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F, double U0,
		double U1, double V0, double V1, double Z0, double Z1, EHit Hit);

	/** A section drawn in the building's (v, z), with holes, run along u from U0 to U1. */
	FFormDrawing& AlongU(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> SectionVZ, TConstArrayView<TArray<FVector2D>> Holes, double U0, double U1, EHit Hit);

	/** A section drawn in the building's (u, z), with holes, run along v from V0 to V1. */
	FFormDrawing& AlongV(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> SectionUZ, TConstArrayView<TArray<FVector2D>> Holes, double V0, double V1, EHit Hit);

	/** An outline on the building's plan (u, v), with holes, raised from Z0 to Z1. */
	FFormDrawing& Raised(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> OutlineUV, TConstArrayView<TArray<FVector2D>> Holes, double Z0, double Z1, EHit Hit);

	/** A turned piece: a section (radius, height) turned about the vertical through a stage point. */
	FFormDrawing& Turned(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& Foot,
		FFormProfile Section, EHit Hit);

	/** A turned piece whose axis runs from a stage point along a direction (a door pin, a knocker's plate). */
	FFormDrawing& TurnedAbout(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& Foot,
		const FVector& Axis, FFormProfile Section, EHit Hit);

	/** A round rod between two stage points. */
	FFormDrawing& Rod(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& A, const FVector& B,
		double Radius, EHit Hit);

	/** A rectangular bar between two stage points, Width across along a direction square to it and Height the other way. */
	FFormDrawing& Bar(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FVector& A, const FVector& B,
		const FVector& Across, double Width, double Height, EHit Hit);

	/**
	 * A relief over a curve of the building's (v, z) section, running across u from UFrom to UTo: the carving stands to the
	 * curve's right as it is walked (up, for a curve walked towards +v), sampled on Grid, with a block Back deep behind
	 * it. Height(S, U) is the carving at S metres along the curve and at u. It is how Form draws a surface of rows over a
	 * curved bed (tiles, rafters) and a section swept along a curve that is not in plan (a gable's edge).
	 */
	FFormDrawing& ReliefAlongU(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> CurveVZ, double UFrom, double UTo, double Grid, double Back, TFunction<double(double S, double U)> Height);

	/** The same over a curve of the building's (u, z) section, running across v from VFrom to VTo (a scorpion's tail). */
	FFormDrawing& ReliefAlongV(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<FVector2D> CurveUZ, double VFrom, double VTo, double Grid, double Back, TFunction<double(double S, double V)> Height);

	/** A relief over a flat ground square to the building's plan at Z, from (U0, V0) over Size, carving up: Height(U, V). */
	FFormDrawing& ReliefOnPlan(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		double U0, double V0, const FVector2D& Size, double Z, double Grid, double Back, TFunction<double(double U, double V)> Height);

	/** The height of a section's top at A across it, linear between its points; zero outside it. */
	double TopAt(TConstArrayView<FSectionPoint> Top, double A);

	/**
	 * A polyline measured along its length, as a relief's path is: where a distance along it falls. A relief's height
	 * is asked at U metres along its path; this finds the point of the drawing there.
	 */
	struct FMeasuredLine
	{
		TArray<FVector2D> Points;
		TArray<double> Lengths;

		explicit FMeasuredLine(TArray<FVector2D> InPoints);
		double Length() const { return Lengths.IsEmpty() ? 0.0 : Lengths.Last(); }
		FVector2D At(double Distance) const;
	};

	/** The signed area of a loop: positive when it runs anticlockwise in (A, B). */
	double SignedArea(TConstArrayView<FVector2D> Loop);

	/** Whether a point lies inside a loop (either way round). */
	bool Contains(TConstArrayView<FVector2D> Loop, const FVector2D& Point);

	/**
	 * The regions of a rectangle left open by axis-square boxes over it: the rectangle cut on every box's edges into a grid,
	 * the cells no box covers joined, and each joined region's boundary traced with the region on its left (so an outer
	 * boundary runs anticlockwise and the boundary round an island clockwise).
	 */
	TArray<TArray<FVector2D>> OpenRegions(double A0, double A1, double B0, double B1, TConstArrayView<FBox2D> Covered);

	/** Raises each region of traced boundaries on the plan (anticlockwise outlines, each with the clockwise islands inside it). */
	void RaisedRegions(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* LocalSurface, const FBuildingFrame& F,
		TConstArrayView<TArray<FVector2D>> Loops, double Z0, double Z1, EHit Hit);

	/** Points sampled along a function of v between two stations, every Step at most, both ends and every knot included. */
	TArray<double> Stations(double From, double To, double Step, TConstArrayView<double> Knots = {});
}
