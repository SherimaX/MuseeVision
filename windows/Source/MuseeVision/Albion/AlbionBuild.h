#pragma once

#include "CoreMinimal.h"
#include "Albion/AlbionKit.h"
#include "Albion/AlbionPlan.h"

/**
 * Albion's geometry, by material slot (AAlbionStructure writes each slot to its component's section). The builders
 * are split by trade: masonry and floors (AlbionMasonry.cpp), the columns and their carving (AlbionColumns.cpp), the
 * ironwork and lead (AlbionIron.cpp), the glass (AlbionGlass.cpp), the furniture (AlbionFurniture.cpp).
 */
namespace AlbionBuild
{
	using AlbionKit::FMeshData;

	/** The material slots. The shafts' ten stones are last, in the Details board's order A … J. */
	enum ESlot : int32
	{
		SlotBuff, SlotRed, SlotSlate, SlotDressing, SlotCarved, SlotRuskin, SlotName,
		SlotExtBuff, SlotExtRed, SlotExtDressing,
		SlotTile, SlotYork, SlotTileBorder, SlotThreshold,
		SlotIron, SlotLead, SlotBronze,
		SlotGlass, SlotTristram, SlotLancetWest, SlotLancetEast,
		SlotOak, SlotCaseGlass, SlotCeiling, SlotFelt,
		SlotGuard,          // never drawn: collision only (the cases' guards, so no one perches on a vitrine)
		SlotShaft0,
		SlotCount = SlotShaft0 + 10
	};

	/** The components (AAlbionStructure's order). */
	enum EComponent : int32 { CompMasonry, CompExterior, CompFloor, CompIron, CompGlass, CompFurniture, CompCount };

	/** Which component each slot's section is in. */
	inline EComponent ComponentOf(int32 Slot)
	{
		if (Slot >= SlotShaft0) { return CompMasonry; }
		switch (Slot)
		{
		case SlotBuff: case SlotRed: case SlotSlate: case SlotDressing: case SlotCarved: case SlotRuskin: case SlotName: return CompMasonry;
		case SlotExtBuff: case SlotExtRed: case SlotExtDressing: return CompExterior;
		case SlotTile: case SlotYork: case SlotTileBorder: case SlotThreshold: return CompFloor;
		case SlotIron: case SlotLead: case SlotBronze: return CompIron;
		case SlotGlass: case SlotTristram: case SlotLancetWest: case SlotLancetEast: return CompGlass;
		default: return CompFurniture;   // oak, the cases' glass, the porch's ceiling
		}
	}

	/** The default material of each slot (the Albion ones made by Scripts/albion_materials.py). */
	const TCHAR* DefaultMaterial(int32 Slot);

	/** The ten stones (the Details board): name, where they come from, what their capital carries. */
	struct FStone
	{
		const TCHAR* Letter;
		const TCHAR* Name;
		const TCHAR* Material;
		double X, Y;
		int32 Capital;      // AlbionColumns' capital designs (0 … 9)
	};
	const FStone& Stone(int32 Index);

	struct FParts
	{
		FMeshData Slot[SlotCount];
		FMeshData& operator[](int32 S) { return Slot[S]; }
		const FMeshData& operator[](int32 S) const { return Slot[S]; }
	};

	void BuildMasonry(FParts& P);
	void BuildFloors(FParts& P);
	void BuildPorch(FParts& P);
	void BuildExterior(FParts& P);
	void BuildColumns(FParts& P);
	void BuildIron(FParts& P);
	void BuildGlass(FParts& P);
	void BuildFurniture(FParts& P);

	/** All of it. */
	FParts BuildAll();

	// ---------------------------------------------------------------- shared by the builders

	/** The stone band (red) or buff course at height Z (inside or out). */
	inline bool InBand(double Z)
	{
		if (Z < AlbionPlan::FirstBand) { return false; }
		const double K = FMath::Fmod(Z - AlbionPlan::FirstBand, AlbionPlan::BandPitch);
		return K < AlbionPlan::BandHeight;
	}

	/** The cuts between the bands up to Top. */
	inline TArray<double> BandCuts(double Top)
	{
		TArray<double> Out = {AlbionPlan::Skirting};
		for (double Z = AlbionPlan::FirstBand; Z < Top; Z += AlbionPlan::BandPitch)
		{
			Out.Add(Z);
			Out.Add(Z + AlbionPlan::BandHeight);
		}
		return Out;
	}

	/** The lancets' opening (the glass line's outline), on a face whose U is the plan y (side walls). */
	AlbionKit::FOpening LancetOpening(double U, double Grow = 0.0);
	/** The court door (pointed), on the north wall (U = plan x). */
	AlbionKit::FOpening CourtDoorOpening(double Grow = 0.0);

	/** The vault halves (the glass's arcs) in the transverse section: 0/1 the nave's west and east halves, 2/3 the west
	 *  aisle's west and east halves, 4/5 the east aisle's. */
	AlbionKit::FArcHalf VaultHalf(int32 Index);
}
