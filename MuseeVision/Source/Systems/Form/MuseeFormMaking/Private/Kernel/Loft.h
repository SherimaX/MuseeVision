#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	class FMeshBuilder;

	/**
	 * A loft set out as corresponding points: Rings[k][i] is point i of section k, in metres, each section a closed loop
	 * in a plane square to the loft's z axis, anticlockwise in (x, y) seen from +z, rising with k. Point i of every
	 * section is joined to point i of the next by a straight line. A point may repeat its neighbour in one section and
	 * part from it in the next: the face between them there closes to a point (a stopped chamfer's stop).
	 */
	struct FLoftGrid
	{
		TArray<TArray<FVector>> Rings;

		/** For each point index: whether the surface keeps an arris along it (a corner of the sections). */
		TArray<bool> CrispAround;

		/** For each section: whether the surface keeps an arris round it (a stop, a change of taper). */
		TArray<bool> CrispAcross;
	};

	/**
	 * Adds a loft's side (smooth between its arrises: each vertex takes the mean of the faces round it within its smooth
	 * patch) in SideSlot, and the flat ends that close it, facing down its z axis at the first section and up it at the
	 * last, in EndSlot. An end whose section has closed to a point needs no face. The side's U runs up the loft and its V
	 * round it, in metres; the ends' UVs are their (x, y). Returns false, adding nothing further, when an end's outline
	 * can't be triangulated (it crosses itself).
	 */
	bool Loft(FMeshBuilder& Mesh, const FLoftGrid& Grid, FName SideSlot, FName EndSlot);
}
