#pragma once

#include "CoreMinimal.h"

/**
 * What a piece's own primitive carries for Matter's masters: its Custom Primitive Data, which Studio's caster sets on
 * each piece's static mesh component (the stage contract: the stage's origin, then the piece's table index).
 *
 * The table index is the piece's row in its stage's piece table; until that table exists the masters read it as the
 * piece's seed, so that no two pieces of one stone or plaster repeat. A piece left at zero shares the pattern of every
 * other piece left at zero.
 */
namespace Musee::Matter::PieceData
{
	/** The stage's origin, in engine units: slots 0, 1 and 2. */
	inline constexpr int32 StageOriginIndex = 0;

	/** The piece's table index: slot 3. */
	inline constexpr int32 TableIndex = 3;
}
