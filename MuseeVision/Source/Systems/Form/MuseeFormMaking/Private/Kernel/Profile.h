#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	/** How two drawn elements meet, and so how the light turns there. */
	enum class EProfileJoint : uint8
	{
		/** Smooth where the two elements meet tangent, crisp where they turn. */
		Auto,
		/** An arris: each side keeps its own normal. */
		Crisp,
		/** One normal across the joint, the mean of the two sides': a joint the eye should read as rounded. */
		Smooth,
	};

	/**
	 * One station of a resolved profile. Its normals face the profile's right, (dB, -dA) for a step (dA, dB): the
	 * profile is drawn with the solid on its left.
	 */
	struct FProfileVertex
	{
		/** (A, B), metres. */
		FVector2D Point = FVector2D::ZeroVector;

		/** The unit surface normal at the end of the segment arriving here. */
		FVector2D NormalIn = FVector2D::ZeroVector;

		/** The unit surface normal at the start of the segment leaving here. */
		FVector2D NormalOut = FVector2D::ZeroVector;

		/** Distance along the profile's chords from its first station, metres: the V of the UVs. */
		double LengthMeters = 0.0;

		/**
		 * Distance along the profile as it is drawn from its first station, metres: an arc's own length round the arc, a
		 * line's. What a caller means by a distance along a drawn line (an opening's centre on a curved wall).
		 */
		double DrawnMeters = 0.0;

		/** True where the two sides keep their own normals (an arris or an end of an open profile's drawing). */
		bool bCrisp = false;
	};

	/**
	 * A profile resolved into its stations at a chord tolerance: what a sweep, a lathe, an extrusion and a cap are
	 * made from. A closed profile's last segment runs from its last station back to its first.
	 */
	struct FProfileStations
	{
		TArray<FProfileVertex> Vertices;
		bool bClosed = false;

		/** A closed profile's whole length, closing segment included: the V at the end of its last segment. */
		double ClosedLengthMeters = 0.0;

		/** A closed profile's whole length as drawn, closing segment included. */
		double ClosedDrawnMeters = 0.0;

		int32 NumSegments() const { return bClosed ? Vertices.Num() : FMath::Max(0, Vertices.Num() - 1); }

		/** The V of the UVs at the end of a segment. */
		double LengthAtSegmentEnd(int32 Segment) const;

		/** A segment's length as drawn (the closing segment's, for a closed profile's last). */
		double SegmentDrawnMeters(int32 Segment) const;

		/** The area the closed profile encloses in the (A, B) plane: positive when it runs counter-clockwise (A to B). */
		double SignedAreaSquareMeters() const;

		/** The profile run the other way: the same line with its solid on the other side, so every normal turns over. */
		FProfileStations Reversed() const;

		/** The stations' points, in order. */
		TArray<FVector2D> Points() const;
	};

	/**
	 * A profile: a line drawn in its own plane (A, B) in metres, from straight lines and exact arcs, crisp or smooth at
	 * each joint. It is a moulding's section, the outline of a slab or a plan path. An arc is kept as drawn and only
	 * divided when the profile is resolved, at stations set by the chord tolerance, with each station's normal taken
	 * from the arc itself.
	 *
	 * The profile is drawn with its solid on the left, so each segment faces its right: for a wall moulding with A out
	 * of the wall and B up, draw from the bottom, out, up and back.
	 */
	class FProfile
	{
	public:
		explicit FProfile(const FVector2D& StartMeters);

		/** A straight line from the current point. Joint is how it meets the element before it. */
		FProfile& LineTo(const FVector2D& EndMeters, EProfileJoint Joint = EProfileJoint::Auto);

		/**
		 * The elliptical arc (CenterA + RadiusA cos t, CenterB + RadiusB sin t) for t from FromRadians to ToRadians
		 * (either way round). Where it does not start at the current point, a straight line joins them first, and Joint
		 * is how that line meets the element before it.
		 */
		FProfile& Arc(const FVector2D& CenterMeters, double RadiusAMeters, double RadiusBMeters, double FromRadians,
			double ToRadians, EProfileJoint Joint = EProfileJoint::Auto);

		/** A circular arc: the elliptical arc with equal radii. */
		FProfile& Arc(const FVector2D& CenterMeters, double RadiusMeters, double FromRadians, double ToRadians,
			EProfileJoint Joint = EProfileJoint::Auto);

		/**
		 * Closes the profile. Where the drawing does not end at its start, a straight line closes it; AtLast is how that
		 * line meets the last element, and AtFirst is how the closing (line or element) meets the first element.
		 */
		FProfile& Close(EProfileJoint AtLast = EProfileJoint::Auto, EProfileJoint AtFirst = EProfileJoint::Auto);

		/** The profile's stations: lines at their ends, arcs divided within the chord tolerance. */
		FProfileStations Resolve(double ChordToleranceMeters) const;

	private:
		enum class EElement : uint8
		{
			Line,
			Arc,
		};

		struct FElement
		{
			EElement Kind = EElement::Line;
			EProfileJoint Joint = EProfileJoint::Auto;
			FVector2D End = FVector2D::ZeroVector;
			FVector2D Center = FVector2D::ZeroVector;
			double RadiusA = 0.0;
			double RadiusB = 0.0;
			double FromRadians = 0.0;
			double ToRadians = 0.0;
		};

		FVector2D Start;
		TArray<FElement> Elements;
		bool bClosed = false;
		EProfileJoint JointAtLast = EProfileJoint::Auto;
		EProfileJoint JointAtFirst = EProfileJoint::Auto;
	};
}
