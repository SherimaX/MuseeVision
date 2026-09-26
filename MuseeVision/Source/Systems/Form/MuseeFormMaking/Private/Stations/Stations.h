#pragma once

#include "CoreMinimal.h"

/**
 * Stations: the places where a curve is divided into straight chords. Every curve in the museum is divided by one
 * number, the chord tolerance: no chord departs from its curve by more than that distance, whatever the radius. A
 * station list is a pure function of the curve, the tolerance and the stations it must include, so two surfaces
 * that meet along a curve and take the same list meet vertex for vertex, with no cracks and no T-junctions (the old
 * Rotunda's MakeStations, windows/Source/MuseeVision/Rotunda/RotundaStructure.cpp, is the model).
 *
 * Lists run from the first argument to the second, both included, in either direction.
 */
namespace Musee::Form
{
	/**
	 * The museum's chord tolerance, 1 mm, used when a caller passes none. Form owns this one number
	 * (Docs/Architecture.md, Form); Studio passes the recorded value.
	 */
	inline constexpr double DefaultChordToleranceMeters = 0.001;

	/**
	 * No chord turns more than a right angle, so a curve smaller than the tolerance still reads as turning and a full
	 * circle keeps at least four sides.
	 */
	inline constexpr double MaxStationStepRadians = UE_DOUBLE_HALF_PI;

	/** How far the chord of an arc of this radius and step departs from the arc at its middle (its sagitta), in metres. */
	double ArcChordError(double RadiusMeters, double StepRadians);

	/** The fewest equal chords that keep an arc of this radius and sweep within the tolerance. */
	int32 ArcSegmentCount(double RadiusMeters, double SweepRadians, double ToleranceMeters);

	/**
	 * Stations (angles, radians) on a circular arc. Every required angle between the ends is a station, and each gap
	 * between required stations is divided into the fewest equal steps that keep it within the tolerance. Surfaces of
	 * different radii that must share stations pass the largest radius among them.
	 */
	TArray<double> ArcStations(double RadiusMeters, double FromRadians, double ToRadians, double ToleranceMeters,
		TConstArrayView<double> RequiredRadians = {});

	/**
	 * Stations (parameters t) on the elliptical arc (RadiusA cos t, RadiusB sin t). A circle takes ArcStations; an
	 * ellipse takes equal parameter steps bounded by its largest radius (see CurveStations).
	 */
	TArray<double> EllipseStations(double RadiusAMeters, double RadiusBMeters, double FromRadians, double ToRadians,
		double ToleranceMeters);

	/**
	 * Stations on a smooth parametric curve whose second derivative never exceeds SecondDerivativeBound (metres per
	 * unit of parameter squared). A chord over a parameter step h departs from such a curve by at most
	 * h^2 * bound / 8 (the error bound of linear interpolation), so each gap between required stations takes the
	 * fewest equal steps that keep that bound within the tolerance. A bound of zero (a straight line) takes one step.
	 */
	TArray<double> CurveStations(double From, double To, double SecondDerivativeBound, double ToleranceMeters,
		TConstArrayView<double> Required = {});

	/**
	 * The largest distance, in metres, between a curve and its chords between consecutive stations, found by
	 * sampling each span SamplesPerSpan times. A measurement for checks and tests, not a way to place stations.
	 */
	double MeasureChordError(TFunctionRef<FVector(double)> Curve, TConstArrayView<double> Stations, int32 SamplesPerSpan);
}
