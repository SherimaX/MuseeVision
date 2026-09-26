#pragma once

#include "CoreMinimal.h"
#include "Chenghuai/ChenghuaiGeometry.h"
#include "Chenghuai/ChenghuaiRoof.h"

/**
 * One building of the house, built as Qing official-style carpentry (清式小式大木) on a stone-kerbed platform, from a
 * spec in its own frame: u along its front, v from its front eave columns to the back, z up (ChenghuaiKit::FLocal).
 *
 * - The platform (台基): a brick body with a stone kerb (阶条石) round its top, a footing course (土衬) at the ground, the
 *   floor of square bricks inside; stone steps (踏跺) with sloping side stones (垂带) at the doors.
 * - Columns with a slight taper (收分) on stone bases (柱顶石 with a raised drum, 鼓镜), red.
 * - The frame: lintels (檐枋) and boards (垫板) between the columns under the eave purlin; at every column a stack of beams
 *   (抱头梁, 五架梁 / 七架梁, 三架梁 or the rolled roof's 月梁) on struts (瓜柱) carrying the purlins (檩); plain red or
 *   painted in the Suzhou manner (苏式彩画, the atlas) by rank.
 * - The roof: round rafters (檐椽, 花架椽, 脑椽) purlin to purlin, square flying rafters (飞椽) at the eave, eave boards
 *   (连檐, 瓦口), red boarding (望板), the lime bed; the rise steepens towards the ridge (举架, 5-7-9 tenths); tiles
 *   (ChenghuaiRoof); a moulded ridge with scorpion tails (清水脊) or a rolled ridge (过垄脊 of a 卷棚 roof); edge tiles
 *   down the gables (排山).
 * - Hard gables (硬山): brick gable walls to the roof with corbelled ends (墀头) under the eaves; a sealed back eave
 *   (封护檐) where the back wall is the house's outer wall; the walls' base (下碱) in rubbed brick, their upper parts
 *   hairline-jointed or lime-pointed, plastered white inside.
 * - The front (装修): partition doors (隔扇) with lattice heads and panelled aprons, support-and-remove windows (支摘窗)
 *   on brick sill walls, transoms (横披), in their frames (槛框); lattice bars over mulberry paper.
 */
namespace ChenghuaiHall
{
	using ChenghuaiKit::FLocal;
	using ChenghuaiBuild::FChMeshes;

	/** What fills a bay of the front. */
	enum class EBay : uint8
	{
		Doors,        // partition doors, closed
		DoorsOpen,    // partition doors, the middle pair swung open
		Windows,      // support-and-remove windows on a sill wall
		Wall,         // brick
		Open          // nothing (a gallery, the gate)
	};

	enum class ERidge : uint8 { Qingshui, Rolled, None };

	/** An opening through a gable wall (a gallery's way through, a door into an ear room). */
	struct FGableOpening
	{
		bool bEnd = false;        // false: the wall at U0; true: at U1
		double V0 = 0.0, V1 = 0.0;
		double Top = 2.6;         // above the floor
		bool bFrame = true;       // a red door frame in it
	};

	/** Steps up to the platform's front (or back) edge. */
	struct FSteps
	{
		double U = 0.0;           // centre
		double Width = 1.6;
		bool bBack = false;       // at the back edge
		double Going = 0.30;
	};

	struct FHallSpec
	{
		FString Name;
		FLocal L;
		TArray<double> ColU;                     // column axes along u, the ends included (in the gable walls)
		double U0 = 0.0, U1 = 10.0;              // the gable walls' outer faces
		double GableT = 0.5;
		double Floor = 0.45;
		double PlatformFront = -0.6;             // v of the platform's front edge
		double PlatformBack = 5.0;               // v of its back edge
		double PlatformU0 = 0.0, PlatformU1 = 10.0;
		double ColH = 3.0;
		double ColD = 0.28;
		TArray<double> PurlinV;                  // front eave purlin (0) … back eave purlin
		TArray<double> Rise;                     // 举 per step (0 between a rolled roof's ridge pair)
		bool bRolled = false;
		ChenghuaiRoof::ETiles Tiles = ChenghuaiRoof::ETiles::He;
		ERidge Ridge = ERidge::Qingshui;
		double FacadeV = 0.0;                    // the lattice line (a veranda in front of it when > 0)
		bool bBackWall = true;
		double BackWallV0 = 4.8, BackWallV1 = 5.3;
		bool bSealedBack = true;                 // 封护檐
		bool bBackColumns = false;               // columns on the back eave line (a gallery)
		TArray<double> InnerColV;                // lines of interior columns (the main hall's rear 金柱)
		double EaveFront = 0.9;                  // the eave's reach past the front eave purlin
		double EaveBack = 0.6;                   // past the back eave purlin (open back) or the back wall (sealed)
		TArray<EBay> Bays;
		bool bPainted = false;
		int32 WallPart = ChenghuaiBuild::BrickHairline;
		int32 BaseCourse = ChenghuaiBuild::BrickFine;
		bool bGableWalls = true;
		bool bInteriorPlaster = true;
		TArray<FGableOpening> GableOpenings;
		TArray<FSteps> Steps;
		/** Mitred roof ends for a gallery's corner: the roof's end at U0 (or U1) follows u = U0 ± (v − vRef). */
		int32 MitreAtU0 = 0, MitreAtU1 = 0;      // 0 none; +1 / −1: the slant's sign
		double MitreRefV = 0.0;
		bool bStartOpen = false, bEndOpen = false; // no gable wall at that end (a gallery joins there)
		double RoofU0 = 0.0, RoofU1 = 0.0;       // the roof's ends (0 = the gable walls' outer faces)
		int32 PaintRow = 0;                      // which row of the beam atlas its lintels use
		int32 TimberPart = ChenghuaiBuild::RedLacquer;   // the timber's finish (the garden's: chestnut)
		int32 FloorPart = -1;                    // the floor's tiles (−1: by rank)
		bool bColumns = true;                    // the columns (the festooned gate builds its own)
		bool bFrames = true;                     // the beams across (the festooned gate builds its own)
		bool bPlatform = true;
		bool bRafterEnds = true;                 // the rafters' painted ends (plain timber: none)
		TArray<double> FrameU;                   // where the beams across stand (empty: at the free columns)
		double GableBackV = -1.0;                // the gable walls' back end (−1: the back wall's outer face)
		TArray<int32> SkipPurlins;               // purlins another roof already has (the festooned gate's valley)
		int32 PlatformPart = ChenghuaiBuild::BrickPointed;   // the platform's body (the garden's: bluestone)
		int32 Lattice = -1;                      // the lattice of its doors and windows (an ELattice; −1: by rank)
		int32 LatticeOpenBay = -1;               // the open middle bay's (−1: as the rest)
	};

	/** The heights: the eave columns' tops and the purlins' seats (the rafters' undersides). */
	struct FSection
	{
		double Zc = 0.0;
		TArray<double> Seat;
		double RidgeV = 0.0;
		double RollHalf = 0.0;
		double BedAt(double V) const;       // the tiles' bed (the lime's top) at v (front slope for v < RidgeV)
		double BoardAt(double V) const;     // the boarding's underside (the rafters' top)
		const FHallSpec* Spec = nullptr;
	};

	FSection MakeSection(const FHallSpec& H);

	/** Builds the whole building into the sections. */
	void BuildHall(FChMeshes& Out, const FHallSpec& H);

	/** The lattice pattern of a panel. */
	enum class ELattice : uint8 { Grid, Lantern, StepBrocade, IceCrack };   // IceCrack: 冰裂纹, the Suzhou gardens' own

	/**
	 * A lattice panel W × H (in the frame F: u across, v through, z up, from (U, V, Z)): its bars over mulberry paper on
	 * the side v + Depth. Frame part: the bars; the paper in the Paper part.
	 */
	void LatticePanel(FChMeshes& Out, const FLocal& F, double U, double V, double Z, double W, double H, ELattice Pattern, int32 FramePart, bool bPaper = true);

	/**
	 * A partition door leaf (隔扇) standing in the frame F from (U, V) to U + W, Z to Z + H: stiles, five rails, a lattice
	 * head over paper, a waist panel and a carved apron panel.
	 */
	void DoorLeaf(FChMeshes& Out, const FLocal& F, double U, double V, double Z, double W, double H, int32 FramePart, ELattice Pattern);
}
