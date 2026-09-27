#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	class FMeshBuilder;

	/**
	 * Carries a mesh drawn in a development onto its curved ground: the coffers drawn flat and set into a dome, the panes
	 * cut on a plane and bent onto a sphere. Map takes a point of the development (metres) to its place on the ground.
	 *
	 * The mesh is divided first so that it follows the ground within the tolerance. Each edge is divided into the fewest
	 * equal parts whose carried chords stray no further than the tolerance from the carried edge; the count depends on the
	 * edge's two points alone, so two triangles that share an edge divide it alike and a closed mesh stays closed. A
	 * triangle whose edges divide, or whose carried middle strays from the plane of its carried corners, is filled with a
	 * lattice as fine as its edges and triangulated afresh. Normals are carried by the inverse transpose of the map's
	 * Jacobian, so what faced out of a surface in the development faces out of it on the ground; UVs, colours and slots are
	 * kept, and a new point takes its triangle's attributes where it falls.
	 */
	void CarryOntoGround(const FMeshBuilder& Developed, TFunctionRef<FVector(const FVector&)> Map, double ToleranceMeters, FMeshBuilder& Out);
}
