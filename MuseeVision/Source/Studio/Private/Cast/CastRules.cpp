#include "Cast/CastRules.h"

#include "MaterialShared.h"
#include "Materials/MaterialInterface.h"

namespace Musee::Studio
{
	namespace
	{
		/**
		 * The engine's default card budget (FMeshBuildSettings::MaxLumenMeshCards): enough for a compact piece seen
		 * from its six axis directions.
		 */
		constexpr int32 CompactPieceCards = 12;

		/**
		 * A room-scale piece folds around corners the default budget leaves uncaptured. The old museum's bake gave
		 * every piece longer than 10 m 32 cards, which covered its room shells without surface-cache holes
		 * (windows/Source/MuseeVisionEditor/MuseeBakeLibrary.cpp, 2026).
		 */
		constexpr int32 RoomScaleCards = 32;
		constexpr double RoomScaleMeters = 10.0;

		/** A distance field at the engine's standard voxel density; translucent parts get none. */
		constexpr float StandardFieldResolution = 1.0f;
	}

	ECastKind KindOf(const UMaterialInterface& Material)
	{
		const bool bWater = Material.GetShadingModels().HasShadingModel(MSM_SingleLayerWater);
		return (IsTranslucentBlendMode(Material) || bWater) ? ECastKind::Translucent : ECastKind::Opaque;
	}

	const TCHAR* PartSuffix(ECastKind Kind)
	{
		return Kind == ECastKind::Translucent ? TEXT("_Translucent") : TEXT("");
	}

	int32 LumenCardsFor(ECastKind Kind, double LargestExtentMeters)
	{
		if (Kind == ECastKind::Translucent)
		{
			return 0;
		}
		return LargestExtentMeters > RoomScaleMeters ? RoomScaleCards : CompactPieceCards;
	}

	FMeshBuildSettings BuildSettingsFor(ECastKind Kind, double LargestExtentMeters)
	{
		const bool bOpaque = Kind == ECastKind::Opaque;
		FMeshBuildSettings Settings;
		Settings.bRecomputeNormals = false;
		Settings.bRecomputeTangents = false;
		Settings.bComputeWeightedNormals = false;
		Settings.bUseMikkTSpace = true;
		Settings.bRemoveDegenerates = false;
		// Parts are placed by rotation and translation only, never mirrored, so no reversed index buffer.
		Settings.bBuildReversedIndexBuffer = false;
		Settings.bUseHighPrecisionTangentBasis = true;
		Settings.bUseFullPrecisionUVs = true;
		Settings.bUseBackwardsCompatibleF16TruncUVs = false;
		Settings.bGenerateLightmapUVs = false;
		Settings.SrcLightmapIndex = 0;
		Settings.DstLightmapIndex = 0;
		Settings.BuildScale3D = FVector::OneVector;
		Settings.bGenerateDistanceFieldAsIfTwoSided = false;
		Settings.DistanceFieldResolutionScale = bOpaque ? StandardFieldResolution : 0.0f;
		Settings.MaxLumenMeshCards = LumenCardsFor(Kind, LargestExtentMeters);
		return Settings;
	}

	FMeshNaniteSettings NaniteSettingsFor(ECastKind Kind)
	{
		FMeshNaniteSettings Settings;
		Settings.bEnabled = Kind == ECastKind::Opaque;
		Settings.bExplicitTangents = true;
		Settings.KeepPercentTriangles = 1.0f;
		Settings.TrimRelativeError = 0.0f;
		Settings.GenerateFallback = ENaniteGenerateFallback::Enabled;
		Settings.FallbackTarget = ENaniteFallbackTarget::PercentTriangles;
		Settings.FallbackPercentTriangles = 1.0f;
		return Settings;
	}
}
