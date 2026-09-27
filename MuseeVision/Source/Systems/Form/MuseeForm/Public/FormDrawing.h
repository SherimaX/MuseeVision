#pragma once

#include "CoreMinimal.h"
#include "FormBasin.h"
#include "FormCeiling.h"
#include "FormDome.h"
#include "FormExtrusion.h"
#include "FormFace.h"
#include "FormLathe.h"
#include "FormRelief.h"
#include "FormStair.h"
#include "FormSweep.h"
#include "FormWall.h"
#include "Misc/TVariant.h"
#include "MuseeFrame.h"
#include "MuseeKey.h"

/** The shape a drawing draws: one word of Form's vocabulary. */
using FFormShape = TVariant<FFormExtrusion, FFormSweep, FFormLathe, FFormFace, FFormWall, FFormBasin, FFormStair, FFormCeiling, FFormDome,
	FFormRelief>;

/**
 * A drawing of one made thing of a stage, in Form's language: a wall, a coping, a slab, a post. It says what shape it
 * is, where it stands, what it is finished in, whether it rides on a moving body, and how it collides. Studio hands
 * every drawing to Form's making (MuseeFormMaking's Make), which cuts it into pieces: one closed solid per drawing (a
 * glazed ceiling also gives its pane), each part of it finished by the surface the drawing names for it.
 *
 * A drawing is data: it says nothing about triangles, and it is the same in the editor and the game.
 */
struct FFormDrawing
{
	/** The piece's name within its stage, unique and dot-scoped: wall.north, pond.coping, stair. */
	FName Name;

	/** The finish schedule's surface for the whole drawing (Data/Stages/<stage>/Finishes.json): wall, coping, floor. */
	FName Surface;

	/** Surfaces for named parts of the shape (a wall's back, a basin's lining); parts not named here take Surface. */
	TMap<FName, FName> PartSurfaces;

	/**
	 * A material the drawing brings by nature, overriding the finish schedule: a palette key with its working and
	 * history (Data/Matter/Palette.json). Empty: the finish schedule decides.
	 */
	FName Material;
	FName Working;
	FName History;

	/** The moving body the piece rides on, or the none key for a piece that stands still. */
	FMuseeKey Body;

	/** The collision profile (Musee::Collision: Solid, Walkable), or NAME_None for a piece nothing collides with. */
	FName Collision;

	/** False for glass and water, which Nanite can't render. */
	bool bNanite = true;

	/** Where the shape's own frame stands in its stage's frame (its body's frame when Body is set). */
	FMuseeFrame Frame;

	/** Who drew it (the stage's key, or a system such as Site for a blind), so a finding goes to its cause. */
	FName Drawer;

	FFormShape Shape;

	/** A drawing of a shape, named and finished. */
	template <typename TShape>
	[[nodiscard]] static FFormDrawing Of(FName InName, FName InSurface, TShape&& InShape)
	{
		FFormDrawing Drawing;
		Drawing.Name = InName;
		Drawing.Surface = InSurface;
		Drawing.Shape.Set<std::decay_t<TShape>>(Forward<TShape>(InShape));
		return Drawing;
	}

	/** The surface a part of the shape is finished in. */
	[[nodiscard]] FName SurfaceOf(FName Part) const
	{
		const FName* Found = PartSurfaces.Find(Part);
		return Found ? *Found : Surface;
	}
};
