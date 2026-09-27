#pragma once

#include "CoreMinimal.h"
#include "FormArris.h"
#include "FormProfile.h"

/** One course of a coursed laying: a row of units of one height, their lengths in turn. */
struct FFormCourse
{
	/** The course's height (across the courses), joint to joint centre, metres. */
	double HeightMeters = 0.0;

	/** The units' lengths along the course, joint centre to joint centre, metres, taken in turn and repeated. */
	TArray<double> LengthsMeters;

	/** Where the course's first joint stands along it from the setting-out origin, metres: the bond's offset. */
	double StartMeters = 0.0;
};

/** One ring of a radial laying: slabs of equal angle between two circles about the setting-out origin. */
struct FFormRing
{
	double InnerMeters = 0.0;
	double OuterMeters = 0.0;

	/** How many slabs make the ring. */
	int32 Units = 0;

	/** Where its first radial joint stands, degrees from the setting-out x axis towards its y axis. */
	double StartDegrees = 0.0;
};

/**
 * A laid surface: a floor, a pavement or a facing set out in units within the stock they are cut from, with real joints
 * between them, as a mason lays flags and ashlar and a joiner lays parquet (Form's Lay). The surface covers an outline
 * with holes in the drawing frame's (x, y) plane, and every unit is its own solid ThicknessMeters thick facing +z, cut to
 * the outline and the holes, with its arrises as laid. Between units runs a joint of JointMeters: open (the hairline
 * between parquet boards), or filled with mortar or grout to FillMeters below the face.
 *
 * The units are set out by one of three rules, in a setting-out frame at Origin turned by AngleDegrees:
 * - **a module** repeated on a lattice: the unit outlines of one module (a tile, a pair of herringbone boards, a
 *   Versailles panel's frame, lattice and diamonds) laid again at every i LatticeU + j LatticeV;
 * - **courses** up the setting-out y axis, each of its own height and lengths, taken in turn and repeated (ashlar, flags
 *   in courses, boards of random lengths);
 * - **rings** of radial slabs about the origin (a round floor, a disc cut in sectors).
 * Unit outlines are drawn joint centre to joint centre; the joint is taken from them, half from each side, and a unit
 * butts the outline and the holes, as a flag is cut to the wall it meets.
 *
 * It makes two pieces: the units, named as the drawing, and, when the joints are filled, the filling, named
 * <Name>.joints.
 */
struct FFormLaid
{
	/** The units' faces and sides. */
	static constexpr const TCHAR* UnitPart = TEXT("unit");
	/** The joints' filling. */
	static constexpr const TCHAR* JointPart = TEXT("joint");

	/** What is laid: the outline, closed, in the frame's (x, y), metres, and holes left unlaid. */
	FFormProfile Outline;
	TArray<FFormProfile> Holes;

	/** The units' thickness, metres. */
	double ThicknessMeters = 0.0;

	/** Every joint's width, metres. More than zero: units never touch, as real ones never quite do. */
	double JointMeters = 0.0;

	/** How far below the face the joints are filled, metres (zero: flush); negative leaves them open. */
	double FillMeters = -1.0;

	/** The arrises round each unit's face: a chamfer makes V-joints. */
	FFormArris Arris;

	/** The setting-out frame: its origin and how far its x axis is turned from the drawing's, degrees towards y. */
	FVector2D OriginMeters = FVector2D::ZeroVector;
	double AngleDegrees = 0.0;

	/** A module: its unit outlines about the origin, laid again at every i LatticeU + j LatticeV. */
	TArray<FFormProfile> Module;
	FVector2D LatticeU = FVector2D::ZeroVector;
	FVector2D LatticeV = FVector2D::ZeroVector;

	/** Courses, from the origin up the setting-out y axis, taken in turn and repeated to cover the outline. */
	TArray<FFormCourse> Courses;

	/** Rings about the origin. */
	TArray<FFormRing> Rings;

	/**
	 * The largest face the material's stock gives, (length, width) in metres: no unit may be longer or wider
	 * (Form.WithinStock). Zero: the stock doesn't bound the units.
	 */
	FVector2D StockMeters = FVector2D::ZeroVector;
};
