#pragma once

#include "CoreMinimal.h"
#include "FormMesh.h"
#include "Stone/StoneRecord.h"

namespace Musee::ArtMaking
{
	/** A scholar's rock to form: its kind, its height and the radius its foot keeps within (metres), and its seed. */
	struct FStoneSpec
	{
		const FArtStoneKind* Kind = nullptr;
		double HeightMeters = 0.0;
		double FootRadiusMeters = 0.0;
		int32 Seed = 0;
	};

	/** What is wrong with a kind of rock's record for forming a rock of it, one clause each. */
	[[nodiscard]] TArray<FString> StoneKindFaults(const FArtStoneKind& Kind);

	/**
	 * A rock's width: as wide as its foot's radius lets it be, its foot the kind's share of its widest, less the depth its
	 * skin is wrinkled by (which may stand proud of its form). Zero when its foot's radius can't hold its wrinkles.
	 */
	[[nodiscard]] double StoneWidthMeters(const FStoneSpec& Spec);

	/** The size of the cells its surface is found on: its surface lies within about a cell of its form. */
	[[nodiscard]] double StoneCellMeters(const FStoneSpec& Spec);

	/**
	 * A rock's carved stand (FArtStoneStand), carved to that rock, in the rock's frame moved down to the ground under the
	 * stand's feet: the rock stands on the recess's floor, RecessFloorMeters above that ground.
	 */
	struct FStoneStandForm
	{
		/** The stand's surface, empty when it can't be carved. */
		FFormMesh Mesh;
		/** Where the rock's foot sits: the floor of the recess, above the ground. */
		double RecessFloorMeters = 0.0;
		/**
		 * The recess's wall, as its distance from the rock's axis at equal angles from the rock's x axis towards its y
		 * (StandAngles of them), and the stand's side at its lip, the same way.
		 */
		TArray<double> RecessRadiiMeters;
		TArray<double> OutlineRadiiMeters;
	};

	/** The angles the recess and outline are carved at, all round. */
	inline constexpr int32 StandAngles = 128;

	/** What is wrong with the stands' record for carving a stand, one clause each. */
	[[nodiscard]] TArray<FString> StoneStandFaults(const FArtStoneStand& Stand);

	/**
	 * Carves a stand to a rock (formed by BuildStone, its surface found on cells of CellMeters), as a scholar's-rock stand
	 * is carved to its stone: the recess in its top follows the rock's own outline up the recess's depth, at each angle
	 * the farthest the rock's surface reaches there (and at the angles either side), a cell of its surface and the
	 * carver's clearance out from it; the lip runs round the recess at an even width, so the stand's plan is the rock's
	 * foot grown by its lip, never a disc; its side falls plumb from the lip with a half-round bead moulded round it, to its
	 * underside, which stands on short round feet spaced evenly round it just inside its side.
	 */
	[[nodiscard]] FStoneStandForm CarveStoneStand(const FFormMesh& Rock, double CellMeters, const FArtStoneStand& Stand);

	/**
	 * A scholar's rock as one closed surface in its own frame (its foot's centre at the origin on the plane z 0, z up, y
	 * its front), re-expressed from the as-built museum's Taihu rock (windows/Source/MuseeVision/Nature/MuseeTaihuRock.cpp).
	 * The rock is the solid a signed distance field describes, as water left it:
	 * - its body: at each height, an ellipse its kind's profile gives, the section's centre swaying as it rises, cut flat
	 *   at its foot (a rock is sawn to stand) and under its top, a ridge of the kind's peaks over saddles;
	 * - its holes worn right through (透, 漏), each round a line through its body, mostly front to back;
	 * - its pits worn into its skin, and its skin wrinkled (皺), drawn out up its height where water ran down it.
	 * Edges where these meet are rounded, as water rounds them. The surface is found by surface nets on a grid of the
	 * kind's cells up the rock's height: a vertex in every cell the surface crosses, at the mean of the crossings on its
	 * edges, facing along the field's gradient; a quad across every edge of the grid the surface crosses. UV0 is metres,
	 * across (x + y) and up. The same spec forms the same rock.
	 */
	void BuildStone(const FStoneSpec& Spec, FFormMesh& Out);
}
