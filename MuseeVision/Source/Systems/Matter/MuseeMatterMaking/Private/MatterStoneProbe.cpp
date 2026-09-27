#include "MatterStoneProbe.h"

namespace Musee::Matter
{
namespace
{
	/** The Substrate Slab's own defaults (MaterialExpressionSubstrate.h), so an uninstanced probe renders as the
	 *  engine's neutral slab. */
	constexpr float SlabDiffuseAlbedo = 0.18f;
	constexpr float SlabRoughness = 0.5f;

	/** The relative index whose normal-incidence reflectance is the Slab's default F0 of 0.04. */
	constexpr float SlabRefractiveIndex = 1.5f;

	/** Structure as fine across the beds as along them, the least a bedding can be stretched. */
	constexpr float Unstretched = 1.0f;

	/** A hollow that gathers only the rain falling on itself. */
	constexpr float OwnRainOnly = 1.0f;

	/** Every history input starts at zero: a face new from the workshop, dry, clean, unworn and sheltered. */
	constexpr float FreshFace = 0.0f;

	/** A stone set on its natural bed, its bedding level: the bed normal is the piece's z axis. */
	const FLinearColor NaturalBed(0.0f, 0.0f, 1.0f);
	const FLinearColor NeutralGrey(SlabDiffuseAlbedo, SlabDiffuseAlbedo, SlabDiffuseAlbedo);
	const FLinearColor Still(0.0f, 0.0f, 0.0f);

	constexpr const TCHAR* Substance = TEXT("Substance");
	constexpr const TCHAR* Working = TEXT("Working");
	constexpr const TCHAR* Response = TEXT("Response");
	constexpr const TCHAR* Weather = TEXT("Weather");
	constexpr const TCHAR* Place = TEXT("Place");
	constexpr const TCHAR* Snow = TEXT("Snow");

	constexpr const TCHAR* LibraryFrame = TEXT("/Project/Matter/Frame.ush");
	constexpr const TCHAR* LibraryStone = TEXT("/Project/Matter/Stone.ush");
	constexpr const TCHAR* LibraryHistory = TEXT("/Project/Matter/History.ush");

	void Add(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, int32 Components, const FLinearColor& Default,
		const TCHAR* Unit, const TCHAR* Meaning)
	{
		FMasterParameter& Parameter = Recipe.Parameters.AddDefaulted_GetRef();
		Parameter.Name = FName(Name);
		Parameter.Group = FName(Group);
		Parameter.Components = Components;
		Parameter.Default = Default;
		Parameter.Unit = Unit;
		Parameter.Meaning = Meaning;
	}

	void AddScalar(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, float Default, const TCHAR* Unit, const TCHAR* Meaning)
	{
		Add(Recipe, Name, Group, 1, FLinearColor(Default, Default, Default), Unit, Meaning);
	}

	void AddColor(FMasterRecipe& Recipe, const TCHAR* Name, const TCHAR* Group, const FLinearColor& Default, const TCHAR* Unit, const TCHAR* Meaning)
	{
		Add(Recipe, Name, Group, 3, Default, Unit, Meaning);
	}

	/** A call input named after the node it reads. */
	FMasterPin Reads(const TCHAR* Node, int32 Components)
	{
		return FMasterPin{ FName(Node), Components, FMasterSource{ FName(Node), NAME_None } };
	}

	/** A call input named Pin that reads Node's output Output (none for its return value). */
	FMasterPin Reads(const TCHAR* Pin, int32 Components, const TCHAR* Node, const TCHAR* Output)
	{
		return FMasterPin{ FName(Pin), Components, FMasterSource{ FName(Node), Output ? FName(Output) : NAME_None } };
	}

	void AddSubstanceAndWorking(FMasterRecipe& Recipe)
	{
		AddColor(Recipe, TEXT("DiffuseAlbedo"), Substance, NeutralGrey, TEXT("1"), TEXT("Linear diffuse albedo of the substance as worked, from its record"));
		AddScalar(Recipe, TEXT("RefractiveIndex"), Substance, SlabRefractiveIndex, TEXT("1"), TEXT("Refractive index of the substance's surface mineral in the visible, from its record"));
		AddScalar(Recipe, TEXT("Porosity"), Substance, FreshFace, TEXT("1"), TEXT("Open porosity as a volume fraction, from the substance's record"));
		AddScalar(Recipe, TEXT("GrainMillimeters"), Substance, FreshFace, TEXT("mm"), TEXT("Grain or crystal size: the finest structure the face shows"));
		AddScalar(Recipe, TEXT("BedMillimeters"), Substance, FreshFace, TEXT("mm"), TEXT("Bed thickness: the coarsest structure the face shows"));
		AddScalar(Recipe, TEXT("BedAspect"), Substance, Unstretched, TEXT("1"), TEXT("How many times finer the structure is across the bedding than along it"));
		AddScalar(Recipe, TEXT("HurstExponent"), Substance, FreshFace, TEXT("1"), TEXT("Self-affinity of the face: relief falls by 2^-H for each halving of scale"));
		AddScalar(Recipe, TEXT("RoughnessSpread"), Substance, FreshFace, TEXT("1"), TEXT("Half-range of ln(roughness) over the face"));
		AddScalar(Recipe, TEXT("ToneSpread"), Substance, FreshFace, TEXT("1"), TEXT("Half-range of ln(albedo) over the face, from bed to bed and grain to grain"));
		AddColor(Recipe, TEXT("BedNormal"), Substance, NaturalBed, TEXT("1"), TEXT("Bed normal in the piece's axes, from the stage's piece table"));
		AddScalar(Recipe, TEXT("PieceSeed"), Substance, FreshFace, TEXT("1"), TEXT("The piece's seed, an integer from its key, so that no two pieces of one stone repeat"));
		AddScalar(Recipe, TEXT("Roughness"), Working, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the worked face (sawn, honed, polished)"));
		AddScalar(Recipe, TEXT("Haziness"), Working, FreshFace, TEXT("1"), TEXT("Haze of the worked face: the weight of its wide second specular lobe"));
		AddScalar(Recipe, TEXT("TextureDepthMillimeters"), Working, FreshFace, TEXT("mm"), TEXT("Mean texture depth of the worked face (ISO 13473-1)"));
		AddScalar(Recipe, TEXT("ReliefMillimeters"), Working, FreshFace, TEXT("mm"), TEXT("Amplitude of the worked face's relief"));
	}

	void AddHistory(FMasterRecipe& Recipe)
	{
		AddScalar(Recipe, TEXT("WettingDepthMillimeters"), Response, FreshFace, TEXT("mm"), TEXT("Depth of the body whose pores govern its appearance: its optical sampling depth"));
		AddScalar(Recipe, TEXT("WornRoughness"), Response, FreshFace, TEXT("1"), TEXT("The steady roughness abrasion by feet or hands carries the face toward"));
		AddScalar(Recipe, TEXT("WornHaziness"), Response, FreshFace, TEXT("1"), TEXT("The steady haziness of the worn face"));
		AddScalar(Recipe, TEXT("WearPassages"), Response, FreshFace, TEXT("passages"), TEXT("Passages that carry the face 63 percent of the way to its worn state (0: it never wears)"));
		AddColor(Recipe, TEXT("SoilAlbedo"), Response, NeutralGrey, TEXT("1"), TEXT("Diffuse albedo of the deposit the place lays down"));
		AddScalar(Recipe, TEXT("DepositionPerYear"), Response, FreshFace, TEXT("1/yr"), TEXT("First-order rate at which deposit covers bare area"));
		AddScalar(Recipe, TEXT("WashRemovalPerYear"), Response, FreshFace, TEXT("1/yr"), TEXT("First-order rate at which a full rain wash clears deposit"));
		AddScalar(Recipe, TEXT("RainHistoryMillimeters"), Weather, FreshFace, TEXT("mm"), TEXT("Rain water on a level, fully exposed reference surface, net of evaporation (Time)"));
		AddColor(Recipe, TEXT("RainVelocityMetersPerSecond"), Weather, Still, TEXT("m/s"), TEXT("The falling rain's velocity in world axes: the wind's x and y, the fall's -z (Time)"));
		AddScalar(Recipe, TEXT("SnowWaterMillimeters"), Weather, FreshFace, TEXT("mm"), TEXT("Snow water equivalent lying on the reference surface (Time)"));
		AddScalar(Recipe, TEXT("SkyExposure"), Place, FreshFace, TEXT("1"), TEXT("Fraction of free-field precipitation that reaches the point (Form)"));
		AddScalar(Recipe, TEXT("Cavity"), Place, FreshFace, TEXT("1"), TEXT("0 on an open face, 1 deep in a recess that wash, wind and cleaning never reach (Form)"));
		AddScalar(Recipe, TEXT("WashExposure"), Place, FreshFace, TEXT("1"), TEXT("Fraction of a full rain wash that runs over the point (Form)"));
		AddScalar(Recipe, TEXT("PondingDepthMillimeters"), Place, FreshFace, TEXT("mm"), TEXT("Depth of the point below its hollow's spill level (Form)"));
		AddScalar(Recipe, TEXT("HollowDepthMillimeters"), Place, FreshFace, TEXT("mm"), TEXT("Depth of the point's hollow, from its spill level to its lowest point (Form)"));
		AddScalar(Recipe, TEXT("RunoffConcentration"), Place, OwnRainOnly, TEXT("1"), TEXT("Catchment area draining into the hollow per unit area of the hollow (Form)"));
		AddScalar(Recipe, TEXT("PassagesPerYear"), Place, FreshFace, TEXT("1/yr"), TEXT("Feet or hands passing over the point each year (Site's traffic field)"));
		AddScalar(Recipe, TEXT("AgeYears"), Place, FreshFace, TEXT("yr"), TEXT("Years since the face was worked"));
		AddColor(Recipe, TEXT("SnowAlbedo"), Snow, NeutralGrey, TEXT("1"), TEXT("Diffuse albedo of lying snow in the visible, from the snow record"));
		AddScalar(Recipe, TEXT("SnowRoughness"), Snow, SlabRoughness, TEXT("1"), TEXT("Slab roughness of the snow's surface, from the snow record"));
		AddScalar(Recipe, TEXT("SnowOpticalWaterMillimeters"), Snow, FreshFace, TEXT("mm"), TEXT("Snow water equivalent under which the cover hides 63 percent of the face"));
	}

	/** The stone's face: its tone, its varied roughness and its relief, from its substance and bedding. */
	FMasterCall StoneFaceCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("StoneFace");
		Call.Includes = { LibraryFrame, LibraryStone };
		Call.Code = TEXT(R"(FMatterStone Stone;
Stone.GrainMillimeters = GrainMillimeters;
Stone.BedMillimeters = BedMillimeters;
Stone.BedAspect = BedAspect;
Stone.HurstExponent = HurstExponent;
Stone.RoughnessSpread = RoughnessSpread;
Stone.ToneSpread = ToneSpread;
Stone.ReliefMillimeters = ReliefMillimeters;
const FMatterStoneFace Face = MatterStoneFace(MatterMetersFromEngine(PiecePosition), BedNormal, PieceSeed, Stone, Roughness);
FaceRoughness = Face.Roughness;
ReliefSlope = Face.ReliefSlope;
return Face.Tone;)");
		Call.Inputs = {
			Reads(TEXT("PiecePosition"), 3, MasterInput::LocalPosition, nullptr),
			Reads(TEXT("BedNormal"), 3), Reads(TEXT("PieceSeed"), 1), Reads(TEXT("GrainMillimeters"), 1),
			Reads(TEXT("BedMillimeters"), 1), Reads(TEXT("BedAspect"), 1), Reads(TEXT("HurstExponent"), 1),
			Reads(TEXT("RoughnessSpread"), 1), Reads(TEXT("ToneSpread"), 1), Reads(TEXT("ReliefMillimeters"), 1),
			Reads(TEXT("Roughness"), 1),
		};
		Call.ReturnComponents = 1;
		Call.Outputs = { FMasterPin{ TEXT("FaceRoughness"), 1, {} }, FMasterPin{ TEXT("ReliefSlope"), 3, {} } };
		return Call;
	}

	/** The surface law over the stone's face. */
	FMasterCall HistoryCall()
	{
		FMasterCall Call;
		Call.Name = TEXT("History");
		Call.Includes = { LibraryHistory };
		Call.Code = TEXT(R"(const float SurfaceIndexF0 = MatterDielectricF0(RefractiveIndex);
FMatterSurface Surface;
Surface.DiffuseAlbedo = DiffuseAlbedo * Tone;
Surface.F0 = SurfaceIndexF0.xxx;
Surface.Roughness = FaceRoughness;
Surface.Haziness = Haziness;
Surface.Slope = SlopeTangent.xy;
FMatterWeather Weather;
Weather.RainHistoryMillimeters = RainHistoryMillimeters;
Weather.RainVelocityMetersPerSecond = RainVelocityMetersPerSecond;
Weather.SnowWaterMillimeters = SnowWaterMillimeters;
FMatterPlace Place;
Place.NormalWorld = NormalWorld;
Place.SkyExposure = SkyExposure;
Place.Cavity = Cavity;
Place.WashExposure = WashExposure;
Place.PondingDepthMillimeters = PondingDepthMillimeters;
Place.HollowDepthMillimeters = HollowDepthMillimeters;
Place.RunoffConcentration = RunoffConcentration;
Place.PassagesPerYear = PassagesPerYear;
Place.AgeYears = AgeYears;
FMatterResponse Response;
Response.Porosity = Porosity;
Response.WettingDepthMillimeters = WettingDepthMillimeters;
Response.TextureDepthMillimeters = TextureDepthMillimeters;
Response.WornRoughness = WornRoughness;
Response.WornHaziness = WornHaziness;
Response.WearPassages = WearPassages;
Response.SoilAlbedo = SoilAlbedo;
Response.DepositionPerYear = DepositionPerYear;
Response.WashRemovalPerYear = WashRemovalPerYear;
FMatterSnow Snow;
Snow.Albedo = SnowAlbedo;
Snow.Roughness = SnowRoughness;
Snow.OpticalWaterMillimeters = SnowOpticalWaterMillimeters;
const FMatterSurface Marked = MatterHistory(Surface, Weather, Place, Response, Snow);
SurfaceF0 = Marked.F0;
SurfaceRoughness = Marked.Roughness;
SurfaceHaziness = Marked.Haziness;
SurfaceNormal = MatterSlopeNormal(Marked.Slope);
return Marked.DiffuseAlbedo;)");
		Call.Inputs = {
			Reads(TEXT("DiffuseAlbedo"), 3), Reads(TEXT("Tone"), 1, TEXT("StoneFace"), nullptr), Reads(TEXT("RefractiveIndex"), 1),
			Reads(TEXT("FaceRoughness"), 1, TEXT("StoneFace"), TEXT("FaceRoughness")), Reads(TEXT("Haziness"), 1),
			Reads(TEXT("SlopeTangent"), 3), Reads(TEXT("NormalWorld"), 3, MasterInput::VertexNormalWorld, nullptr),
			Reads(TEXT("RainHistoryMillimeters"), 1), Reads(TEXT("RainVelocityMetersPerSecond"), 3), Reads(TEXT("SnowWaterMillimeters"), 1),
			Reads(TEXT("SkyExposure"), 1), Reads(TEXT("Cavity"), 1), Reads(TEXT("WashExposure"), 1),
			Reads(TEXT("PondingDepthMillimeters"), 1), Reads(TEXT("HollowDepthMillimeters"), 1), Reads(TEXT("RunoffConcentration"), 1),
			Reads(TEXT("PassagesPerYear"), 1), Reads(TEXT("AgeYears"), 1),
			Reads(TEXT("Porosity"), 1), Reads(TEXT("WettingDepthMillimeters"), 1), Reads(TEXT("TextureDepthMillimeters"), 1),
			Reads(TEXT("WornRoughness"), 1), Reads(TEXT("WornHaziness"), 1), Reads(TEXT("WearPassages"), 1),
			Reads(TEXT("SoilAlbedo"), 3), Reads(TEXT("DepositionPerYear"), 1), Reads(TEXT("WashRemovalPerYear"), 1),
			Reads(TEXT("SnowAlbedo"), 3), Reads(TEXT("SnowRoughness"), 1), Reads(TEXT("SnowOpticalWaterMillimeters"), 1),
		};
		Call.ReturnComponents = 3;
		Call.Outputs = {
			FMasterPin{ TEXT("SurfaceF0"), 3, {} }, FMasterPin{ TEXT("SurfaceRoughness"), 1, {} },
			FMasterPin{ TEXT("SurfaceHaziness"), 1, {} }, FMasterPin{ TEXT("SurfaceNormal"), 3, {} },
		};
		return Call;
	}
}

FMasterRecipe StoneProbeRecipe()
{
	FMasterRecipe Recipe;
	Recipe.PackagePath = TEXT("/Game/Generated/Matter");
	Recipe.AssetName = TEXT("M_Matter_Stone_Probe");
	Recipe.bUsedWithNanite = true;
	AddSubstanceAndWorking(Recipe);
	AddHistory(Recipe);
	Recipe.Calls = { StoneFaceCall(), HistoryCall() };

	FMasterTransform& Slope = Recipe.Transforms.AddDefaulted_GetRef();
	Slope.Name = TEXT("SlopeTangent");
	Slope.Input = FMasterSource{ TEXT("StoneFace"), TEXT("ReliefSlope") };
	Slope.From = TRANSFORMSOURCE_Local;
	Slope.To = TRANSFORM_Tangent;

	const FName History(TEXT("History"));
	Recipe.Slab.DiffuseAlbedo = FMasterSource{ History, NAME_None };
	Recipe.Slab.F0 = FMasterSource{ History, TEXT("SurfaceF0") };
	Recipe.Slab.Roughness = FMasterSource{ History, TEXT("SurfaceRoughness") };
	Recipe.Slab.Haziness = FMasterSource{ History, TEXT("SurfaceHaziness") };
	Recipe.Slab.Normal = FMasterSource{ History, TEXT("SurfaceNormal") };
	return Recipe;
}
}
