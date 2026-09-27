#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * An opening through a wall: a door, a window, an arch. It is set out on the wall's face: its centre is a distance
 * along the face's plan path, and its sides are cut square through the wall (radially, on a curved wall), so the
 * reveals meet both faces at right angles; or, for a straight passage, parallel to its axis.
 */
struct FFormWallOpening
{
	/**
	 * The opening's centre, metres along the wall's face from the start of its plan path (for a closed path with
	 * corners, from its first corner). An opening lies within one straight or smooth stretch, clear of its corners.
	 */
	double CenterMeters = 0.0;

	/** The clear width between the jambs, metres along the face. */
	double WidthMeters = 0.0;

	/** The sill above the wall's base, metres; zero for a door, whose opening runs to the floor. */
	double SillMeters = 0.0;

	/** The head above the wall's base, metres: the lintel's soffit, or the springing of an arched head. */
	double HeadMeters = 0.0;

	/** A semicircular head of half the width's radius, springing at HeadMeters. */
	bool bArched = false;

	/**
	 * A straight passage: its jambs and soffit run parallel to the opening's axis (the face's normal where the axis meets
	 * it), as a doorway is cut through a drum or an oval, instead of square to a curved face at every point. WidthMeters is
	 * then the clear width across the axis, and CenterMeters the distance along the face to where the axis meets it. On a
	 * straight wall the two are the same.
	 */
	bool bStraight = false;

	/** The top of the opening above the wall's base: the crown of an arch, or the lintel's soffit. */
	[[nodiscard]] double CrownMeters() const { return HeadMeters + (bArched ? 0.5 * WidthMeters : 0.0); }
};

/**
 * A wall: a solid of one thickness standing on a plan path, straight or curved (an ellipse's arc, a drum), with
 * openings cut through it and their reveals lined. The path is the line of the wall's face; the wall's thickness lies to
 * the path's right, so the face looks to its left (for a room drawn clockwise on the plan, the face is the room's inner
 * face). The wall is one closed solid of these parts, each of which the drawing may give its own surface:
 */
struct FFormWall
{
	/** The face seen from the path's left. */
	static constexpr const TCHAR* FacePart = TEXT("face");
	/** The other face, ThicknessMeters to the path's right. */
	static constexpr const TCHAR* BackPart = TEXT("back");
	/** The linings of every opening: jambs, soffit or head, and sill. */
	static constexpr const TCHAR* RevealPart = TEXT("reveal");
	/** The top of the wall. */
	static constexpr const TCHAR* TopPart = TEXT("top");
	/** The bed the wall stands on. */
	static constexpr const TCHAR* BedPart = TEXT("bed");
	/** The ends of an open path. */
	static constexpr const TCHAR* EndPart = TEXT("end");

	/** The face's line in plan, (x east, y south), metres. */
	FFormProfile Plan;

	/** How far the back stands behind the face, metres, when the wall is of one thickness (Back undrawn). */
	double ThicknessMeters = 0.0;

	/**
	 * The back's line in plan, when the wall fills between two faces instead of standing at one thickness: the passage
	 * masonry between a flat port face and a drum, whose wedges run from nothing where the two touch. It lies to the
	 * face's right and runs the same way as Plan. Each point of the face finds its back straight across the wall (along
	 * the face's normal, or a straight passage's axis). Undrawn (no start): the back is ThicknessMeters behind the face.
	 */
	FFormProfile Back;

	/** Whether the wall fills between two drawn faces. */
	[[nodiscard]] bool HasBack() const { return !Back.Start.IsEmpty(); }

	/** The wall's bed above the drawing frame's origin, and its height from there, metres. */
	double BaseMeters = 0.0;
	double HeightMeters = 0.0;

	TArray<FFormWallOpening> Openings;
};
