#pragma once

#include "CoreMinimal.h"
#include "FormAxes.h"
#include "FormProfile.h"
#include "MuseeFrame.h"

/**
 * The oval's setting-out, as a mason's rod would carry it: an ellipse about the stage's origin and its parallels (the
 * curves a fixed distance off it along its normal: the wall's outer face, the coping's die, the tray's rim), with
 * arc lengths along any parallel, U(t, d) = s(t) + d·θ(t), s the ellipse's own arc length and θ its normal's turning, so
 * a distance measured on one parallel finds its point on another radially (the sheet's §3.5).
 */
namespace Musee::Nympheas
{
	/** Frames set out by their axes are Form's (FormAxes.h). */
	using Musee::Form::FrameOfAxes;

	class FOvalCurve
	{
	public:
		/** The ellipse x = A cos t, y = B sin t (t from east towards south), metres. */
		FOvalCurve(double InA, double InB);

		double A() const { return SemiA; }
		double B() const { return SemiB; }

		FVector2D Point(double T) const;

		/** The unit normal out of the ellipse at T. */
		FVector2D Outward(double T) const;

		/** The point D out along the normal from the ellipse at T (inward for negative D). */
		FVector2D Parallel(double T, double D) const { return Point(T) + Outward(T) * D; }

		/** The length of the whole parallel at D. */
		double Length(double D) const;

		/** The arc length along the parallel at D from t = 0 to T, T in [0, 2π]. */
		double ArcAt(double T, double D) const;

		/** The parameter where the arc length along the parallel at D reaches U, U taken round whole turns. */
		double TAt(double U, double D) const;

		/** The parameter in [Low, High] where the parallel at D crosses the line y = Y, found by bisection. */
		double TWhereY(double Y, double D, double Low, double High) const;

		/** The parameter in [Low, High] where the parallel at D crosses the line x = X, found by bisection. */
		double TWhereX(double X, double D, double Low, double High) const;

		/** The parallel at D sampled at Stations even steps of t from From to To, both ends included. */
		TArray<FVector2D> Stations(double D, double From, double To, int32 Stations) const;

	private:
		/** The normal's direction at T, unwrapped so it runs from 0 to 2π with t. */
		double Turning(double T) const;

		double SemiA;
		double SemiB;
		/** The ellipse's arc length at even steps of t round one turn. */
		TArray<double> ArcTable;
	};

	/** An open path through points, smooth at every joint: a curve given by its stations. */
	FFormProfile SmoothPath(TConstArrayView<FVector2D> Points);

	/** A uniform Catmull-Rom spline through points, Subdivisions steps per span, the ends' tangents from their spans. */
	TArray<FVector2D> CatmullRom(TConstArrayView<FVector2D> Points, int32 Subdivisions);

	/** The length of a polyline. */
	double PolylineLength(TConstArrayView<FVector2D> Points);

	/** The stretch of a polyline between two distances along it, cut at both. */
	TArray<FVector2D> PolylineBetween(TConstArrayView<FVector2D> Points, double From, double To);

	/** The point and unit direction a distance along a polyline. */
	void PolylineAt(TConstArrayView<FVector2D> Points, double Distance, FVector2D& OutPoint, FVector2D& OutDirection);
}
