#pragma once

#include "CoreMinimal.h"

/**
 * The ground under Élan as the shaft cuts it (§7): crushed stone under the Atrium's slab, topsoil, subsoil, clay-with-
 * flints with a solution pipe dropping into the chalk, and the Upper Chalk with its bedding partings. The rule gives,
 * at each point of the cut face (θ the plan angle round the axis, z the height), how far the face stands proud of its
 * nominal cylinder r 2.660 towards the axis, and which layer is there. Its noise is seeded periodic Perlin noise round
 * the cylinder keyed `cube.ground`, and its stones lie in jittered cells that wrap round the face, so the face closes
 * on itself without a seam. The rule is the as-built Cube's (CubeStructure.cpp:238-390), re-expressed.
 */
namespace Musee::Cube::Strata
{
	/** The layers, top down. */
	enum class ELayer : uint8
	{
		CrushedStone,
		Topsoil,
		Subsoil,
		ClayWithFlints,
		Chalk,
	};

	inline constexpr int32 Layers = 5;

	/** The layer's name, as its surface and piece are named: crushed-stone, topsoil, subsoil, clay-with-flints, chalk. */
	[[nodiscard]] const TCHAR* LayerName(ELayer Layer);

	/** The heights of the four contacts at a plan angle, top down: hardcore | topsoil | subsoil | clay | chalk. */
	struct FContacts
	{
		double Z[4] = {};
	};

	/** The contacts at a plan angle (radians): the rule's waving means, and the solution pipe in the clay | chalk contact. */
	[[nodiscard]] FContacts ContactsAt(double Theta);

	/** The layer the rule puts at a point of the face. */
	[[nodiscard]] ELayer LayerAt(double Theta, double Z);

	/** How far the face stands proud of the nominal face at a point, metres; negative where it lies behind it. */
	[[nodiscard]] double ProudAt(double Theta, double Z);

	/** The deepest the rule's relief ever lies behind the nominal face, metres: the crumbly soils' two terms at their troughs. */
	[[nodiscard]] double DeepestRecess();

	/** The most the rule's relief ever stands proud of the nominal face, metres. */
	[[nodiscard]] double HighestProud();

	/**
	 * The ammonite's carving over its ground, a plane square to the shaft behind the chalk's face: at (X, Y) metres from
	 * the coil's centre (X round the face, Y up), how far the exposed half of the shell stands in front of a plane that
	 * lies Behind metres behind its mid-plane; zero off the shell.
	 */
	[[nodiscard]] double AmmoniteAt(double X, double Y, double Behind);
}
