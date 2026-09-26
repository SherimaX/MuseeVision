#pragma once

#include "CoreMinimal.h"
#include "Chenghuai/ChenghuaiKit.h"

/**
 * Chenghuai's architecture as mesh sections, one per material (AChenghuaiStructure writes them into its components), in
 * plan metres: x east, plan y south, z up, the Rotunda's centre at the origin (Chenghuai/ChenghuaiPlan.h).
 */
namespace ChenghuaiBuild
{
	using ChenghuaiKit::FPart;

	/** Every section; the comment names the material (Materials board). */
	enum EChPart : int32
	{
		// Masonry and stone (the Structure component, with collision).
		BrickFine,       // 干摆 rubbed dry-laid grey brick: the gate, the screen wall, the corbels, sill walls, the main hall's base
		BrickHairline,   // 丝缝 grey brick with hairline joints: the main hall's and the side halls' walls
		BrickPointed,    // 淌白 lime-pointed grey brick: the front and rear rows, the enclosing walls, platforms' faces
		PlasterWhite,    // 粉墙 white lime render: the court walls' upper parts, the garden's walls
		PlasterRoom,     // 麻刀灰 · 大白 hemp-fibre lime, whitened: the rooms' walls inside
		StoneKerb,       // 青白石 Fangshan stone: kerbs, steps, column bases, the gate's drums, the screen wall's heart
		FloorLarge,      // 尺七方砖 54 cm tiles, tung-oil soaked: the main hall and the ear rooms
		FloorSmall,      // 尺四方砖 45 cm tiles, ground joints: side halls, front and rear rows, galleries, the gate hall
		CourtPaving,     // the courts' square paving bricks
		CourtPath,       // 城砖 wall bricks on edge: the cross paths of the courts, the porch
		GardenPebble,    // 花街铺地 pebble mosaic: the garden's paths
		GardenGround,    // the garden's earth under its planting (moss and soil)
		Rockery,         // 青石 / 太湖石: the rockery, the pond's edging stones
		PondBed,         // the pond's dark stone bed
		BlueStone,       // 青石 bluestone slabs: the garden's steps, the bridge, the water pavilion's base
		Engraved,        // 书条石: the twelve engraved stones in the walk's wall (an atlas, T_ch_shutiao)
		// Timber (the Frame component, with collision).
		RedLacquer,      // 铁红 iron-oxide red: columns, lattice frames, doors
		RafterRed,       // the same red on the rafters and their boarding (椽, 望板), darkened by age and smoke
		PaintedBeam,     // 苏式彩画: beams and boards painted in the Suzhou manner (atlas texture)
		GreenLacquer,    // green: the festooned gate's screen door, the entry yard's screen door
		BlackLacquer,    // 黑漆: the gate's leaves; the plaques' boards
		Nanmu,           // 楠木 waxed: the main hall's carved screens (花罩), the cases' plinths, the tables
		Chestnut,        // 栗壳色 chestnut lacquer: all the garden's timber
		Paper,           // 桑皮纸 mulberry paper in laminated glass: every lattice's glazing
		Brass,           // brass: door knockers, hinges and pins
		Plaques,         // the plaques and couplets (an atlas of their boards and gilt characters)
		CaseSilk,        // the cases' silk-covered back panels and beds
		CaseLamp,        // the cases' lamp diffusers (warm, lit day and night)
		LanternSilk,     // the palace lanterns' silk panels (宫灯): lit from within at night
		// Roofs (the Roof component, no collision).
		Tiles,           // grey clay tiles
		RoofMortar,      // the tiles' lime bed, the roof's flashing
		Ridges,          // the ridges' brick and tile bodies
		// Glass (no collision): the cases' low-iron glass.
		Glass,
		// Not drawn: the visitor's guards (collision only).
		Guard,
		PartCount
	};

	struct FChMeshes
	{
		FPart Parts[PartCount];
		FPart& operator[](int32 I) { return Parts[I]; }
		const FPart& operator[](int32 I) const { return Parts[I]; }
	};

	/** Builds every section. */
	FChMeshes BuildMeshes();

	/** A soft source of light: its centre, facing, along, size (metres) and lumens. */
	struct FChLight
	{
		FVector Centre = FVector::ZeroVector;
		FVector Facing = FVector(0, 0, -1);
		FVector Along = FVector(1, 0, 0);
		double Width = 1.0;
		double Height = 0.05;
		double Lumens = 500.0;
		bool bShadows = true;
		bool bDaylit = false;   // a window's glow: follows the daylight (else a lamp: night and day)
		bool bNight = false;    // a lantern: lit only after dusk
		FString Name;
	};

	/** The rooms' lights: the paper windows' glow (by day) and the discreet gallery lamps (the cases' canopies). */
	TArray<FChLight> Lights();
}
