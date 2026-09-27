#include "MatterMasterParameters.h"
#include "MatterMasterParts.h"
#include "MatterMasters.h"
#include "MatterNames.h"

// The masters of textiles and thin sheets: an opaque cloth with its fibres' sheen, and a thin sheet that scatters the
// light it passes, lit from both faces.

namespace Musee::Matter
{
namespace
{
	using namespace MasterParts;
	using namespace MasterParameter;

	const FLinearColor NeutralGrey(SlabDiffuseAlbedo, SlabDiffuseAlbedo, SlabDiffuseAlbedo);
	/** An uninstanced sheet passes nothing: an opaque sheet until its record says what it transmits. */
	const FLinearColor Opaque(0.0f, 0.0f, 0.0f);

	FMasterSettings OpaqueNanite(bool bTwoSided)
	{
		FMasterSettings Settings;
		Settings.BlendMode = BLEND_Opaque;
		Settings.bUsedWithNanite = true;
		Settings.bTwoSided = bTwoSided;
		return Settings;
	}

	void AddClothParameters(FMasterRecipe& Recipe)
	{
		AddColour(Recipe, DiffuseAlbedo, SubstanceGroup, NeutralGrey, TEXT("1"), TEXT("Linear diffuse albedo of the dyed fibre, from the substance's record"));
		AddScalar(Recipe, RefractiveIndex, SubstanceGroup, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the fibre, from the substance's record"));
		AddScalar(Recipe, GrainMillimeters, SubstanceGroup, Unset, TEXT("mm"), TEXT("The finest structure the cloth shows: its thread, from the substance's record"));
		AddScalar(Recipe, BedMillimeters, SubstanceGroup, Unset, TEXT("mm"), TEXT("The coarsest: the length of its yarn's slubs, from the substance's record"));
		AddScalar(Recipe, BedAspect, SubstanceGroup, Unstretched, TEXT("1"), TEXT("How many times coarser the structure is along the warp than across it"));
		AddScalar(Recipe, HurstExponent, SubstanceGroup, Unset, TEXT("1"), TEXT("Self-affinity of the face: relief falls by 2^-H for each halving of scale"));
		AddScalar(Recipe, ToneSpread, SubstanceGroup, Unset, TEXT("1"), TEXT("Half-range of ln(albedo) over the face"));
		AddScalar(Recipe, RoughnessSpread, SubstanceGroup, Unset, TEXT("1"), TEXT("Half-range of ln(roughness) over the face"));
		AddScalar(Recipe, Roughness, SubstanceGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the fibres' highlight, from the substance's record"));
		AddScalar(Recipe, Sheen, SubstanceGroup, Unset, TEXT("1"), TEXT("Weight of the sheen the standing fibre ends give (Substrate's fuzz amount)"));
		AddScalar(Recipe, SheenRoughness, SubstanceGroup, SlabRoughness, TEXT("1"), TEXT("How widely the sheen spreads about grazing (Substrate's fuzz roughness)"));
		AddScalar(Recipe, Haziness, WorkingGroup, Unset, TEXT("1"), TEXT("Haze of the worked cloth's face, from the working's record"));
		AddScalar(Recipe, ReliefMillimeters, WorkingGroup, Unset, TEXT("mm"), TEXT("Amplitude of the cloth's relief as it is laid, from the working's record"));
		AddPieceTableIndex(Recipe);
		AddHistoryInputs(Recipe, EPores::Open);
	}

	/** The cloth's face: its yarn's slubs along the warp (Cloth.ush). */
	FMasterCall ClothFaceCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Face");
		Call.Includes = { LibraryFrame, LibraryCloth };
		Call.Code = TEXT(R"(FMatterStone Yarn;
Yarn.GrainMillimeters = GrainMillimeters;
Yarn.BedMillimeters = BedMillimeters;
Yarn.BedAspect = BedAspect;
Yarn.HurstExponent = HurstExponent;
Yarn.RoughnessSpread = RoughnessSpread;
Yarn.ToneSpread = ToneSpread;
Yarn.ReliefMillimeters = ReliefMillimeters;
const FMatterStoneFace Face = MatterClothFace(FaceMeters, PieceTableIndex, Yarn, Roughness);
FaceRoughness = Face.Roughness;
ReliefSlope = Face.ReliefSlope;
return Face.Tone;)");
		Call.Inputs = {
			Reads(TEXT("FaceMeters"), 2, MasterInput::TexCoord0, nullptr), Reads(PieceTableIndex, 1),
			Reads(GrainMillimeters, 1), Reads(BedMillimeters, 1), Reads(BedAspect, 1), Reads(HurstExponent, 1),
			Reads(RoughnessSpread, 1), Reads(ToneSpread, 1), Reads(ReliefMillimeters, 1), Reads(Roughness, 1),
		};
		Call.ReturnComponents = 1;
		Call.Outputs = { Writes(TEXT("FaceRoughness"), 1), Writes(TEXT("ReliefSlope"), 3) };
		return Call;
	}

	/** The fibres' surface under the law; the relief's slope is already in the face's tangent axes. */
	FMasterCall ClothHistoryCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("History");
		Call.Includes = { LibraryHistory, LibraryCloth };
		Call.Code = HistoryDeclarations(EPores::Open, TextureDepthMillimeters) + TEXT(R"(FMatterSurface Surface = MatterClothSurface(DiffuseAlbedo * Tone, RefractiveIndex, FaceRoughness, Slope.xy);
Surface.Haziness = Haziness;
)") + HistoryApplied() + TEXT(R"(SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
SurfaceHaziness = Marked.Haziness;
SurfaceNormal = MatterSlopeNormal(Marked.Slope);
return Marked.DiffuseAlbedo;)");
		Call.Inputs = {
			Reads(DiffuseAlbedo, 3), Reads(TEXT("Tone"), 1, TEXT("Face"), nullptr), Reads(RefractiveIndex, 1),
			Reads(TEXT("FaceRoughness"), 1, TEXT("Face"), TEXT("FaceRoughness")), Reads(Haziness, 1),
			Reads(TEXT("Slope"), 3, TEXT("Face"), TEXT("ReliefSlope")),
		};
		Call.Inputs.Append(HistoryPins(EPores::Open, TextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("SurfaceF0"), 3), Writes(TEXT("SurfaceRoughness"), 1), Writes(TEXT("SurfaceHaziness"), 1), Writes(TEXT("SurfaceNormal"), 3) };
		return Call;
	}

	void AddSheetParameters(FMasterRecipe& Recipe)
	{
		AddColour(Recipe, DiffuseAlbedo, SubstanceGroup, NeutralGrey, TEXT("1"), TEXT("The sheet's diffuse reflectance, linear, from the substance's record"));
		AddScalar(Recipe, RefractiveIndex, SubstanceGroup, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the sheet's fibre or glass, from the substance's record"));
		AddColour(Recipe, SheetTransmittance, SubstanceGroup, Opaque, TEXT("1"), TEXT("The sheet's diffuse transmittance, linear, from the substance's record"));
		AddScalar(Recipe, Roughness, WorkingGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the sheet's face as worked, from the working's record"));
		AddHistoryInputs(Recipe, EPores::Open);
	}

	/** The sheet's face under the law, and the path Substrate's two-sided wrap needs to pass what it transmits. */
	FMasterCall SheetCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Sheet");
		Call.Includes = { LibraryHistory, LibrarySheet };
		Call.Code = HistoryDeclarations(EPores::Open, TextureDepthMillimeters) + TEXT(R"(const FMatterSurface Surface = MatterSheetSurface(DiffuseAlbedo, RefractiveIndex, Roughness);
)") + HistoryApplied() + TEXT(R"(SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
MeanFreePath = MatterWrapMeanFreePathCentimeters(SheetTransmittance);
return Marked.DiffuseAlbedo;)");
		Call.Inputs = { Reads(DiffuseAlbedo, 3), Reads(RefractiveIndex, 1), Reads(SheetTransmittance, 3), Reads(Roughness, 1) };
		Call.Inputs.Append(HistoryPins(EPores::Open, TextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("SurfaceF0"), 3), Writes(TEXT("SurfaceRoughness"), 1), Writes(TEXT("MeanFreePath"), 3) };
		return Call;
	}
}

FMasterRecipe ClothRecipe(UMaterialParameterCollection& Moment)
{
	FMasterRecipe Recipe = StartRecipe(MasterName(FName(Masters::Cloth)), Moment, OpaqueNanite(false));
	AddClothParameters(Recipe);
	Recipe.Calls = { ClothFaceCall(), ClothHistoryCall() };
	Recipe.Slab.DiffuseAlbedo = From(TEXT("History"));
	Recipe.Slab.F0 = From(TEXT("History"), TEXT("SurfaceF0"));
	Recipe.Slab.Roughness = From(TEXT("History"), TEXT("SurfaceRoughness"));
	Recipe.Slab.Haziness = From(TEXT("History"), TEXT("SurfaceHaziness"));
	Recipe.Slab.Normal = From(TEXT("History"), TEXT("SurfaceNormal"));
	// The sheen is the fibres' own colour, scattered back by their standing ends.
	Recipe.Slab.FuzzAmount = From(Sheen);
	Recipe.Slab.FuzzColor = From(TEXT("History"));
	Recipe.Slab.FuzzRoughness = From(SheenRoughness);
	return Recipe;
}

FMasterRecipe SheetRecipe(UMaterialParameterCollection& Moment)
{
	FMasterRecipe Recipe = StartRecipe(MasterName(FName(Masters::Sheet)), Moment, OpaqueNanite(true));
	AddSheetParameters(Recipe);
	Recipe.Calls = { SheetCall() };
	Recipe.Slab.DiffuseAlbedo = From(TEXT("Sheet"));
	Recipe.Slab.F0 = From(TEXT("Sheet"), TEXT("SurfaceF0"));
	Recipe.Slab.Roughness = From(TEXT("Sheet"), TEXT("SurfaceRoughness"));
	Recipe.Slab.SSSMFP = From(TEXT("Sheet"), TEXT("MeanFreePath"));
	Recipe.Slab.SubSurface = MSS_TwoSidedWrap;
	return Recipe;
}
}
