#pragma once

#include "ChenghuaiFrame.h"
#include "ChenghuaiPlan.h"
#include "CoreMinimal.h"
#include "FormDrawing.h"

struct FFormOpening;

/**
 * The Qing hall (清式小式大木), Chenghuai's one composition, drawn fourteen times and for every gallery (§3.1): a building on
 * a stone-kerbed platform, set out from a spec in its own frame. Columns with a slight taper stand on stone bases; lintels
 * and boards join their heads; frames of beams and struts at the free columns carry round purlins, whose seats rise by the
 * 举架 rule (five, seven, nine tenths) to the ridge; round rafters and square flying rafters carry the boarding and the
 * lime bed; tiles run in rows down the bed to the eave; a moulded or rolled ridge crowns it. Hard gables of brick close
 * the ends, a back wall (the house's outer wall) the back; the front is filled bay by bay with partition doors,
 * support-and-remove windows, transoms and lattice over mulberry paper.
 */
namespace Musee::Chenghuai
{
	/** What fills a bay of the front. */
	enum class EBay : uint8
	{
		/** Partition doors (隔扇), shut. */
		Doors,
		/** Partition doors, the middle pair swung 90° into the room. */
		DoorsOpen,
		/** Support-and-remove windows (支摘窗) on a brick sill wall. */
		Windows,
		/** Brick. */
		Wall,
		/** Nothing: a gallery, the gate. */
		Open,
	};

	enum class ERidge : uint8
	{
		/** The moulded 清水脊 with its scorpion tails. */
		Qingshui,
		/** The rolled 过垄脊 of a 卷棚 roof. */
		Rolled,
		/** No ridge: a single slope. */
		None,
	};

	/** The lattice pattern of a front's panels (§3.6). */
	enum class ELattice : uint8
	{
		/** Cells ≈ 0.13 × 0.15. */
		Grid,
		/** 灯笼框: one ring of cells round an open centre with a light cross. */
		Lantern,
		/** 步步锦: two rings, the ties alternating. */
		StepBrocade,
		/** 冰裂纹: convex cells cut by successive straight cuts. */
		IceCrack,
	};

	/** A way through a gable: at the U0 or the U1 end, from V0 to V1, Top over the floor, framed in red. */
	struct FGableOpening
	{
		bool bAtU1 = false;
		double V0 = 0.0;
		double V1 = 0.0;
		double Top = 0.0;
	};

	/** One building of the table (§3.1): the spec's fields in its own frame, metres. */
	struct FHallSpec
	{
		/** Its name in piece names: front-row. */
		FString Name;
		/** The room it encloses (its lattice is chenghuai.<Room>.lattice), or empty for an open building. */
		FString Room;
		FBuildingFrame Frame;
		/** The gables' outer faces along u (the roof's ends when there are no gables), their thickness (0: none), and the ends
		 *  that stand open (no gable wall: another building's gable, or a gallery's corner). */
		double U0 = 0.0;
		double U1 = 0.0;
		double GableT = 0.0;
		bool bOpenAtU0 = false;
		bool bOpenAtU1 = false;
		/** The column axes along u, the ends included (standing in the gables). */
		TArray<double> ColU;
		double Floor = 0.0;
		bool bPlatform = true;
		double PlatformU0 = 0.0;
		double PlatformU1 = 0.0;
		double PlatformFront = 0.0;
		double PlatformBack = 0.0;
		/** The eave columns' height (floor to top) and diameter. */
		double ColH = 0.0;
		double ColD = 0.0;
		/** The purlins' stations along v, front eave to back eave, and the rise of each step between them (举). */
		TArray<double> PurlinV;
		TArray<double> Rise;
		bool bRolled = false;
		bool bTong = false;
		ERidge Ridge = ERidge::Qingshui;
		/** The lattice line's v; negative for a building with no front. */
		double FacadeV = -1.0;
		bool bBackWall = false;
		double BackWallV0 = 0.0;
		double BackWallV1 = 0.0;
		/** A sealed back eave (封护檐): the back wall carries the roof's back edge. */
		bool bSealedBack = false;
		/** Columns on the back eave line (a gallery, a pavilion). */
		bool bBackColumns = false;
		/** Lines of interior columns (the main hall's rear 金柱). */
		TArray<double> InnerColV;
		double EaveFront = 0.0;
		double EaveBack = 0.0;
		TArray<EBay> Bays;
		/** Painted (苏式彩画) on its lintels, boards, beams and purlins. */
		bool bPainted = false;
		/** Surfaces: the timber, the rafters and boarding, the gables' body, their base, the floor, the platform's body. */
		const TCHAR* Timber = TEXT("timber.red");
		const TCHAR* Rafters = TEXT("timber.rafter");
		const TCHAR* WallBody = TEXT("brick.hairline");
		const TCHAR* WallBase = TEXT("brick.rubbed");
		const TCHAR* FloorSurface = TEXT("floor.chisi");
		const TCHAR* PlatformBody = TEXT("brick.pointed");
		/** A lime skin inside the room. */
		bool bPlaster = true;
		TArray<FGableOpening> GableOpenings;
		/** A mitred end at U0 or U1 (a gallery's corner): the end follows u = end + sign (v - MitreRefV); 0 for none. */
		int32 MitreAtU0 = 0;
		int32 MitreAtU1 = 0;
		double MitreRefV = 0.0;
		/** The roof's own ends, when they are not the gables' faces. */
		bool bRoofSpan = false;
		double RoofU0 = 0.0;
		double RoofU1 = 0.0;
		/** Where the frames across stand (empty: at every free column). */
		TArray<double> FrameU;
		bool bColumns = true;
		bool bFrames = true;
		/** Where the gables end behind (negative: the back wall's outer face). */
		double GableBackV = -1.0;
		/** Purlins another roof already has (the 垂花门's valley). */
		TArray<int32> SkipPurlins;
		ELattice Lattice = ELattice::Lantern;
		ELattice OpenBayLattice = ELattice::Lantern;
	};

	/**
	 * A building's section by the rule (§3.1): the columns' top, the purlins' radius and seats (the rafters' undersides), the
	 * ridge, the eave's stations, and the lime bed's curve, a monotone cubic (Fritsch-Carlson) through its knots.
	 */
	struct FHallSection
	{
		double Zc = 0.0;
		double PurlinR = 0.0;
		TArray<double> Seat;
		int32 RidgeI = 0;
		int32 RidgeJ = 0;
		double RidgeV = 0.0;
		/** The eave's edge, the round rafters' end, the back edge and the back round rafters' end, along v. */
		double EaveF = 0.0;
		double EaveR = 0.0;
		double BackEdge = 0.0;
		double BackR = 0.0;
		bool bOpenBack = false;
		TArray<double> KnotV;
		TArray<double> KnotZ;
		TArray<double> KnotSlope;
		const FHallSpec* Spec = nullptr;

		static FHallSection Of(const FHallSpec& Spec);

		/** The lime bed's top (the tiles' bed) at v. */
		double Bed(double V) const;
		/** The rafters' top (the boarding's underside), straight purlin to purlin and on past the eave purlins. */
		double RafterTop(double V) const;
		/** The flying rafters' underside, front or back, flatter than the round rafters. */
		double FlyUnder(double V, bool bBack) const;
		/** The slab's underside: the boarding over the round rafters, the flying boarding past them. */
		double Under(double V) const;
		/** The top of a column standing under the purlin at index I (a facade column, a 金柱): under its lintel's seat. */
		double InnerColumnTop(int32 I) const;
		/** The purlin standing at v, or INDEX_NONE. */
		int32 PurlinAt(double V) const;
	};

	/** The roof's span along u at v: its ends, following a mitre where there is one. */
	void RoofSpan(const FHallSpec& Spec, double V, double& OutA, double& OutB);

	/**
	 * The tiles over their bed (§3.4) at FromRows metres across the rows from the first lower pan's axis and FromEave metres
	 * up the slope from the eave's edge: the lower pan's or the upper tile's top, with its course's lip.
	 */
	double TileHeight(const Plan::FTileStyle& Style, bool bTong, double FromRows, double FromEave);

	/** Draws a whole building into the pieces. */
	void DrawHall(TArray<FFormDrawing>& Out, const FHallSpec& Spec);

	/** The Glazed opening of a room's lattice and the Open openings of its open door bays, facing out of the room. */
	void HallOpenings(const FHallSpec& Spec, TArray<FFormOpening>& Out);

	/** A spec mirrored about the line x = Axis (the inner court's east halls and galleries): the same building, facing back. */
	FHallSpec MirroredAbout(const FHallSpec& Spec, double Axis, const FString& Name, const FString& Room);
}
