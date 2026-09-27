#pragma once

#include "CoreMinimal.h"
#include "HallOfLightParts.h"

/**
 * The hall's two ends, as a mason sets them out: each is a vertical cylinder about the neighbour's axis (the Rotunda's
 * drum at the west, the Atrium's base at the east), and everything that runs the hall's length stops on the inner face
 * of the 50 mm lining laid on it (sec. 3.1). A run between the ends is cut to those faces in plan, so a floor, a course
 * of the plinth or a transom meets the lining face to face along its curve.
 */
namespace Musee::HallOfLight
{
	enum class EHallEnd : uint8
	{
		West,
		East,
	};

	/**
	 * A face at one end: a circle in plan about the neighbour's axis, moved into the hall along x by Shift (a face set in
	 * from the lining, as an end post's or a collar's). The hall lies on its concave side as seen from the neighbour: at
	 * larger x than the Rotunda's circle, at smaller x than the Atrium's.
	 */
	struct FEndFace
	{
		EHallEnd End = EHallEnd::West;
		double Radius = 0.0;
		double Shift = 0.0;

		/** +1 at the west (into the hall is +x), -1 at the east. */
		[[nodiscard]] double Inward() const { return End == EHallEnd::West ? 1.0 : -1.0; }

		[[nodiscard]] FVector2D Centre() const;

		/** The face's x at a plan y (|y| below the radius). */
		[[nodiscard]] double X(double Y) const;

		/** The face's point at y, and its angle about the centre in degrees (from +x towards +y). */
		[[nodiscard]] FVector2D At(double Y) const { return FVector2D(X(Y), Y); }
		[[nodiscard]] double AngleDegrees(double Y) const;
	};

	/** A lining's inner face (R 11.250 at the west, R 14.850 at the east), or a face set in from it by Shift. */
	[[nodiscard]] FEndFace InnerFace(EHallEnd End, double Shift = 0.0);

	/** A port face, the lining's outer face on the neighbour (R 11.200, R 14.800). */
	[[nodiscard]] FEndFace PortFace(EHallEnd End);

	/** Where a run along the hall stops: on a face, or on a plane square to the axis. */
	struct FRunLimit
	{
		TOptional<FEndFace> Face;
		double PlaneX = 0.0;

		[[nodiscard]] static FRunLimit OnFace(const FEndFace& InFace);
		[[nodiscard]] static FRunLimit OnPlane(double X);

		/** The limit's x at a plan y. */
		[[nodiscard]] double X(double Y) const { return Face ? Face->X(Y) : PlaneX; }

		/** Of the limit's x over [Y0, Y1], the one nearest the hall's middle: where a square end clears the face. */
		[[nodiscard]] double Innermost(double Y0, double Y1, double Towards) const;
	};

	/** Draws a face's arc from one plan y to another onto a profile, crisp where it meets the element before. */
	void AddFaceArc(FFormProfile& Profile, const FEndFace& Face, double FromY, double ToY);

	/** The plan outline between two limits over y in [Y0, Y1]: straight sides along the hall, each end on its limit. */
	[[nodiscard]] FFormProfile PlanOutline(const FRunLimit& West, const FRunLimit& East, double Y0, double Y1);

	/** A block between two limits over [Y0, Y1], from Bottom to Top: drawn in plan, so its ends are cut to their faces. */
	[[nodiscard]] FFormDrawing PlanRun(const FString& Name, FName Surface, const FRunLimit& West, const FRunLimit& East, double Y0, double Y1,
		double Bottom, double Top, const Musee::Collision::FClass* Collision);

	/** The frame of a section drawn in (y, z) and pushed along the hall from X. */
	[[nodiscard]] FMuseeFrame AlongHall(double X);

	/** A section drawn in (y, z) (A is y, B is z), pushed along the hall from X0 to X1; stand it in AlongHall(X0). */
	[[nodiscard]] FFormExtrusion SectionPushed(TConstArrayView<FVector2D> Section, double X0, double X1);

	/**
	 * A member of a moulded section in (y, z) run the hall's length between two limits: the section pushed along the hall
	 * between square ends, each where it just clears its face, and at a face a wedge of the section's bounding rectangle
	 * cut to the face in plan (its moulding, a chamfer, stops at the wedge: Form can't cut a swept section's end to a
	 * curve, KIT-BACKLOG "chamfered sweep ends"). Adds the pieces Name, Name.west and Name.east.
	 */
	void SectionRun(TArray<FFormDrawing>& Out, const FString& Name, FName Surface, TConstArrayView<FVector2D> Section, const FRunLimit& West,
		const FRunLimit& East, const Musee::Collision::FClass* Collision);
}
