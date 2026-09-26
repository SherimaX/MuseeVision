#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	class FMeshBuilder;
	struct FProfileStations;
	struct FSweepFrame;

	/**
	 * Sweeps a profile through one run of frames: a moulding along a wall, a drum round its axis, a jamb up its
	 * height. Along the run the surface is smooth (a run ends wherever the path turns a corner); across the profile it
	 * is smooth or crisp as the profile was drawn. U runs along the travel and V along the profile, both in metres.
	 * Where the profile is smooth, neighbouring segments share their vertices; where it is crisp, or at a closed
	 * profile's UV seam, each side keeps its own at the same point.
	 */
	void Sweep(FMeshBuilder& Mesh, TConstArrayView<FSweepFrame> Run, const FProfileStations& Profile);

	/**
	 * Closes a closed profile's end at a frame with a flat cap, concave profiles included: at the run's start
	 * (bRunEnd false) or its end. The cap faces the way the profile's surfaces face: out of the solid for a
	 * counter-clockwise profile (back along the run at its start, forward at its end), into the void for a clockwise
	 * one, so the capped sweep is one consistently oriented shell. The cap's UVs are metres in its own plane. It uses
	 * the profile's stations and no others, so it meets the sweep vertex for vertex. Returns false, adding nothing,
	 * for an open profile or one whose outline crosses itself.
	 */
	bool Cap(FMeshBuilder& Mesh, const FSweepFrame& Frame, const FProfileStations& Profile, bool bRunEnd);
}
