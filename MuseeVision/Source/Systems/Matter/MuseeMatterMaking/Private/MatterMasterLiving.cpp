#include "MatterMasterParameters.h"
#include "MatterMasterParts.h"
#include "MatterMasters.h"
#include "MatterNames.h"

// The living master: a plant's leaf, petal, stamen or stem, its species' colour from its vertices, lit from both faces.

namespace Musee::Matter
{
namespace
{
	using namespace MasterParts;
	using namespace MasterParameter;

	FMasterSettings TwoSidedNanite()
	{
		FMasterSettings Settings;
		Settings.BlendMode = BLEND_Opaque;
		Settings.bUsedWithNanite = true;
		Settings.bTwoSided = true;
		return Settings;
	}

	void AddLivingParameters(FMasterRecipe& Recipe)
	{
		AddScalar(Recipe, RefractiveIndex, SubstanceGroup, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the cuticle's wax, from the tissue's record"));
		AddScalar(Recipe, Roughness, SubstanceGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the cuticle, from the tissue's record"));
		AddScalar(Recipe, TissueTransmittanceRatio, SubstanceGroup, Unset, TEXT("1"), TEXT("What the tissue transmits as a fraction of what it reflects, from its record"));
		AddHistoryInputs(Recipe, EPores::Sealed);
	}

	/** The tissue under the law, its colour the species' own (Nature's vertex colour), and the path Substrate's two-sided
	 *  wrap needs to pass what it transmits. */
	FMasterCall TissueCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Tissue");
		Call.Includes = { LibraryHistory, LibraryLiving };
		Call.Code = HistoryDeclarations(EPores::Sealed, TextureDepthMillimeters) + TEXT(R"(const FMatterSurface Surface = MatterTissueSurface(SpeciesColour, RefractiveIndex, Roughness);
)") + HistoryApplied() + TEXT(R"(SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
MeanFreePath = MatterWrapMeanFreePathCentimeters(MatterTissueTransmittance(Marked.DiffuseAlbedo, TissueTransmittanceRatio));
return Marked.DiffuseAlbedo;)");
		Call.Inputs = {
			Reads(TEXT("SpeciesColour"), 3, MasterInput::VertexColor, nullptr), Reads(RefractiveIndex, 1), Reads(Roughness, 1),
			Reads(TissueTransmittanceRatio, 1),
		};
		Call.Inputs.Append(HistoryPins(EPores::Sealed, TextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("SurfaceF0"), 3), Writes(TEXT("SurfaceRoughness"), 1), Writes(TEXT("MeanFreePath"), 3) };
		return Call;
	}
}

FMasterRecipe LivingRecipe(UMaterialParameterCollection& Moment)
{
	FMasterRecipe Recipe = StartRecipe(MasterName(FName(Masters::Living)), Moment, TwoSidedNanite());
	AddLivingParameters(Recipe);
	Recipe.Calls = { TissueCall() };
	Recipe.Slab.DiffuseAlbedo = From(TEXT("Tissue"));
	Recipe.Slab.F0 = From(TEXT("Tissue"), TEXT("SurfaceF0"));
	Recipe.Slab.Roughness = From(TEXT("Tissue"), TEXT("SurfaceRoughness"));
	Recipe.Slab.SSSMFP = From(TEXT("Tissue"), TEXT("MeanFreePath"));
	Recipe.Slab.SubSurface = MSS_TwoSidedWrap;
	return Recipe;
}
}
