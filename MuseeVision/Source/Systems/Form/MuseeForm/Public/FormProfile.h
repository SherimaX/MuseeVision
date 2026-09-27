#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "FormProfile.generated.h"

/** How two drawn elements of a profile meet, and so how the light turns there. */
UENUM()
enum class EFormJoint : uint8
{
	/** Smooth where the two elements meet tangent, crisp where they turn. */
	Auto,
	/** An arris: each side keeps its own normal. */
	Crisp,
	/** One normal across the joint: a joint the eye should read as rounded. */
	Smooth,
};

/**
 * An elliptical arc of a profile, kept as drawn: the points (CenterA + RadiusA cos t, CenterB + RadiusB sin t) for t from
 * FromDegrees to ToDegrees, either way round. A circular arc has equal radii.
 */
USTRUCT()
struct MUSEEFORM_API FFormArc
{
	GENERATED_BODY()

	/** The centre, (A, B) in metres. */
	UPROPERTY()
	TArray<double> CenterMeters;

	/** The radius along A, then along B, in metres. One value is a circle. */
	UPROPERTY()
	TArray<double> RadiiMeters;

	UPROPERTY(meta = (Units = "Degrees"))
	double FromDegrees = 0.0;

	UPROPERTY(meta = (Units = "Degrees"))
	double ToDegrees = 0.0;
};

/** One element of a profile: a straight line to a point, or an arc. Where an arc does not start at the current point, a straight line joins them first. */
USTRUCT()
struct MUSEEFORM_API FFormProfileStep
{
	GENERATED_BODY()

	/** A line's end, (A, B) in metres. Empty for an arc. */
	UPROPERTY()
	TArray<double> Line;

	/** The arc, when Line is empty. */
	UPROPERTY()
	FFormArc Arc;

	/** How this element meets the one before it. */
	UPROPERTY()
	EFormJoint Joint = EFormJoint::Auto;

	[[nodiscard]] bool IsArc() const { return Line.IsEmpty(); }
};

/**
 * A profile: a line drawn in its own plane (A, B), in metres, from straight lines and exact arcs, crisp or smooth at
 * each joint. It is the section of a moulding, the outline of a slab or a leaf, and the path a wall or a moulding runs
 * along in plan. Arcs are kept as drawn; the making divides them at the one chord tolerance.
 *
 * - **As a section** (swept or turned), it is drawn with its solid on the left, so each segment faces its right: for a
 *   wall moulding with A out of the wall and B up, draw from the bottom, out, up and back.
 * - **As a plan path** (A is x east, B is y south), whatever is swept along it lies to the walker's right: into the
 *   loop for a loop drawn clockwise on the plan (north up), out of it for one drawn anticlockwise.
 *
 * The moulding catalogue (Data/Form/Profiles.json) holds profiles by key, named for what they are
 * (`cove.quarter-ellipse.1450`), never for a room.
 */
USTRUCT()
struct MUSEEFORM_API FFormProfile
{
	GENERATED_BODY()

	/** The catalogue key, for a catalogue profile; the none key for one drawn in code. */
	UPROPERTY()
	FMuseeKey Key;

	/** Where the drawing starts, (A, B) in metres. */
	UPROPERTY()
	TArray<double> Start;

	UPROPERTY()
	TArray<FFormProfileStep> Steps;

	/** Whether the drawing returns to its start: a closed section or loop. */
	UPROPERTY()
	bool Closed = false;

	/** Where the section comes from: a measured drawing, a board, a pattern book. */
	UPROPERTY()
	FString Source;

	/** A profile starting at a point. */
	[[nodiscard]] static FFormProfile From(const FVector2D& StartMeters);

	/** A straight line from the current point. */
	FFormProfile& LineTo(const FVector2D& EndMeters, EFormJoint Joint = EFormJoint::Auto);

	/** An elliptical arc about a centre (either way round), joined to the current point by a line where it starts elsewhere. */
	FFormProfile& ArcAbout(const FVector2D& CenterMeters, double RadiusAMeters, double RadiusBMeters, double FromDegrees,
		double ToDegrees, EFormJoint Joint = EFormJoint::Auto);

	/** Closes the drawing back to its start. */
	FFormProfile& Close();

	/** A closed polygon or an open polyline through points, crisp at every corner. */
	[[nodiscard]] static FFormProfile Polyline(TConstArrayView<FVector2D> PointsMeters, bool bClosed);

	/** The closed rectangle between two corners, drawn anticlockwise in (A, B), so its solid is inside. */
	[[nodiscard]] static FFormProfile Rectangle(const FVector2D& MinMeters, const FVector2D& MaxMeters);

	/**
	 * The closed ellipse (CenterA + RadiusA cos t, CenterB + RadiusB sin t), starting at t = StartDegrees. As a plan path
	 * (x east, y south), bClockwise runs it clockwise on the plan, t increasing from east towards south, which puts
	 * whatever is swept along it inside the loop; anticlockwise puts it outside.
	 */
	[[nodiscard]] static FFormProfile Ellipse(const FVector2D& CenterMeters, double RadiusAMeters, double RadiusBMeters,
		double StartDegrees, bool bClockwise);

	/** The start as a point. */
	[[nodiscard]] FVector2D StartPoint() const;

	/** Whether every point and arc has its numbers (two for a point, one or two radii); if not, OutWhy says where. */
	[[nodiscard]] bool IsWellFormed(FString* OutWhy = nullptr) const;
};
