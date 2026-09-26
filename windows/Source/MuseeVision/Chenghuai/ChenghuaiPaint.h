#pragma once

#include "CoreMinimal.h"
#include "Chenghuai/ChenghuaiKit.h"

/**
 * The painted timber's texture atlas (T_ch_suhua, Scripts/chenghuai_textures.py): 8 rows of 4096 × 512 in a 4096²
 * image; a painted face maps its length to the row's width and its height to (part of) the row's height.
 *
 *   0-3  the eave lintel and board (檐枋 + 垫板) of a bay: end bands (箍头), scrolls (卡子) and the big arched panel
 *        (包袱) in the middle with an ink landscape, each row a different one
 *   4    a plain painted beam (箍头 at the ends, blue and green between): interior beams, backs and undersides
 *   5    a purlin's band (檩)
 *   6    the rafters' ends: 8 cells of round rafter heads (blue, with the 'dragon's eye' rings), 8 of the flying rafters'
 *        square heads (green with a gilt 万)
 *   7    the festooned gate's boards and the plaques' frames
 */
namespace ChenghuaiPaint
{
	using ChenghuaiKit::FPart;
	using ChenghuaiKit::FLocal;

	constexpr int32 Rows = 8;
	inline double RowV0(int32 Row) { return double(Row) / Rows; }
	inline double RowV1(int32 Row) { return double(Row + 1) / Rows; }

	/**
	 * A painted box in the frame L (u0 … u1 along its length): its long faces (±v) take the atlas row's band [B0, B1]
	 * (0 = the row's top, 1 = its bottom) with u mapped across the row's width; the underside and top take the middle
	 * of the plain row (4); the ends the band too.
	 */
	void Box(FPart& Part, const FString& Name, const FLocal& L, double U0, double U1, double V0, double V1, double Z0, double Z1,
			 int32 Row, double B0 = 0.0, double B1 = 1.0);

	/** A painted round member A → B (a purlin): the row's band round it, repeated every Repeat metres along it. */
	void Rod(FPart& Part, const FString& Name, const FVector& A, const FVector& B, double R, int32 Segments, int32 Row, double Repeat);

	/** A disc (the end of a round rafter) or a square (a flying rafter's end) facing N, the atlas cell (0-15) on it. */
	void EndCap(FPart& Part, const FVector& Centre, const FVector& N, const FVector& Up, double Half, bool bRound, int32 Cell);

	/** A rectangle (HalfW × HalfH about Centre, facing N, Up its up) showing the atlas rectangle UV (min, max). */
	void AtlasQuad(FPart& Part, const FVector& Centre, const FVector& N, const FVector& Up, double HalfW, double HalfH, const FBox2D& UV);

	/** A disc of radius R showing the atlas rectangle UV. */
	void AtlasDisc(FPart& Part, const FVector& Centre, const FVector& N, const FVector& Up, double R, const FBox2D& UV);
}

/**
 * The plaques' atlas (T_ch_plaques, Scripts/chenghuai_textures.py; Names board): 4096², black lacquer boards with gilt
 * characters, read right to left; discs; the couplet; the screen wall's carved heart. UV rectangles (0-1, v down).
 */
namespace ChenghuaiPlaques
{
	enum EPlaque : int32
	{
		Linchi, Tianqing, Changnan, Qingbi,          // row 0: 1024 × 512 each
		Tingyun, Shujuan, Woyou, Linquan,            // row 1
		Zhiyu, Jianshan, Youmu, Chenghuaitang,       // row 2
		DiscZheng, DiscZhong, DiscZhuang, DiscZhai,  // row 3: 512 × 512 discs
		DiscJi, DiscXiang, DiscRu, DiscYi,
		CoupletRight, CoupletLeft,                   // rows 4-5: 256 × 1024 upright boards
		ScreenHeart,                                 // rows 4-5: 1024 × 1024 at x 1024
		Count
	};

	inline FBox2D Rect(int32 P)
	{
		auto R = [](double X0, double Y0, double W, double H) { return FBox2D(FVector2D(X0 / 4096.0, Y0 / 4096.0), FVector2D((X0 + W) / 4096.0, (Y0 + H) / 4096.0)); };
		if (P < 12) { return R(1024.0 * (P % 4), 512.0 * (P / 4), 1024, 512); }
		if (P < 20) { return R(512.0 * (P - 12), 1536.0, 512, 512); }
		if (P == CoupletRight) { return R(0, 2048, 256, 1024); }
		if (P == CoupletLeft) { return R(256, 2048, 256, 1024); }
		return R(1024, 2048, 1024, 1024);
	}
}
