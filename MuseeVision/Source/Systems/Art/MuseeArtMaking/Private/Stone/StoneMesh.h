#pragma once

#include "CoreMinimal.h"
#include "Stone/StoneRecord.h"

class FFormMesh;

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
