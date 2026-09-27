#include "MatterMasterParameters.h"
#include "MatterMasterParts.h"
#include "MatterMasters.h"
#include "MatterNames.h"

// The masters of what light passes into: a glass pane, still water, and a painting's paint layer.

namespace Musee::Matter
{
namespace
{
	using namespace MasterParts;
	using namespace MasterParameter;

	const FLinearColor NeutralGrey(SlabDiffuseAlbedo, SlabDiffuseAlbedo, SlabDiffuseAlbedo);

	void AddDielectricParameters(FMasterRecipe& Recipe)
	{
		AddScalar(Recipe, RefractiveIndex, SubstanceGroup, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the interface, from the substance's record"));
		AddScalar(Recipe, Roughness, WorkingGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the surface, from the working's record"));
		AddHistoryInputs(Recipe, EPores::Open);
	}

	/** A glass pane's face: its interface under the law, and what it transmits through half the pane. */
	FMasterCall PaneCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Pane");
		Call.Includes = { LibraryGlass };
		Call.Code = HistoryDeclarations(EPores::Open, TextureDepthMillimeters) + TEXT(R"(const FMatterSurface Surface = MatterClearPane(RefractiveIndex, Roughness);
)") + HistoryApplied() + TEXT(R"(Transmittance = MatterSoiledTransmittance(MatterPaneFaceTransmittance(AbsorptionPerMillimeter, PaneThicknessMillimeters), MatterSoilCover(Place, Response));
SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
return Marked.DiffuseAlbedo;)");
		Call.Inputs = { Reads(RefractiveIndex, 1), Reads(Roughness, 1), Reads(AbsorptionPerMillimeter, 3), Reads(PaneThicknessMillimeters, 1) };
		Call.Inputs.Append(HistoryPins(EPores::Open, TextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("Transmittance"), 3), Writes(TEXT("SurfaceF0"), 3), Writes(TEXT("SurfaceRoughness"), 1) };
		return Call;
	}

	/** Still water: its surface under the law, and the volume beneath it, the water's own absorption and the dye's. */
	FMasterCall WaterCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Water");
		Call.Includes = { LibraryMedium };
		Call.Code = HistoryDeclarations(EPores::Open, TextureDepthMillimeters) + TEXT(R"(const FMatterSurface Surface = MatterStillSurface(RefractiveIndex, Roughness);
)") + HistoryApplied() + TEXT(R"(const float3 Absorption = AbsorptionPerMillimeter + DyeAbsorptionPerMillimeter;
Extinction = MatterExtinctionPerCentimeter(Absorption, ScatteringPerMillimeter);
Albedo = MatterSingleScatteringAlbedo(Absorption, ScatteringPerMillimeter);
SurfaceRoughness = Marked.Roughness;
return MatterSpecularFromF0(Marked.F0.x);)");
		Call.Inputs = {
			Reads(RefractiveIndex, 1), Reads(Roughness, 1), Reads(AbsorptionPerMillimeter, 3), Reads(ScatteringPerMillimeter, 3),
			Reads(DyeAbsorptionPerMillimeter, 3),
		};
		Call.Inputs.Append(HistoryPins(EPores::Open, TextureDepthMillimeters));
		Call.ReturnComponents = 1;
		Call.Outputs = { Writes(TEXT("Extinction"), 3), Writes(TEXT("Albedo"), 3), Writes(TEXT("SurfaceRoughness"), 1) };
		return Call;
	}

	/** A painting's paint layer: the work's image laid across its canvas at the work's size, under its varnish and the
	 *  law. The face's UV0 runs in metres from the image's top-left corner (Art's canvas). */
	FMasterCall CanvasCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Canvas");
		Call.Includes = { LibraryPaint };
		Call.Code = HistoryDeclarations(EPores::Open, TextureDepthMillimeters) + TEXT(R"(const float2 ImageCoordinates = MatterCanvasCoordinates(FaceMeters, float2(ImageWidthMeters, ImageHeightMeters));
const float3 ImageLinear = Texture2DSample(Image, ImageSampler, ImageCoordinates).rgb;
const FMatterSurface Surface = MatterPaintSurface(ImageLinear, PaintWhiteAlbedo, RefractiveIndex, Roughness);
)") + HistoryApplied() + TEXT(R"(SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
return Marked.DiffuseAlbedo;)");
		Call.Inputs = {
			Reads(TEXT("FaceMeters"), 2, MasterInput::TexCoord0, nullptr),
			Reads(Image, 0), Reads(ImageWidthMeters, 1), Reads(ImageHeightMeters, 1),
			Reads(PaintWhiteAlbedo, 3), Reads(RefractiveIndex, 1), Reads(Roughness, 1),
		};
		Call.Inputs.Append(HistoryPins(EPores::Open, TextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("SurfaceF0"), 3), Writes(TEXT("SurfaceRoughness"), 1) };
		return Call;
	}
}

FMasterRecipe GlassRecipe(UMaterialParameterCollection& Moment)
{
	FMasterSettings Settings;
	Settings.BlendMode = BLEND_TranslucentColoredTransmittance;
	Settings.bUsedWithNanite = false;
	Settings.bSurfaceTranslucency = true;
	FMasterRecipe Recipe = StartRecipe(MasterName(FName(Masters::Glass)), Moment, Settings);
	AddDielectricParameters(Recipe);
	AddColour(Recipe, AbsorptionPerMillimeter, SubstanceGroup, FLinearColor::Black, TEXT("1/mm"), TEXT("The glass's absorption coefficient at the primaries, from its record"));
	AddScalar(Recipe, PaneThicknessMillimeters, WorkingGroup, Unset, TEXT("mm"), TEXT("The glass the make-up puts in the light's path, from the working's record"));
	Recipe.Calls = { PaneCall() };
	Recipe.Transmittances = { FMasterTransmittance{ TEXT("PaneVolume"), From(TEXT("Pane"), TEXT("Transmittance")) } };
	Recipe.Slab.DiffuseAlbedo = From(TEXT("Pane"));
	Recipe.Slab.F0 = From(TEXT("Pane"), TEXT("SurfaceF0"));
	Recipe.Slab.Roughness = From(TEXT("Pane"), TEXT("SurfaceRoughness"));
	Recipe.Slab.SSSMFP = From(TEXT("PaneVolume"), TEXT("MFP"));
	Recipe.Slab.SubSurface = MSS_SimpleVolume;
	return Recipe;
}

FMasterRecipe WaterRecipe(UMaterialParameterCollection& Moment)
{
	FMasterSettings Settings;
	Settings.BlendMode = BLEND_Opaque;
	Settings.bUsedWithNanite = false;
	FMasterRecipe Recipe = StartRecipe(MasterName(FName(Masters::Water)), Moment, Settings);
	AddDielectricParameters(Recipe);
	AddColour(Recipe, AbsorptionPerMillimeter, SubstanceGroup, FLinearColor::Black, TEXT("1/mm"), TEXT("The water's absorption coefficient at the primaries, from its record"));
	AddColour(Recipe, ScatteringPerMillimeter, SubstanceGroup, FLinearColor::Black, TEXT("1/mm"), TEXT("The water's scattering coefficient at the primaries, from its record"));
	AddColour(Recipe, DyeAbsorptionPerMillimeter, WorkingGroup, FLinearColor::Black, TEXT("1/mm"), TEXT("What a dye adds to the water's absorption, from the working's record"));
	Recipe.Calls = { WaterCall() };
	Recipe.Surface = EMasterSurface::Water;
	Recipe.Water.Specular = From(TEXT("Water"));
	Recipe.Water.Roughness = From(TEXT("Water"), TEXT("SurfaceRoughness"));
	Recipe.Water.WaterAlbedo = From(TEXT("Water"), TEXT("Albedo"));
	Recipe.Water.WaterExtinction = From(TEXT("Water"), TEXT("Extinction"));
	return Recipe;
}

FMasterRecipe PaintRecipe(UMaterialParameterCollection& Moment)
{
	FMasterSettings Settings;
	Settings.BlendMode = BLEND_Opaque;
	Settings.bUsedWithNanite = true;
	FMasterRecipe Recipe = StartRecipe(MasterName(FName(Masters::Paint)), Moment, Settings);
	AddTexture(Recipe, Image, WorkGroup, EngineDefaultTexture, TEXT("The work's own image, a photograph of the painting, which each work's paint instance carries"));
	AddScalar(Recipe, ImageWidthMeters, WorkGroup, Unset, TEXT("m"), TEXT("The work's width, which its image spans, which each work's paint instance carries"));
	AddScalar(Recipe, ImageHeightMeters, WorkGroup, Unset, TEXT("m"), TEXT("The work's height, which its image spans, which each work's paint instance carries"));
	AddColour(Recipe, PaintWhiteAlbedo, SubstanceGroup, NeutralGrey, TEXT("1"), TEXT("The reflectance the image's white stands for: the painting's white, from the paint's record"));
	AddDielectricParameters(Recipe);
	Recipe.Calls = { CanvasCall() };
	Recipe.Slab.DiffuseAlbedo = From(TEXT("Canvas"));
	Recipe.Slab.F0 = From(TEXT("Canvas"), TEXT("SurfaceF0"));
	Recipe.Slab.Roughness = From(TEXT("Canvas"), TEXT("SurfaceRoughness"));
	return Recipe;
}
}
