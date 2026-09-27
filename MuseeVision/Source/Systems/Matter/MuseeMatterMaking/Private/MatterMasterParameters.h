#pragma once

#include "CoreMinimal.h"

/**
 * The names of the masters' parameters: the one list the master recipes declare and the instance composer writes, so a
 * parameter cannot be declared by one and missed by the other. Each is written from one record field (see
 * MatterInstances.cpp); the unit is in the name or stated in the recipe.
 */
namespace Musee::Matter::MasterParameter
{
	// The substance (Data/Matter/Shading/Substances).
	inline constexpr const TCHAR* DiffuseAlbedo = TEXT("DiffuseAlbedo");
	inline constexpr const TCHAR* RefractiveIndex = TEXT("RefractiveIndex");
	inline constexpr const TCHAR* Porosity = TEXT("Porosity");
	inline constexpr const TCHAR* WettingDepthMillimeters = TEXT("WettingDepthMillimeters");
	inline constexpr const TCHAR* GrainMillimeters = TEXT("GrainMillimeters");
	inline constexpr const TCHAR* BedMillimeters = TEXT("BedMillimeters");
	inline constexpr const TCHAR* BedAspect = TEXT("BedAspect");
	inline constexpr const TCHAR* HurstExponent = TEXT("HurstExponent");
	inline constexpr const TCHAR* ToneSpread = TEXT("ToneSpread");
	inline constexpr const TCHAR* RoughnessSpread = TEXT("RoughnessSpread");
	inline constexpr const TCHAR* AbsorptionPerMillimeter = TEXT("AbsorptionPerMillimeter");
	inline constexpr const TCHAR* ScatteringPerMillimeter = TEXT("ScatteringPerMillimeter");
	inline constexpr const TCHAR* MetalF0 = TEXT("MetalF0");
	inline constexpr const TCHAR* MetalF82 = TEXT("MetalF82");
	inline constexpr const TCHAR* FilmAlbedo = TEXT("FilmAlbedo");
	inline constexpr const TCHAR* FilmRefractiveIndex = TEXT("FilmRefractiveIndex");
	inline constexpr const TCHAR* FilmRoughness = TEXT("FilmRoughness");
	inline constexpr const TCHAR* FilmCover = TEXT("FilmCover");
	inline constexpr const TCHAR* PaintWhiteAlbedo = TEXT("PaintWhiteAlbedo");
	inline constexpr const TCHAR* LatewoodFraction = TEXT("LatewoodFraction");
	inline constexpr const TCHAR* LatewoodTone = TEXT("LatewoodTone");
	inline constexpr const TCHAR* RayFleck = TEXT("RayFleck");
	inline constexpr const TCHAR* RayTone = TEXT("RayTone");
	inline constexpr const TCHAR* RayLengthMillimeters = TEXT("RayLengthMillimeters");
	inline constexpr const TCHAR* RayWidthMillimeters = TEXT("RayWidthMillimeters");
	inline constexpr const TCHAR* Sheen = TEXT("Sheen");
	inline constexpr const TCHAR* SheenRoughness = TEXT("SheenRoughness");
	inline constexpr const TCHAR* SheetTransmittance = TEXT("SheetTransmittance");
	inline constexpr const TCHAR* TissueTransmittanceRatio = TEXT("TissueTransmittanceRatio");

	// The working (Data/Matter/Shading/Workings).
	inline constexpr const TCHAR* Roughness = TEXT("Roughness");
	inline constexpr const TCHAR* RoughnessAcross = TEXT("RoughnessAcross");
	inline constexpr const TCHAR* Haziness = TEXT("Haziness");
	inline constexpr const TCHAR* TextureDepthMillimeters = TEXT("TextureDepthMillimeters");
	inline constexpr const TCHAR* ReliefMillimeters = TEXT("ReliefMillimeters");
	inline constexpr const TCHAR* PaneThicknessMillimeters = TEXT("PaneThicknessMillimeters");
	inline constexpr const TCHAR* DyeAbsorptionPerMillimeter = TEXT("DyeAbsorptionPerMillimeter");
	inline constexpr const TCHAR* LeafF0 = TEXT("LeafF0");
	inline constexpr const TCHAR* LeafF82 = TEXT("LeafF82");
	inline constexpr const TCHAR* LeafRoughness = TEXT("LeafRoughness");
	inline constexpr const TCHAR* LeafTextureDepthMillimeters = TEXT("LeafTextureDepthMillimeters");
	inline constexpr const TCHAR* FinishRefractiveIndex = TEXT("FinishRefractiveIndex");
	inline constexpr const TCHAR* FinishCover = TEXT("FinishCover");
	inline constexpr const TCHAR* StainTransmittance = TEXT("StainTransmittance");

	// The history (Data/Matter/Shading/Histories).
	inline constexpr const TCHAR* AgeYears = TEXT("AgeYears");
	inline constexpr const TCHAR* PassagesPerYear = TEXT("PassagesPerYear");
	inline constexpr const TCHAR* SkyExposure = TEXT("SkyExposure");
	inline constexpr const TCHAR* KeptWet = TEXT("KeptWet");
	inline constexpr const TCHAR* DepositionPerYear = TEXT("DepositionPerYear");
	inline constexpr const TCHAR* WashRemovalPerYear = TEXT("WashRemovalPerYear");
	inline constexpr const TCHAR* SoilAlbedo = TEXT("SoilAlbedo");
	inline constexpr const TCHAR* WornRoughness = TEXT("WornRoughness");
	inline constexpr const TCHAR* WornHaziness = TEXT("WornHaziness");
	inline constexpr const TCHAR* WearPassages = TEXT("WearPassages");

	// A piece's own (its Custom Primitive Data, which Studio's caster sets) and a work's own (Art's image and its size,
	// which a work's paint instance carries; MatterPaintLayer.h).
	inline constexpr const TCHAR* PieceTableIndex = TEXT("PieceTableIndex");
	inline constexpr const TCHAR* Image = TEXT("Image");
	inline constexpr const TCHAR* ImageWidthMeters = TEXT("ImageWidthMeters");
	inline constexpr const TCHAR* ImageHeightMeters = TEXT("ImageHeightMeters");
}
