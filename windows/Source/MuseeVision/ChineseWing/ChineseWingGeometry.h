#pragma once

#include "CoreMinimal.h"
#include "ChineseWing/ChineseWingKit.h"

/**
 * The Chinese Wing's architecture as mesh sections (AChineseWingStructure builds its components from these), in
 * plan metres: x east, plan y south, z up, the Rotunda's centre at the origin (MuseePlan::ChineseWing).
 */
namespace ChineseWingBuild
{
	struct FWingMeshes
	{
		// The Structure component (collision): walls, the vestibule, floors, stone.
		ChineseWingKit::FPart Walls;       // 0: whitewash, the court's walls with the moon gate
		ChineseWingKit::FPart Plaster;     // 1: the vestibule's plaster
		ChineseWingKit::FPart Paving;      // 2: grey granite, the vestibule and the walks
		ChineseWingKit::FPart Gravel;      // 3: the garden's pebbles and the bamboo beds' mulch
		ChineseWingKit::FPart GreyStone;   // 4: the moon gate's ring and sill, the kerb, column bases, the beds' kerbs
		ChineseWingKit::FPart Bank;        // 5: the pond's bank and basin
		ChineseWingKit::FPart Edging;      // 6: the pond's edging stones
		ChineseWingKit::FPart Plinths;     // 7: the ceramics' drum plinths and the terrace benches
		// The Frame component (collision): timber.
		ChineseWingKit::FPart Timber;      // 0: columns, beams, purlins, rafters, brackets, fretwork, boarding, rail, cases
		ChineseWingKit::FPart Seats;       // 1: the seat rails (美人靠)
		ChineseWingKit::FPart Beds;        // 2: the table cases' beds
		// The Roof component (no collision).
		ChineseWingKit::FPart Tiles;       // 0: the grey tiles
		ChineseWingKit::FPart Mortar;      // 1: the tiles' bed, the flashing, the walls' copings and corbels
		ChineseWingKit::FPart Ridges;      // 2: the copings' ridges and hips
		// Not drawn, the visitor's collision: a low fence round the pond, a wall over each seat rail's back.
		ChineseWingKit::FPart Guard;
	};

	/** Builds every section. */
	FWingMeshes BuildMeshes();

	/** A strip of light: its centre, facing (unit), and the width and height of the source (metres). */
	struct FLightStrip
	{
		FVector Centre = FVector::ZeroVector;
		FVector Facing = FVector::UpVector;
		FVector Along = FVector::ForwardVector;
		double Width = 1.0;
		double Height = 0.02;
	};

	/** Under the eaves: one strip per walk, hidden inside the column line, facing up at the boarding. */
	TArray<FLightStrip> EaveStrips();

	/** Path lights: under every seat rail's plank and the terrace benches, facing down onto the paving. */
	TArray<FLightStrip> PathStrips();

	/** The vestibule's ceiling light. */
	FLightStrip VestibuleStrip();

	/** The columns' centres on the floor (plan metres). */
	TArray<FVector2D> ColumnCentres();
}
