#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"

class UMaterialInterface;

namespace Musee::Studio
{
	/** How the renderer draws a cast part, which decides how it is built. */
	enum class ECastKind : uint8
	{
		/** Stone, plaster, wood, metal: Nanite geometry, a mesh distance field and Lumen cards. */
		Opaque,
		/** Glass and water: drawn by the translucency and water passes, which Nanite does not rasterise. */
		Translucent,
	};

	/** The revision of the cast rules. It is part of every mesh's input hash, so changing a rule recasts every mesh. */
	inline constexpr int64 CastRulesRevision = 1;

	/** The kind a material asks for: translucent blend modes and single-layer water are Translucent. */
	ECastKind KindOf(const UMaterialInterface& Material);

	/** What a part's asset name adds to its cast's name: nothing for the opaque part, "_Translucent" for the other. */
	const TCHAR* PartSuffix(ECastKind Kind);

	/** How many Lumen cards a part is given, from its kind and its largest extent in metres. */
	int32 LumenCardsFor(ECastKind Kind, double LargestExtentMeters);

	/**
	 * The build settings of a part: the drawing's normals and tangents kept, full-precision UVs (they run to tens of
	 * metres), no lightmap UVs (Lumen lights the museum), and a distance field and cards for opaque parts only.
	 */
	FMeshBuildSettings BuildSettingsFor(ECastKind Kind, double LargestExtentMeters);

	/**
	 * The Nanite settings of a part: on for opaque parts, off for translucent ones. The fallback mesh keeps every
	 * triangle, because complex collision and non-Nanite ray tracing read it: the visitor walks against the piece as drawn.
	 */
	FMeshNaniteSettings NaniteSettingsFor(ECastKind Kind);
}
