#pragma once

#include "CoreMinimal.h"
#include "Chenghuai/ChenghuaiGeometry.h"
#include "Chenghuai/ChenghuaiRoof.h"

/**
 * Pieces shared by the house and the garden: free-standing walls with their openings and tiled copings, moon gates and
 * lattice windows (漏窗), paving, kerbs.
 */
namespace ChenghuaiParts
{
	using ChenghuaiKit::FLocal;
	using ChenghuaiBuild::FChMeshes;

	enum class EHole : uint8
	{
		Door,          // a rectangular doorway with a red frame and a stone threshold
		DoorPlain,     // a doorway with a stone surround (the garden's)
		Window,        // a lattice window (漏窗) through the wall
		BlindWindow,   // a lattice panel on one face only
		MoonGate       // a round opening, a flat stone threshold, a grey brick ring round it
	};

	/** An opening in a wall: along the wall from U0 to U1, from Z0 to Z1 (a moon gate: centre (U0+U1)/2, radius (U1−U0)/2). */
	struct FHole
	{
		EHole Kind = EHole::Door;
		double U0 = 0.0, U1 = 1.0, Z0 = 0.0, Z1 = 2.4;
		int32 Pattern = 0;          // the lattice: 0 ice-crack circle, 1 coins, 2 hexagons, 3 crabapple, 4 octagon, 5 fan
		bool bBlindOnFront = true;  // a blind window's face: the frame's −v side (front) or +v
	};

	/** A wall's recipe: its base course and its body (both faces), its height and coping. */
	struct FWallSpec
	{
		int32 BasePart = ChenghuaiBuild::BrickFine;
		int32 BodyPart = ChenghuaiBuild::BrickPointed;
		int32 FrontFace = -1;      // a skin on the −v face (e.g. white lime on a garden face), −1: none
		int32 BackFace = -1;       // a skin on the +v face
		double BaseH = 0.93;
		double Height = 3.3;
		bool bCoping = true;
		double Foot = 0.0;         // the ground at its foot
	};

	/**
	 * A straight wall in the frame L: along u from U0 to U1, through v from V0 to V1 (its thickness), with its holes;
	 * a tiled coping over it (corbel course, lime bed, butterfly tiles both sides, a ridge).
	 */
	void Wall(FChMeshes& Out, const FLocal& L, double U0, double U1, double V0, double V1, const FWallSpec& W, const TArray<FHole>& Holes);

	/** A tiled coping over a wall top at Z (u from U0 to U1, centred at VC, the wall Thick thick). */
	void Coping(FChMeshes& Out, const FLocal& L, double U0, double U1, double VC, double Thick, double Z);

	/** A level paving slab (top at Z) over a plan outline, 12 cm deep, in a part. */
	void Paving(FChMeshes& Out, int32 Part, const TArray<FVector2D>& Outline, double Z, double Depth = 0.12);

	/** A plan rectangle as an outline. */
	TArray<FVector2D> Rect(double X0, double Y0, double X1, double Y1);

	/** A kerb of stone round a plan rectangle (a bed, a tree pit): W wide, its top at Z. */
	void KerbRect(FChMeshes& Out, double X0, double Y0, double X1, double Y1, double W, double Z0, double Z1, int32 Part);
}
