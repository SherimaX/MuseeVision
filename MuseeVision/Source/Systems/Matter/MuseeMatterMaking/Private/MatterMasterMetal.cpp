#include "MatterMasterParameters.h"
#include "MatterMasterParts.h"
#include "MatterMasters.h"
#include "MatterNames.h"

// The metal master, the daguerreotype's plate, and the gilt leaf any host master can carry as a vertical layer.

namespace Musee::Matter
{
namespace
{
	using namespace MasterParts;
	using namespace MasterParameter;

	const FLinearColor SlabF0Grey(SlabF0, SlabF0, SlabF0);
	const FLinearColor NeutralGrey(SlabDiffuseAlbedo, SlabDiffuseAlbedo, SlabDiffuseAlbedo);

	FMasterSettings OpaqueNanite()
	{
		FMasterSettings Settings;
		Settings.BlendMode = BLEND_Opaque;
		Settings.bUsedWithNanite = true;
		return Settings;
	}

	void AddMetalParameters(FMasterRecipe& Recipe)
	{
		AddColour(Recipe, MetalF0, SubstanceGroup, SlabF0Grey, TEXT("1"), TEXT("The metal's reflectance at normal incidence, linear Rec.709, from its record"));
		AddColour(Recipe, MetalF82, SubstanceGroup, SlabF0Grey, TEXT("1"), TEXT("The metal's reflectance at 82 degrees, where its edge colour is fitted, from its record"));
		AddColour(Recipe, FilmAlbedo, SubstanceGroup, NeutralGrey, TEXT("1"), TEXT("Diffuse albedo of the film over the metal: the working's patina or the substance's tarnish"));
		AddScalar(Recipe, FilmRefractiveIndex, SubstanceGroup, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the film's mineral"));
		AddScalar(Recipe, FilmRoughness, SubstanceGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the film's surface"));
		AddScalar(Recipe, FilmCover, HistoryGroup, Unset, TEXT("1"), TEXT("Fraction of the metal the film covers: the working's patina, or the tarnish its history has grown"));
		AddScalar(Recipe, Roughness, WorkingGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness along the grain of the worked face, from the working's record"));
		AddScalar(Recipe, RoughnessAcross, WorkingGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness across the grain, from the working's record"));
		AddHistoryInputs(Recipe, EPores::Sealed);
	}

	/** The metal's face under its film and its history. Its grain runs along the mesh's tangent, the piece's length. */
	FMasterCall MetalCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Metal");
		Call.Includes = { LibraryMetal };
		Call.Code = HistoryDeclarations(EPores::Sealed, TextureDepthMillimeters) + TEXT(R"(const float2 Lobe = MatterBrushedLobe(Roughness, RoughnessAcross);
const float Cover = MatterWornFilmCover(FilmCover, MatterWornFraction(Place, Response));
const FMatterSurface Surface = MatterFilmed(MatterBareMetal(MetalF0, Lobe.x), FilmAlbedo, FilmRefractiveIndex, FilmRoughness, Cover);
)") + HistoryApplied() + TEXT(R"(SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
SurfaceAnisotropy = Lobe.y;
SurfaceEdgeTint = MatterFilmedEdgeTint(MatterEdgeTintFromF82(MetalF0, MetalF82), Cover);
return Marked.DiffuseAlbedo;)");
		Call.Inputs = {
			Reads(MetalF0, 3), Reads(MetalF82, 3), Reads(FilmAlbedo, 3), Reads(FilmRefractiveIndex, 1), Reads(FilmRoughness, 1),
			Reads(FilmCover, 1), Reads(Roughness, 1), Reads(RoughnessAcross, 1),
		};
		Call.Inputs.Append(HistoryPins(EPores::Sealed, TextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("SurfaceF0"), 3), Writes(TEXT("SurfaceRoughness"), 1), Writes(TEXT("SurfaceAnisotropy"), 1), Writes(TEXT("SurfaceEdgeTint"), 3) };
		return Call;
	}

	/** A daguerreotype's face: its silver, with the work's image as the particles' cover over it, under the law. The face's
	 *  UV0 runs in metres from the image's top-left corner, as a canvas's does (Art's plate). */
	FMasterCall PlateCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Plate");
		Call.Includes = { LibraryPlate };
		Call.Code = HistoryDeclarations(EPores::Sealed, TextureDepthMillimeters) + TEXT(R"(const float2 ImageCoordinates = MatterCanvasCoordinates(FaceMeters, float2(ImageWidthMeters, ImageHeightMeters));
const float3 ImageLinear = Texture2DSample(Image, ImageSampler, ImageCoordinates).rgb;
const FMatterSurface Surface = MatterPlateSurface(ImageLinear, MetalF0, Roughness, FilmAlbedo, FilmRefractiveIndex, FilmRoughness);
)") + HistoryApplied() + TEXT(R"(SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
SurfaceEdgeTint = MatterFilmedEdgeTint(MatterEdgeTintFromF82(MetalF0, MetalF82), MatterPlateCover(ImageLinear));
return Marked.DiffuseAlbedo;)");
		Call.Inputs = {
			Reads(TEXT("FaceMeters"), 2, MasterInput::TexCoord0, nullptr),
			Reads(Image, 0), Reads(ImageWidthMeters, 1), Reads(ImageHeightMeters, 1),
			Reads(MetalF0, 3), Reads(MetalF82, 3), Reads(FilmAlbedo, 3), Reads(FilmRefractiveIndex, 1), Reads(FilmRoughness, 1), Reads(Roughness, 1),
		};
		Call.Inputs.Append(HistoryPins(EPores::Sealed, TextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("SurfaceF0"), 3), Writes(TEXT("SurfaceRoughness"), 1), Writes(TEXT("SurfaceEdgeTint"), 3) };
		return Call;
	}

	/** Gold leaf under the surface law, and how much of it the wear has left. */
	FMasterCall LeafCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Leaf");
		Call.Includes = { LibraryMetal };
		Call.Code = HistoryDeclarations(EPores::Sealed, LeafTextureDepthMillimeters) + TEXT(R"(const FMatterSurface Surface = MatterBareMetal(LeafF0, LeafRoughness);
)") + HistoryApplied() + TEXT(R"(LeafSurfaceF0 = Marked.F0;
LeafSurfaceRoughness = Marked.Roughness;
LeafEdgeTint = MatterEdgeTintFromF82(LeafF0, LeafF82);
LeafCover = MatterLeafCover(MatterWornFraction(Place, Response));
return Marked.DiffuseAlbedo;)");
		Call.Inputs = { Reads(LeafF0, 3), Reads(LeafF82, 3), Reads(LeafRoughness, 1) };
		Call.Inputs.Append(HistoryPins(EPores::Sealed, LeafTextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("LeafSurfaceF0"), 3), Writes(TEXT("LeafSurfaceRoughness"), 1), Writes(TEXT("LeafEdgeTint"), 3), Writes(TEXT("LeafCover"), 1) };
		return Call;
	}
}

FMasterRecipe MetalRecipe(UMaterialParameterCollection& Moment, bool bGilt)
{
	const FName Layer = bGilt ? FName(Masters::Gilt) : NAME_None;
	FMasterRecipe Recipe = StartRecipe(MasterName(FName(Masters::Metal), Layer), Moment, OpaqueNanite());
	AddMetalParameters(Recipe);
	Recipe.Calls = { MetalCall() };
	Recipe.Slab.DiffuseAlbedo = From(TEXT("Metal"));
	Recipe.Slab.F0 = From(TEXT("Metal"), TEXT("SurfaceF0"));
	Recipe.Slab.F90 = From(TEXT("Metal"), TEXT("SurfaceEdgeTint"));
	Recipe.Slab.Roughness = From(TEXT("Metal"), TEXT("SurfaceRoughness"));
	Recipe.Slab.Anisotropy = From(TEXT("Metal"), TEXT("SurfaceAnisotropy"));
	if (bGilt)
	{
		AddGiltLayer(Recipe);
	}
	return Recipe;
}

FMasterRecipe PlateRecipe(UMaterialParameterCollection& Moment)
{
	FMasterRecipe Recipe = StartRecipe(MasterName(FName(Masters::Plate)), Moment, OpaqueNanite());
	AddTexture(Recipe, Image, WorkGroup, EngineDefaultTexture, TEXT("The work's own image, the plate photographed facing a dark field, which each work's plate instance carries"));
	AddScalar(Recipe, ImageWidthMeters, WorkGroup, Unset, TEXT("m"), TEXT("The work's width, which its image spans, which each work's plate instance carries"));
	AddScalar(Recipe, ImageHeightMeters, WorkGroup, Unset, TEXT("m"), TEXT("The work's height, which its image spans, which each work's plate instance carries"));
	AddColour(Recipe, MetalF0, SubstanceGroup, SlabF0Grey, TEXT("1"), TEXT("The silver's reflectance at normal incidence, linear Rec.709, from its record"));
	AddColour(Recipe, MetalF82, SubstanceGroup, SlabF0Grey, TEXT("1"), TEXT("The silver's reflectance at 82 degrees, where its edge colour is fitted, from its record"));
	AddColour(Recipe, FilmAlbedo, WorkingGroup, NeutralGrey, TEXT("1"), TEXT("Diffuse albedo of the image's particles, from the working's record"));
	AddScalar(Recipe, FilmRefractiveIndex, WorkingGroup, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the particles' face, from the working's record"));
	AddScalar(Recipe, FilmRoughness, WorkingGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the particles' face, from the working's record"));
	AddScalar(Recipe, Roughness, WorkingGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the polished silver, from the working's record"));
	AddHistoryInputs(Recipe, EPores::Sealed);
	Recipe.Calls = { PlateCall() };
	Recipe.Slab.DiffuseAlbedo = From(TEXT("Plate"));
	Recipe.Slab.F0 = From(TEXT("Plate"), TEXT("SurfaceF0"));
	Recipe.Slab.F90 = From(TEXT("Plate"), TEXT("SurfaceEdgeTint"));
	Recipe.Slab.Roughness = From(TEXT("Plate"), TEXT("SurfaceRoughness"));
	return Recipe;
}

void AddGiltLayer(FMasterRecipe& Recipe)
{
	const FLinearColor LeafDefault(SlabF0, SlabF0, SlabF0);
	AddColour(Recipe, LeafF0, WorkingGroup, LeafDefault, TEXT("1"), TEXT("Gold leaf's reflectance at normal incidence, linear Rec.709, from the gilt working's record"));
	AddColour(Recipe, LeafF82, WorkingGroup, LeafDefault, TEXT("1"), TEXT("Gold leaf's reflectance at 82 degrees, from the gilt working's record"));
	AddScalar(Recipe, LeafRoughness, WorkingGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the leaf, from the gilt working's record"));
	AddScalar(Recipe, LeafTextureDepthMillimeters, WorkingGroup, Unset, TEXT("mm"), TEXT("Mean texture depth of the gilt face, from the gilt working's record"));
	Recipe.Calls.Add(LeafCall());
	Recipe.Surface = EMasterSurface::LayeredSlab;
	Recipe.Layer.DiffuseAlbedo = From(TEXT("Leaf"));
	Recipe.Layer.F0 = From(TEXT("Leaf"), TEXT("LeafSurfaceF0"));
	Recipe.Layer.F90 = From(TEXT("Leaf"), TEXT("LeafEdgeTint"));
	Recipe.Layer.Roughness = From(TEXT("Leaf"), TEXT("LeafSurfaceRoughness"));
	Recipe.LayerCoverage = From(TEXT("Leaf"), TEXT("LeafCover"));
}
}
