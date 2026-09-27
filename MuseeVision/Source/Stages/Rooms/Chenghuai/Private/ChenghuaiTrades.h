#pragma once

#include "ChenghuaiHall.h"
#include "CoreMinimal.h"
#include "FormDrawing.h"

struct FSiteDescription;

/**
 * Chenghuai by trade, each drawing its part of the stage into the description's pieces: the buildings (§3.1-3.6), the
 * gates (§3.7), the free-standing walls with their openings, copings and leak windows (§3.5), the courts and their things
 * (§3.8), the garden (§3.9), the rockery and its pavilion (§3.9), the cases (§3.10) and the furniture, lanterns and
 * plaques (§3.10). The site's declarations (footprint, port, openings, anchors, enclosures) are ChenghuaiSite's.
 */
namespace Musee::Chenghuai
{
	/** Every building of the table, as drawn: the house's halls and galleries, the 垂花门's roofs, the garden's buildings. */
	TArray<FHallSpec> Buildings();

	void DrawBuildings(TArray<FFormDrawing>& Out);
	void DrawGates(TArray<FFormDrawing>& Out);
	void DrawWalls(TArray<FFormDrawing>& Out);
	void DrawCourts(TArray<FFormDrawing>& Out);
	void DrawGarden(TArray<FFormDrawing>& Out);
	void DrawRockery(TArray<FFormDrawing>& Out);
	void DrawCases(TArray<FFormDrawing>& Out);
	void DrawFurniture(TArray<FFormDrawing>& Out);
	void DescribeSite(FSiteDescription& Out);

	/**
	 * A wall's tiled coping (§3.5): a corbel course, a lime bed rising to a crown, 合瓦 rows down both sides, and the coping's
	 * ridge; along a frame's u from A0 to A1, centred on v = Centre over a wall Thick thick, its bed at Z.
	 */
	void DrawCoping(TArray<FFormDrawing>& Out, const FString& Name, const FBuildingFrame& F, double A0, double A1, double Centre, double Thick,
		double Z);

	/** The flights of steps (§3.2): stone treads by Blondel's rule, with their sloping side stones. */
	void DrawFlights(TArray<FFormDrawing>& Out);

	/** The rockery's surface over the plan, metres above the datum (0 off its mass): where the pine stands, and the seats. */
	double RockeryHeight(const FVector2D& Point);
}
