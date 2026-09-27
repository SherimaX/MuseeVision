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
 * A barrier that keeps the visitor back from a work while the eye passes on to it: a box that stops only the body
 * (Musee::Collision::Barrier), with no matter of its own. The Atrium's stele keeps the visitor off the Starry Night by the
 * box of its foot's plan, 2.2 m high.
 */
struct FSiteBarrier
{
	/** The barrier's key, under its stage: atrium.stele.barrier. */
	FMuseeKey Key;

	/** The box's centre and turn, in the stage's frame. */
	FMuseeFrame Frame;

	/** Half the box's size along its frame's axes, metres. */
	FVector HalfSizeMeters = FVector::ZeroVector;
};

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

	/**
	 * The bodies that ride on another body, each Body anchor's key to the key of the body carrying it: the Atrium car's
	 * leaves and the mast's head ride the car, so they go where it goes and turn or slide on it. A body not named here
	 * hangs from the stage's frame. Every Body anchor's frame stays in the stage's frame, where the body is drawn.
	 */
	TMap<FMuseeKey, FMuseeKey> BodyParents;

	/** The stage's own pieces, each drawn in Form's language and named uniquely within the stage. */
	TArray<FFormDrawing> Pieces;

	/** The boxes that stop only the visitor's body round its works. */
	TArray<FSiteBarrier> Barriers;
};

namespace Musee::Site
{
	/**
	 * Whether a description holds together: a stage key; every port, anchor, enclosure and opening key under it; piece
	 * names unique; every enclosure's pieces drawn; every port's class named, and every vertical port's well declared;
	 * every riding body and the body it rides declared as Body anchors, with no body carried round a loop back to itself;
	 * every barrier's key under it, and its box of some size. Each break is a finding (Site.Described) drawn by the stage.
	 */
	[[nodiscard]] MUSEESITE_API TArray<FMuseeFinding> CheckDescription(const FSiteDescription& Description);
}
