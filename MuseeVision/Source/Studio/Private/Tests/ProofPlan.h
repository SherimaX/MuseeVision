#pragma once

#include "CoreMinimal.h"
#include "Make/MakePlan.h"

namespace Musee::Studio
{
	/** The shape of Studio's proof. A test changes it to see exactly what a make recasts. */
	struct FProofShape
	{
		/** The columns' height: 2.8 m, seven lower diameters of 0.4 m. */
		double ColumnHeightMeters = 2.8;
	};

	/**
	 * Where the proof is made: Studio's own folder, /Game/Generated/Studio, laid out as the museum's is under
	 * /Game/Generated, so the proof never touches a museum asset.
	 */
	FMakePaths ProofPaths();

	/**
	 * Studio's proof: one stage, Proof, of a glazed stone wall between two lathed columns, which the proof's museum
	 * level places by a translation. It runs every step of the make (both cast rules, the split of a mixed piece, a
	 * piece placed twice, a stage level and a museum level of Level Instances) on real geometry. The proof is dressed
	 * in two stock engine materials, one opaque and one translucent; it makes no claim about any substance.
	 */
	FMakePlan ProofPlan(const FProofShape& Shape = FProofShape());
}
