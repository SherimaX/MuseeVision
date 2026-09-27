#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "FormEnclosure.h"
#include "FormOpening.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"
#include "SiteFootprint.h"
#include "SitePort.h"

/**
 * Everything a stage says about itself, in its own frame and in metres: which stage it is, the ground it stands on,
 * the spaces it encloses and their openings, the ports where it meets other stages, the anchors the systems furnish,
 * and its pieces as Form drawings. It holds no exposure, no light level, no finish override, no world position and no
 * other stage: those belong to the systems, the schedules and the site plan.
 */
struct FSiteDescription
{
	/** The stage's key: nympheas. Every key below is scoped under it. */
	FMuseeKey Stage;

	/** Its name for visitors. */
	FText DisplayName;

	FSiteFootprint Footprint;

	TArray<FFormEnclosure> Enclosures;

	TArray<FFormOpening> Openings;

	TArray<FSitePort> Ports;

	/** The places the systems furnish (Wall, Mount, Bed, Water, Body, Use, Seat), in the stage's frame. */
	TArray<FMuseeStageAnchor> Anchors;

	/** The stage's own pieces, each drawn in Form's language and named uniquely within the stage. */
	TArray<FFormDrawing> Pieces;
};

namespace Musee::Site
{
	/**
	 * Whether a description holds together: a stage key; every port, anchor, enclosure and opening key under it; piece
	 * names unique; every enclosure's pieces drawn; every port's class named. Each break is a finding (Site.Described)
	 * drawn by the stage.
	 */
	[[nodiscard]] MUSEESITE_API TArray<FMuseeFinding> CheckDescription(const FSiteDescription& Description);
}
