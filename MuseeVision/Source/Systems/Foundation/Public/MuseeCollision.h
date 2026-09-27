#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

/**
 * The museum's five collision classes, each an engine channel with a profile of the same name, defined once in
 * `Config/DefaultEngine.ini` (section `/Script/Engine.CollisionProfile`). Form assigns them to the pieces it makes;
 * Motion, Visitor and Site query them. Collision comes from the same solids as what is seen, and it is used only for
 * queries and the body's movement: nothing in the museum is a free rigid body.
 */
namespace Musee::Collision
{
	/** One collision class: the engine channel it answers on, and the profile that gives a primitive that channel. */
	struct FClass
	{
		ECollisionChannel Channel;
		const TCHAR* ProfileName;

		[[nodiscard]] FName Profile() const { return FName(ProfileName); }
	};

	/** A piece that isn't walked on: a wall, a column, a frame, a pane, a moving body's sides. Blocks everything. */
	inline constexpr FClass Solid{ECC_GameTraceChannel1, TEXT("Solid")};

	/** A piece whose upper faces the body walks on, within the human measure's slope: floors, treads, paving. Blocks everything. */
	inline constexpr FClass Walkable{ECC_GameTraceChannel2, TEXT("Walkable")};

	/** The visitor's body. Blocked by pieces; unseen, so it doesn't stop sight or the camera. */
	inline constexpr FClass Body{ECC_GameTraceChannel3, TEXT("Body")};

	/**
	 * Clearance: the channel of clearance queries (a travel volume swept against everything else, a reach agent), which
	 * pieces and the body block; and the profile of a volume that senses the body in a mechanism's next travel slice,
	 * which only overlaps the body.
	 */
	inline constexpr FClass Clearance{ECC_GameTraceChannel4, TEXT("Clearance")};

	/**
	 * A barrier that stops only the visitor's body, as a gallery's rail or a plinth's step keeps a visitor back from a
	 * work while the eye passes on to it: the box of the Atrium stele's foot, 2.2 m high (atrium.md section 3.7). Sight,
	 * the camera and every query but the body's pass through it; it is not matter, so a mechanism's clearance ignores it.
	 */
	inline constexpr FClass Barrier{ECC_GameTraceChannel5, TEXT("Barrier")};
}
