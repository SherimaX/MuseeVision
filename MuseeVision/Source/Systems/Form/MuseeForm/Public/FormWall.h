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

	/**
	 * The head above the wall's base, metres: the lintel's soffit, or the springing of an arched head, which may lie below the
	 * sill where the wall is the upper course of a block the arch passes through (SillHalfWidthMeters).
	 */
	double HeadMeters = 0.0;

	/**
	 * An arched head springing at HeadMeters from both jambs: a semicircle of half the width's radius, unless
	 * ArchRadiusMeters says otherwise.
	 */
	bool bArched = false;

	/**
	 * The radius of an arched head's intrados, metres. Zero: a semicircle. Larger than half the width: a segmental arch
	 * of that radius through both springings, its centre below them (a portal's flat arch); or, with bPointed, a
	 * two-centred pointed arch whose two arcs of that radius each spring from one jamb and meet at the crown, their
	 * centres on the springing line (an equilateral arch's radius is the width).
	 */
	double ArchRadiusMeters = 0.0;

	/** The arched head is pointed: two arcs of ArchRadiusMeters meeting at the crown. */
	bool bPointed = false;

	/**
	 * A straight passage: its jambs and soffit run parallel to the opening's axis (the face's normal where the axis meets
	 * it), as a doorway is cut through a drum or an oval, instead of square to a curved face at every point. WidthMeters is
	 * then the clear width across the axis, and CenterMeters the distance along the face to where the axis meets it. On a
	 * straight wall the two are the same.
	 */
	bool bStraight = false;

	/** Half the clear width, metres. */
	[[nodiscard]] double HalfWidthMeters() const { return 0.5 * WidthMeters; }

	/** The radius of the head's arcs: half the width for a semicircle. Meaningless for a flat head. */
	[[nodiscard]] double IntradosRadiusMeters() const { return ArchRadiusMeters > 0.0 ? ArchRadiusMeters : HalfWidthMeters(); }

	/** Whether the arched head's radius suits its kind: at least half the width, as a segment or two arcs must be. */
	[[nodiscard]] bool HasBuildableHead() const
	{
		return !bArched || IntradosRadiusMeters() >= HalfWidthMeters();
	}

	/** How far the head's crown stands above its springing, metres: zero for a flat head. */
	[[nodiscard]] double RiseMeters() const
	{
		if (!bArched)
		{
			return 0.0;
		}
		const double Half = HalfWidthMeters();
		const double Radius = IntradosRadiusMeters();
		if (bPointed)
		{
			return FMath::Sqrt(FMath::Max(0.0, Radius * Radius - FMath::Square(Radius - Half)));
		}
		return Radius - FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Half * Half));
	}

	/**
	 * The head's soffit above the wall's base at an offset across the opening's axis (clamped to the jambs), metres: the
	 * lintel's, or the arch's intrados.
	 */
	[[nodiscard]] double HeadAt(double AcrossMeters) const
	{
		if (!bArched)
		{
			return HeadMeters;
		}
		const double Half = HalfWidthMeters();
		const double Radius = IntradosRadiusMeters();
		const double Across = FMath::Min(FMath::Abs(AcrossMeters), Half);
		if (bPointed)
		{
			return HeadMeters + FMath::Sqrt(FMath::Max(0.0, Radius * Radius - FMath::Square(Across + Radius - Half)));
		}
		const double CentreBelow = FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Half * Half));
		return HeadMeters - CentreBelow + FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Across * Across));
	}

	/** The top of the opening above the wall's base: the crown of an arch, or the lintel's soffit. */
	[[nodiscard]] double CrownMeters() const { return HeadMeters + RiseMeters(); }

	/**
	 * Half the opening's width at its sill: the jambs', or, where an arch springs below the sill (the upper course of a
	 * block a segmental arch passes through, whose bed lies above the springing), the arch's half-chord at the sill, where
	 * its intrados comes down to it.
	 */
	[[nodiscard]] double SillHalfWidthMeters() const
	{
		const double Half = HalfWidthMeters();
		if (!bArched || HeadMeters >= SillMeters)
		{
			return Half;
		}
		const double Radius = IntradosRadiusMeters();
		const double Below = SillMeters - HeadMeters;
		if (bPointed)
		{
			return FMath::Max(0.0, FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Below * Below)) - Radius + Half);
		}
		const double CentreBelow = FMath::Sqrt(FMath::Max(0.0, Radius * Radius - Half * Half));
		return FMath::Sqrt(FMath::Max(0.0, Radius * Radius - FMath::Square(Below + CentreBelow)));
	}
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

	/**
	 * The wall's top, when it isn't level at HeightMeters: its line drawn on the face's development, (U metres along the
	 * face from the start of its plan path, Z above the base), in lines and arcs running forward in U, one height at each
	 * distance: a lining that follows the arc of the vault above it, a gable's rake, a tympanum under an arch. Before its
	 * start and past its end the top runs level at that end's height. Undrawn (no start): the top is level.
	 */
	FFormProfile Top;

	/** Whether the top follows a drawn line. */
	[[nodiscard]] bool HasDrawnTop() const { return !Top.Start.IsEmpty(); }

	TArray<FFormWallOpening> Openings;
};
