#pragma once

#include "CoreMinimal.h"
#include "Chenghuai/ChenghuaiKit.h"

/**
 * Real grey clay tiles for Chenghuai's roofs, laid as they are in Beijing (Materials board, 布筒瓦 and 合瓦):
 *
 * - 筒瓦 (Tong): concave pans (板瓦) in rows, the joints between them covered by half-round cover tiles (筒瓦); drip tiles
 *   (滴水) under the pans at the eave and round end tiles (勾头) on the covers. On the gate, the main hall and the
 *   festooned gate: the higher rank.
 * - 合瓦 (He, "butterfly"): pans laid face up (底瓦) and pans laid face down over their joints (盖瓦); a drip tile under
 *   each lower pan and an edge tile (花边瓦) on each upper one. Everywhere else.
 *
 * Tiles lie in courses: each course's lower end stands proud of the next (a lip), so the rows read as overlapping tiles
 * in grazing light, not as corrugated sheet. Rows are cut exactly on valleys, hips or the end of a slope (the Limits,
 * half-planes c·p ≤ k), with a cap of the row's own section there. Copied and generalised from the Chinese Wing's
 * cloister roofs (ChineseWingGeometry.cpp), which is being retired.
 */
namespace ChenghuaiRoof
{
	using ChenghuaiKit::FPart;
	using ChenghuaiKit::FProfile;

	/** A slope that carries rows of tiles: d down it (towards the eave), s along it; the bed's top Bed(d, s). */
	struct FSlope
	{
		FVector2D Origin = FVector2D::ZeroVector;
		FVector2D DDir = FVector2D(1, 0);
		FVector2D SDir = FVector2D(0, 1);
		TFunction<double(double, double)> Bed;
		TArray<FVector> Limits;
		double From = 0.0, To = 1.0;       // the rows' extent in d: from the ridge's foot to the eave's edge
		bool bOrnaments = true;             // drip and end tiles where a row reaches the eave

		FVector D3() const { return FVector(DDir.X, DDir.Y, 0.0); }
		FVector S3() const { return FVector(SDir.X, SDir.Y, 0.0); }
		FVector At(double D, double S, double Up) const
		{
			const FVector2D P = Origin + DDir * D + SDir * S;
			return FVector(P.X, P.Y, Bed(D, S) + Up);
		}
		FVector Normal(double D, double S) const
		{
			constexpr double E = 0.005;
			const double GD = (Bed(D + E, S) - Bed(D - E, S)) / (2 * E);
			const double GS = (Bed(D, S + E) - Bed(D, S - E)) / (2 * E);
			const FVector2D G = DDir * GD + SDir * GS;
			return FVector(-G.X, -G.Y, 1.0).GetSafeNormal();
		}
		double CrossSlope(double D, double S) const
		{
			constexpr double E = 0.005;
			return (Bed(D, S + E) - Bed(D, S - E)) / (2 * E);
		}
	};

	enum class ETiles : uint8 { Tong, He };

	/** Sizes (metres): rows every Pitch; the lower pan's and the upper tile's half-widths; the courses' length and lip. */
	struct FTileStyle
	{
		ETiles Kind = ETiles::He;
		double Pitch = 0.26;
		double PanHalf = 0.12;
		double TopHalf = 0.12;
		double PanCourse = 0.19;
		double TopCourse = 0.19;
		double Lip = 0.009;

		static FTileStyle Tong()
		{
			FTileStyle S;
			S.Kind = ETiles::Tong;
			S.Pitch = 0.26;
			S.PanHalf = 0.115;
			S.TopHalf = 0.065;
			S.PanCourse = 0.19;
			S.TopCourse = 0.30;
			return S;
		}
		static FTileStyle He()
		{
			FTileStyle S;
			S.Kind = ETiles::He;
			S.Pitch = 0.25;
			S.PanHalf = 0.115;
			S.TopHalf = 0.105;
			S.PanCourse = 0.17;
			S.TopCourse = 0.17;
			return S;
		}
	};

	/** Rows of tiles over a slope from s = SA to SB (rows at k·Pitch + Offset). */
	void TileRows(FPart& Part, const FSlope& Slope, double SA, double SB, const FTileStyle& Style, double Offset = 0.0);

	/** One row of lower pans or of upper tiles, centred at s = Centre, over d in [DFrom, DTo] (clipped by the Limits). */
	void TileRow(FPart& Part, const FSlope& Slope, double Centre, bool bUpper, const FTileStyle& Style, double DFrom, double DTo);
}
