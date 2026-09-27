#include "MatterMasterParameters.h"
#include "MatterMasterParts.h"
#include "MatterMasters.h"
#include "MatterNames.h"

// The wood master: oak sawn into boards, each laid with its grain along its face's UV u, under an oil or a wax.

namespace Musee::Matter
{
namespace
{
	using namespace MasterParts;
	using namespace MasterParameter;

	const FLinearColor NeutralGrey(SlabDiffuseAlbedo, SlabDiffuseAlbedo, SlabDiffuseAlbedo);
	/** An unstained body keeps its own colour. */
	const FLinearColor Unstained(1.0f, 1.0f, 1.0f);

	FMasterSettings OpaqueNanite()
	{
		FMasterSettings Settings;
		Settings.BlendMode = BLEND_Opaque;
		Settings.bUsedWithNanite = true;
		return Settings;
	}

	void AddWoodParameters(FMasterRecipe& Recipe)
	{
		AddColour(Recipe, DiffuseAlbedo, SubstanceGroup, NeutralGrey, TEXT("1"), TEXT("Linear diffuse albedo of the unfinished earlywood, from the substance's record"));
		AddScalar(Recipe, RefractiveIndex, SubstanceGroup, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the cell wall, from the substance's record"));
		AddScalar(Recipe, GrainMillimeters, SubstanceGroup, Unset, TEXT("mm"), TEXT("The finest structure: the earlywood's vessels, from the substance's record"));
		AddScalar(Recipe, BedMillimeters, SubstanceGroup, Unset, TEXT("mm"), TEXT("The coarsest: a growth ring's width, from the substance's record"));
		AddScalar(Recipe, BedAspect, SubstanceGroup, Unstretched, TEXT("1"), TEXT("How many times coarser the structure is along the grain than across it"));
		AddScalar(Recipe, HurstExponent, SubstanceGroup, Unset, TEXT("1"), TEXT("Self-affinity of the face: relief falls by 2^-H for each halving of scale"));
		AddScalar(Recipe, ToneSpread, SubstanceGroup, Unset, TEXT("1"), TEXT("Half-range of ln(albedo) over the face"));
		AddScalar(Recipe, RoughnessSpread, SubstanceGroup, Unset, TEXT("1"), TEXT("Half-range of ln(roughness) over the face"));
		AddScalar(Recipe, LatewoodFraction, SubstanceGroup, Unset, TEXT("1"), TEXT("The fraction of each growth ring that is latewood"));
		AddScalar(Recipe, LatewoodTone, SubstanceGroup, Unstretched, TEXT("1"), TEXT("The latewood's albedo as a fraction of the earlywood's"));
		AddScalar(Recipe, RayFleck, SubstanceGroup, Unset, TEXT("1"), TEXT("The fraction of a quarter-sawn face the ray fleck covers"));
		AddScalar(Recipe, RayTone, SubstanceGroup, Unstretched, TEXT("1"), TEXT("The rays' albedo as a fraction of the wood around them"));
		AddScalar(Recipe, RayLengthMillimeters, SubstanceGroup, Unset, TEXT("mm"), TEXT("A ray fleck's length along the grain"));
		AddScalar(Recipe, RayWidthMillimeters, SubstanceGroup, Unset, TEXT("mm"), TEXT("A ray fleck's width across the grain"));
		AddScalar(Recipe, Roughness, WorkingGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the finished face, from the working's record"));
		AddScalar(Recipe, Haziness, WorkingGroup, Unset, TEXT("1"), TEXT("Haze of the finished face: the weight of its wide second specular lobe"));
		AddScalar(Recipe, ReliefMillimeters, WorkingGroup, Unset, TEXT("mm"), TEXT("Depth of the open pores the finish leaves, from the working's record"));
		AddScalar(Recipe, FinishRefractiveIndex, WorkingGroup, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the oil or wax the finish soaks into the face"));
		AddScalar(Recipe, FinishCover, WorkingGroup, Unset, TEXT("1"), TEXT("Fraction of the face the finish fills"));
		AddColour(Recipe, StainTransmittance, WorkingGroup, Unstained, TEXT("1"), TEXT("What a treatment (fuming) leaves in the wood's colour, as a factor on its albedo"));
		AddPieceTableIndex(Recipe);
		AddHistoryInputs(Recipe, EPores::Open);
	}

	/** A board's face: its rings, rays and pores along its grain (Wood.ush). */
	FMasterCall BoardCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Board");
		Call.Includes = { LibraryFrame, LibraryWood };
		Call.Code = TEXT(R"(FMatterStone Fibre;
Fibre.GrainMillimeters = GrainMillimeters;
Fibre.BedMillimeters = BedMillimeters;
Fibre.BedAspect = BedAspect;
Fibre.HurstExponent = HurstExponent;
Fibre.RoughnessSpread = RoughnessSpread;
Fibre.ToneSpread = ToneSpread;
Fibre.ReliefMillimeters = ReliefMillimeters;
FMatterWood Wood;
Wood.LatewoodFraction = LatewoodFraction;
Wood.LatewoodTone = LatewoodTone;
Wood.RayFleck = RayFleck;
Wood.RayTone = RayTone;
Wood.RayLengthMillimeters = RayLengthMillimeters;
Wood.RayWidthMillimeters = RayWidthMillimeters;
const FMatterStoneFace Face = MatterWoodFace(BoardMeters, PieceTableIndex, Fibre, Wood, Roughness);
FaceRoughness = Face.Roughness;
ReliefSlope = Face.ReliefSlope;
return Face.Tone;)");
		Call.Inputs = {
			Reads(TEXT("BoardMeters"), 2, MasterInput::TexCoord0, nullptr), Reads(PieceTableIndex, 1),
			Reads(GrainMillimeters, 1), Reads(BedMillimeters, 1), Reads(BedAspect, 1), Reads(HurstExponent, 1),
			Reads(RoughnessSpread, 1), Reads(ToneSpread, 1), Reads(ReliefMillimeters, 1),
			Reads(LatewoodFraction, 1), Reads(LatewoodTone, 1), Reads(RayFleck, 1), Reads(RayTone, 1),
			Reads(RayLengthMillimeters, 1), Reads(RayWidthMillimeters, 1), Reads(Roughness, 1),
		};
		Call.ReturnComponents = 1;
		Call.Outputs = { Writes(TEXT("FaceRoughness"), 1), Writes(TEXT("ReliefSlope"), 3) };
		return Call;
	}

	/** The finish soaked into the board, then the surface law. The relief's slope is already in the face's tangent axes,
	 *  whose u runs along the grain. */
	FMasterCall FinishCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("History");
		Call.Includes = { LibraryHistory, LibraryWood };
		Call.Code = HistoryDeclarations(EPores::Open, TextureDepthMillimeters) + TEXT(R"(FMatterSurface Surface;
Surface.DiffuseAlbedo = MatterFinishedWoodAlbedo(DiffuseAlbedo * StainTransmittance * Tone, FinishRefractiveIndex, FinishCover);
Surface.F0 = MatterFinishedWoodF0(RefractiveIndex, FinishRefractiveIndex, FinishCover);
Surface.Roughness = FaceRoughness;
Surface.Haziness = Haziness;
Surface.Slope = Slope.xy;
)") + HistoryApplied() + TEXT(R"(SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
SurfaceHaziness = Marked.Haziness;
SurfaceNormal = MatterSlopeNormal(Marked.Slope);
return Marked.DiffuseAlbedo;)");
		Call.Inputs = {
			Reads(DiffuseAlbedo, 3), Reads(StainTransmittance, 3), Reads(TEXT("Tone"), 1, TEXT("Board"), nullptr),
			Reads(RefractiveIndex, 1), Reads(FinishRefractiveIndex, 1), Reads(FinishCover, 1),
			Reads(TEXT("FaceRoughness"), 1, TEXT("Board"), TEXT("FaceRoughness")), Reads(Haziness, 1),
			Reads(TEXT("Slope"), 3, TEXT("Board"), TEXT("ReliefSlope")),
		};
		Call.Inputs.Append(HistoryPins(EPores::Open, TextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("SurfaceF0"), 3), Writes(TEXT("SurfaceRoughness"), 1), Writes(TEXT("SurfaceHaziness"), 1), Writes(TEXT("SurfaceNormal"), 3) };
		return Call;
	}
}

FMasterRecipe WoodRecipe(UMaterialParameterCollection& Moment)
{
	FMasterRecipe Recipe = StartRecipe(MasterName(FName(Masters::Wood)), Moment, OpaqueNanite());
	AddWoodParameters(Recipe);
	Recipe.Calls = { BoardCall(), FinishCall() };
	Recipe.Slab.DiffuseAlbedo = From(TEXT("History"));
	Recipe.Slab.F0 = From(TEXT("History"), TEXT("SurfaceF0"));
	Recipe.Slab.Roughness = From(TEXT("History"), TEXT("SurfaceRoughness"));
	Recipe.Slab.Haziness = From(TEXT("History"), TEXT("SurfaceHaziness"));
	Recipe.Slab.Normal = From(TEXT("History"), TEXT("SurfaceNormal"));
	return Recipe;
}
}
