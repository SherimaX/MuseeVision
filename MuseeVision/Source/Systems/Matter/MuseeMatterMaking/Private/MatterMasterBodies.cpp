#include "MatterMasterParameters.h"
#include "MatterMasterParts.h"
#include "MatterMasters.h"
#include "MatterNames.h"

// The masters of opaque dielectric bodies whose face shows their structure: stone, marble and white.

namespace Musee::Matter
{
namespace
{
	using namespace MasterParts;
	using namespace MasterParameter;

	const FLinearColor NeutralGrey(SlabDiffuseAlbedo, SlabDiffuseAlbedo, SlabDiffuseAlbedo);

	/** Every opaque body is used on Nanite pieces. */
	FMasterSettings OpaqueNanite()
	{
		FMasterSettings Settings;
		Settings.BlendMode = BLEND_Opaque;
		Settings.bUsedWithNanite = true;
		return Settings;
	}

	void AddBodyParameters(FMasterRecipe& Recipe, bool bBedded)
	{
		AddColour(Recipe, DiffuseAlbedo, SubstanceGroup, NeutralGrey, TEXT("1"), TEXT("Linear diffuse albedo of the substance, from its record"));
		AddScalar(Recipe, RefractiveIndex, SubstanceGroup, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the face's interface, from the substance's record"));
		AddScalar(Recipe, GrainMillimeters, SubstanceGroup, Unset, TEXT("mm"), TEXT("The finest structure the face shows, from the substance's record"));
		AddScalar(Recipe, BedMillimeters, SubstanceGroup, Unset, TEXT("mm"), TEXT("The coarsest: a bed's thickness, or the reach of the tool that laid a white body"));
		if (bBedded)
		{
			AddScalar(Recipe, BedAspect, SubstanceGroup, Unstretched, TEXT("1"), TEXT("How many times finer the structure is across the bedding than along it"));
		}
		AddScalar(Recipe, HurstExponent, SubstanceGroup, Unset, TEXT("1"), TEXT("Self-affinity of the face: relief falls by 2^-H for each halving of scale"));
		AddScalar(Recipe, ToneSpread, SubstanceGroup, Unset, TEXT("1"), TEXT("Half-range of ln(albedo) over the face"));
		AddScalar(Recipe, RoughnessSpread, SubstanceGroup, Unset, TEXT("1"), TEXT("Half-range of ln(roughness) over the face"));
		AddScalar(Recipe, Roughness, WorkingGroup, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the worked face, from the working's record"));
		AddScalar(Recipe, Haziness, WorkingGroup, Unset, TEXT("1"), TEXT("Haze of the worked face: the weight of its wide second specular lobe"));
		AddScalar(Recipe, ReliefMillimeters, WorkingGroup, Unset, TEXT("mm"), TEXT("Amplitude of the worked face's relief, from the working's record"));
		AddPieceTableIndex(Recipe);
		AddHistoryInputs(Recipe, EPores::Open);
	}

	/** The face of a bedded stone: its tone, its varied roughness and its relief (Stone.ush). */
	FMasterCall StoneFaceCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Face");
		Call.Includes = { LibraryFrame, LibraryStone };
		Call.Code = TEXT(R"(FMatterStone Stone;
Stone.GrainMillimeters = GrainMillimeters;
Stone.BedMillimeters = BedMillimeters;
Stone.BedAspect = BedAspect;
Stone.HurstExponent = HurstExponent;
Stone.RoughnessSpread = RoughnessSpread;
Stone.ToneSpread = ToneSpread;
Stone.ReliefMillimeters = ReliefMillimeters;
const FMatterStoneFace Face = MatterStoneFace(MatterMetersFromEngine(PiecePosition), MatterNaturalBed(), PieceTableIndex, Stone, Roughness);
FaceRoughness = Face.Roughness;
ReliefSlope = Face.ReliefSlope;
return Face.Tone;)");
		Call.Inputs = {
			Reads(TEXT("PiecePosition"), 3, MasterInput::LocalPosition, nullptr), Reads(PieceTableIndex, 1),
			Reads(GrainMillimeters, 1), Reads(BedMillimeters, 1), Reads(BedAspect, 1), Reads(HurstExponent, 1),
			Reads(RoughnessSpread, 1), Reads(ToneSpread, 1), Reads(ReliefMillimeters, 1), Reads(Roughness, 1),
		};
		Call.ReturnComponents = 1;
		Call.Outputs = { Writes(TEXT("FaceRoughness"), 1), Writes(TEXT("ReliefSlope"), 3) };
		return Call;
	}

	/** The face of an unbedded white body (White.ush). */
	FMasterCall WhiteFaceCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("Face");
		Call.Includes = { LibraryFrame, LibraryWhite };
		Call.Code = TEXT(R"(const FMatterStone Structure = MatterWhiteStructure(GrainMillimeters, BedMillimeters, HurstExponent, ToneSpread, RoughnessSpread, ReliefMillimeters);
const FMatterStoneFace Face = MatterWhiteFace(MatterMetersFromEngine(PiecePosition), PieceTableIndex, Structure, Roughness);
FaceRoughness = Face.Roughness;
ReliefSlope = Face.ReliefSlope;
return Face.Tone;)");
		Call.Inputs = {
			Reads(TEXT("PiecePosition"), 3, MasterInput::LocalPosition, nullptr), Reads(PieceTableIndex, 1),
			Reads(GrainMillimeters, 1), Reads(BedMillimeters, 1), Reads(HurstExponent, 1),
			Reads(RoughnessSpread, 1), Reads(ToneSpread, 1), Reads(ReliefMillimeters, 1), Reads(Roughness, 1),
		};
		Call.ReturnComponents = 1;
		Call.Outputs = { Writes(TEXT("FaceRoughness"), 1), Writes(TEXT("ReliefSlope"), 3) };
		return Call;
	}

	/** The surface law over a body's face, giving the Slab's inputs. */
	FMasterCall BodyHistoryCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("History");
		Call.Includes = { LibraryHistory };
		Call.Code = HistoryDeclarations(EPores::Open, TextureDepthMillimeters) + TEXT(R"(FMatterSurface Surface;
Surface.DiffuseAlbedo = DiffuseAlbedo * Tone;
Surface.F0 = MatterDielectricF0(RefractiveIndex).xxx;
Surface.Roughness = FaceRoughness;
Surface.Haziness = Haziness;
Surface.Slope = SlopeTangent.xy;
)") + HistoryApplied() + TEXT(R"(SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
SurfaceHaziness = Marked.Haziness;
SurfaceNormal = MatterSlopeNormal(Marked.Slope);
return Marked.DiffuseAlbedo;)");
		Call.Inputs = {
			Reads(DiffuseAlbedo, 3), Reads(TEXT("Tone"), 1, TEXT("Face"), nullptr), Reads(RefractiveIndex, 1),
			Reads(TEXT("FaceRoughness"), 1, TEXT("Face"), TEXT("FaceRoughness")), Reads(Haziness, 1), Reads(TEXT("SlopeTangent"), 3),
		};
		Call.Inputs.Append(HistoryPins(EPores::Open, TextureDepthMillimeters));
		Call.ReturnComponents = 3;
		Call.Outputs = { Writes(TEXT("SurfaceF0"), 3), Writes(TEXT("SurfaceRoughness"), 1), Writes(TEXT("SurfaceHaziness"), 1), Writes(TEXT("SurfaceNormal"), 3) };
		return Call;
	}

	/** A body master: its face call, the law, the relief brought into the face's tangent axes, and its Slab. */
	FMasterRecipe BodyRecipe(const FString& AssetName, UMaterialParameterCollection& Moment, bool bBedded)
	{
		FMasterRecipe Recipe = StartRecipe(AssetName, Moment, OpaqueNanite());
		AddBodyParameters(Recipe, bBedded);
		Recipe.Calls = { bBedded ? StoneFaceCall() : WhiteFaceCall(), BodyHistoryCall() };

		FMasterTransform& Slope = Recipe.Transforms.AddDefaulted_GetRef();
		Slope.Name = TEXT("SlopeTangent");
		Slope.Input = From(TEXT("Face"), TEXT("ReliefSlope"));
		Slope.From = TRANSFORMSOURCE_Local;
		Slope.To = TRANSFORM_Tangent;

		Recipe.Slab.DiffuseAlbedo = From(TEXT("History"));
		Recipe.Slab.F0 = From(TEXT("History"), TEXT("SurfaceF0"));
		Recipe.Slab.Roughness = From(TEXT("History"), TEXT("SurfaceRoughness"));
		Recipe.Slab.Haziness = From(TEXT("History"), TEXT("SurfaceHaziness"));
		Recipe.Slab.Normal = From(TEXT("History"), TEXT("SurfaceNormal"));
		return Recipe;
	}
}

FMasterRecipe StoneRecipe(UMaterialParameterCollection& Moment)
{
	return BodyRecipe(MasterName(FName(Masters::Stone)), Moment, true);
}

FMasterRecipe MarbleRecipe(UMaterialParameterCollection& Moment)
{
	FMasterRecipe Recipe = BodyRecipe(MasterName(FName(Masters::Marble)), Moment, true);
	AddColour(Recipe, AbsorptionPerMillimeter, SubstanceGroup, FLinearColor::Black, TEXT("1/mm"), TEXT("Absorption coefficient of the body at the primaries, from the substance's record"));
	AddColour(Recipe, ScatteringPerMillimeter, SubstanceGroup, FLinearColor::Black, TEXT("1/mm"), TEXT("Reduced scattering coefficient of the body at the primaries, from the substance's record"));

	FMasterCall Subsurface;
	Subsurface.Name = TEXT("Subsurface");
	Subsurface.Includes = { LibraryMedium };
	Subsurface.Code = TEXT("return MatterMeanFreePathCentimeters(AbsorptionPerMillimeter, ScatteringPerMillimeter);");
	Subsurface.Inputs = { Reads(AbsorptionPerMillimeter, 3), Reads(ScatteringPerMillimeter, 3) };
	Subsurface.ReturnComponents = 3;
	Recipe.Calls.Add(Subsurface);

	Recipe.Slab.SSSMFP = From(TEXT("Subsurface"));
	Recipe.Slab.SubSurface = MSS_Diffusion;
	return Recipe;
}

FMasterRecipe WhiteRecipe(UMaterialParameterCollection& Moment, bool bGilt)
{
	const FName Layer = bGilt ? FName(Masters::Gilt) : NAME_None;
	FMasterRecipe Recipe = BodyRecipe(MasterName(FName(Masters::White), Layer), Moment, false);
	if (bGilt)
	{
		AddGiltLayer(Recipe);
	}
	return Recipe;
}
}
